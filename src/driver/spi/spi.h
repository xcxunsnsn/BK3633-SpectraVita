/**
 ****************************************************************************************
 *
 * @file spi.h
 *
 * @brief SPI Driver for SPI operation.
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef _SPI_H_
#define _SPI_H_

/* 
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions



//************************************************************//
//SPI0
//************************************************************//
#define BASEADDR_SPI                                           0x00806200

#define SPI_REG0X0_CFG                                          *((volatile unsigned long *) (0x00806200+0x0*4))
#define POS_SPI_REG0X0_BYTE_INTLVAL                             24
#define POS_SPI_REG0X0_SPIEN                                    23
#define POS_SPI_REG0X0_MSTEN                                    22
#define POS_SPI_REG0X0_CKPHA                                    21
#define POS_SPI_REG0X0_CKPOL                                    20
#define POS_SPI_REG0X0_LSB_FIRST                                19
#define POS_SPI_REG0X0_BIT_WDTH                                 18
#define POS_SPI_REG0X0_WIRE3_EN                                 17
#define POS_SPI_REG0X0_SLV_RELEASE_INTEN                        16
#define POS_SPI_REG0X0_SPI_CKR                                  8
#define POS_SPI_REG0X0_RXFIFO_INT_EN                            7
#define POS_SPI_REG0X0_TXFIFO_INT_EN                            6
#define POS_SPI_REG0X0_RXOVF_EN                                 5
#define POS_SPI_REG0X0_TXUDF_EN                                 4
#define POS_SPI_REG0X0_RXFIFO_INT_LEVEL                         2
#define POS_SPI_REG0X0_TXFIFO_INT_LEVEL                         0

#define SPI_REG0X1_CN                                           *((volatile unsigned long *) (0x00806200+0x1*4))
#define POS_SPI_REG0X1_RX_TRANS_LEN                             20
#define POS_SPI_REG0X1_TX_TRANS_LEN                             8
#define POS_SPI_REG0X1_RX_FINISH_INT_EN                         3
#define POS_SPI_REG0X1_TX_FINISH_INT_EN                         2
#define POS_SPI_REG0X1_RX_EN                                    1
#define POS_SPI_REG0X1_TX_EN                                    0

#define SPI_REG0X2_STAT                                         *((volatile unsigned long *) (0x00806200+0x2*4))
#define POS_SPI_REG0X2_RXFIFO_CLR                               17
#define POS_SPI_REG0X2_TXFIFO_CLR                               16
#define POS_SPI_REG0X2_RX_FINISH_INT                            14
#define POS_SPI_REG0X2_TX_FINISH_INT                            13
#define POS_SPI_REG0X2_RXOVF                                    12
#define POS_SPI_REG0X2_TXUDF                                    11
#define POS_SPI_REG0X2_SLV_RELEASE_INT                          10
#define POS_SPI_REG0X2_RXFIFO_INT                               9
#define POS_SPI_REG0X2_TXFIFO_INT                               8
#define POS_SPI_REG0X2_RXFIFO_RD_READY                          2
#define POS_SPI_REG0X2_TXFIFO_WR_READY                          1

#define SPI_REG0X3_DAT                                          *((volatile unsigned long *) (0x00806200+0x3*4))
#define POS_SPI_REG0X3_SPI_DAT                                  0
#define SPI_REG0X3_DAT_ADDR                                     (0x0080620C)



/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */
///SPI Character format
enum SPI_CHARFORMAT
{
    SPI_CHARFORMAT_8BIT = 0x00UL,
    SPI_CHARFORMAT_16BIT = 0x01UL
};

enum CKPHA
{
     CKPHA_CLK1 = 0x00UL,
     CKPHA_CLK2 = 0x01UL
};

enum CKPOL
{
     CKPOL_L = 0x00UL,
     CKPOL_H = 0x01UL
};

typedef void (*spi_write_cb)(void);
typedef void (*spi_read_cb)(void);
typedef struct
{
    uint8_t                transfer_start;  
    volatile uint8_t   spi_state;
    spi_write_cb       write_complete_cb;  
    spi_read_cb        read_complete_cb;
} spi_param_t;


void spi_init(uint8_t mode,uint8_t freq_div,uint8_t bit_wdth);
void spi_write_read(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size);
void spi_master_test(void);

void spi_send(uint16_t w_size);
void spi_waitbusying(void);
void spi_isr(void);
void spi_write(uint8_t *wbuf, uint32_t w_size);
void spi_read( uint8_t *rbuf, uint32_t r_size);

void spi_receiv(uint16_t r_size);
uint8_t spi_send_state_get(void);
void spi_dma_write_result_callback(void);
void spi_dma_read_result_callback(void);
void spi_dma_write(uint8_t *buffer,uint16_t buffer_len,spi_write_cb result_callback);
void spi_dma_read(uint8_t *buffer,uint16_t buffer_len,spi_read_cb result_callback);

void spi_slave_isr(void);

/// @} UART
#endif /* _UART_H_ */
