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


layer::layer(std::string& name, layer_enum& t, sublayer_t& nsbl, sublayer_t& sbl, dimension_t& w,
		dimension_t& h, bool& fr, bool& wr, bool& br, weight_compress_t& wc, channel_t& ci, channel_t& co,
		kernel_t& k, pad_t& p, stride_t& s, bool& r, pooling_enum& pt, pooling_size_t& psize, pooling_size_t& ppad,
		pooling_stride_t& pstride,
		feature_t& factor,ibuf_enum& ibufa, ibuf_enum& ibufb, ibuf_enum& ibufc,
		shift_t& dpi,shift_t& dpo, shift_t& wpo, shift_t& bpo){

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
	assert((psize>=1) && (psize<=MAX_POOLING_SIZE));

	if (psize==1){
		assert(pstride==1);
	}

	if (pstride==1){
		assert(psize==1);
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
	config.aligned_output_channels = ceil(1.0*config.output_channels/CO_STRIDE)*CO_STRIDE;

	config.output_width = 1 + (config.input_width + 2*config.pad - config.kernel_size)/config.stride;
	config.output_height = 1 + (config.input_height + 2*config.pad - config.kernel_size)/config.stride;

	config.pooled_width = 1 + (config.output_width + 2*config.pooling_pad - config.pooling_size)/config.pooling_stride;
	config.pooled_height = 1 + (config.output_height + 2*config.pooling_pad - config.pooling_size)/config.pooling_stride;

	config.num_of_bias_blocks = config.aligned_output_channels/NUM_OF_BYTES_PER_TRANSACTION;
	assert(config.num_of_bias_blocks < MAX_NUM_OF_BIAS_BLOCKS);

	config.num_of_weight_blocks = config.kernel_size*config.kernel_size*config.aligned_input_channels*config.aligned_output_channels/(TWN_ONE_BYTE_DOTS*NUM_OF_BYTES_PER_TRANSACTION);
	assert(config.num_of_weight_blocks < MAX_NUM_OF_WEIGHT_BLOCKS);

	assert(config.kernel_size*config.kernel_size*config.num_of_ci_strides*config.num_of_co_strides<WEIGHT_BUF_DEPTH);


	if(config.layer_name.compare("conv2") == 0){
		config.size_of_input_features = BATCH_NUM * config.input_height*config.input_width*config.aligned_input_channels;
	}else{
		config.size_of_input_features = config.input_height*config.input_width*config.aligned_input_channels;
	}

	if (config.pooling_type==AVG_POOLING){
		config.size_of_output_features = 4*(config.pooled_height*config.pooled_width*config.aligned_output_channels);
	}else if ((config.layer_type==EXPAND3x3) || (config.layer_type==EXPAND1x1)){
		config.size_of_output_features = 2*(config.pooled_height*config.pooled_width*config.aligned_output_channels);
	}else{
		config.size_of_output_features = (config.pooled_height*config.pooled_width*config.aligned_output_channels);
	}
	config.size_of_weights_and_bias = (config.num_of_bias_blocks+config.num_of_weight_blocks)*NUM_OF_BYTES_PER_TRANSACTION;

	config.factor = factor;
	config.ibuf_type_a = ibufa;
	config.ibuf_type_b = ibufb;
	config.ibuf_type_c = ibufc;

	LOG(CONSOLE)<<config.layer_name<<endl;
	LOG(CONSOLE)<<"config.num_of_weight_blocks="<<config.num_of_weight_blocks<<endl;
	LOG(CONSOLE)<<"config.num_of_bias_blocks="<<config.num_of_bias_blocks<<endl;

	if (config.pooling_size>config.pooling_stride){
		LOG(CONSOLE)<<config.layer_name<<": pooling_size>pooling_stride which might introduce redundant computing"<<endl;
	}

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
	//resnet instruction format
	//8bits: factor
	//2bits: ibuf_type_a
	//2bits: ibuf_type_b
	//2bits: ibuf_type_c
	insts.ires = 0;
	insts.ires |= ((int(config.ibuf_type_c))<<12);
	insts.ires |= ((int(config.ibuf_type_b))<<10);
	insts.ires |= ((int(config.ibuf_type_a))<<8);
	insts.ires |= (int(config.factor & 0xff));
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
	int num_weights_bias = config.size_of_weights_and_bias/NUM_OF_BYTES_PER_TRANSACTION;
	weights = new weight_block_t[num_weights_bias];
	if (weights == NULL){
		LOG(ERROR)<<"Error: failed to allocate memory(weights+bias)"<<endl;
	}else{
		memset(weights,0,sizeof(weight_block_t)*num_weights_bias);
	}
}

void layer::LoadGeneratedWeight(ifstream& input){
	weight_t weight = 0;
	int num = config.kernel_size*config.kernel_size*config.aligned_input_channels;
    int twn_output_channels=config.output_channels/TWN_ONE_BYTE_DOTS;
    int twn_aligned_output_channels=config.aligned_output_channels/TWN_ONE_BYTE_DOTS;

	LOG(CONSOLE)<<"LoadGenerateWeight Num="<<num<<endl;
	for(int i=0;i<num;i++){
        for(int j=0;j<twn_aligned_output_channels;j++){
		    weight_block_index_t w = j%NUM_OF_BYTES_PER_TRANSACTION;
		    int index = (i*twn_aligned_output_channels+j)/NUM_OF_BYTES_PER_TRANSACTION;
            if(j<twn_output_channels){
		        input>>weight;
		        weights[index].w[w] = weight;
            }else{
		        weights[index].w[w] = 0; 
            }
		    LOG(INFO)<<setw(4)<<weights[index].w[w]<<" ";
		    if(  (w+1) % (NUM_OF_BYTES_PER_TRANSACTION/2) == 0 ){
		    	LOG(INFO)<<endl;
		    }
        }
	}
}
void layer::LoadGeneratedBias(ifstream& input){
	weight_block_index_t index = config.num_of_weight_blocks;
	weight_block_index_t indexNew =0;
	weight_t weight = 0;
	int num = config.aligned_output_channels ;
	LOGNO(CONSOLE)<<"LoadGeneratedBias Num="<<num<<endl;
	for(int i=0;i<num;i++){
		weight_block_index_t w = i%NUM_OF_BYTES_PER_TRANSACTION;
		indexNew = index + (i/NUM_OF_BYTES_PER_TRANSACTION);
        if(i<config.output_channels){
		    input>>weight;
		    weights[indexNew].w[w] = weight;
        }else{
		    weights[indexNew].w[w] = 0;
        }
		LOG(INFO)<<setw(4)<<weights[indexNew].w[w]<<" ";
		if(w == (NUM_OF_BYTES_PER_TRANSACTION-1)){
			LOG(INFO)<<endl;
		}
	}
}



void layer::AllocateMemoryForInputFeature(){
	input_features = new feature_block_t[config.size_of_input_features/NUM_OF_BYTES_PER_TRANSACTION];
	if (input_features == NULL){
		LOG(ERROR)<<"Error: failed to allocate memory(input_features)"<<endl;
	}else{
		memset(input_features,0,sizeof(feature_block_t)*config.size_of_input_features/NUM_OF_BYTES_PER_TRANSACTION);
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


	LOG(INFO)<<"load generated feature:"<<endl;

	int num = config.input_height * config.input_width * config.input_channels;

	for (int i = 0;i < num;i++){
		for (channel_t m=0;m<BATCH_NUM;m++){
			input>>input_features[i].f[m];
			LOG(INFO)<<setw(4)<<input_features[i].f[m]<<" ";
		}
		LOG(INFO)<<endl;
	}
}


