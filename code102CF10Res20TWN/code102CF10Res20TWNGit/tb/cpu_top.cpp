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




    LOG(CONSOLE)<<"Ending test the cnn inference accelerator"<<endl;
    return ret ;
}
