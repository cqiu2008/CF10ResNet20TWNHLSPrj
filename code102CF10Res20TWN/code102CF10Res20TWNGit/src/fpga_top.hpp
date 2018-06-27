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
#ifndef FPGA_TOP_HPP__
#define FPGA_TOP_HPP__

#define AP_INT_MAX_W                        4096

#if 1

//#include"ap_shift_reg.h"
#include<hls_stream.h>
#include<hls_video.h>
#include<iostream>
#include <iomanip>
#include<fstream>
#include<cstring>
#include<cassert>

#include "common.hpp"
#include "logger.hpp"

const long int WEIGHTS_BUF_LENGTH = 1000;
const long int INPUT_FEATURE_BUF_LENGTH = 1000;
const long int OUTPUT_FEATURE_BUF_LENGTH = 1000; 


struct configuration{
	pad_t pad;
	bool relu;
	stride_t stride;
	kernel_t kernel_size;
	layer_type_t layer_type;
	shift_t dpi;
	shift_t dpo;
	shift_t wpo;
	shift_t bpo;
	bool freuse;
	bool wreuse;
	bool breuse;
	sublayer_t sbl;
	sublayer_t nsbl;
	weight_compress_t wcompress;
	pooling_type_t pooling_type;
	pooling_size_t pooling_pad;
	pooling_size_t pooling_size;
	pooling_stride_t pooling_stride;

	dimension_t pooled_width;
	dimension_t pooled_height;
	dimension_t input_width;
	dimension_t input_height;
	channel_t input_channels;
	channel_t num_of_ci_strides;
	channel_t aligned_input_channels;
	dimension_t output_width;
	dimension_t output_height;
	channel_t output_channels;
	channel_t num_of_co_strides;
	channel_t aligned_output_channels;
	channel_t num_of_cixco_strides;
	feature_block_index_t num_of_width_ci_strides;
	feature_block_index_t num_of_output_points;
	pooling_size_t num_of_pooling_points;
	dimension_t output_width_div_pstride;
	dimension_t output_height_div_pstride;
	dimension_t col_div_pstride;
	channel_t size_of_point_group;
	dimension_t max_row_available;
	feature_block_index_t input_feature_block_index;
	feature_block_index_t output_feature_block_index;
	feature_t factor;
	ibuf_type_t ibuf_type_a;
	ibuf_type_t ibuf_type_b;
	ibuf_type_t ibuf_type_c;
    bool is_first_layer;
};


template<int N, typename T_SRC,typename T_DST,int W>
void ExtMemToApFixSync(T_SRC src, T_DST dst[W]){
#pragma HLS INLINE
	ext_mem_loop:
	for (int i=0;i<W;i++){
	#pragma HLS UNROLL
		hls::AXIGetBitFields(src,i*N,N,dst[i]);
	}
}


template<int N, typename T_SRC, typename T_DST, int W>
void ApFixToExtMemSync(T_SRC &src, T_DST dst[W]){
#pragma HLS INLINE
	ap_fix_loop:
	for (int i=0;i<W;i++){
	#pragma HLS UNROLL
		hls::AXISetBitFields(src,i*N,N,dst[i]);
	}
}


void Accelerator(struct instruction_group_t& insts,wblock_t* w,fblock_t* inf,fblock_t* outf);


#endif
#endif

