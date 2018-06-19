#include"layer.hpp"


layer::~layer(){
}


layer::layer(const layer& l){
	LOG(CONSOLE)<<"layer::invalid copy constructor"<<endl;
}


layer& layer::operator=(const layer& l){
	LOG(CONSOLE)<<"layer::invalid assignment"<<endl;
	return *this;
}


layer::layer(std::string name, layer_enum t, sublayer_t nsbl, sublayer_t sbl, dimension_t w,
		dimension_t h, bool fr, bool wr, bool br, weight_compress_t wc, channel_t ci, channel_t co,
		kernel_t k, pad_t p, stride_t s, bool r, pooling_enum pt, pooling_size_t psize, pooling_size_t ppad,
		pooling_stride_t pstride,
		ibuf_enum ibufa, ibuf_enum ibufb, ibuf_enum ibufc,
		shift_t dpi,shift_t dpo, shift_t wpo, shift_t bpo){

	assert((t>=0) && (t<NUM_OF_LAYER_TYPE));
	assert(nsbl>0);
	assert((sbl>=0)&&(sbl<=(nsbl-1)));
	assert(w==h);
	assert((w>=1) && (w<=MAX_FEATURE_DIMENSION));
	assert((h>=1) && (h<=MAX_FEATURE_DIMENSION));
	assert((ci>=1) && (ci<=MAX_CHANNEL_NUM));
	assert(w*(ci>>BYTES_PER_TRANSACTION_MASK)<LINE_BUF_DEPTH);
	assert((co>=1) && (co<=MAX_CHANNEL_NUM));
	assert((k>=1) && (k<=MAX_KERNEL_SIZE));
	assert(k*k*ci/NUM_OF_BYTES_PER_TRANSACTION<=RAM_36K_DEPTH);//deadlock limitation
	assert((s>=1) && (s<=MAX_KERNEL_STRIDE));
	assert((pt>=0)&&(pt<NUM_OF_POOLING_TYPE));
//	assert((psize>=1) && (psize<=MAX_POOLING_SIZE));//psize=0,no psize

	if (psize==1){
		assert(pstride==1);
	}

	if (pstride==1){
		assert(psize==1);
	}

	if (config.pooling_size>config.pooling_stride){
		LOG(CONSOLE)<<config.layer_name<<": pooling_size>pooling_stride which might introduce redundant computing"<<endl;
	}

	config.layer_name = name;
	config.layer_type = t;

	weights = NULL;
	input_features = NULL;
	output_features = NULL;

	config.sbl = sbl;
	config.nsbl = nsbl;

	config.dpi = dpi;
	config.dpo = dpo;
	config.wpo = wpo;
	config.bpo = bpo;

	config.freuse = fr;
	config.wreuse = wr;
	config.breuse = br;

	config.wcompress = wc;

	config.pooling_pad = ppad;
	config.pooling_type = pt;
	config.pooling_size = psize;
	config.pooling_stride = pstride;

	config.stride = s;
	config.pad = p;
	config.relu = r;
	config.kernel_size = k;

	config.input_width = w;
	config.input_height = h;

	config.input_channels = ci;
	config.output_channels = co;

	config.weight_offset = 0;
	config.input_feature_offset = 0;
	config.output_feature_offset = 0;

	config.num_of_ci_strides = ceil(1.0*config.input_channels/CI_STRIDE);
	config.aligned_input_channels = config.num_of_ci_strides*CI_STRIDE;

	config.num_of_co_strides = ceil(1.0*config.output_channels/CO_STRIDE);
	config.aligned_output_channels = ceil(1.0*config.output_channels/CI_STRIDE)*NUM_OF_BYTES_PER_TRANSACTION;

	config.output_width = 1 + (config.input_width + 2*config.pad - config.kernel_size)/config.stride;
	config.output_height = 1 + (config.input_height + 2*config.pad - config.kernel_size)/config.stride;

	config.pooled_width = 1 + (config.output_width + 2*config.pooling_pad - config.pooling_size)/config.pooling_stride;
	config.pooled_height = 1 + (config.output_height + 2*config.pooling_pad - config.pooling_size)/config.pooling_stride;

	config.num_of_bias_blocks = config.aligned_output_channels/NUM_OF_BYTES_PER_TRANSACTION;
	assert(config.num_of_bias_blocks < MAX_NUM_OF_BIAS_BLOCKS);

	config.num_of_weight_blocks = config.kernel_size*config.kernel_size*config.aligned_input_channels*config.aligned_output_channels/NUM_OF_BYTES_PER_TRANSACTION;
	assert(config.num_of_weight_blocks < MAX_NUM_OF_WEIGHT_BLOCKS);

	assert(config.kernel_size*config.kernel_size*config.num_of_ci_strides*config.num_of_co_strides<WEIGHT_BUF_DEPTH);

	config.size_of_input_features = config.input_height*config.input_width*config.aligned_input_channels;
	if (config.pooling_type==AVG_POOLING){
		config.size_of_output_features = 4*(config.pooled_height*config.pooled_width*config.aligned_output_channels);
	}else if ((config.layer_type==EXPAND3x3) || (config.layer_type==EXPAND1x1)){
		config.size_of_output_features = 2*(config.pooled_height*config.pooled_width*config.aligned_output_channels);
	}else{
		config.size_of_output_features = (config.pooled_height*config.pooled_width*config.aligned_output_channels);
	}
	config.size_of_weights_and_bias = (config.num_of_bias_blocks+config.num_of_weight_blocks)*NUM_OF_BYTES_PER_TRANSACTION;
}


void layer::MakeInstructionGroup(){
	//misc instruction format
	//4bits: layer_type
	//1bits: relu
	//1bits: pad
	//4bits: kernel_size
	//4bits: stride
	//2bits: pooling_type
	//5bits: pooling_size
	//5bits: pooling_stride
	//1bits: pooling_pad
	//1bits: freuse
	//2bits: weight_compress
	//1bits: wreuse
	//1bits: breuse
	insts.imisc = 0;
	insts.imisc |= ((int(config.layer_type&0xf))<<28);
	insts.imisc |= ((int(config.relu&1))<<27);
	insts.imisc |= ((int((config.pad>0)&1))<<26);
	insts.imisc |= ((int(config.kernel_size&0xf))<<22);
	insts.imisc |= ((int(config.stride&0xf))<<18);
	insts.imisc |= ((int(config.pooling_type&0x3))<<16);
	insts.imisc |= ((int(config.pooling_size&0x1f))<<11);
	insts.imisc |= ((int(config.pooling_stride&0x1f))<<6);
	insts.imisc |= ((int(config.pooling_pad&0x1))<<5);
	insts.imisc |= ((int(config.freuse&0x1))<<4);
	insts.imisc |= ((int(config.wcompress&0x3))<<2);
	insts.imisc |= ((int(config.wreuse&0x1))<<1);
	insts.imisc |= (int(config.breuse&0x1));
	//shift instruction format
	//5bits: dpi
	//5bits: dpo
	//5bits: bpo
	//5bits: wpo
	//6bits: nsbl
	//6bits: sbl
	insts.ishift = 0;
	insts.ishift |= ((int(config.dpi&0x1f))<<27);
	insts.ishift |= ((int(config.dpo&0x1f))<<22);
	insts.ishift |= ((int(config.bpo&0x1f))<<17);
	insts.ishift |= ((int(config.wpo&0x1f))<<12);
	insts.ishift |= ((int(config.nsbl&0x3f))<<6);
	insts.ishift |= int(config.sbl&0x3f);
	//dimension instruction format
	//8bits: width,height
	//12bits: input channel
	//12bits: output channel
	insts.idimension = 0;
	insts.idimension |= ((int(config.input_width&0xff))<<24);
	insts.idimension |= ((int(config.input_channels&0xfff))<<12);
	insts.idimension |= (int(config.output_channels&0xfff));
}


void layer::PrintLayer(){
	LOG(CONSOLE)<<config.layer_name<<"\t";
	LOG(CONSOLE)<<config.layer_type<<"\t";
	LOG(CONSOLE)<<config.input_width<<"\t";
	LOG(CONSOLE)<<config.input_height<<"\t";
	LOG(CONSOLE)<<config.pooled_width<<"\t";
	LOG(CONSOLE)<<config.pooled_height<<"\t";
	LOG(CONSOLE)<<config.input_channels<<"\t";
	LOG(CONSOLE)<<config.output_channels<<"\t";
	LOG(CONSOLE)<<config.kernel_size<<"\t";
	LOG(CONSOLE)<<config.pad<<"\t";
	LOG(CONSOLE)<<config.stride<<"\t";
	LOG(CONSOLE)<<config.relu<<"\t";
	LOG(CONSOLE)<<config.pooling_type<<"\t";
	LOG(CONSOLE)<<config.pooling_size<<"\t";
	LOG(CONSOLE)<<config.pooling_pad<<"\t";
	LOG(CONSOLE)<<config.pooling_stride<<"\t\t";
	LOG(CONSOLE)<<config.dpi<<"\t";
	LOG(CONSOLE)<<config.dpo<<"\t";
	LOG(CONSOLE)<<config.wpo<<"\t";
	LOG(CONSOLE)<<config.bpo<<"\t";
	LOG(CONSOLE)<<endl;
}


void layer::AllocateMemoryForWeightAndBias(){
	weights = new weight_block_t[config.size_of_weights_and_bias/NUM_OF_BYTES_PER_TRANSACTION];
	if (weights == NULL){
		LOG(ERROR)<<"Error: failed to allocate memory(weights+bias)"<<endl;
	}else{
		memset(weights,0,sizeof(weight_block_t)*config.size_of_weights_and_bias/NUM_OF_BYTES_PER_TRANSACTION);
	}
}


void layer::ReleaseMemoryForWeightAndBias(){
	if (weights!=NULL){
		delete[] weights;
	}
}


void layer::UpdateMemoryForWeightAndBias(weight_block_t* newptr){
	ReleaseMemoryForWeightAndBias();
	weights = newptr;
}


void layer::LoadGeneratedBias(ifstream& input){

	weight_block_index_t index = config.num_of_weight_blocks;


	LOG(INFO)<<"load generated bias:"<<endl;


	for (channel_t co_div=0;co_div<config.aligned_output_channels;co_div+=NUM_OF_BYTES_PER_TRANSACTION){
		for (channel_t nb=0;nb<NUM_OF_BYTES_PER_TRANSACTION;nb++){
			if ((co_div+nb)<config.output_channels){
				input>>weights[index].w[nb];
			}else{
				weights[index].w[nb] = 0;
			}

//			LOG(INFO)<<setw(4)<<weights[index].w[nb]<<" ";

		}
		index++;
	}


	LOG(INFO)<<endl;

}


void layer::LoadGeneratedWeight(ifstream& input){

	weight_t weight = 0;


//	LOGNO(INFO)<<"load generated weight:"<<endl;


	for (channel_t i=0;i<config.output_channels;i++){
		for (channel_t j=0;j<config.input_channels;j++){

//			LOGNO(INFO)<<"co="<<i<<" ci="<<j<<endl;

			for (kernel_t k=0;k<config.kernel_size;k++){
				for (kernel_t l=0;l<config.kernel_size;l++){
					input>>weight;

//					LOGNO(INFO)<<setw(4)<<weight<<" ";

					channel_t t = config.aligned_output_channels/NUM_OF_BYTES_PER_TRANSACTION;
					weight_block_index_t weight_index = k*config.kernel_size + l;
					weight_index *= config.aligned_input_channels*t;
					weight_index += j*t + i/NUM_OF_BYTES_PER_TRANSACTION;
					weight_block_index_t w = i%NUM_OF_BYTES_PER_TRANSACTION;
					weights[weight_index].w[w] = weight;
				}

				LOG(INFO)<<endl;
			}
			LOG(INFO)<<endl;
		}
		LOG(INFO)<<endl;
	}

#if DEBUG == 1
	LOG(INFO)<<"transformed weight:"<<endl;
	weight_block_index_t index = 0;
	for (kernel_t k=0;k<config.kernel_size;k++){
		for (kernel_t l=0;l<config.kernel_size;l++){
			for (channel_t j=0;j<config.input_channels;j+=CI_STRIDE){
				for (channel_t n=0;n<CI_STRIDE;n++){
					for (channel_t i=0;i<config.output_channels;i+=NUM_OF_BYTES_PER_TRANSACTION){
						for (channel_t m=0;m<NUM_OF_BYTES_PER_TRANSACTION;m++){
							LOG(INFO)<<setw(4)<<weights[index].w[m]<<" ";
						}
						index = index + 1;
					}
				}
			}
			LOG(INFO)<<endl;
		}
		LOG(INFO)<<endl;
	}
#endif
}


void layer::AllocateMemoryForInputFeature(){
	input_features = new feature_block_t[config.size_of_input_features/NUM_OF_BYTES_PER_TRANSACTION];
	if (input_features == NULL){
		LOG(ERROR)<<"Error: failed to allocate memory(input_features)"<<endl;
	}else{
		memset(input_features,0,sizeof(feature_block_t)*config.size_of_input_features/NUM_OF_BYTES_PER_TRANSACTION);
	}
}


void layer::ReleaseMemoryForInputFeature(){
	if (input_features != NULL){
		delete[] input_features;
	}
}


void layer::UpdateMemoryForInputFeature(feature_block_t* newptr){
	ReleaseMemoryForInputFeature();
	input_features = newptr;
}


void layer::UpdateMemoryForOutputFeature(feature_block_t* newptr){
	output_features = newptr;
}


void layer::LoadGeneratedFeatureMap(ifstream& input){

	feature_block_index_t index = 0;

#if DEBUG == 1
	LOG(INFO)<<"load generated feature:"<<endl;
#endif

	for (dimension_t j=0;j<config.input_height;j++){
		for (dimension_t k=0;k<config.input_width;k++){
#if DEBUG == 1
			LOG(INFO)<<"row="<<j<<" col="<<k<<endl;
#endif
			for (channel_t i=0;i<config.input_channels;i+=CI_STRIDE){
				for (channel_t m=0;m<CI_STRIDE;m+=NUM_OF_BYTES_PER_TRANSACTION){
					for (channel_t l=0;l<NUM_OF_BYTES_PER_TRANSACTION;l++){
						if ((i+m+l)<config.input_channels){
							input>>input_features[index].f[l];
#if DEBUG == 1
							LOG(INFO)<<setw(4)<<input_features[index].f[l]<<" ";
#endif
						}else{
							input_features[index].f[l] = 0;
						}
					}
					index = index + 1;
				}
			}
#if DEBUG == 1
			LOG(INFO)<<endl;
#endif
		}
#if DEBUG == 1
		LOG(INFO)<<endl;
#endif
	}

#if DEBUG == 1
	LOG(INFO)<<"transformed feature:"<<endl;
	for (channel_t i=0;i<config.input_channels;i++){
		LOG(INFO)<<"channel "<<i<<endl;
		for (dimension_t j=0;j<config.input_height;j++){
			for (dimension_t k=0;k<config.input_width;k++){
				channel_t m = config.num_of_ci_strides*(CI_STRIDE/NUM_OF_BYTES_PER_TRANSACTION);
				feature_block_index_t index = (j*config.input_width+k)*m + i/NUM_OF_BYTES_PER_TRANSACTION;
				LOG(INFO)<<setw(4)<<input_features[index].f[i%NUM_OF_BYTES_PER_TRANSACTION]<<" ";
			}
			LOG(INFO)<<endl;
		}
		LOG(INFO)<<endl;
	}
	LOG(INFO)<<endl;
#endif
}


int layer::CheckConvolutionResults(const char* filename){
	feature_t feature;
	int error_count = 0;
	feature_block_index_t index = 0;

	ifstream input(filename);
	if (!input.is_open()){
		LOG(ERROR)<<"failed to open "<<config.layer_name<<endl;
		return -1;
	}

#if DEBUG == 1
	LOG(INFO)<<"output features:"<<endl;
	feature_block_index_t idx = 0;
	for (dimension_t row=0;row<config.pooled_height;row++){
		for (dimension_t col=0;col<config.pooled_width;col++){
			for (int co_div=0;co_div<2*config.aligned_output_channels;co_div+=CI_STRIDE){
				for (channel_t co_idx=0;co_idx<CI_STRIDE;co_idx+=NUM_OF_BYTES_PER_TRANSACTION){
					for (channel_t nb=0;nb<NUM_OF_BYTES_PER_TRANSACTION;nb++){
						LOG(INFO)<<setw(4)<<output_features[idx].f[nb]<<" ";
					}
					idx++;
				}
			}
			LOG(INFO)<<endl;
		}
		LOG(INFO)<<endl;
	}
#endif

	LOG(CONSOLE)<<"checking results from "<<(unsigned long)output_features<<endl;

	if (config.pooling_type==AVG_POOLING){
		accumulator_t accumulator = 0;
		accumulator_t* output_results = (accumulator_t*)output_features;
		for (dimension_t row=0;row<config.pooled_height;row++){
			for (dimension_t col=0;col<config.pooled_width;col++){
				for (int co=0;co<config.aligned_output_channels;co++){
					input>>accumulator;
					if (accumulator!=output_results[index]){
						LOG(CONSOLE)<<"["<<row<<"]["<<col<<"]["<<co<<"]:";
						LOG(CONSOLE)<<" expected "<<accumulator;
						LOG(CONSOLE)<<" got "<<output_results[index]<<endl;
						LOG(INFO)<<"["<<row<<"]["<<col<<"]["<<co<<"]:";
						LOG(INFO)<<" expected "<<accumulator;
						LOG(INFO)<<" got "<<output_results[index]<<endl;
						error_count++;
					}
#if DEBUG == 1
					else{
						LOG(INFO)<<"["<<row<<"]["<<col<<"]["<<co<<"]:"<<feature<<endl;
					}
#endif
					index++;
				}
			}
		}
	}else if ((config.layer_type==EXPAND1x1)||(config.layer_type==EXPAND3x3)){
		for (dimension_t row=0;row<config.pooled_height;row++){
			for (dimension_t col=0;col<config.pooled_width;col++){
				for (int co_div=0;co_div<2*config.aligned_output_channels;co_div+=CI_STRIDE){
					for (channel_t co_idx=0;co_idx<CI_STRIDE;co_idx+=NUM_OF_BYTES_PER_TRANSACTION){
						for (channel_t nb=0;nb<NUM_OF_BYTES_PER_TRANSACTION;nb++){
							input>>feature;
							if ((feature!=output_features[index].f[nb]) && (((config.layer_type==EXPAND3x3)&&(co_div>=config.aligned_output_channels))||((config.layer_type==EXPAND1x1)&&(co_div<config.aligned_output_channels)))){
								LOG(CONSOLE)<<"["<<row<<"]["<<col<<"]["<<(co_div+co_idx+nb)<<"]:";
								LOG(CONSOLE)<<" expected "<<feature;
								LOG(CONSOLE)<<" got "<<output_features[index].f[nb]<<endl;
								LOG(INFO)<<"["<<row<<"]["<<col<<"]["<<(co_div+co_idx+nb)<<"]:";
								LOG(INFO)<<" expected "<<feature;
								LOG(INFO)<<" got "<<output_features[index].f[nb]<<endl;
								error_count++;
							}
		#if DEBUG == 1
							else{
								LOG(INFO)<<"["<<row<<"]["<<col<<"]["<<(co_div+co_idx+nb)<<"]:"<<feature<<endl;
							}
		#endif
						}
					}
					index++;
				}
			}
		}
	}else{
		for (dimension_t row=0;row<config.pooled_height;row++){
			for (dimension_t col=0;col<config.pooled_width;col++){
				for (int co_div=0;co_div<config.aligned_output_channels;co_div+=CI_STRIDE){
					for (channel_t co_idx=0;co_idx<CI_STRIDE;co_idx+=NUM_OF_BYTES_PER_TRANSACTION){
						for (channel_t nb=0;nb<NUM_OF_BYTES_PER_TRANSACTION;nb++){
							if ((co_div+co_idx+nb)<config.aligned_output_channels){
								input>>feature;
								if (feature!=output_features[index].f[nb]){
									LOG(CONSOLE)<<"["<<row<<"]["<<col<<"]["<<(co_div+co_idx+nb)<<"]:";
									LOG(CONSOLE)<<" expected "<<feature;
									LOG(CONSOLE)<<" got "<<output_features[index].f[nb]<<endl;
									LOG(INFO)<<"["<<row<<"]["<<col<<"]["<<(co_div+co_idx+nb)<<"]:";
									LOG(INFO)<<" expected "<<feature;
									LOG(INFO)<<" got "<<output_features[index].f[nb]<<endl;
									error_count++;
								}
#if DEBUG == 1
								else{
									LOG(INFO)<<"["<<row<<"]["<<col<<"]["<<(co_div+co_idx+nb)<<"]:"<<feature<<endl;
								}
#endif
							}
						}
						index++;
					}
				}
			}
		}
	}

	input.close();

	return error_count;
}
