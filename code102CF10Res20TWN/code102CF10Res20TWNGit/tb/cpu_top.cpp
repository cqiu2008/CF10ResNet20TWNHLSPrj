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
    LOG(CONSOLE)<<"Starting to test the cnn inference accelerator1"<<endl;

//    timeval start,end;
    network *net = new cresnet20();
    net->CreateNetwork();
    LOGNO(CONSOLE)<<"Ending test the cnn inference accelerator1"<<endl;

    return ret ;
}
