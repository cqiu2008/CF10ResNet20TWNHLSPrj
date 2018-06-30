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
wblock_t bias[RAM_36K_DEPTH];
twn_wblock_t weights[WEIGHT_BUF_DEPTH][CI_STRIDE];//2*16=32
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
static void trans_pxi_strm(
    fblock_t pxi_value[CI_STRIDE],
    feature_t pxi_batch[BATCH_NUM][CI_STRIDE]
    ){
#pragma HLS inline
    for(channel_t ci_stride=0;ci_stride<CI_STRIDE;ci_stride++){
    #pragma HLS UNROLL
        feature_t pxi_batch_tmp[BATCH_NUM];
        ExtMemToApFixSync<DATA_WIDTH,fblock_t,feature_t,BATCH_NUM>(pxi_value[ci_stride],pxi_batch_tmp);
        for(channel_t batch_num=0;batch_num<BATCH_NUM;batch_num++){
        #pragma HLS UNROLL
            pxi_batch[batch_num][ci_stride] = pxi_batch_tmp[batch_num];
        }
    }
}


static void get_weights(
    int depth,
    twn_wblock_t weights[WEIGHT_BUF_DEPTH][CI_STRIDE],
    twn_weight_t weights_value[CO_STRIDE][CI_STRIDE]
        ){
//#pragma HLS FUNCTION_INSTANTIATE variable =
#pragma HLS inline
    twn_wblock_t weights_tmp[CI_STRIDE];
    #pragma HLS ARRAY_PARTITION  variable = weights_tmp complete dim = 0 
    for(channel_t ci_stride=0;ci_stride<CI_STRIDE;ci_stride++){
    #pragma HLS UNROLL
        twn_weight_t weights_array[CO_STRIDE];
        weights_tmp[ci_stride] =  weights[depth][ci_stride];
        ExtMemToApFixSync<TWN_DATA_WIDTH,twn_wblock_t,twn_weight_t,CO_STRIDE>(weights_tmp[ci_stride],weights_array);
        for(channel_t co_stride=0;co_stride<CO_STRIDE;co_stride++){
        #pragma HLS UNROLL
            weights_value[co_stride][ci_stride] = weights_array[co_stride];
        }
    }
}

static accumulator_t twn_macc_2d(
        const feature_t pix[CI_STRIDE],
        const twn_weight_t weights[CI_STRIDE]
        ){
#pragma HLS INLINE 
        accumulator_t result_tmp=0;
    for(channel_t ci_stride=0;ci_stride<CI_STRIDE;ci_stride++){
    #pragma HLS UNROLL
        if(weights[ci_stride] == 1){
            result_tmp +=pix[ci_stride];
        }else if(weights[ci_stride] == 3){
            result_tmp -=pix[ci_stride];
        }
    }
    return result_tmp;
}

static void trans_pxo_strm(
    pxo_strm_t& pxo_strm_value,
    accumulator_t acc_val[CO_STRIDE][BATCH_NUM]
    ){
#pragma HLS inline
        for(channel_t co_stride=0;co_stride<CO_STRIDE;co_stride++){
        #pragma HLS UNROLL
            for(channel_t batch_num=0;batch_num<BATCH_NUM;batch_num++){
            #pragma HLS UNROLL
                int i = batch_num+co_stride*BATCH_NUM;
		        hls::AXIGetBitFields(pxo_strm_value,i*ACC_DATA_WIDTH,ACC_DATA_WIDTH,acc_val[co_stride][batch_num]);
            }
        }
}

static void trans_bias(
    weight_t bias_s1[CI_STRIDE],
    accumulator_t bias_s2[CI_STRIDE],
    int bpo_shift 
    ){
#pragma HLS inline
    for(channel_t co=0;co<CI_STRIDE;co++){
    #pragma HLS UNROLL
        if(bpo_shift > 0){
            bias_s2 = accumulator_t(bias_s1[co] >> bpo_shift);
        }else{
            bias_s2 = accumulator_t(bias_s1[co] << (-bpo_shift));
        }
    }
}

static void trans_rounding(
    accumulator_t& after_bias,
    accumulator_t& active_s1,
    int dpo_shift,
    accumulator_t half_value
    ){
#pragma HLS inline
    if(dpo_shift >0){
        active_s1 = (after_bias+half_value) >> dpo_shift;
    }else{
        active_s1 = (after_bias+half_value) << (-dpo_shift);
    }
}

static void trans_addbuf(
    feature_t &ibuf_trans,
    bool bufia_en,
    accumulator_t &active_s1,
    accumulator_t &active_s2
    ){
#pragma HLS inline
    if(bufia_en){
        active_s2=active_s1+ibuf_trans;
    }else{                 
        active_s2=active_s1;
    }
}

static void trans_trim(
    accumulator_t& active_s1,
    feature_t& active_s2
    ){
#pragma HLS inline
    if(active_s1 > 127){
        active_s2 = 127; 
    }else if(active_s1 < -127){
        active_s2 = -128; 
    }else{
        active_s2 = feature_t(active_s1);
    }
}

static void trans_relu(
    feature_t& active_s3,
    feature_t& active_s4
    ){
#pragma HLS inline
    if(config.relu){
        active_s4 = (active_s3>0)?active_s3:0;
    }else{
        active_s4 = active_s3;
    }
}






static void trans_ibuf(
        fblock_t bufia[CI_STRIDE],
        feature_t ibuf_trans[CI_STRIDE][BATCH_NUM]
        ){
#pragma HLS inline
    feature_t ibuf_tmp[BATCH_NUM];
    for(channel_t co=0;co<CI_STRIDE;co++){
    #pragma HLS UNROLL
        ExtMemToApFixSync<DATA_WIDTH,fblock_t,feature_t,BATCH_NUM>(bufia[co],ibuf_tmp);
        for(channel_t batch_num=0;batch_num<BATCH_NUM;batch_num++){
        #pragma HLS UNROLL
            ibuf_trans[co][batch_num]=ibuf_tmp[batch_num]; 
        }
    }
}

static void WriteBackToRAM(
        hls_pxi_strm_t &hls_pxo_strm,
        wblock_t bias[RAM_36K_DEPTH],
        bool bufia_en,
        fblock_t bufia[IBUF_DEPTH][CI_STRIDE],
        fblock_t bufo[IBUF_DEPTH][CI_STRIDE]
        ){
#pragma HLS INLINE OFF
    pxo_strm_t pxo_strm_value;
    weight_t bias_s1[CI_STRIDE];
    accumulator_t bias_s2[CI_STRIDE];
    accumulator_t acc_val[CO_STRIDE][BATCH_NUM];
    accumulator_t after_bias[CI_STRIDE][BATCH_NUM];
    accumulator_t active_s1[CI_STRIDE][BATCH_NUM];
    accumulator_t active_s2[CI_STRIDE][BATCH_NUM];
    feature_t     active_s3[CI_STRIDE][BATCH_NUM];
    feature_t     active_s4[CI_STRIDE][BATCH_NUM];
    fblock_t      active_s5[CI_STRIDE];
    feature_t     ibuf_trans[CI_STRIDE][BATCH_NUM];
    #pragma HLS ARRAY_PARTITION variable =  acc_val complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  bias_s1 complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  bias_s2 complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  after_bias complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  active_s1 complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  active_s2 complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  active_s3 complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  active_s4 complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  active_s5 complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  ibuf_trans complete dim = 0

    dimension_t w_sum=config.output_width;
    dimension_t h_sum=config.output_height;
    channel_t co_trn_sum = CEIL_DIV(config.aligned_output_channels,NUM_OF_BYTES_PER_TRANSACTION);
    int bpo_shift = config.bpo-config.dpi-config.wpo;
    int dpo_shift = config.dpi_config.wpo-config.dpo;
    accumulator_t half_value = 1 << (dpo_shift-1);
    bool relu = config.relu;
////        feature_t ibuf_trans[BATCH_NUM][CI_STRIDE]
//// +bias ,Xn+1 = MXn + 0 or bufia,relue 
    L_WRITEBACK_H:
    for(dimension_t h=0;h<h_sum;h++){
    #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
        L_WRITEBACK_L:
        for(dimension_t w=0;w<w_sum;w++){
        #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
            L_COMPUTE_CO_TRN:
            for(channel_t co_trn=0;co_trn<co_trn_sum;co_trn++){
            #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 2 MAX =4  
            #pragma PIPELINE II=1
                channel_t co_trn_rem = co_trn % (CO_STRIDE/NUM_OF_BYTES_PER_TRANSACTION);
                if( 0 == co_trn_rem){
                    hls_pxo_strm >> pxo_strm_value;
                    trans_pxo_strm(pxo_strm_value,acc_val);
                }
                ExtMemToApFixSync<DATA_WIDTH,wblock_t,weight_t,CI_STRIDE>(bias[co_trn],bias_s1);
                trans_bias(bias_s1,bias_s2,bpo_shift);
                int depth = co_trn+w*co_trn_sum+h*w_sum*co_trn_sum;
                trans_ibuf(bufia[depth],ibuf_trans);
                for(channel_t co=0;co<CI_STRIDE;co++){
                #pragma HLS UNROLL
                    for(channel_t batch_num=0;batch_num<BATCH_NUM;batch_num++){
                    #pragma HLS UNROLL
                        after_bias[co][batch_num]=acc_val[co_trn_rem*CI_STRIDE+co][batch_num]+bias_s2[co];
                        trans_rounding(after_bias[co][batch_num],active_s1[co][batch_num],dpo_shift,half_value);
                        trans_addbuf(ibuf_trans[co][batch_num],bufia_en,active_s1[co][batch_num],active_s2[co][batch_num]);
                        trans_trim(active_s2[co][batch_num],active_s3[co][batch_num]);
                        trans_relu(active_s3,active_s4);
                    }
                    ApFixToExtMemSync<DATA_WIDTH,fblock_t,feature_t,BATCH_NUM>(active_s5[co],active_s4[co]);
                    bufo[depth][co] = active_s5[co];
                }
            }
        }
    }
}
static void ComputeElement(
        hls_pxi_strm_t &hls_pxi_strm,
        hls_pxi_strm_t &hls_pxo_strm,
        twn_wblock_t weights[WEIGHT_BUF_DEPTH][CI_STRIDE],
    ){
#pragma HLS INLINE OFF
    pxi_strm_t pxi_strm_value;
    pxo_strm_t pxo_strm_value;
    fblock_t pxi_value[CI_STRIDE];
    feature_t pxi_batch[BATCH_NUM][CI_STRIDE];

    accumulator_t cur_acc[CO_STRIDE][BATCH_NUM];
    accumulator_t out_acc[CO_STRIDE][BATCH_NUM];
    accumulator_t out_result[CO_STRIDE][BATCH_NUM];

    twn_weight_t weights_value[CO_STRIDE][CI_STRIDE];

    #pragma HLS ARRAY_PARTITION variable =  pxi_value complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  pxi_batch complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  cur_acc complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  out_acc complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  out_result complete dim = 0
    #pragma HLS ARRAY_PARTITION variable =  weights_value complete dim = 0

    dimension_t w_sum=config.output_width;
    dimension_t h_sum=config.output_height;
    filter_t kh_sum = config.kernel_size;
    filter_t kw_sum = config.kernel_size;
    channel_t ci_group_sum = CEIL_DIV(config.aligned_input_channels,CI_STRIDE);
    channel_t co_group_sum = CEIL_DIV(config.aligned_output_channels,CO_STRIDE);
    int depth ;
    filter_t kk_sum;
    filter_t k;

    L_COMPUTE_H:
    for(dimension_t h=0;h<h_sum;h++){
    #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
        L_COMPUTE_L:
        for(dimension_t w=0;w<w_sum;w++){
        #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
            L_COMPUTE_CO_GROUP:
            for(channel_t co_group=0;co_group<co_group_sum;co_group++){
            #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 2 MAX =4  
                L_COMPUTE_KH:
                for(filter_t kh=0;kh<kh_sum;kh++){
                #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 3 
                    L_COMPUTE_KW:
                    for(filter_t kw=0;kw<kw_sum;kw++){
                    #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 3 
                        L_COMPUTE_CI_GROUP:
                        for(channel_t ci_group=0;ci_group<ci_group_sum;ci_group++){
                        #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 2 MAX = 4 
                        #pragma PIPELINE II=1
                            hls_pxi_strm >> pxi_strm_value;
                            ApFixToExtMemSync<FBLOCK_WIDTH,pxi_strm_t,fblock_t,CI_STRIDE>(pxi_strm_value,pxi_value);
                            trans_pxi_strm(pxi_value,pxi_batch);
                            kk_sum = kw_sum*kh_sum;
                            k=kw+kh*kw_sum;
                            depth = ci_group*kk_sum*co_group_sum + k*co_group_sum+co_group;
                            get_weights(depth,weights,weights_value);
                            for(channel_t co_stride=0;co_stride<CO_STRIDE;co_stride++){
                            #pragma HLS UNROLL
                                for(channel_t batch_num=0;batch_num<BATCH_NUM;batch_num++){
                                #pragma HLS UNROLL
                                    cur_acc[co_stride][batch_num]=twn_macc_2d(pxi_batch[batch_num],weights_value[co_stride]);
                                    if((ci_group == 0) && (kh == 0) && (kw == 0)){
                                        out_acc[co_stride][batch_num]=cur_acc[co_stride][batch_num];
                                    }else{                                               
                                        out_acc[co_stride][batch_num]+=cur_acc[co_stride][batch_num];
                                    }
                                    if((ci_group == (ci_group_sum-1)) 
                                        && (kh == (kh_sum-1))
                                            && (kw == (kw_sum-1))){
                                        out_result[co_stride][batch_num] = out_acc[co_stride][batch_num];
		                                hls::AXISetBitFields(pxo_strm_value,(batch_num+co_stride*BATCH_NUM)*ACC_DATA_WIDTH,ACC_DATA_WIDTH,out_result[co_stride][batch_num]);
                                    }
                                }
                            }
                            if((ci_group == (ci_group_sum-1)) 
                                    && (kh == (kh_sum-1))
                                        && (kw == (kw_sum-1))){
                                hls_pxo_strm << pxo_strm_value;
                            }
                        }//ci_group
                    }//kw
                }//kh
            }//co_group
        }//w
    }//h
}

static void LoadFeatureFromBuf(
        fblock_t bufim[IBUF_DEPTH][CI_STRIDE],
        hls_pxi_strm_t &hls_pxi_strm){
#pragma HLS INLINE OFF
    dimension_t width=config.input_width; 
    dimension_t height=config.input_height; 
    dimension_t w_sum=config.output_width;
    dimension_t h_sum=config.output_height;
    filter_t kh_sum = config.kernel_size;
    filter_t kw_sum = config.kernel_size;
    dimension_t stride = config.stride;
    channel_t ci_group_sum = CEIL_DIV(config.aligned_input_channels,CI_STRIDE);
    channel_t co_group_sum = CEIL_DIV(config.aligned_output_channels,CO_STRIDE);
    pxi_strm_t pxi_strm_value;
    int h_first=0;
    int w_first=0;
    int h_addr=0;
    int w_addr=0;
    bool jump=0;
    int depth=0;
    fblock_t pxi_value[CI_STRIDE];
    L_LOAD_IMG_FROM_RAM_H:
    for(dimension_t h=0;h<h_sum;h++){
    #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
        L_LOAD_IMG_FROM_RAM_L:
        for(dimension_t w=0;w<w_sum;w++){
        #pragma HLS LOOP_TRIPCOUNT MIN = 8 AVG = 16 MAX = 32 
            L_LOAD_IMG_FROM_RAM_CO_GROUP:
            for(channel_t co_group=0;co_group<co_group_sum;co_group++){
            #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 2 MAX =4  
                L_LOAD_IMG_FROM_RAM_KH:
                for(filter_t kh=0;kh<kh_sum;kh++){
                #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 3 
                    L_LOAD_IMG_FROM_RAM_KW:
                    for(filter_t kw=0;kw<kw_sum;kw++){
                    #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 3 
                    L_LOAD_IMG_FROM_RAM_CI_GROUP:
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
                            	pxi_value[ci_stride]= jump ? fblock_t(0) : bufim[depth][ci_stride];
//                                if(config.ibuf_type_a == M){
//                                   pxi_value[ci_stride]= jump ? fblock_t(0) : ibufa[depth][ci_stride];
//                                }else if(config.ibuf_type_b == M){
//                                   pxi_value[ci_stride]= jump ? fblock_t(0) : ibufb[depth][ci_stride];
//                                }else if(config.ibuf_type_c == M){
//                                   pxi_value[ci_stride]= jump ? fblock_t(0) : ibufc[depth][ci_stride];
//                                }
                            }
                            ApFixToExtMemSync<FBLOCK_WIDTH,pxi_strm_t,fblock_t,CI_STRIDE>(pxi_strm_value,pxi_value);
                            hls_pxi_strm << pxi_strm_value;
                        }
                    }
                }
            }
        }
    }

}

static void CopyImageFromDRAM(fblock_t* inf){
#pragma HLS inline off
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
    channel_t co_bias_sum= CEIL_DIV(config.aligned_output_channels,NUM_OF_BYTES_PER_TRANSACTION);
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
                    weights[depth][ci_stride] = twn_w[0];
                }else{
                    weights[depth][ci_stride] = twn_w[1];
                }
#ifndef __SYNTHESIS__
                weight_t w_array[8];
                ExtMemToApFixSync<DATA_WIDTH,twn_wblock_t,weight_t,8>(weights[depth][ci_stride],w_array);
                for(int i=0;i<8;i++){
                	LOG(INFO)<<"facc::weights="<<setbase(10)<<setw(4)<<w_array[i]<<endl;
                }
#endif
            }
        }
	}
//====Load Bias
	int twn_size_of_co_aligned = config.aligned_output_channels;
    w_buff_offset = 0;
    addr_offset = config.aligned_input_channels * config.aligned_output_channels * config.kernel_size * config.kernel_size / (4 * NUM_OF_BYTES_PER_TRANSACTION);
    memcpy(w_buff,&w[addr_offset],twn_size_of_co_aligned);
L_LOAD_BIAS_FROM_DRAM:
    for(channel_t co_bias=0;co_bias<co_bias_sum;co_bias++){
    #pragma HLS LOOP_TRIPCOUNT MIN = 1 AVG = 1 MAX = 9 
    #pragma HLS PIPELINE II = 1
        bias[co_bias] = w_buff[w_buff_offset++];
    }
}


static void DataProcess(
    twn_wblock_t weights[WEIGHT_BUF_DEPTH][CI_STRIDE],
    wblock_t bias[RAM_36K_DEPTH],
    fblock_t bufim[IBUF_DEPTH][CI_STRIDE],
    bool bufia_en,
    fblock_t bufia[IBUF_DEPTH][CI_STRIDE],
    fblock_t bufo[IBUF_DEPTH][CI_STRIDE]
    ){
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
    hls_pxi_strm_t hls_pxi_strm;
    hls_pxi_strm_t hls_pxo_strm;
#pragma HLS STREAM variable = hls_pxi_strm depth = 512 dim = 1
#pragma HLS STREAM variable = hls_pxo_strm depth = 512 dim = 1
    LoadFeatureFromBuf(bufim,hls_pxi_strm);
    ComputeElement(hls_pxi_strm,hls_pxo_strm,weights);
    WriteBackToRAM(hls_pxo_strm,bias,bufia_en,bufia,bufo);
}




void Accelerator(struct instruction_group_t& insts, wblock_t* w, fblock_t* inf, fblock_t* outf){
#pragma HLS TOP
#pragma HLS INTERFACE s_axilite port=insts bundle = axilite register
#pragma HLS INTERFACE s_axilite port = return bundle = axilite register

#pragma HLS INTERFACE m_axi depth=OUTPUT_FEATURE_BUF_LENGTH port=outf offset=slave bundle = memorybus0 register
#pragma HLS INTERFACE m_axi depth=INPUT_FEATURE_BUF_LENGTH port=inf offset=slave bundle = memorybus1 register
#pragma HLS INTERFACE m_axi depth=WEIGHTS_BUF_LENGTH port=w offset=slave bundle = memorybus2 register

#pragma HLS ARRAY_PARTITION  variable = ibufa complete dim = 2
#pragma HLS ARRAY_PARTITION  variable = ibufb complete dim = 2
#pragma HLS ARRAY_PARTITION  variable = ibufc complete dim = 2
#pragma HLS ARRAY_PARTITION  variable = weights complete dim = 2
#pragma HLS RESOURCE variable = ibufa core = RAM_S2P_BRAM latency = 3
#pragma HLS RESOURCE variable = ibufb core = RAM_S2P_BRAM latency = 3
#pragma HLS RESOURCE variable = ibufc core = RAM_S2P_BRAM latency = 3
#pragma HLS RESOURCE variable = weights core = RAM_S2P_BRAM latency = 3


#ifndef __SYNTHESIS__
	LOG(CONSOLE)<<"Accelerator is running"<<endl;
#endif
	DecodeInstruction(insts);
    CopyWeightAndBiasFromDRAM(w);
    if(config.is_first_layer){
        CopyImageFromDRAM(inf);
    }
    if(config.ibuf_comb == M_OUT_NADD){
        DataProcess(weights,bias,ibufa,0,ibufc,ibufb);
    }else if(config.ibuf_comb == M_ADD_OUT){
        DataProcess(weights,bias,ibufa,1,ibufb,ibufc);
    }else if(config.ibuf_comb == OUT_NADD_M){
        DataProcess(weights,bias,ibufc,0,ibufb,ibufa);
    }else if(config.ibuf_comb == OUT_M_ADD){
        DataProcess(weights,bias,ibufb,1,ibufc,ibufa);
    }else if(config.ibuf_comb == NADD_OUT_M){
        DataProcess(weights,bias,ibufc,0,ibufa,ibufb);
    }else if(config.ibuf_comb == ADD_M_OUT){
        DataProcess(weights,bias,ibufb,1,ibufa,ibufc);
    }
}
#endif
