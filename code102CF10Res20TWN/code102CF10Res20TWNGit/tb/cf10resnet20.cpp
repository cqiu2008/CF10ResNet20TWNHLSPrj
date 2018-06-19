#include "cf10resnet20.hpp"

cresnet20::cresnet20():network(20){
    LOG(CONSOLE)<<"cifar10 resnet20 construct"<<endl;
}

cresnet20::~cresnet20(){
    LOG(CONSOLE)<<"cifar10 resnet20 deconstruct"<<endl;
}

cresnet20::cresnet20(const cresnet20& cres20):network(cres20){
    LOG(CONSOLE)<<"cresnet20:: invalid copy constructor"<<endl;
}

cresnet20& cresnet20::operator=(const cresnet20& cres20){
    LOG(CONSOLE)<<"cresnet20:: invalid assignment"<<endl;
    return *this;
}
//SBL::SubLayer
//NSBL::Number of SubLayer
//BUFA,BUFB,BUFC,
//BUFn+2=W*BUFn+BUFn+1, n={A,B,C}
//BUFA,BUFB,BUFC={0:NADD,1:ADD,2:MULTIPLY,3:OUTPUT}
//CASE0:: BUFA=2,BUFB=0,BUFC=3 => BUFC=W*BUFA
//CASE1:: BUFA=2,BUFB=1,BUFC=3 => BUFC=W*BUFA+BUFB
//CASE2:: BUFA=0,BUFB=2,BUFC=3 => BUFC=W*BUFB
//CASE3:: BUFA=1,BUFB=2,BUFC=3 => BUFC=W*BUFB+BUFA
//CASE4:: ... ... Total 12 CASES
void cresnet20::CreateNetwork(){
	//Layer  (NAME,    Type,NSBL,SBL,   W,  H,freuse,wreuse,breuse,         WC,CI, CO,K,KPad,Stride, Relu,          PT,PSIZE,PPAD,PSTRIDE,  BUFA,   BUFB,   BUFC, DPI,DPO,WPO,BPO)
	AddLayer("conv2",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 16,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  M   ,   OUT ,   NADD,   4,  4,  7,  5);
	AddLayer("conv3",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 16,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  ADD ,   M   ,   OUT ,   4,  4,  7,  5);
	AddLayer("conv4",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 16,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  NADD,   OUT ,   M   ,   4,  5,  7,  5);
	AddLayer("conv5",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 16,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  OUT ,   M   ,   ADD ,   5,  3,  7,  5);
	AddLayer("conv6",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 16,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  M   ,   OUT ,   NADD,   3,  4,  7,  5);
	AddLayer("conv7",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 16,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  ADD ,   M   ,   OUT ,   4,  4,  7,  5);
	AddLayer("conv8",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 32,1,   0,     2,false, MAX_POOLING,    0,   0,      0,  NADD,   OUT ,   M   ,   4,  4,  7,  5);
	AddLayer("conv9",  CNV,    1,  0,  32, 32, false, false, false, UNCOMPRESS,16, 32,3,   1,     2, true, MAX_POOLING,    0,   0,      0,  OUT ,   NADD,   M   ,   4,  4,  7,  5);
	AddLayer("conv10", CNV,    1,  0,  16, 16, false, false, false, UNCOMPRESS,32, 32,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  M   ,   ADD ,   OUT ,   4,  4,  7,  5);
	AddLayer("conv11", CNV,    1,  0,  16, 16, false, false, false, UNCOMPRESS,32, 32,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  NADD,   OUT ,   M   ,   4,  4,  7,  5);
	AddLayer("conv12", CNV,    1,  0,  16, 16, false, false, false, UNCOMPRESS,32, 32,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  OUT ,   M   ,   ADD ,   4,  4,  7,  5);
	AddLayer("conv13", CNV,    1,  0,  16, 16, false, false, false, UNCOMPRESS,32, 32,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  M   ,   OUT ,   NADD,   4,  4,  7,  5);
	AddLayer("conv14", CNV,    1,  0,  16, 16, false, false, false, UNCOMPRESS,32, 32,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  ADD ,   M   ,   OUT ,   4,  4,  7,  5);
	AddLayer("conv15", CNV,    1,  0,  16, 16, false, false, false, UNCOMPRESS,32, 64,1,   0,     2,false, MAX_POOLING,    0,   0,      0,  NADD,   OUT ,   M   ,   4,  4,  7,  5);
	AddLayer("conv16", CNV,    1,  0,  16, 16, false, false, false, UNCOMPRESS,32, 64,3,   1,     2, true, MAX_POOLING,    0,   0,      0,  OUT ,   NADD,   M   ,   4,  4,  7,  5);
	AddLayer("conv17", CNV,    1,  0,   8,  8, false, false, false, UNCOMPRESS,64, 64,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  M   ,   ADD ,   OUT ,   4,  4,  7,  5);
	AddLayer("conv18", CNV,    1,  0,   8,  8, false, false, false, UNCOMPRESS,64, 64,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  NADD,   OUT ,   M   ,   4,  4,  7,  5);
	AddLayer("conv19", CNV,    1,  0,   8,  8, false, false, false, UNCOMPRESS,64, 64,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  OUT ,   M   ,   ADD ,   4,  4,  7,  5);
	AddLayer("conv20", CNV,    1,  0,   8,  8, false, false, false, UNCOMPRESS,64, 64,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  M   ,   OUT ,   NADD,   4,  4,  7,  5);
	AddLayer("conv21", CNV,    1,  0,   8,  8, false, false, false, UNCOMPRESS,64, 64,3,   1,     1, true, MAX_POOLING,    0,   0,      0,  ADD ,   M   ,   OUT ,   4,  4,  7,  5);
}
