#ifndef DRIVER_HPP__
#define DRIVER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>

#include"common.hpp"

#if RUN_FOR_SIMULATION!=1
#include <sys/mman.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#endif

#include"layer.hpp"
#include"logger.hpp"
#include"fpga_top.hpp"
#include"network.hpp"

// axilite
// 0x000 : Control signals
//         bit 0  - ap_start (Read/Write/COH)
//         bit 1  - ap_done (Read/COR)
//         bit 2  - ap_idle (Read)
//         bit 3  - ap_ready (Read)
//         bit 7  - auto_restart (Read/Write)
//         others - reserved
// 0x004 : Global Interrupt Enable Register
//         bit 0  - Global Interrupt Enable (Read/Write)
//         others - reserved
// 0x008 : IP Interrupt Enable Register (Read/Write)
//         bit 0  - Channel 0 (ap_done)
//         bit 1  - Channel 1 (ap_ready)
//         others - reserved
// 0x00c : IP Interrupt Status Register (Read/TOW)
//         bit 0  - Channel 0 (ap_done)
//         bit 1  - Channel 1 (ap_ready)
//         others - reserved
// 0x010 : Data signal of config_pad_V
//         bit 4~0 - config_pad_V[4:0] (Read/Write)
//         others  - reserved
// 0x014 : reserved
// 0x018 : Data signal of config_relu
//         bit 0  - config_relu[0] (Read/Write)
//         others - reserved
// 0x01c : reserved
// 0x020 : Data signal of config_stride_V
//         bit 3~0 - config_stride_V[3:0] (Read/Write)
//         others  - reserved
// 0x024 : reserved
// 0x028 : Data signal of config_kernel_size_V
//         bit 4~0 - config_kernel_size_V[4:0] (Read/Write)
//         others  - reserved
// 0x02c : reserved
// 0x030 : Data signal of config_layer_type
//         bit 2~0 - config_layer_type[2:0] (Read/Write)
//         others  - reserved
// 0x034 : reserved
// 0x038 : Data signal of config_dpi_V
//         bit 4~0 - config_dpi_V[4:0] (Read/Write)
//         others  - reserved
// 0x03c : reserved
// 0x040 : Data signal of config_dpo_V
//         bit 4~0 - config_dpo_V[4:0] (Read/Write)
//         others  - reserved
// 0x044 : reserved
// 0x048 : Data signal of config_wpo_V
//         bit 4~0 - config_wpo_V[4:0] (Read/Write)
//         others  - reserved
// 0x04c : reserved
// 0x050 : Data signal of config_bpo_V
//         bit 4~0 - config_bpo_V[4:0] (Read/Write)
//         others  - reserved
// 0x054 : reserved
// 0x058 : Data signal of config_pooling_type
//         bit 1~0 - config_pooling_type[1:0] (Read/Write)
//         others  - reserved
// 0x05c : reserved
// 0x060 : Data signal of config_pooling_pad_V
//         bit 11~0 - config_pooling_pad_V[11:0] (Read/Write)
//         others   - reserved
// 0x064 : reserved
// 0x068 : Data signal of config_pooling_size_V
//         bit 11~0 - config_pooling_size_V[11:0] (Read/Write)
//         others   - reserved
// 0x06c : reserved
// 0x070 : Data signal of config_pooling_stride_V
//         bit 5~0 - config_pooling_stride_V[5:0] (Read/Write)
//         others  - reserved
// 0x074 : reserved
// 0x078 : Data signal of config_pooled_width_V
//         bit 10~0 - config_pooled_width_V[10:0] (Read/Write)
//         others   - reserved
// 0x07c : reserved
// 0x080 : Data signal of config_pooled_height_V
//         bit 10~0 - config_pooled_height_V[10:0] (Read/Write)
//         others   - reserved
// 0x084 : reserved
// 0x088 : Data signal of config_input_width_V
//         bit 10~0 - config_input_width_V[10:0] (Read/Write)
//         others   - reserved
// 0x08c : reserved
// 0x090 : Data signal of config_input_height_V
//         bit 10~0 - config_input_height_V[10:0] (Read/Write)
//         others   - reserved
// 0x094 : reserved
// 0x098 : Data signal of config_input_channels_V
//         bit 11~0 - config_input_channels_V[11:0] (Read/Write)
//         others   - reserved
// 0x09c : reserved
// 0x0a0 : Data signal of config_num_of_ci_strides_V
//         bit 11~0 - config_num_of_ci_strides_V[11:0] (Read/Write)
//         others   - reserved
// 0x0a4 : reserved
// 0x0a8 : Data signal of config_aligned_input_channels_V
//         bit 11~0 - config_aligned_input_channels_V[11:0] (Read/Write)
//         others   - reserved
// 0x0ac : reserved
// 0x0b0 : Data signal of config_output_width_V
//         bit 10~0 - config_output_width_V[10:0] (Read/Write)
//         others   - reserved
// 0x0b4 : reserved
// 0x0b8 : Data signal of config_output_height_V
//         bit 10~0 - config_output_height_V[10:0] (Read/Write)
//         others   - reserved
// 0x0bc : reserved
// 0x0c0 : Data signal of config_output_channels_V
//         bit 11~0 - config_output_channels_V[11:0] (Read/Write)
//         others   - reserved
// 0x0c4 : reserved
// 0x0c8 : Data signal of config_num_of_co_strides_V
//         bit 11~0 - config_num_of_co_strides_V[11:0] (Read/Write)
//         others   - reserved
// 0x0cc : reserved
// 0x0d0 : Data signal of config_aligned_output_channels_V
//         bit 11~0 - config_aligned_output_channels_V[11:0] (Read/Write)
//         others   - reserved
// 0x0d4 : reserved
// 0x0d8 : Data signal of config_num_of_cixco_strides_V
//         bit 11~0 - config_num_of_cixco_strides_V[11:0] (Read/Write)
//         others   - reserved
// 0x0dc : reserved
// 0x0e0 : Data signal of config_num_of_width_ci_strides_V
//         bit 25~0 - config_num_of_width_ci_strides_V[25:0] (Read/Write)
//         others   - reserved
// 0x0e4 : reserved
// 0x0e8 : Data signal of config_num_of_output_points_V
//         bit 25~0 - config_num_of_output_points_V[25:0] (Read/Write)
//         others   - reserved
// 0x0ec : reserved
// 0x0f0 : Data signal of config_num_of_pooling_points_V
//         bit 11~0 - config_num_of_pooling_points_V[11:0] (Read/Write)
//         others   - reserved
// 0x0f4 : reserved
// 0x0f8 : Data signal of config_output_width_div_pstride_V
//         bit 10~0 - config_output_width_div_pstride_V[10:0] (Read/Write)
//         others   - reserved
// 0x0fc : reserved
// 0x100 : Data signal of config_output_height_div_pstride_V
//         bit 10~0 - config_output_height_div_pstride_V[10:0] (Read/Write)
//         others   - reserved
// 0x104 : reserved
// 0x108 : Data signal of w_V
//         bit 31~0 - w_V[31:0] (Read/Write)
// 0x10c : reserved
// 0x110 : Data signal of inf_V
//         bit 31~0 - inf_V[31:0] (Read/Write)
// 0x114 : reserved
// 0x118 : Data signal of outf_V
//         bit 31~0 - outf_V[31:0] (Read/Write)
// 0x11c : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XACCELERATOR_AXILITE_ADDR_AP_CTRL                                 0x000
#define XACCELERATOR_AXILITE_ADDR_GIE                                     0x004
#define XACCELERATOR_AXILITE_ADDR_IER                                     0x008
#define XACCELERATOR_AXILITE_ADDR_ISR                                     0x00c
#define XACCELERATOR_AXILITE_ADDR_CONFIG_PAD_V_DATA                       0x010
#define XACCELERATOR_AXILITE_BITS_CONFIG_PAD_V_DATA                       5
#define XACCELERATOR_AXILITE_ADDR_CONFIG_RELU_DATA                        0x018
#define XACCELERATOR_AXILITE_BITS_CONFIG_RELU_DATA                        1
#define XACCELERATOR_AXILITE_ADDR_CONFIG_STRIDE_V_DATA                    0x020
#define XACCELERATOR_AXILITE_BITS_CONFIG_STRIDE_V_DATA                    4
#define XACCELERATOR_AXILITE_ADDR_CONFIG_KERNEL_SIZE_V_DATA               0x028
#define XACCELERATOR_AXILITE_BITS_CONFIG_KERNEL_SIZE_V_DATA               5
#define XACCELERATOR_AXILITE_ADDR_CONFIG_LAYER_TYPE_DATA                  0x030
#define XACCELERATOR_AXILITE_BITS_CONFIG_LAYER_TYPE_DATA                  3
#define XACCELERATOR_AXILITE_ADDR_CONFIG_DPI_V_DATA                       0x038
#define XACCELERATOR_AXILITE_BITS_CONFIG_DPI_V_DATA                       5
#define XACCELERATOR_AXILITE_ADDR_CONFIG_DPO_V_DATA                       0x040
#define XACCELERATOR_AXILITE_BITS_CONFIG_DPO_V_DATA                       5
#define XACCELERATOR_AXILITE_ADDR_CONFIG_WPO_V_DATA                       0x048
#define XACCELERATOR_AXILITE_BITS_CONFIG_WPO_V_DATA                       5
#define XACCELERATOR_AXILITE_ADDR_CONFIG_BPO_V_DATA                       0x050
#define XACCELERATOR_AXILITE_BITS_CONFIG_BPO_V_DATA                       5
#define XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_TYPE_DATA                0x058
#define XACCELERATOR_AXILITE_BITS_CONFIG_POOLING_TYPE_DATA                2
#define XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_PAD_V_DATA               0x060
#define XACCELERATOR_AXILITE_BITS_CONFIG_POOLING_PAD_V_DATA               12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_SIZE_V_DATA              0x068
#define XACCELERATOR_AXILITE_BITS_CONFIG_POOLING_SIZE_V_DATA              12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_POOLING_STRIDE_V_DATA            0x070
#define XACCELERATOR_AXILITE_BITS_CONFIG_POOLING_STRIDE_V_DATA            6
#define XACCELERATOR_AXILITE_ADDR_CONFIG_POOLED_WIDTH_V_DATA              0x078
#define XACCELERATOR_AXILITE_BITS_CONFIG_POOLED_WIDTH_V_DATA              11
#define XACCELERATOR_AXILITE_ADDR_CONFIG_POOLED_HEIGHT_V_DATA             0x080
#define XACCELERATOR_AXILITE_BITS_CONFIG_POOLED_HEIGHT_V_DATA             11
#define XACCELERATOR_AXILITE_ADDR_CONFIG_INPUT_WIDTH_V_DATA               0x088
#define XACCELERATOR_AXILITE_BITS_CONFIG_INPUT_WIDTH_V_DATA               11
#define XACCELERATOR_AXILITE_ADDR_CONFIG_INPUT_HEIGHT_V_DATA              0x090
#define XACCELERATOR_AXILITE_BITS_CONFIG_INPUT_HEIGHT_V_DATA              11
#define XACCELERATOR_AXILITE_ADDR_CONFIG_INPUT_CHANNELS_V_DATA            0x098
#define XACCELERATOR_AXILITE_BITS_CONFIG_INPUT_CHANNELS_V_DATA            12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_CI_STRIDES_V_DATA         0x0a0
#define XACCELERATOR_AXILITE_BITS_CONFIG_NUM_OF_CI_STRIDES_V_DATA         12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_ALIGNED_INPUT_CHANNELS_V_DATA    0x0a8
#define XACCELERATOR_AXILITE_BITS_CONFIG_ALIGNED_INPUT_CHANNELS_V_DATA    12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_WIDTH_V_DATA              0x0b0
#define XACCELERATOR_AXILITE_BITS_CONFIG_OUTPUT_WIDTH_V_DATA              11
#define XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_HEIGHT_V_DATA             0x0b8
#define XACCELERATOR_AXILITE_BITS_CONFIG_OUTPUT_HEIGHT_V_DATA             11
#define XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_CHANNELS_V_DATA           0x0c0
#define XACCELERATOR_AXILITE_BITS_CONFIG_OUTPUT_CHANNELS_V_DATA           12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_CO_STRIDES_V_DATA         0x0c8
#define XACCELERATOR_AXILITE_BITS_CONFIG_NUM_OF_CO_STRIDES_V_DATA         12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_ALIGNED_OUTPUT_CHANNELS_V_DATA   0x0d0
#define XACCELERATOR_AXILITE_BITS_CONFIG_ALIGNED_OUTPUT_CHANNELS_V_DATA   12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_CIXCO_STRIDES_V_DATA      0x0d8
#define XACCELERATOR_AXILITE_BITS_CONFIG_NUM_OF_CIXCO_STRIDES_V_DATA      12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_WIDTH_CI_STRIDES_V_DATA   0x0e0
#define XACCELERATOR_AXILITE_BITS_CONFIG_NUM_OF_WIDTH_CI_STRIDES_V_DATA   26
#define XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_OUTPUT_POINTS_V_DATA      0x0e8
#define XACCELERATOR_AXILITE_BITS_CONFIG_NUM_OF_OUTPUT_POINTS_V_DATA      26
#define XACCELERATOR_AXILITE_ADDR_CONFIG_NUM_OF_POOLING_POINTS_V_DATA     0x0f0
#define XACCELERATOR_AXILITE_BITS_CONFIG_NUM_OF_POOLING_POINTS_V_DATA     12
#define XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_WIDTH_DIV_PSTRIDE_V_DATA  0x0f8
#define XACCELERATOR_AXILITE_BITS_CONFIG_OUTPUT_WIDTH_DIV_PSTRIDE_V_DATA  11
#define XACCELERATOR_AXILITE_ADDR_CONFIG_OUTPUT_HEIGHT_DIV_PSTRIDE_V_DATA 0x100
#define XACCELERATOR_AXILITE_BITS_CONFIG_OUTPUT_HEIGHT_DIV_PSTRIDE_V_DATA 11
#define XACCELERATOR_AXILITE_ADDR_W_V_DATA                                0x108
#define XACCELERATOR_AXILITE_BITS_W_V_DATA                                32
#define XACCELERATOR_AXILITE_ADDR_INF_V_DATA                              0x110
#define XACCELERATOR_AXILITE_BITS_INF_V_DATA                              32
#define XACCELERATOR_AXILITE_ADDR_OUTF_V_DATA                             0x118
#define XACCELERATOR_AXILITE_BITS_OUTF_V_DATA                             32

#define XAXICDMA_CR_OFFSET	        0x00000000  /**< Control register              > */
#define XAXICDMA_SR_OFFSET	        0x00000004  /**< Status register               > */
#define XAXICDMA_CDESC_OFFSET       0x00000008  /**< Current descriptor pointer    > */
#define XAXICDMA_CDESC_MSB_OFFSET   0x0000000C  /**< Current descriptor pointer    > */
#define XAXICDMA_TDESC_OFFSET	    0x00000010  /**< Tail descriptor pointer       > */
#define XAXICDMA_TDESC_MSB_OFFSET   0x00000014  /**< Tail descriptor pointer       > */
#define XAXICDMA_SRCADDR_OFFSET	    0x00000018  /**< Source address register       > */
#define XAXICDMA_SRCADDR_MSB_OFFSET 0x0000001C  /**< Source address register       > */
#define XAXICDMA_DSTADDR_OFFSET	    0x00000020  /**< Destination address register  > */
#define XAXICDMA_DSTADDR_MSB_OFFSET 0x00000024  /**< Destination address register  > */
#define XAXICDMA_BTT_OFFSET         0x00000028  /**< Bytes to transfer             > */

#define XAXICDMA_XR_IRQ_ALL_MASK 0x00007000 /**< All interrupts */
#define XAXICDMA_CR_RESET_MASK 0x00000004   /**< Reset DMA engine */
#define XAXICDMA_CR_SGMODE_MASK 0x00000008  /**< Scatter gather mode */
#define XAXICDMA_XR_IRQ_IOC_MASK 0x00001000   /**< Completion interrupt */
#define XAXICDMA_XR_IRQ_DELAY_MASK 0x00002000 /**< Delay interrupt */
#define XAXICDMA_XR_IRQ_ERROR_MASK 0x00004000 /**< Error interrupt */
#define XAXICDMA_XR_IRQ_ALL_MASK 0x00007000   /**< All interrupts */
#define XAXICDMA_XR_IRQ_SIMPLE_ALL_MASK 0x00005000 /**< All interrupts for simple only mode */

const off_t SHARED_DRAM_WEIGHTS_CPU = 0x38000000;
const off_t SHARED_DRAM_DATA_CPU = 0x3c000000;
const off_t XAXICDMA_RESULT_BASEADDR = 0x3f00000;
const off_t MAX_CDMA_BYTES = 0x7FFFFF;

const off_t AXILITE_BASE_ADDR =0x00A0000000;
const off_t AXICDMA_BASE_ADDR = 0x00A0010000;
const size_t AXILITE_MEM_SIZE = 0xFFFF;  // actual address range is 64KB

const off_t SHARED_DRAM_BASE_ADDR = 0x0080000000;
const size_t SHARED_DRAM_MEM_SIZE = 0x1E000000;

#if RUN_FOR_SIMULATION!=1
void XFPGA_InterruptGlobalEnable();
void XFPGA_InterruptGlobalDisable();
void XFPGA_InterruptEnable(uint32_t Mask);
void XFPGA_InterruptDisable(uint32_t Mask);
void XFPGA_InterruptClear(uint32_t Mask);
uint32_t XFPGA_InterruptGetEnabled();
uint32_t XFPGA_InterruptGetStatus();
#endif

void XFPGA_Release();
void XFPGA_Run(layer* layer);
void XFPGA_Initialize(network* net);
void copyImageToSharedDRAM(network *net);
void copyWeightsToSharedDRAM(network *net);

#endif
