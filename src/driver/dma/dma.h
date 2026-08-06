#ifndef __BK_DMA_H__
#define __BK_DMA_H__
#include <stddef.h>     // standard definition
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include "user_config.h"
#include "intc.h"
#include "dma.h"
#include "BK3633_RegList.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    MEM_READ_REQ=0,
    I2S0_REQ=1,
    UART1_REQ=2,
    UART2_REQ=3,
    SPI_REQ=4,
    USB_REQ=5,
}dma_req_mux_t;


/**
 * @brief data width definition *        
 */
typedef enum
{
    DMA_DW_8B           = 0 ,
    DMA_DW_16B              ,
    DMA_DW_32B        
} dma_dw_t;
typedef int (*dma_func_cb)(uint32_t param);


#define     DMA_ADDR_FIX        (0x0U)
#define     DMA_ADDR_INC        (0x1U)
#define     DMA_ADDR_LOOP       (0x5U)
#define     DMA_INT_FINISH      (0x1U)
#define     DMA_INT_HALF_FINISH (0x2U)


#define BASEADDR_GENER_DMA                                      0x00803000
#define GENER_DMA_REG0X0                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x0*4))
#define POS_GENER_DMA_REG0X0_TRAN_LEN                           16
#define POS_GENER_DMA_REG0X0_CHN_PRIOPRIITY                     12
#define POS_GENER_DMA_REG0X0_DEST_ADDR_LOOP                      11
#define POS_GENER_DMA_REG0X0_SRC_SDDR_LOOP                       10
#define POS_GENER_DMA_REG0X0_DEST_ADDR_INC                       9
#define POS_GENER_DMA_REG0X0_SRC_ADDR_INC                        8
#define POS_GENER_DMA_REG0X0_DEST_DATA_WIDTH                     6
#define POS_GENER_DMA_REG0X0_SRC_DATA_WIDTH                      4
#define POS_GENER_DMA_REG0X0_DMA_MODE                            3
#define POS_GENER_DMA_REG0X0_HALF_FINISH_INTEN                   2
#define POS_GENER_DMA_REG0X0_FINISH_INTEN                        1
#define POS_GENER_DMA_REG0X0_DMA_EN                              0


#define GENER_DMA_REG0X1                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x1*4))
#define GENER_DMA_REG0X2                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x2*4))
#define GENER_DMA_REG0X3                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x3*4))
#define GENER_DMA_REG0X4                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x4*4))
#define GENER_DMA_REG0X5                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x5*4))
#define GENER_DMA_REG0X6                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x6*4))
#define GENER_DMA_REG0X7                                     *((volatile unsigned long *) (BASEADDR_GENER_DMA+0x7*4))
#define POS_GENER_DMA_REG0X7_DEST_WR_INTLV                       16
#define POS_GENER_DMA_REG0X7_SRC_RD_INTLV                        12
#define POS_GENER_DMA_REG0X7_DEST_REQ_MUX                        5
#define POS_GENER_DMA_REG0X7_SRC_REQ_MUX                         0

#define GENER_DMA_REG0XD1                                    *((volatile unsigned long *) (BASEADDR_GENER_DMA+0xd1*4))





/**
 * @brief function definition *        
 */

void dma_config_write(uint8_t channel,uint8_t req_mux,uint32_t src_addr,uint32_t dst_addr,uint16_t buf_len,uint8_t dest_data_width);
void dma_config_read(uint8_t channel,uint8_t req_mux,uint32_t src_addr,uint32_t dst_addr,uint16_t buf_len,uint8_t src_data_width);

void dma_isr(void);

#ifdef __cplusplus
}
#endif

#endif /* __BK_DMA_H__ */
