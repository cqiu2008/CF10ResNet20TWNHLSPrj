//----------------------------------------------------------------
//  FPGA Accelerator For CNN Inference
//----------------------------------------------------------------
//
//  File:   fpga_top.hpp
//  FPGA-Side Functions for FPGA Accelerator
//
//  (c) hrt_fpga , 2018-06
//
//----------------------------------------------------------------
#include "fpga_top.hpp"
using namespace std;

struct configuration config = {0};
twn_wblock_t bias[RAM_36K_DEPTH];
twn_wblock_t weights[CI_STRIDE][WEIGHT_BUF_DEPTH];

static inline void DecodeInstruction(struct instruction_group_t& insts){
	config.dpi = (insts.ishift>>27)&0x1f;
	config.dpo = (insts.ishift>>22)&0x1f;
	config.wpo = (insts.ishift>>17)&0x1f;
	config.bpo = (insts.ishift>>12)&0x1f;
	config.nsbl = (insts.ishift>>6)&0x3f;
	config.sbl = insts.ishift&0x3f;
	config.input_channels = (insts.idimension>>12)&0xfff;
	config.output_channels = (insts.idimension)&0xfff;
	config.input_width = (insts.idimension>>24)&0xff;
	config.input_height = (insts.idimension>>24)&0xff;
	config.layer_type = (insts.imisc>>28)&0xf;
	config.relu = (insts.imisc>>27)&0x1;
	config.kernel_size = (insts.imisc>>22) & 0xf;
	config.pad = ((insts.imisc>>26)&1) ? pad_t(config.kernel_size/2) : (pad_t)0;
	config.stride = (insts.imisc>>18) & 0xf;
	config.pooling_type = (insts.imisc>>16)&0x3;
	config.pooling_size = (insts.imisc>>11)&0x1f;
	config.pooling_stride = (insts.imisc>>6)&0x1f;
	config.pooling_pad = ((insts.imisc>>5)&0x1) ? pad_t(config.pooling_size/2) : pad_t(0);
	config.freuse = ((insts.imisc>>4)&0x1);
	config.wcompress = (insts.imisc>>2) & 0x3;
	config.wreuse = ((insts.imisc>>1)&0x1);
	config.breuse = (insts.imisc&0x1);
	config.num_of_ci_strides = CEIL_DIV(config.input_channels,CI_STRIDE);
	config.aligned_input_channels = config.num_of_ci_strides*CI_STRIDE;
	config.num_of_co_strides = CEIL_DIV(config.output_channels,CO_STRIDE);
	config.aligned_output_channels = CEIL_DIV(config.output_channels,CI_STRIDE)*CI_STRIDE;
	config.num_of_cixco_strides = config.num_of_ci_strides*config.num_of_co_strides;
	config.num_of_pooling_points = config.pooling_size*config.pooling_size;
	config.num_of_width_ci_strides = config.num_of_ci_strides*config.input_width;
	config.output_width = 1 + (config.input_width + 2*config.pad - config.kernel_size)/config.stride;
	config.output_height = 1 + (config.input_height + 2*config.pad - config.kernel_size)/config.stride;
	config.num_of_output_points = config.output_height*config.output_width;
	config.output_height_div_pstride = config.output_height/config.pooling_stride;
	config.output_width_div_pstride = config.output_width/config.pooling_stride;
	config.pooled_width = 1 + (config.output_width + 2*config.pooling_pad - config.pooling_size)/config.pooling_stride;
	config.pooled_height = 1 + (config.output_height + 2*config.pooling_pad - config.pooling_size)/config.pooling_stride;
	config.max_row_available = -1;
	config.input_feature_block_index = 0;
	config.output_feature_block_index = config.sbl*(config.aligned_output_channels>>BYTES_PER_TRANSACTION_MASK);
	config.factor = insts.ires & 0xff;
	config.ibuf_type_a = (insts.ires >> 8) & 0x3;
	config.ibuf_type_b = (insts.ires >> 10) & 0x3;
	config.ibuf_type_b = (insts.ires >> 12) & 0x3;
}


static void CopyWeightAndBiasFromDRAM(wblock_t* w){
#pragma HLS inline off
	wblock_t w_buff[RAM_36K_DEPTH];
	int twn_size_of_kk_co_aligned = config.aligned_output_channels * config.kernel_size * config.kernel_size / 4;
    channel_t co_group_sum = CEIL_DIV(config.aligned_output_channels,CO_STRIDE);
    channel_t ci_group_sum = CEIL_DIV(config.aligned_input_channels,CI_STRIDE);
    filter_t kk_sum = config.kernel_size * config.kernel_size; 
    int addr_offset = 0;
    int w_buff_offset = 0;
//====Load Weights 
L_LOAD_CI_WEIGHTS_FROM_DRAM:
	for(channel_t ci=0;ci<config.aligned_input_channels;ci++){
		#pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 32 MAX = 64 
        addr_offset = ci * twn_size_of_kk_co_aligned / NUM_OF_BYTES_PER_TRANSACTION;
		memcpy(w_buff,&w[addr_offset],twn_size_of_kk_co_aligned);
    L_LOAD_K_WEIGHTS_FROM_DRAM:
        for(filter_t k=0;k<kk_sum ;k++){
		    #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 9 
        L_LOAD_CO_WEIGHTS_FROM_DRAM:
            for(channel_t co_group=0;co_group<co_group_sum;co_group++){
		        #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 9 
                #pragma HLS PIPELINE II = 1
                int depth = (ci*kk_sum*co_group_sum/CI_STRIDE + k*co_group_sum+co_group);
                channel_t ci_stride = ci % CI_STRIDE;
                twn_wblock_t twn_w[2];
                if(co_group % 2 ==0){
             	    ExtMemToApFixSync<TWN_DATA_WIDTH*CO_STRIDE,wblock_t,twn_wblock_t,2>(w_buff[w_buff_offset++],twn_w);
                    weights[ci_stride][depth] = twn_w[0];
                }else{
                    weights[ci_stride][depth] = twn_w[1];
                }
            }
        }
	}
//====Load Bias
	int twn_size_of_co_aligned = config.aligned_output_channels / 4;
    w_buff_offset = 0;
    addr_offset = config.aligned_input_channels * config.aligned_output_channels * config.kernel_size * config.kernel_size / (4 * NUM_OF_BYTES_PER_TRANSACTION);
    memcpy(w_buff,&w[addr_offset],twn_size_of_co_aligned);
L_LOAD_BIAS_FROM_DRAM:
    for(channel_t co_group=0;co_group<co_group_sum;co_group++){
    #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 9 
    #pragma HLS PIPELINE II = 1
        twn_wblock_t twn_w[2];
        if(co_group % 2 ==0){
            ExtMemToApFixSync<TWN_DATA_WIDTH*CO_STRIDE,wblock_t,twn_wblock_t,2>(w_buff[w_buff_offset++],twn_w);
            bias[co_group] = twn_w[0];
        }else{
            bias[co_group] = twn_w[1];
        }
    }
}


void Accelerator(struct instruction_group_t& insts, wblock_t* w, fblock_t* inf, fblock_t* outf){


#pragma HLS TOP
#pragma HLS INTERFACE s_axilite port=insts bundle = axilite register
#pragma HLS INTERFACE s_axilite port = return bundle = axilite register

#pragma HLS INTERFACE m_axi depth=OUTPUT_FEATURE_BUF_LENGTH port=outf offset=slave bundle = memorybus0 register
#pragma HLS INTERFACE m_axi depth=INPUT_FEATURE_BUF_LENGTH port=inf offset=slave bundle = memorybus1 register
#pragma HLS INTERFACE m_axi depth=WEIGHTS_BUF_LENGTH port=w offset=slave bundle = memorybus2 register

	DecodeInstruction(insts);

	LOG(CONSOLE)<<"Accelerator is running"<<endl;

		CopyWeightAndBiasFromDRAM(w);
//		ComputeConvolutionResults(inf,outf);

}
