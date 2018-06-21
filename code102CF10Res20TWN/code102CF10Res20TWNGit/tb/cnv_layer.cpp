#include"cnv_layer.hpp"


cnv_layer::~cnv_layer(){
}


cnv_layer::cnv_layer(const cnv_layer& cnv):layer(cnv){
}


cnv_layer& cnv_layer::operator=(const cnv_layer& cnv){
	LOG(CONSOLE)<<"invalid assignment"<<endl;
	return *this;
}


cnv_layer::cnv_layer(std::string name, layer_enum t, sublayer_t nsbl, sublayer_t sbl, dimension_t w,
		dimension_t h, bool fr, bool wr, bool br, weight_compress_t wc, channel_t ci, channel_t co,
		kernel_t k, pad_t p, stride_t s, bool r, pooling_enum pt, pooling_size_t psize, pooling_size_t ppad,
		pooling_stride_t pstride,feature_t factor,ibuf_enum ibufa, ibuf_enum ibufb, ibuf_enum ibufc,
		shift_t dpi,shift_t dpo, shift_t wpo, shift_t bpo
		):layer(name,t,nsbl,sbl,w,h,fr,wr,br,wc,ci,co,k,p,s,r,pt,psize,ppad,pstride,factor,ibufa,ibufb,ibufc,dpi,dpo,wpo,bpo){

	AllocateMemoryForWeightAndBias();
//	if(this->config.layer_name.compare("conv2") == 0){
//		AllocateMemoryForInputFeature();
//	}
}


void cnv_layer::TransformConfiguration(){
	LOG(CONSOLE)<<"Transform Configuration"<<endl;

	dimension_t transformed_intermediate_width = 1 + (config.input_width + 2*config.pad - config.kernel_size)/config.stride;
	dimension_t transformed_intermediate_height = 1 + (config.input_height + 2*config.pad - config.kernel_size)/config.stride;
	channel_t transformed_input_channels = config.kernel_size*config.kernel_size*config.input_channels;
	channel_t transformed_input_channel_blocks = ceil(1.0*transformed_input_channels/CI_STRIDE);

	config.pad = 0;
	config.stride = 1;
	config.input_width = transformed_intermediate_width;
	config.input_height = transformed_intermediate_height;
	config.num_of_ci_strides = transformed_input_channel_blocks;
	config.size_of_input_features = config.input_height*config.input_width*config.num_of_ci_strides*NUM_OF_BYTES_PER_TRANSACTION;

	channel_t original_input_channels = config.input_channels;
	filter_t original_filter_size = config.kernel_size*config.kernel_size;
	channel_t original_aligned_input_channels = config.aligned_input_channels;
	weight_block_index_t original_num_of_weight_blocks = config.num_of_weight_blocks;

	config.input_channels = config.kernel_size*config.kernel_size*config.input_channels;
	config.kernel_size = 1;
	config.aligned_input_channels = ceil(1.0*config.input_channels/CI_STRIDE)*CI_STRIDE;
	config.num_of_weight_blocks = config.kernel_size*config.kernel_size*config.aligned_input_channels*config.aligned_output_channels/NUM_OF_BYTES_PER_TRANSACTION;
	config.size_of_weights_and_bias = (config.num_of_weight_blocks+config.num_of_bias_blocks)*NUM_OF_BYTES_PER_TRANSACTION;
}


void cnv_layer::TransformFeatureMap(){
	LOG(CONSOLE)<<"Transform Featuremap"<<endl;

	dimension_t transformed_intermediate_width = 1 + (config.input_width + 2*config.pad - config.kernel_size)/config.stride;
	dimension_t transformed_intermediate_height = 1 + (config.input_height + 2*config.pad - config.kernel_size)/config.stride;
	channel_t transformed_input_channels = config.kernel_size*config.kernel_size*config.input_channels;
	channel_t transformed_input_channel_blocks = ceil(1.0*transformed_input_channels/CI_STRIDE);

	int num_of_feature_blocks = transformed_intermediate_width*transformed_intermediate_height*transformed_input_channel_blocks;
	feature_block_t* transformed_input_features = new feature_block_t[num_of_feature_blocks];
	if (transformed_input_features == NULL){
		LOG(ERROR)<<"Error: failed to allocate memory(transformed_input_features)"<<endl;
		return;
	}else{
		memset(transformed_input_features,0,sizeof(feature_block_t)*num_of_feature_blocks);
	}

	for (dimension_t row=0;row<transformed_intermediate_height;row++){
		for (dimension_t col=0;col<transformed_intermediate_width;col++){
			for (channel_t ci_div=0;ci_div<transformed_input_channels;ci_div+=CI_STRIDE){
				for (channel_t ci_idx=0;ci_idx<CI_STRIDE;ci_idx++){
					channel_t actual_ci = ci_div + ci_idx;
					feature_block_index_t fbindex = row*transformed_intermediate_width*transformed_input_channel_blocks + col*transformed_input_channel_blocks + actual_ci/CI_STRIDE;
					if (actual_ci<transformed_input_channels){
						filter_t original_filter = actual_ci / config.input_channels;
						kernel_t original_kx = original_filter % config.kernel_size;
						kernel_t original_ky = original_filter / config.kernel_size;
						dimension_t original_col = col*config.stride + original_kx - config.pad;
						assert((original_col>=0) && (original_col<config.input_width));
						dimension_t original_row = row*config.stride + original_ky - config.pad;
						assert((original_row>=0) && (original_row<config.input_height));
						feature_block_index_t bindex = original_row*config.input_width + original_col;
						transformed_input_features[fbindex].f[actual_ci%CI_STRIDE] = input_features[bindex].f[actual_ci % config.input_channels];
					}else{
						transformed_input_features[fbindex].f[actual_ci%CI_STRIDE] = 0;
					}
				}
			}
		}
	}

	UpdateMemoryForInputFeature(transformed_input_features);

	config.pad = 0;
	config.stride = 1;
	config.input_width = transformed_intermediate_width;
	config.input_height = transformed_intermediate_height;
	config.num_of_ci_strides = transformed_input_channel_blocks;
	config.size_of_input_features = config.input_height*config.input_width*config.num_of_ci_strides*NUM_OF_BYTES_PER_TRANSACTION;
}


void cnv_layer::TransformWeightsAndBias(){
	weight_t weight = 0;

	LOG(CONSOLE)<<"Transform Weights and Bias"<<endl;

	channel_t original_input_channels = config.input_channels;
	filter_t original_filter_size = config.kernel_size*config.kernel_size;
	channel_t original_aligned_input_channels = config.aligned_input_channels;
	weight_block_index_t original_num_of_weight_blocks = config.num_of_weight_blocks;

	config.input_channels = config.kernel_size*config.kernel_size*config.input_channels;
	config.kernel_size = 1;
	config.aligned_input_channels = ceil(1.0*config.input_channels/CI_STRIDE)*CI_STRIDE;
	config.num_of_weight_blocks = config.kernel_size*config.kernel_size*config.aligned_input_channels*config.aligned_output_channels/NUM_OF_BYTES_PER_TRANSACTION;
	config.size_of_weights_and_bias = (config.num_of_weight_blocks + config.num_of_bias_blocks)*NUM_OF_BYTES_PER_TRANSACTION;

	weight_block_t* new_weights = new weight_block_t[config.size_of_weights_and_bias/NUM_OF_BYTES_PER_TRANSACTION];
	if (new_weights==NULL){
		LOG(CONSOLE)<<"failed to allocate memory for new_weights"<<endl;
		return;
	}else{
		memset(new_weights,0,sizeof(weight_block_t)*config.size_of_weights_and_bias/NUM_OF_BYTES_PER_TRANSACTION);

		ifstream input("dataset/inweights.bin");
		if (!input.is_open()){
			LOG(CONSOLE)<<"failed to open inweights.bin"<<endl;
			return;
		}

		for (channel_t i=0;i<config.output_channels;i++){
			for (channel_t j=0;j<config.input_channels;j++){
					for (kernel_t k=0;k<config.kernel_size;k++){
						for (kernel_t l=0;l<config.kernel_size;l++){
							input>>weight;
							channel_t t = config.aligned_output_channels / NUM_OF_BYTES_PER_TRANSACTION;
							channel_t original_ci = j / original_filter_size;
							channel_t original_filter = j % original_filter_size;
							channel_t actual_ci = original_filter*original_input_channels + original_ci;
							weight_block_index_t weight_index = k*config.kernel_size + l;
							weight_index *= config.aligned_input_channels*t;
							weight_index += actual_ci*t + i / NUM_OF_BYTES_PER_TRANSACTION;
							weight_block_index_t w = i % NUM_OF_BYTES_PER_TRANSACTION;
							new_weights[weight_index].w[w] = weight;
					}
				}
			}
		}

		weight_block_index_t index = config.num_of_weight_blocks;
		for (channel_t co_div=0;co_div<config.aligned_output_channels;co_div+=NUM_OF_BYTES_PER_TRANSACTION){
			for (channel_t nb=0;nb<NUM_OF_BYTES_PER_TRANSACTION;nb++){
				if ((co_div+nb)<config.output_channels){
					input>>new_weights[index].w[nb];
				}else{
					new_weights[index].w[nb] = 0;
				}
			}
			index++;
		}

		input.close();

		UpdateMemoryForWeightAndBias(new_weights);
	}
}
