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

#if 1
struct configuration config = {0};
twn_wblock_t bias[RAM_36K_DEPTH];
twn_wblock_t weights[CI_STRIDE][WEIGHT_BUF_DEPTH];//2*16=32
fblock_t ibufa[IBUF_DEPTH][CI_STRIDE];//4*2*16=128
fblock_t ibufb[IBUF_DEPTH][CI_STRIDE];//4*2*16=128
fblock_t ibufc[IBUF_DEPTH][CI_STRIDE];//4*2*16=128





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
    config.is_first_layer = (insts.ires >> 14) & 0x1;
    //ibufa,  ibufb,  ibufc,
    // M   ,   OUT ,   NADD,
    // M   ,   ADD ,   OUT ,
    // OUT ,   NADD,   M   ,
    // OUT ,   M   ,   ADD ,
    // NADD,   OUT ,   M   ,
    //  ADD,   M   ,   OUT ,
    if((config.ibuf_type_a == M) && (config.ibuf_type_b == OUT) && (config.ibuf_type_a == NADD)){
        config.ibuf_comb=M_OUT_NADD;
    }else if((config.ibuf_type_a == M) && (config.ibuf_type_b == ADD) && (config.ibuf_type_a == OUT)){
        config.ibuf_comb=M_ADD_OUT;
    }else if((config.ibuf_type_a == OUT) && (config.ibuf_type_b == NADD) && (config.ibuf_type_a == M)){
        config.ibuf_comb=OUT_NADD_M;
    }else if((config.ibuf_type_a == OUT) && (config.ibuf_type_b == M) && (config.ibuf_type_a == ADD)){
        config.ibuf_comb=OUT_M_ADD;
    }else if((config.ibuf_type_a == NADD) && (config.ibuf_type_b == OUT) && (config.ibuf_type_a == M)){
        config.ibuf_comb=NADD_OUT_M;
    }else if((config.ibuf_type_a == ADD) && (config.ibuf_type_b == M) && (config.ibuf_type_a == OUT)){
        config.ibuf_comb=ADD_M_OUT;
    }else{
        config.ibuf_comb=M_OUT_NADD;
    }
}

static void LoadFeatureFromBuf(fblock_t ibuf[IBUF_DEPTH][CI_STRIDE]){
#pragma HLS INLINE OFF
    dimension_t width=config.input_width; 
    dimension_t height=config.input_height; 
    dimension_t w_sum=config.output_width-1;
    dimension_t h_sum=config.output_height-1;
    filter_t kh_sum = config.kernel_size;
    filter_t kw_sum = config.kernel_size;
    dimension_t stride = config.stride;
    channel_t ci_group_sum = CEIL_DIV(config.aligned_input_channels,CI_STRIDE);
    px_strm_t px_strm_value;
    int h_first=0;
    int w_first=0;
    int h_addr=0;
    int w_addr=0;
    bool jump=0;
    int depth=0;
    fblock_t px_value[CI_STRIDE];
L_LOAD_IMG_FROM_RAM_H:
    for(dimension_t h=0;h<h_sum;h++){
    #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
        for(dimension_t w=0;w<w_sum;w++){
        #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
            for(filter_t kh=0;kh<kh_sum;kh++){
            #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 3 
                for(filter_t kw=0;kw<kw_sum;kw++){
                #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 3 
                    for(channel_t ci_group=0;ci_group<ci_group_sum;ci_group++){
                    #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 2 MAX = 4 
                    #pragma PIPELINE II=1
                        h_first=h*stride-config.pad;
                        w_first=w*stride-config.pad;
                        h_addr=h_first+kh;
                        w_addr=w_first+kw;
                        jump=(h_addr <0) || (h_addr >=height) ||
                              (w_addr <0) || (w_addr >=width) ;
                        depth = (h_addr*width+w_addr)*ci_group_sum+ci_group;
                        for(channel_t ci_stride=0;ci_stride<CI_STRIDE;ci_stride++){
                        #pragma HLS UNROLL
                        	px_value[ci_stride]= jump ? fblock_t(0) : ibuf[depth][ci_stride];
//                            if(config.ibuf_type_a == M){
//                               px_value[ci_stride]= jump ? fblock_t(0) : ibufa[depth][ci_stride];
//                            }else if(config.ibuf_type_b == M){
//                               px_value[ci_stride]= jump ? fblock_t(0) : ibufb[depth][ci_stride];
//                            }else if(config.ibuf_type_c == M){
//                               px_value[ci_stride]= jump ? fblock_t(0) : ibufc[depth][ci_stride];
//                            }
                        }
                        ApFixToExtMemSync<FBLOCK_WIDTH,px_strm_t,fblock_t,CI_STRIDE>(px_strm_value,px_value);
                    }
                }
            }
        }
    }

}

static void CopyImageFromDRAM(fblock_t* inf){
#pragma HLS inline off
#pragma HLS ARRAY_PARTITION  variable = ibufa complete dim = 2
#pragma HLS ARRAY_PARTITION  variable = ibufb complete dim = 2
#pragma HLS ARRAY_PARTITION  variable = ibufc complete dim = 2

#pragma HLS RESOURCE variable = ibufa core = RAM_S2P_BRAM latency = 3
#pragma HLS RESOURCE variable = ibufb core = RAM_S2P_BRAM latency = 3
#pragma HLS RESOURCE variable = ibufc core = RAM_S2P_BRAM latency = 3
    int img_len = config.input_width * config.input_height;
    channel_t ci_group_sum =  config.input_channels/CI_STRIDE; 
    int size_of_itrans = sizeof(fblock_t)*CI_STRIDE;
    int addr_offset = 0;
    int ibuf_offset = 0;
    for(int i=0;i<img_len;i++){
    #pragma HLS LOOP_TRIPCOUNT MIN = 1024 AVG = 1024 MAX = 1024 
        for(channel_t ci_group=0;ci_group<ci_group_sum;ci_group++){
        #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 1
            ibuf_offset = i*ci_group_sum+ci_group;
            addr_offset = ibuf_offset*CI_STRIDE; 
            if(config.ibuf_type_a == M){
                memcpy(ibufa[ibuf_offset],&inf[addr_offset],size_of_itrans);
            }else if(config.ibuf_type_b == M){
                memcpy(ibufb[ibuf_offset],&inf[addr_offset],size_of_itrans);
            }else if(config.ibuf_type_c == M){
                memcpy(ibufc[ibuf_offset],&inf[addr_offset],size_of_itrans);
            }
        }
    }
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
#ifndef __SYNTHESIS__
                weight_t w_array[8];
                ExtMemToApFixSync<DATA_WIDTH,twn_wblock_t,weight_t,8>(weights[ci_stride][depth],w_array);
                for(int i=0;i<8;i++){
                	LOG(INFO)<<"facc::weights="<<setbase(10)<<setw(4)<<w_array[i]<<endl;
                }
#endif
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


//static void ComputeConvolutionResults(){
////#pragma HLS INLINE OFF
////#pragma HLS DATAFLOW
//    LoadFeatureFromBuf();
//}
//#pragma HLS inline off

void Accelerator(struct instruction_group_t& insts, wblock_t* w, fblock_t* inf, fblock_t* outf){


#pragma HLS TOP
#pragma HLS INTERFACE s_axilite port=insts bundle = axilite register
#pragma HLS INTERFACE s_axilite port = return bundle = axilite register

#pragma HLS INTERFACE m_axi depth=OUTPUT_FEATURE_BUF_LENGTH port=outf offset=slave bundle = memorybus0 register
#pragma HLS INTERFACE m_axi depth=INPUT_FEATURE_BUF_LENGTH port=inf offset=slave bundle = memorybus1 register
#pragma HLS INTERFACE m_axi depth=WEIGHTS_BUF_LENGTH port=w offset=slave bundle = memorybus2 register

#ifndef __SYNTHESIS__
	LOG(CONSOLE)<<"Accelerator is running"<<endl;
#endif
	DecodeInstruction(insts);
    CopyWeightAndBiasFromDRAM(w);
    if(config.is_first_layer){
        CopyImageFromDRAM(inf);
    }
    if(config.ibuf_comb == M_OUT_NADD){
        LoadFeatureFromBuf(ibufa);
    }
//		ComputeConvolutionResults(inf,outf);

}
#endif
