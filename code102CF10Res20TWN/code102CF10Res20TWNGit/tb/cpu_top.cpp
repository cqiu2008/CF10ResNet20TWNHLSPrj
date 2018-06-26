//----------------------------------------------------------------
//  FPGA Accelerator For CNN Inference
//----------------------------------------------------------------
//
//  File:   cpu_top.cpp
//  CPU-Side Functions for FPGA Accelerator
//
//  (c) hrt_fpga , 2018-06
//
//----------------------------------------------------------------
#include "cpu_top.hpp"


int main(){
    int ret = 0;
    LOG(CONSOLE)<<"Starting to test the cnn inference accelerator"<<endl;
	assert((MAX_CHANNEL_NUM%CO_STRIDE)==0);
	assert((MAX_KERNEL_SIZE & (MAX_KERNEL_SIZE-1))==0);
	assert((CO_STRIDE%NUM_OF_BYTES_PER_TRANSACTION)==0);
	assert(CI_STRIDE*CO_STRIDE<=MAX_NUM_OF_DSP_AVAILABLE);
	LOG(CONSOLE)<<"fully connected layer is not implemented"<<endl;
    timeval start,end;
    network *net = new cresnet20();
    net->CreateNetwork();
    assert((STARTING_LAYER>=0) && (STARTING_LAYER<net->GetNumOfLayers()));
    net->LoadFeatureMap(STARTING_LAYER);
    net->LoadWeightAndBias();
    LOG(CONSOLE)<<"finished LoadWeightAndBias"<<endl;
    net->StatMemoryUsage();
    XFPGA_Initialize(net);
	LOG(CONSOLE)<<"ready to copy weight to SharedDRAM"<<endl;
	copyWeightsToSharedDRAM(net);
	LOG(CONSOLE)<<"ready to copy features to SharedDRAM"<<endl;
	copyImageToSharedDRAM(net);

#if 1
	for(numlayers_t i=0;i<net->GetNumOfLayers();i++){
		LOG(CONSOLE)<<"computing layer "<<i<<" : "<<net->GetLayer(i)->config.layer_name<<endl;
		//LOG(CONSOLE)<<"CPU: inf="<<(unsigned long)net->GetLayer(i)->input_features;
		//LOG(CONSOLE)<<" outf="<<(unsigned long)net->GetLayer(i)->output_features;
		//LOG(CONSOLE)<<" w="<<(unsigned long)net->GetLayer(i)->weights<<endl;
		net->GetLayer(i)->MakeInstructionGroup();
		gettimeofday(&start,NULL);
		XFPGA_Run(net->GetLayer(i));
		gettimeofday(&end,NULL);
		uint64_t elapsedus = (end.tv_usec - start.tv_usec) + ((uint64_t)(end.tv_sec-start.tv_sec))*1000000;
		LOG(CONSOLE)<<"computation duration : "<<(1.0*elapsedus/1000)<<"ms"<<endl;
		LOG(CONSOLE)<<"check results for layer "<<i<<" : "<<net->GetLayer(i)->config.layer_name<<endl;
	}
    XFPGA_Release();
#endif

    LOG(CONSOLE)<<"Ending test the cnn inference accelerator"<<endl;
    return ret ;
}
