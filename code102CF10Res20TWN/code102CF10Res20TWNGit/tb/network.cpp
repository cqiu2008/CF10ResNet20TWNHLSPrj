#include"network.hpp"

using namespace std;


network::network(int nlayers){
	LOG(CONSOLE)<<"network construct"<<endl;
	num_of_layers = 0;
	size_of_weights = 0;
	size_of_features = 0;
	num_of_layers_combined = 0;
	assert((nlayers>0)&&(nlayers<MAX_NUM_OF_LAYERS));
	layers = new layer*[nlayers];
	if (layers==NULL) LOG(CONSOLE)<<"failed to allocate memory for layers"<<endl;
}


network::network(const network& net){
	num_of_layers = 0;
	size_of_weights = 0;
	size_of_features = 0;
	num_of_layers_combined = 0;
	LOG(CONSOLE)<<"invalid copy constructor"<<endl;
}


network& network::operator=(const network& net){
	LOG(CONSOLE)<<"invalid assignment"<<endl;
	return *this;
}


network::~network(){
	LOG(CONSOLE)<<"network deconstruct"<<endl;
	if (layers!=NULL){
		for (numlayers_t l=0;l<num_of_layers;l++){
			delete layers[l];
			layers[l] = NULL;
		}
		delete[] layers;
	}
	num_of_layers = 0;
	size_of_weights = 0;
	size_of_features = 0;
	num_of_layers_combined = 0;
}


void network::PrintNetwork() {
	LOG(CONSOLE)<<"NAME,\t";
	LOG(CONSOLE)<<"Type,\t";
	LOG(CONSOLE)<<"IW,\t";
	LOG(CONSOLE)<<"IH,\t";
	LOG(CONSOLE)<<"OW,\t";
	LOG(CONSOLE)<<"OH,\t";
	LOG(CONSOLE)<<"CI,\t";
	LOG(CONSOLE)<<"CO,\t";
	LOG(CONSOLE)<<"Kernel,\t";
	LOG(CONSOLE)<<"Pad,\t";
	LOG(CONSOLE)<<"Stride,\t";
	LOG(CONSOLE)<<"Relu,\t";
	LOG(CONSOLE)<<"PT,\t";
	LOG(CONSOLE)<<"PSIZE,\t";
	LOG(CONSOLE)<<"PPAD,\t";
	LOG(CONSOLE)<<"PSTRIDE,\t";
	LOG(CONSOLE)<<"BUFA,\t";
	LOG(CONSOLE)<<"BUFB,\t";
	LOG(CONSOLE)<<"BUFC,\t";
	LOG(CONSOLE)<<"DPI,\t";
	LOG(CONSOLE)<<"DPO,\t";
	LOG(CONSOLE)<<"WPO,\t";
	LOG(CONSOLE)<<"BPO\t"<<endl;
}


void network::AppendLayer(layer* l){
	layers[num_of_layers] = l;
	num_of_layers++;
}


void network::ReleaseLayer(int pos){
	assert((pos>=0)&&(pos<num_of_layers));
	delete layers[pos];
	layers[pos] = NULL;
}


void network::RemoveLayer(int l){
	for (int i=l;i<num_of_layers-1;i++){
		layers[i] = layers[i+1];
	}
	layers[num_of_layers-1] = NULL;
	num_of_layers--;
}


void network::UpdateLayer(int pos, layer* l){
	assert((pos>=0)&&(pos<num_of_layers));
	layers[pos] = l;
}


void network::AddLayer(const char* name, layer_enum t, sublayer_t nsbl, sublayer_t sbl, dimension_t w,
		dimension_t h, bool fr, bool wr, bool br, weight_compress_t wc, channel_t ci, channel_t co,
		kernel_t k, pad_t p, stride_t s, bool r, pooling_enum pt, pooling_size_t psize, pooling_size_t ppad,
		pooling_stride_t pstride, ibuf_enum ibufa, ibuf_enum ibufb, ibuf_enum ibufc,
		shift_t dpi,shift_t dpo, shift_t wpo, shift_t bpo) {

	assert((t>=0) && (t<NUM_OF_LAYER_TYPE));
	switch(t){
	case CNV:
	case EXPAND1x1:
	case EXPAND3x3:
		AppendLayer(new cnv_layer(name,t,nsbl,sbl,w,h,fr,wr,br,wc,ci,co,k,p,s,r,pt,psize,ppad,pstride,ibufa,ibufb,ibufc,dpi,dpo,wpo,bpo));
		break;
	default:
		break;
	}
};


bool network::LoadWeightAndBias(const char* filename){
	ifstream input(filename);
	if (!input.is_open()){
		LOG(ERROR)<<"failed to open "<<filename<<endl;
		return false;
	}

	for (numlayers_t i=0;i<num_of_layers;i++){
		LOG(CONSOLE)<<"loading weight for layer "<<i<<endl;
		layers[i]->LoadGeneratedWeight(input);
		LOG(CONSOLE)<<"loading bias for layer "<<i<<endl;
		layers[i]->LoadGeneratedBias(input);
	}

	input.close();

	return true;
}


bool network::LoadFeatureMap(numlayers_t first_running_layer){
	ifstream input;
	if (first_running_layer>0){
		numlayers_t loading_layer = first_running_layer - 1;
		if (layers[first_running_layer]->config.layer_type==EXPAND3x3){
			loading_layer--;
		}
		input.open(("./outf/"+layers[loading_layer]->config.layer_name).c_str());
		if (!input.is_open()){
			LOG(ERROR)<<"failed to open "<<layers[loading_layer]->config.layer_name<<endl;
			return false;
		}
	}else{
		input.open("./dataset/infeatures.bin");
		if (!input.is_open()){
			LOG(ERROR)<<"failed to open infeatures.bin"<<endl;
			return false;
		}
	}

	layers[first_running_layer]->LoadGeneratedFeatureMap(input);

	input.close();

	return true;
}
