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
//    timeval start,end;
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


    int a=4,b=5;
    int c;
    Accelerator(a,b,c);
    LOG(CONSOLE)<<"c="<<c<<endl;

    XFPGA_Release();

    LOG(CONSOLE)<<"Ending test the cnn inference accelerator"<<endl;
    return ret ;
}
