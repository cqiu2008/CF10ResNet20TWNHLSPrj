#include"cnv_layer.hpp"


cnv_layer::~cnv_layer(){
}


cnv_layer::cnv_layer(const cnv_layer& cnv):layer(cnv){
}


cnv_layer& cnv_layer::operator=(const cnv_layer& cnv){
	LOG(CONSOLE)<<"invalid assignment"<<endl;
	return *this;
}


cnv_layer::cnv_layer(std::string& name, layer_enum& t, sublayer_t& nsbl, sublayer_t& sbl, dimension_t& w,
		dimension_t& h, bool& fr, bool& wr, bool& br, weight_compress_t& wc, channel_t& ci, channel_t& co,
		kernel_t& k, pad_t& p, stride_t& s, bool& r, pooling_enum& pt, pooling_size_t& psize, pooling_size_t& ppad,
		pooling_stride_t& pstride,
		feature_t& factor,ibuf_enum& ibufa, ibuf_enum& ibufb, ibuf_enum& ibufc,
		shift_t& dpi,shift_t& dpo, shift_t& wpo, shift_t& bpo
		):layer(name,t,nsbl,sbl,w,h,fr,wr,br,wc,ci,co,k,p,s,r,pt,psize,ppad,pstride,factor,ibufa,ibufb,ibufc,dpi,dpo,wpo,bpo){

	AllocateMemoryForWeightAndBias();
//	if(this->config.layer_name.compare("conv2") == 0){
//		AllocateMemoryForInputFeature();
//	}
}
