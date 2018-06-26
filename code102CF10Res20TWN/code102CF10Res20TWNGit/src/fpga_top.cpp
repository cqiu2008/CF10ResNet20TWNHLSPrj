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
wblock_t bias[RAM_36K_DEPTH];
wblock_t weights[CI_STRIDE][CO_STRIDE/NUM_OF_BYTES_PER_TRANSACTION][WEIGHT_BUF_DEPTH];

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




void Accelerator(struct instruction_group_t& insts, wblock_t* w, fblock_t* inf, fblock_t* outf){


#pragma HLS TOP
#pragma HLS INTERFACE s_axilite port=insts bundle = axilite register
#pragma HLS INTERFACE s_axilite port = return bundle = axilite register

#pragma HLS INTERFACE m_axi depth=OUTPUT_FEATURE_BUF_LENGTH port=outf offset=slave bundle = memorybus0 register
#pragma HLS INTERFACE m_axi depth=INPUT_FEATURE_BUF_LENGTH port=inf offset=slave bundle = memorybus1 register
#pragma HLS INTERFACE m_axi depth=WEIGHTS_BUF_LENGTH port=w offset=slave bundle = memorybus2 register

	DecodeInstruction(insts);

	LOG(CONSOLE)<<"Accelerator is running"<<endl;

//		CopyWeightAndBiasFromDRAM(w);
//		ComputeConvolutionResults(inf,outf);

}
