#ifndef NETWORK_HPP__
#define NETWORK_HPP__

#include<iostream>
#include"logger.hpp"
#include"layer.hpp"
#include"cnv_layer.hpp"

using namespace std;


class network {
public:
	  network(int num);
	  virtual ~network();
	  network(const network& net);

	  void PrintNetwork();

	  virtual void OptimizeNetwork(){}

	  virtual void CreateNetwork() = 0;

	  virtual void StatMemoryUsage() = 0;

	  bool LoadWeightAndBias();
//	  bool LoadFeatureMap(numlayers_t first_running_layer);

	  numlayers_t GetNumOfLayers(){return num_of_layers;}

	  layer* GetLayer(int l){return layers[l];}

	  int GetSizeOfWeights(){return size_of_weights;}
	  void IncreaseSizeOfWeights(int inc){size_of_weights+=inc;}

	  int GetSizeOfFeatures(){return size_of_features;}
	  void IncreaseSizeOfFeatures(int inc){size_of_features+=inc;}

	  numlayers_t GetStartingLayer(){return STARTING_LAYER-num_of_layers_combined;}

protected:
	  layer **layers;
	  numlayers_t num_of_layers;

	  void RemoveLayer(int pos);
	  void AppendLayer(layer* l);
	  void ReleaseLayer(int pos);
	  void UpdateLayer(int pos, layer* l);

	  void IncreaseNumOfLayersCombined(){num_of_layers_combined++;}

	  virtual void CombineTwoLayers(numlayers_t,numlayers_t,numlayers_t){}

	  numlayers_t GetNumOfLayersCombined(){return num_of_layers_combined;}

	  void AddLayer(std::string  name, layer_enum t, sublayer_t nsbl, sublayer_t sbl,
			  dimension_t w, dimension_t h, bool fr, bool wr, bool br, weight_compress_t wc,
			  channel_t ci, channel_t co, kernel_t k, pad_t p, stride_t s, bool r, pooling_enum pt,
			  pooling_size_t psize, pooling_size_t ppad, pooling_stride_t pstride,
			  feature_t factor,ibuf_enum ibufa, ibuf_enum ibufb, ibuf_enum ibufc,
			  shift_t dpi,shift_t dpo, shift_t wpo, shift_t bpo);

private:
	  network& operator=(const network& net);

private:
	  int size_of_weights;
	  int size_of_features;
	  numlayers_t num_of_layers_combined;
};


#endif
