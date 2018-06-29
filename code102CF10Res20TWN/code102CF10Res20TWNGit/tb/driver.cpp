#include"driver.hpp"


#if RUN_FOR_SIMULATION!=1


static int AXILITE_FD = -1;
static int AXICDMA_FD = -1;
static int SHARED_DRAM_FD = -1;

volatile uint32_t* AXILITE_BUS = NULL;
volatile uint32_t* AXILITECDMA_BUS  = NULL;
volatile char* SHARED_DRAM_PTR = NULL;


#define axilite_write(addr,val)	do{AXILITE_BUS[addr/4]=val;}while(0);
#define axilite_read(addr)	(AXILITE_BUS[addr/4])

#define axilite_cdma_write(addr,val)	do{AXILITECDMA_BUS[addr/4]=val;}while(0);
#define axilite_cdma_read(addr)	(AXILITECDMA_BUS[addr/4])

#define XFPGA_ISSTART() (axilite_read(XACCELERATOR_AXILITE_ADDR_AP_CTRL) & 0x1)
#define XFPGA_IsDone() ((axilite_read(XACCELERATOR_AXILITE_ADDR_AP_CTRL)>>1)&0x1)
#define XFPGA_IsIdle() (((axilite_read(XACCELERATOR_AXILITE_ADDR_AP_CTRL))>>2)&0x1)
#define XFPGA_IsReady() (!((axilite_read(XACCELERATOR_AXILITE_ADDR_AP_CTRL))&0x1))


__attribute__((always_inline))
static inline volatile uint32_t* map_axilite_bus(off_t base_addr) {
	// make sure that base addr is aligned to memory pages...
	base_addr &= ~(getpagesize() - 1);
	AXILITE_FD = open("/dev/mem", O_RDWR);
	if (AXILITE_FD < 0) err(errno, "could not open /dev/mem. need to be root");
	// Map AXILITE memory region to pointer
	volatile uint32_t* axilite = (volatile uint32_t*)mmap(NULL, AXILITE_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, AXILITE_FD, base_addr);
	if (axilite == MAP_FAILED) err(errno, "could not map memory for axilite bus");
	LOG(CONSOLE)<<"axilite_bus address : "<<(unsigned long)axilite<<endl;
	return axilite;
}


__attribute__((always_inline))
static inline volatile uint32_t* map_axilite_cdma_bus(off_t base_addr) {
	// make sure that base addr is aligned to memory pages...
	base_addr &= ~(getpagesize() - 1);
	AXICDMA_FD = open("/dev/mem", O_RDWR);
	if (AXICDMA_FD < 0) err(errno, "could not open /dev/mem. need to be root");
	// Map AXILITE memory region to pointer
	volatile uint32_t* axilite = (volatile uint32_t*)mmap(NULL, AXILITE_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, AXICDMA_FD, base_addr);
	if (axilite == MAP_FAILED) err(errno, "could not map memory for axilite bus");
	LOG(CONSOLE)<<"axilite cdma address : "<<(unsigned long)axilite<<endl;
	return axilite;
}


__attribute__((always_inline))
static inline volatile char* map_shared_dram(off_t base_addr) {
	// make sure that base addr is aligned to memory pages...
	base_addr &= ~(getpagesize() - 1);
	SHARED_DRAM_FD = open("/dev/mem", O_RDWR);
	if (SHARED_DRAM_FD < 0) err(errno, "could not open /dev/mem. need to be root");
	volatile char* pointer = (volatile char*)mmap(NULL, SHARED_DRAM_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, SHARED_DRAM_FD, base_addr);
	if (pointer == MAP_FAILED) err(errno, "could not map memory for SHARED_DRAM bus");
	LOG(CONSOLE)<<"shared dram address : "<<(unsigned long)pointer<<endl;
	return pointer;
}


__attribute__((always_inline))
static inline void release_axilite_bus(volatile uint32_t* axilite) {
	// Release AXILITE memory region (unmap)
	int retval = munmap((void*)axilite, AXILITE_MEM_SIZE);
	if (retval < 0) err(errno, "could not unmap memory region for axilite bus");
	// release file handle
	retval = close(AXILITE_FD);
	if (retval < 0) err(errno, "could not release /dev/mem file handle");
	// set file handle variable s.t. we know it's closed
	AXILITE_FD = -1;
}


__attribute__((always_inline))
static inline void release_axicdma_bus(volatile uint32_t* axicdma)  {
	int retval = munmap((void*)axicdma, AXILITE_MEM_SIZE);
	if (retval < 0) err(errno, "could not unmap memory region for axilite bus");
	retval = close(AXICDMA_FD);
	if (retval < 0) err(errno, "could not release /dev/mem file handle");
	// set file handle variable s.t. we know it's closed
	AXICDMA_FD = -1;
}


__attribute__((always_inline))
static inline void release_shared_dram(volatile char* pointer) {
	// Release SHARED_DRAM memory region (unmap)
	int retval = munmap((void*)pointer, SHARED_DRAM_MEM_SIZE);
	if (retval < 0) err(errno, "could not unmap memory region for SHARED_DRAM bus");
	// release file handle
	retval = close(SHARED_DRAM_FD);
	if (retval < 0) err(errno, "could not release /dev/mem file handle");
	// set file handle variable s.t. we know it's closed
	SHARED_DRAM_FD = -1;
}


__attribute__((always_inline))
static inline bool axilite_open() {
	// Check that it's not yet open
	if (AXILITE_FD > -1) {
		LOG(CONSOLE)<<"axilite bus already open!"<<endl;
		return false;
	}
	// Memory Map Axilite Bus
	AXILITE_BUS = map_axilite_bus(AXILITE_BASE_ADDR);
	return (AXILITE_FD > -1);
}


__attribute__((always_inline))
static inline bool axicdma_open() {
	if (AXICDMA_FD > -1) {
		LOG(CONSOLE)<<"axicdma bus already open!"<<endl;
		return false;
	}
	if (AXICDMA_FD>-1) LOG(CONSOLE)<<"axicdma_open::map_axilite_bus success"<<endl;
	AXILITECDMA_BUS = map_axilite_cdma_bus(AXICDMA_BASE_ADDR);
	return (AXICDMA_FD > -1);
}


__attribute__((always_inline))
static inline bool shared_dram_open() {
	// Check that it's not yet open
	if (SHARED_DRAM_FD > -1) {
		LOG(CONSOLE)<<"SHARED_DRAM already open!"<<endl;
		return false;
	}
	// Memory Map SHARED_DRAM
	SHARED_DRAM_PTR = map_shared_dram(SHARED_DRAM_BASE_ADDR);
	// Make sure the file handle is really set
	if (SHARED_DRAM_PTR == MAP_FAILED) err(errno, "SHARED_DRAM_open::map_SHARED_DRAM failed.\n");
	return (SHARED_DRAM_FD > -1);
}


__attribute__((always_inline))
static inline bool shared_dram_close() {
	// Check that memory file is really open
	if (SHARED_DRAM_FD == -1) {
		LOG(CONSOLE)<<"SHARED_DRAM bus not open!"<<endl;
		return false;
	}
	// Release Memory Region and File handle
	release_shared_dram(SHARED_DRAM_PTR);
	// Make sure file was correctly released
	return (SHARED_DRAM_FD == -1);
}


__attribute__((always_inline))
static inline bool axicdma_close() {
	if (AXICDMA_FD == -1) {
		LOG(CONSOLE)<<"axilite cdma bus not open!"<<endl;
		return false;
	}
	release_axicdma_bus(AXILITECDMA_BUS);
	return (AXICDMA_FD == -1);
}


__attribute__((always_inline))
static inline bool axilite_close() {
	// Check that memory file is really open
	if (AXILITE_FD == -1) {
		LOG(CONSOLE)<<"axilite bus not open!"<<endl;
		return false;
	}
	// Release Memory Region and File handle
	release_axilite_bus(AXILITE_BUS);
	// Make sure file was correctly released
	return (AXILITE_FD == -1);
}


static void XFPGA_SetLayerConfig(layer_t &layer){
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_PAD_V_DATA, layer.config.pad);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_RELU_DATA, layer.config.relu);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_STRIDE_V_DATA, layer.config.stride);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_KERNEL_SIZE_V_DATA, layer.config.kernel_size);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_LAYER_TYPE_DATA, layer.config.layer_type);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_DPI_V_DATA, layer.config.dpi);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_DPO_V_DATA, layer.config.dpo);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_WPO_V_DATA, layer.config.wpo);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_BPO_V_DATA, layer.config.bpo);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_TYPE_DATA, layer.config.pooling_type);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_PAD_V_DATA, layer.config.pooling_pad);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_SIZE_V_DATA, layer.config.pooling_size);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_STRIDE_V_DATA, layer.config.pooling_stride);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_POOLED_WIDTH_V_DATA, layer.config.pooled_width);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_POOLED_HEIGHT_V_DATA, layer.config.pooled_height);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_INPUT_WIDTH_V_DATA, layer.config.input_width);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_INPUT_HEIGHT_V_DATA, layer.config.input_height);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_INPUT_CHANNELS_V_DATA, layer.config.input_channels);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_CI_STRIDES_V_DATA, layer.config.num_of_ci_strides);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_ALIGNED_INPUT_CHANNELS_V_DATA, layer.config.aligned_input_channels);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_WIDTH_V_DATA, layer.config.output_width);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_HEIGHT_V_DATA, layer.config.output_height);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_CHANNELS_V_DATA, layer.config.output_channels);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_CO_STRIDES_V_DATA, layer.config.num_of_co_strides);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_ALIGNED_OUTPUT_CHANNELS_V_DATA, layer.config.aligned_output_channels);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_CIXCO_STRIDES_V_DATA, layer.config.num_of_cixco_strides);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_WIDTH_CI_STRIDES_V_DATA, layer.config.num_of_width_ci_strides);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_OUTPUT_POINTS_V_DATA, layer.config.num_of_output_points);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_POOLING_POINTS_V_DATA, layer.config.num_of_pooling_points);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_WIDTH_DIV_PSTRIDE_V_DATA, layer.config.output_width_div_pstride);
	axilite_write(XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_HEIGHT_DIV_PSTRIDE_V_DATA, layer.config.output_height_div_pstride);
}


void XFPGA_InterruptGlobalEnable() {
	axilite_write(XACCELERATOR_AXILITE_ADDR_GIE, 1);
}


void XFPGA_InterruptGlobalDisable() {
	axilite_write(XACCELERATOR_AXILITE_ADDR_GIE, 0);
}


void XFPGA_InterruptEnable(uint32_t Mask) {
	uint32_t Register = axilite_read(XACCELERATOR_AXILITE_ADDR_IER);
	axilite_write(XACCELERATOR_AXILITE_ADDR_IER, Register | Mask);
}


void XFPGA_InterruptDisable(uint32_t Mask) {
	uint32_t Register = axilite_read(XACCELERATOR_AXILITE_ADDR_IER);
	axilite_write(XACCELERATOR_AXILITE_ADDR_IER, Register & (~Mask));
}


void XFPGA_InterruptClear(uint32_t Mask) {
	axilite_write(XACCELERATOR_AXILITE_ADDR_ISR, Mask);
}


uint32_t XFPGA_InterruptGetEnabled() {
	return axilite_read(XACCELERATOR_AXILITE_ADDR_IER);
}


uint32_t XFPGA_InterruptGetStatus() {
	return axilite_read(XACCELERATOR_AXILITE_ADDR_ISR);
}


void XFPGA_Run(layer_t& layer) {
	XFPGA_SetLayerConfig(layer);
	uint32_t Data = axilite_read(XACCELERATOR_AXILITE_ADDR_AP_CTRL) & 0x80;
	axilite_write(XACCELERATOR_AXILITE_ADDR_AP_CTRL, Data | 0x01);
	while (!XFPGA_IsDone())  { // busy-wait
		usleep(100);
	}
}


void XFPGA_Initialize(network* net) {
	LOG(CONSOLE)<<"XFPGA Driver: Initialize"<<endl;
	axilite_open();
	shared_dram_open();
	axicdma_open();
}


void XFPGA_Release() {
	LOG(CONSOLE)<<"XFPGA Driver: Release"<<endl;
	axilite_close();
	shared_dram_close();
	axicdma_close();
}


void copyImageToDRAM(network *net,numlayers_t id){
	volatile char* ptr = SHARED_DRAM_PTR+net->size_of_weights;
	for (int i=0;i<net->layers[0].config.input_height;i++){
		for (int j=0;j<net->layers[0].config.input_width;j++){
			for (int k=0;k<net->layers[0].config.num_of_ci_strides;k++){
				for (int l=0;l<NUM_OF_BYTES_PER_TRANSACTION;l++){
					feature_block_index_t index = i*net->layers[0].config.input_width*net->layers[0].config.num_of_ci_strides;
					index += j*net->layers[0].config.num_of_ci_strides + k;
					(*ptr) = net->layers[0].input_features[index].f[l];
					ptr++;
				}
			}
		}
	}
	assert((ptr-SHARED_DRAM_PTR-net->size_of_weights)==(net->layers[0].config.input_height*net->layers[0].config.input_width*net->layers[0].config.aligned_input_channels));
}


void copyWeightsToSharedDRAM(network *net){
	volatile char* ptr = SHARED_DRAM_PTR;
	for (numlayers_t i=0;i<net->num_layers;i++){
		for (unsigned int j=0;j<net->layers[i].num_of_weight_blocks+net->layers[i].num_of_bias_blocks;j++){
			for (int k=0;k<NUM_OF_BYTES_PER_TRANSACTION;k++){
				(*ptr) = net->layers[i].weights[j].w[k];
				ptr++;
			}
		}
	}
	assert((ptr-SHARED_DRAM_PTR)==net->size_of_weights);
}


#else


volatile feature_block_t* SHARED_DRAM_PTR = NULL;


void XFPGA_Run(layer* layer) {
	Accelerator(layer->insts,(wblock_t*)layer->weights,(fblock_t*)layer->input_features,(fblock_t*)layer->output_features);
}


void XFPGA_Initialize(network* net) {
	int length = net->GetSizeOfWeights()+net->GetSizeOfFeatures();
	SHARED_DRAM_PTR = new feature_block_t[length/NUM_OF_BYTES_PER_TRANSACTION];
	if (SHARED_DRAM_PTR==NULL){
		LOG(CONSOLE)<<"failed to allocate memory for SHARED_DRAM_PTR"<<endl;
		return;
	}else{
		LOG(CONSOLE)<<"SHARED_DRAM_PTR="<<(unsigned long)SHARED_DRAM_PTR<<endl;
		memset((void*)SHARED_DRAM_PTR,0,sizeof(feature_block_t)*length/NUM_OF_BYTES_PER_TRANSACTION);
	}
}


void XFPGA_Release() {
	delete[] SHARED_DRAM_PTR;
}


void copyImageToSharedDRAM(network *net){
	for (numlayers_t i=0;i<net->GetNumOfLayers();i++){
		if (i==net->GetStartingLayer()){
			memcpy((void*)(SHARED_DRAM_PTR+net->GetLayer(i)->config.input_feature_offset/NUM_OF_BYTES_PER_TRANSACTION),net->GetLayer(i)->input_features,sizeof(feature_t)*net->GetLayer(i)->config.size_of_input_features);
		}
		net->GetLayer(i)->UpdateMemoryForInputFeature((feature_block_t*)(SHARED_DRAM_PTR+net->GetLayer(i)->config.input_feature_offset/NUM_OF_BYTES_PER_TRANSACTION));
		net->GetLayer(i)->UpdateMemoryForOutputFeature((feature_block_t*)(SHARED_DRAM_PTR+net->GetLayer(i)->config.output_feature_offset/NUM_OF_BYTES_PER_TRANSACTION));
		if ((i==(net->GetStartingLayer()+1)) && (net->GetLayer(i-1)->config.layer_type==EXPAND3x3)){
			ifstream inf(("./outf/"+net->GetLayer(i-1)->config.layer_name).c_str());
			if (!inf.is_open()){
				LOG(ERROR)<<"failed to open "<<net->GetLayer(i-1)->config.layer_name<<endl;
				return;
			}
			net->GetLayer(i)->LoadGeneratedFeatureMap(inf);
			inf.close();
		}
	}
}


void copyWeightsToSharedDRAM(network *net){
	weight_block_t* ptr = (weight_block_t*)SHARED_DRAM_PTR;
	for (numlayers_t i=0;i<net->GetNumOfLayers();i++){
		LOG(CONSOLE)<<"size_of_weights_and_bias="<<net->GetLayer(i)->config.size_of_weights_and_bias<<endl;
		memcpy((void*)ptr,net->GetLayer(i)->weights,sizeof(weight_t)*net->GetLayer(i)->config.size_of_weights_and_bias);
//		net->GetLayer(i)->UpdateMemoryForWeightAndBias(ptr);
		ptr += (net->GetLayer(i)->config.size_of_weights_and_bias/NUM_OF_BYTES_PER_TRANSACTION);
	}
}
#endif
