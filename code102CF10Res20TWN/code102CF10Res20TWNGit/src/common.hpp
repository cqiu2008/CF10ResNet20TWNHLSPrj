#ifndef COMMON_HPP__
#define COMMON_HPP__

#include<string>
#include<ap_int.h>


#define SQUEEZENET							1

#define LOAD_BALANCE						1

#define STARTING_LAYER						0

#define RUN_ONE_LAYER						0

#define COMBINE_LAYER						1

#define MULTI_POINT							1

#define RUN_FOR_SIMULATION					1

#define COMPRESS							0
#define ADD_TREE							1

#define DEBUG								0

#define SHARE_DSP48E						1

#define MAX_KERNEL_STRIDE					4
#define MAX_POOLING_STRIDE					16
#define MAX_POOLING_SIZE					16
#define MAX_KERNEL_SIZE						16
#define MAX_FEATURE_DIMENSION				512
#define MAX_NUM_OF_LAYERS 					1000
#define MAX_CHANNEL_NUM						1024

#define CI_STRIDE_MASK						4
#define CO_STRIDE_MASK						7

#define DATA_WIDTH							8

#define RAM_36K_DEPTH						512

#define MAX_NUM_OF_DSP_AVAILABLE			2520

#define DUAL_MACC_SHIFT_BITS				18
#define BIT_16_TO_0							0x00000001ffff
#define BIT_17								0x000000020000

#define NUM_OF_PE_GROUP						4

#define NUM_OF_CO_DIV_PER_GROUP				((CO_STRIDE/NUM_OF_BYTES_PER_TRANSACTION)/NUM_OF_PE_GROUP)
#define WEIGHT_BUF_DEPTH					(RAM_36K_DEPTH)

#define LINE_BUF_DEPTH						(3*RAM_36K_DEPTH)

#define COUNTER_WIDTH						(2*DATA_WIDTH)

#define NUM_OF_PEs							CI_STRIDE

#define BYTES_PER_TRANSACTION_MASK			CI_STRIDE_MASK
#define NUM_OF_BYTES_PER_TRANSACTION		(1<<BYTES_PER_TRANSACTION_MASK)

#define CI_STRIDE							(1<<CI_STRIDE_MASK)
#define CO_STRIDE							(1<<CO_STRIDE_MASK)

#define CO_GROUP_SIZE						(CO_STRIDE/NUM_OF_PE_GROUP)

#define MAX_FEATURE_NUM_PER_CHANNEL			(MAX_FEATURE_DIMENSION*MAX_FEATURE_DIMENSION)
#define MAX_NUM_OF_INPUT_FEATURES			(MAX_FEATURE_NUM_PER_CHANNEL*MAX_CHANNEL_NUM)
#define MAX_NUM_OF_CHANNEL_DIV				CEIL_DIV(MAX_CHANNEL_NUM,NUM_OF_BYTES_PER_TRANSACTION)
#define MAX_CO_DIV_CHANNEL					CEIL_DIV(MAX_CHANNEL_NUM,CO_STRIDE)
#define MAX_NUM_OF_INPUT_FEATURE_BLOCKS		(MAX_NUM_OF_CHANNEL_DIV*MAX_FEATURE_NUM_PER_CHANNEL)

#define MAX_NUM_OF_OUTPUT_FEATURES			(MAX_FEATURE_NUM_PER_CHANNEL*MAX_CHANNEL_NUM)
#define MAX_NUM_OF_OUTPUT_FEATURE_BLOCKS	(MAX_NUM_OF_CHANNEL_DIV*MAX_FEATURE_NUM_PER_CHANNEL)

#define MAX_NUM_OF_WEIGHTS					(MAX_NUM_OF_WEIGHTS_PER_OUTPUT_CHANNEL*MAX_CHANNEL_NUM)

#define MAX_NUM_OF_BIAS_BLOCKS				MAX_NUM_OF_CHANNEL_DIV

#define MAX_NUM_OF_WEIGHT_BLOCKS			(MAX_NUM_OF_CHANNEL_DIV*MAX_KERNEL_SIZE*MAX_KERNEL_SIZE*MAX_CHANNEL_NUM)

//macro declare
#define CEIL_DIV(x,y)						(((x)+(y)-1)/(y))
#define MAX(x,y)							(((x)<(y)) ? (y) : (x))
#define MIN(x,y)							(((x)>(y)) ? (y) : (x))

#define MAX_NUM_OF_BLOCKS					MAX(MAX_NUM_OF_BIAS_BLOCKS,MAX_NUM_OF_CHANNEL_DIV*CI_STRIDE)

#define NBITS2(n)							((n & 2) ? 1 : 0)
#define NBITS4(n)							((n & (0xC)) ? (2 + NBITS2(n >> 2)) : (NBITS2(n)))
#define NBITS8(n)							((n & 0xF0) ? (4 + NBITS4(n >> 4)) : (NBITS4(n)))
#define NBITS16(n)							((n & 0xFF00) ? (8 + NBITS8(n >> 8)) : (NBITS8(n)))
#define NBITS32(n)							((n & 0xFFFF0000) ? (16 + NBITS16(n >> 16)) : (NBITS16(n)))
//非负数n用原码表示所需要的比特数，用补码表示需要增加1个符号位
#define NBITS(n)							((n) == 0 ? 1 : NBITS32((n)) + 1)

#define BATCH_FINISH						(0x0)

typedef ap_int<DATA_WIDTH> weight_t;
typedef ap_int<DATA_WIDTH> feature_t;
typedef ap_int<27> combined_weights_t;
typedef ap_int<35> combined_product_t;
typedef ap_int<2*DATA_WIDTH> product_t;
typedef ap_int<4*DATA_WIDTH> accumulator_t;
typedef ap_uint<COUNTER_WIDTH> counter_t;
typedef ap_uint<4*DATA_WIDTH*CO_STRIDE> accumulators_t;

typedef ap_int<2*DATA_WIDTH+NBITS(CI_STRIDE)-1> partial_sum_t;
typedef ap_uint<DATA_WIDTH*NUM_OF_BYTES_PER_TRANSACTION> fblock_t;
typedef ap_uint<DATA_WIDTH*NUM_OF_BYTES_PER_TRANSACTION> wblock_t;
typedef ap_uint<4*DATA_WIDTH*NUM_OF_BYTES_PER_TRANSACTION> ppblock16_t;
typedef ap_uint<4*DATA_WIDTH*NUM_OF_BYTES_PER_TRANSACTION/4> ppblock4_t;
typedef ap_uint<COUNTER_WIDTH*NUM_OF_BYTES_PER_TRANSACTION> counter_block_t;

typedef ap_uint<DATA_WIDTH> accumulator_index_t;

typedef ap_uint<NBITS(LINE_BUF_DEPTH)+1> buffer_index_t;

typedef ap_int<NBITS(DATA_WIDTH)+1> shift_t;
typedef ap_uint<NBITS(MAX_KERNEL_SIZE/2)+1> pad_t;
typedef ap_int<2*(NBITS(MAX_POOLING_SIZE)+1)> pooling_size_t;
typedef ap_uint<NBITS(MAX_POOLING_STRIDE)+1> pooling_stride_t;

typedef ap_uint<NBITS(MAX_KERNEL_SIZE)> kernel_t;
typedef ap_uint<NBITS(MAX_KERNEL_SIZE*MAX_KERNEL_SIZE)> filter_t;
typedef ap_uint<NBITS(MAX_KERNEL_STRIDE)+1> stride_t;
typedef ap_uint<NBITS(MAX_CHANNEL_NUM)+1> channel_t;
typedef ap_uint<NBITS(MAX_NUM_OF_LAYERS)+1> numlayers_t;
typedef ap_int<NBITS(MAX_FEATURE_DIMENSION)+1> dimension_t;

typedef ap_uint<NBITS(MAX_NUM_OF_INPUT_FEATURE_BLOCKS)+1> feature_block_index_t;
typedef ap_uint<NBITS(MAX_NUM_OF_WEIGHT_BLOCKS+MAX_NUM_OF_BIAS_BLOCKS)+1> weight_block_index_t;

typedef ap_uint<32> misc_instruction_t;

typedef ap_uint<32> shift_instruction_t;

typedef ap_uint<32> dimension_instruction_t;

typedef ap_uint<4> layer_type_t;
typedef ap_uint<2> pooling_type_t;

typedef ap_uint<2> weight_compress_t;

typedef ap_uint<6> sublayer_t;


struct instruction_group_t{
	misc_instruction_t imisc;
	shift_instruction_t ishift;
	dimension_instruction_t idimension;
};


enum layer_enum{
	CNV = 0,
	EXPAND1x1,
	EXPAND3x3,
	FC,
	NUM_OF_LAYER_TYPE
};

enum pooling_enum{
	MAX_POOLING=0,
	AVG_POOLING,
	NUM_OF_POOLING_TYPE
};


enum weight_enum{
	UNCOMPRESS = 0,
	COMPRESS4bits,
	COMPRESS8bits,
	NUM_OF_COMPRESS_TYPE
};

enum ibuf_enum{
	NADD = 0,
	ADD,
	M,
	OUT
};


#define LOW_10BITS_MASK	(0x3ff)
#define TUPLE_SHIFT_BITS	(NBITS(WEIGHT_BUF_DEPTH))
typedef ap_uint<NBITS(WEIGHT_BUF_DEPTH)> widx_t;

#define TUPLE_WIDTH	(DATA_WIDTH+NBITS(WEIGHT_BUF_DEPTH))
typedef ap_uint<TUPLE_WIDTH> feature_tuple_t;

typedef ap_uint<NUM_OF_BYTES_PER_TRANSACTION*TUPLE_WIDTH> feature_tuples_t;


struct accumulator_block_t{
	accumulator_t acc[CO_STRIDE];
};


struct post_process_block_t{
	accumulator_t acc[NUM_OF_BYTES_PER_TRANSACTION];
};


struct weight_block_t{
	weight_t w[NUM_OF_BYTES_PER_TRANSACTION];
};


struct softmax_result_t{
	std::string label;
	float value;
};


struct feature_block_t{
	feature_t f[NUM_OF_BYTES_PER_TRANSACTION];
};


#endif
