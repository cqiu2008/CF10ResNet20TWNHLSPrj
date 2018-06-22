#ifndef LAYER_HPP__
#define LAYER_HPP__

#include<iostream>
#include<fstream>
#include<string>
#include<cassert>
#include<cmath>

#include"../src/common.hpp"
#include"logger.hpp"

using namespace std;


class basic_config_t{
public:
	pad_t pad;
	bool relu;
	bool wreuse;
	bool breuse;
	bool freuse;
	shift_t dpi;
	shift_t dpo;
	shift_t wpo;
	shift_t bpo;
	sublayer_t sbl;
	sublayer_t nsbl;
	stride_t stride;
	kernel_t kernel_size;
	layer_enum layer_type;
	std::string layer_name;
	dimension_t input_width;
	dimension_t input_height;
	channel_t input_channels;
	channel_t output_channels;
	pooling_enum pooling_type;
	pooling_size_t pooling_pad;
	weight_compress_t wcompress;
	pooling_size_t pooling_size;
	pooling_stride_t pooling_stride;
};


class config_t:public basic_config_t{
public:
	int weight_offset;
	int input_feature_offset;
	int output_feature_offset;

	int size_of_input_features;
	int size_of_output_features;
	int size_of_weights_and_bias;

	dimension_t pooled_width;
	dimension_t pooled_height;
	channel_t num_of_ci_strides;
	channel_t aligned_input_channels;
	dimension_t output_width;
	dimension_t output_height;
	channel_t num_of_co_strides;
	channel_t aligned_output_channels;

	weight_block_index_t num_of_bias_blocks;
	weight_block_index_t num_of_weight_blocks;

	feature_t factor;
	ibuf_enum ibuf_type_a;
	ibuf_enum ibuf_type_b;
	ibuf_enum ibuf_type_c;
};


class layer {
public:
	virtual ~layer();
	layer(const layer& l);
	layer(std::string& name, layer_enum& t, sublayer_t& nsbl, sublayer_t& sbl, dimension_t& w,
			dimension_t& h, bool& fr, bool& wr, bool& br, weight_compress_t& wc, channel_t& ci, channel_t& co,
			kernel_t& k, pad_t& p, stride_t& s, bool& r, pooling_enum& pt, pooling_size_t& psize, pooling_size_t& ppad,
			pooling_stride_t& pstride,
			feature_t& factor,ibuf_enum& ibufa, ibuf_enum& ibufb, ibuf_enum& ibufc,
			shift_t& dpi,shift_t& dpo, shift_t& wpo, shift_t& bpo);

	void PrintLayer();
	void MakeInstructionGroup();

	virtual void CompressWeights(){}

//	virtual void TransformFeatureMap(){}
//	virtual void TransformConfiguration(){}
//	virtual void TransformWeightsAndBias(){}

	void LoadGeneratedBias(ifstream& input);
	void LoadGeneratedWeight(ifstream& input);

//	void LoadGeneratedFeatureMap(ifstream& input);

//	void UpdateMemoryForInputFeature(feature_block_t*);
//	void UpdateMemoryForWeightAndBias(weight_block_t*);
//	void UpdateMemoryForOutputFeature(feature_block_t*);

//	int CheckConvolutionResults(const char* filename);

protected:
	void AllocateMemoryForWeightAndBias();
//	void AllocateMemoryForInputFeature();

public:
	config_t config;
	weight_block_t* weights;
	feature_block_t* input_features;
	feature_block_t* output_features;
	struct instruction_group_t insts;

private:
	layer& operator=(const layer& l);
//	void ReleaseMemoryForWeightAndBias();
//	void ReleaseMemoryForInputFeature();
};


#endif
