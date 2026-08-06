
/**
 ****************************************************************************************
 *
 * @file spi.c
 *
 * @brief spi driver
 *
 * Copyright (C) BeKen 2009-2017
 *
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stddef.h>     // standard definition
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "spi.h"       // uart definition
#include "user_config.h"
#include "icu.h" 
#include "uart.h"
#include "gpio.h"
#include "dma.h"       
#include "BK3633_RegList.h"
#include "rf.h"

spi_param_t    spi_param; 



#if(SPI_DRIVER)
unsigned char    *p_spi_txdata;
unsigned char    *p_spi_rxdata;
/*********************************************************************
//mode: 0->master,1->slave
//SPI CLK = PLL clk/(freq_div*2)  
//bit_wdth: 0:8bit,1:16bit
**********************************************************************/
void spi_init(uint8_t mode,uint8_t freq_div,uint8_t bit_wdth)
{
    // SPI clock enable
    SET_SPI_POWER_UP;     
    
    // Enable GPIO P0.4, P0.5, P0.6, P0.7 peripheral function for spi
    gpio_config(0x04,SC_FUN,PULL_NONE);
    gpio_config(0x05,SC_FUN,PULL_NONE);
    gpio_config(0x06,SC_FUN,PULL_NONE);
    gpio_config(0x07,SC_FUN,PULL_NONE);

    SPI_REG0X0_CFG = (0x01UL << POS_SPI_REG0X0_SPIEN)                   
                    | (CKPHA_CLK1 << POS_SPI_REG0X0_CKPHA)
                    | (CKPOL_L << POS_SPI_REG0X0_CKPOL)
                    | (0x00UL << POS_SPI_REG0X0_LSB_FIRST)
                    | (bit_wdth << POS_SPI_REG0X0_BIT_WDTH)
                    | (0x00UL << POS_SPI_REG0X0_WIRE3_EN)
                    | (freq_div << POS_SPI_REG0X0_SPI_CKR) 
                    | (0x01UL << POS_SPI_REG0X0_RXFIFO_INT_EN)
                    | (0x00UL << POS_SPI_REG0X0_TXFIFO_INT_EN)
                    | (0x00UL << POS_SPI_REG0X0_RXOVF_EN)
                    | (0x00UL << POS_SPI_REG0X0_TXUDF_EN)
                    | (0x03UL << POS_SPI_REG0X0_RXFIFO_INT_LEVEL)
                    | (0x00UL << POS_SPI_REG0X0_TXFIFO_INT_LEVEL);

    if(mode==0)
        SPI_REG0X0_CFG |= ( 0x01UL << POS_SPI_REG0X0_MSTEN);
    else
        SPI_REG0X0_CFG |= ( 0x01UL << POS_SPI_REG0X0_SLV_RELEASE_INTEN);
        
    SYS_REG0X4_CLK_SEL &= ~(1<<POS_SYS_REG0X4_SPICLK_SEL);////PLL CLK
    spi_param.spi_state=1;
    spi_param.write_complete_cb=NULL;
    spi_param.read_complete_cb=NULL;
    
}    

void spi_write_read(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size)
{
    uint32_t max_len;
        
    max_len = (w_size > r_size ) ? w_size : r_size;
    
    if(max_len>64)
        return;
    
    spi_waitbusying();
    SPI_REG0X1_CN=0;
    SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_TX_FINISH_INT);
    SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_RX_FINISH_INT);
    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);
    
    SPI_REG0X1_CN = (max_len<<POS_SPI_REG0X1_RX_TRANS_LEN)|(max_len<<POS_SPI_REG0X1_TX_TRANS_LEN);

    while((SPI_REG0X2_STAT &(1<<POS_SPI_REG0X2_TXFIFO_WR_READY))==0);
    
    while(max_len--)
    {
        if(w_size > 0)
        {
            SPI_REG0X3_DAT=*wbuf;
            wbuf++;
            w_size--;
        }
        else
            SPI_REG0X3_DAT=0;

    }
    
    SPI_REG0X1_CN |= (1<<POS_SPI_REG0X1_TX_EN)
                    |(1<<POS_SPI_REG0X1_RX_EN)
                    |(1<<POS_SPI_REG0X1_TX_FINISH_INT_EN)
                    |(1<<POS_SPI_REG0X1_RX_FINISH_INT_EN);
                    
    if(r_size>0)
    {
        while(0==(spi_send_state_get()));
        
        while(r_size)
        {
            if( SPI_REG0X2_STAT & (1<<POS_SPI_REG0X2_RXFIFO_RD_READY) ) // because spi_rx need more time to get buf at the first time.
            {
                *rbuf = (SPI_REG0X3_DAT & 0xff);
                rbuf++;
                r_size--;
            }
        }
    }   
}

void spi_write(uint8_t *wbuf, uint32_t w_size)
{    
    if(w_size <= 0)
    {
        return;    
    }    
    if(w_size > 64)
    {
        w_size = 64;
    }    
    spi_waitbusying();
    SPI_REG0X1_CN=0;
    SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_TX_FINISH_INT);
    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);  
    
    SPI_REG0X1_CN = (w_size<<POS_SPI_REG0X1_TX_TRANS_LEN);

    while((SPI_REG0X2_STAT &(1<<POS_SPI_REG0X2_TXFIFO_WR_READY))==0);
    
    while(w_size)
    {
        SPI_REG0X3_DAT=*wbuf;
        wbuf++;
        w_size--;

    }
    
    SPI_REG0X1_CN |= (1<<POS_SPI_REG0X1_TX_EN) |(1<<POS_SPI_REG0X1_TX_FINISH_INT_EN); 
}

void spi_read( uint8_t *rbuf, uint32_t r_size)
{
    uint16_t tempdata;

    if(r_size <= 0)
    {
        return;
    }
    if(r_size > 4095)
    {
        uart_printf("r_size over \r\n");    
        r_size = 4095;
    }
    spi_waitbusying();

    SPI_REG0X1_CN=0;
    SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_RX_FINISH_INT);
    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);

    while(SPI_REG0X2_STAT&0x4)
    {
        tempdata = SPI_REG0X3_DAT;
        uart_printf("clear rx fifo \r\n");    
    }

    SPI_REG0X1_CN = (r_size<<POS_SPI_REG0X1_RX_TRANS_LEN)|(1<<POS_SPI_REG0X1_RX_EN)|(1<<POS_SPI_REG0X1_RX_FINISH_INT_EN);

    tempdata=0;

    while(1)
    {
        if(SPI_REG0X2_STAT&0x4)  //fifo 有数据
        {
            *rbuf = (SPI_REG0X3_DAT & 0xff);
            r_size--;
            if(r_size == 0)
            {
                break;
            }
            rbuf++;
            tempdata=0;
        }
        else
        {
            tempdata++;
            if(tempdata>30000)
            {
                uart_printf("rx error:r_size=%d\r\n",r_size);    
                break;
            }
        }
    }    
}


void spi_send(uint16_t w_size)
{
    
    SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_TX_FINISH_INT);
    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);

    SPI_REG0X1_CN = (w_size<<POS_SPI_REG0X1_TX_TRANS_LEN);
    SPI_REG0X1_CN |= (1<<POS_SPI_REG0X1_TX_EN)
                        |(0<<POS_SPI_REG0X1_RX_EN)
                        |(1<<POS_SPI_REG0X1_TX_FINISH_INT_EN)
                        |(0<<POS_SPI_REG0X1_RX_FINISH_INT_EN);

}

void spi_receiv(uint16_t r_size)
{
    
    SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_TX_FINISH_INT);
    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);
    
    SPI_REG0X1_CN = (r_size<<POS_SPI_REG0X1_RX_TRANS_LEN);
    SPI_REG0X1_CN |= (0<<POS_SPI_REG0X1_TX_EN)
                        |(1<<POS_SPI_REG0X1_RX_EN)
                        |(0<<POS_SPI_REG0X1_TX_FINISH_INT_EN)
                        |(1<<POS_SPI_REG0X1_RX_FINISH_INT_EN);


}
void spi_isr(void)
{
    SYS_REG0X10_INT_EN &= ~(0x01 << POS_SYS_REG0X10_INT_EN_SPI);
    spi_param.spi_state=1;

    if(SPI_REG0X2_STAT&(1<<POS_SPI_REG0X2_TX_FINISH_INT))
    {
        
        SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_TX_FINISH_INT);
        
        if(spi_param.write_complete_cb!=NULL)
            spi_param.write_complete_cb();
    }
    if(SPI_REG0X2_STAT&(1<<POS_SPI_REG0X2_RX_FINISH_INT))
    {
        SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_RX_FINISH_INT);
        //if(spi_param.read_complete_cb!=NULL)
        //    spi_param.read_complete_cb();
        
    }
    if(SPI_REG0X2_STAT&(1<<POS_SPI_REG0X2_RXOVF))
    {
        SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_RXOVF);
    }
    if(SPI_REG0X2_STAT&(1<<POS_SPI_REG0X2_RXFIFO_INT))
    {
        SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_RXFIFO_INT);
    }
    
}
void spi_waitbusying(void)
{
    while(spi_param.spi_state==0)
    {
       uart_printf("spi_waitbusying\n");
       Delay_us(10);
    }
    spi_param.spi_state=0;
}

uint8_t spi_send_state_get(void)
{
#if(SPI_DRIVER) 
    return spi_param.spi_state;
#else
    return 1;
#endif
}

void spi_dma_write(uint8_t *buffer,uint16_t buffer_len,spi_write_cb result_callback)
{ 
    if(buffer_len>0xfff)
        buffer_len=0xfff;
    
    spi_param.write_complete_cb = result_callback;
    spi_param.read_complete_cb = NULL;
    spi_waitbusying(); 

    dma_config_write(0,SPI_REQ,(uint32_t)buffer,SPI_REG0X3_DAT_ADDR,buffer_len,DMA_DW_8B);    

    spi_send(buffer_len);
    
}

void spi_dma_read(uint8_t *buffer,uint16_t buffer_len,spi_read_cb result_callback)
{ 
    if(buffer_len>0xfff)
        buffer_len=0xfff;
    
    spi_param.read_complete_cb = result_callback;

    spi_waitbusying(); 

    dma_config_read(0,SPI_REQ,SPI_REG0X3_DAT_ADDR,(uint32_t)buffer,buffer_len,DMA_DW_8B);    

    spi_receiv(buffer_len);

}
void spi_dma_write_result_callback(void)
{
    SPI_REG0X1_CN &= ~(1<<POS_SPI_REG0X1_TX_EN);
    uart_printf("write complete\r\n");
}
void spi_dma_read_result_callback(void)
{
    SPI_REG0X1_CN &= ~(1<<POS_SPI_REG0X1_RX_EN);
    uart_printf("read complete\r\n");
}


/*************************************************************************
**函数名称:void spi_master_test(void)
**功能描述:直接调用，可进行做主测试,可以把mosi 与miso接到一起进行测试
*************************************************************************/
void spi_master_test(void)
{
    int i;
    uint8_t spi_wbuf[64];
    uint8_t spi_rbuf[64];
    static uint8_t debug_cnt=0;
    
    debug_cnt++;
    for(i=0;i<64;i++)
    {
        spi_wbuf[i]=i+debug_cnt;
        spi_rbuf[i]=0;
    }
    
    spi_write_read(&spi_wbuf[0],64,&spi_rbuf[0],64);
    
#if 1 
    for(i=0;i<64;i++)
    {
        uart_printf("spi_rbuf[%d]=%x\r\n",i,spi_rbuf[i]);

        if(spi_rbuf[i]!=spi_wbuf[i])
        {
            //uart_printf("read error~~~~spi_wbuf[%x]=%x,spi_rbuf[%x]=%x\n",i,spi_wbuf[i],i,spi_rbuf[i]);
            //while(1);
        }
    }
    
    memset(spi_rbuf,0,64);
#endif 

}

#define SPI_SLAVE_BUFFER_SIZE 80
uint8_t spi_buf[SPI_SLAVE_BUFFER_SIZE],spi_rxbuf[SPI_SLAVE_BUFFER_SIZE];
uint8_t spi_slave_status = 0;


/*************************************************************************
below SPI Slave code
*************************************************************************/
void spi_slave_init(void)
//note:the first tx dta is :0x72 (8bites),0x7232(16bites)
{
        int i;

    SET_SPI_POWER_UP;
    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);

    // Enable GPIO P0.4, P0.5, P0.6, P0.7 peripheral function for spi
    gpio_config(0x04,SC_FUN,PULL_NONE);
    gpio_config(0x05,SC_FUN,PULL_NONE);
    gpio_config(0x06,SC_FUN,PULL_NONE);
    gpio_config(0x07,SC_FUN,PULL_NONE);

  //  gpio_set_monitor(0x04);
  //  gpio_set_monitor(0x05);
   // gpio_set_monitor(0x07);

    SPI_REG0X0_CFG = 0;
    SPI_REG0X0_CFG = (0x00UL << POS_SPI_REG0X0_SPIEN)
                    //|( 0x1 << POS_SPI_REG0X0_MSTEN)           //master mode //set slave mode
                    | (CKPHA_CLK1 << POS_SPI_REG0X0_CKPHA)
                    | (CKPOL_L << POS_SPI_REG0X0_CKPOL)
                    | (SPI_CHARFORMAT_8BIT << POS_SPI_REG0X0_BIT_WDTH)
                    | (0x01UL << POS_SPI_REG0X0_SLV_RELEASE_INTEN)
                    //| (0x01UL << POS_SPI_REG0X0_NSSMD)  //
                    //| (freq_div << POS_SPI_REG0X0_SPI_CKR)  //clk div
                    //| (0x00UL << POS_SPI_REG0X0_RXINT_EN)
                    | (0x01UL << POS_SPI_REG0X0_RXFIFO_INT_EN)  // enable rxint
                    | (0x00UL << POS_SPI_REG0X0_TXFIFO_INT_EN)
                    | (0x00UL << POS_SPI_REG0X0_RXOVF_EN)
                    | (0x00UL << POS_SPI_REG0X0_TXUDF_EN)
                    //| (0x00UL << POS_SPI_REG0X0_RXINT_MODE)
                    | (0x01UL << POS_SPI_REG0X0_RXFIFO_INT_LEVEL)  //rx
                    | (0x01UL << POS_SPI_REG0X0_TXFIFO_INT_LEVEL);
    SPI_REG0X1_CN =   (1<<POS_SPI_REG0X1_TX_EN)
                                |(1<<POS_SPI_REG0X1_RX_EN)
                                |(1<<POS_SPI_REG0X1_TX_FINISH_INT_EN)
                                |(1<<POS_SPI_REG0X1_RX_FINISH_INT_EN);


    SYS_REG0X4_CLK_SEL |= (1<<POS_SYS_REG0X4_SPICLK_SEL);////PLL 96M CLK
        SPI_REG0X0_CFG |= (0x01UL << POS_SPI_REG0X0_SPIEN);

      SPI_REG0X2_STAT |= (0x03UL << POS_SPI_REG0X2_TXFIFO_CLR);


    for(i = 0;i < 10; i++)
    {
        SPI_REG0X3_DAT = *p_spi_txdata;
                p_spi_txdata ++;
    }

    uart_printf("SPI_CTRL 1 slave = %x\r\n",SPI_REG0X0_CFG);


}

void spi_slave_start(void)
{
        SPI_REG0X0_CFG |= 0x20;  //open rxint_en
}

void spi_slave_stop()
{
    SPI_REG0X0_CFG = 0;
    SET_SPI_POWER_DOWN;
    SYS_REG0X10_INT_EN &= ~(0x01 << POS_SYS_REG0X10_INT_EN_SPI);

    //REG_AHB0_ICU_SPICLKCON    = 0x1 ;                     //SPI clock enable
    //    REG_AHB0_ICU_INT_ENABLE  &= ~INT_STATUS_SPI_bit;      //IRQ UART interrupt enable
       SPI_REG0X2_STAT = 0;
}

unsigned char terminal_value;


//after cs high，init REG_APB2_SPI_DAT(10byte)
void spi_slave_done_data(void)
{
    int i;
    if( spi_slave_status == 1) //cs inactinve
    {
        uart_printf("spi len=%x:",terminal_value);
        for(i=0;i<terminal_value;i++)
        {
            uart_printf("%x,",spi_rxbuf[i]);
        }
        uart_printf("\r\n");
        memset(spi_rxbuf,0,SPI_SLAVE_BUFFER_SIZE);

        for(i = 0;i < 10; i++)
        {
            spi_buf[i]++;
            SPI_REG0X3_DAT = spi_buf[i];
        }

        p_spi_txdata =spi_buf;
           p_spi_rxdata = spi_rxbuf;
        spi_slave_status = 0;
    }
}
void spi_slave_isr(void)
{
    unsigned int            rxint;
    unsigned char           rxfifo_en = 0;


    rxint = SPI_REG0X2_STAT & 0x0600;
    if(rxint)
    {
        rxfifo_en = SPI_REG0X2_STAT & 0x0004;
        terminal_value = 0;
        while(rxfifo_en)
        {
            *p_spi_rxdata = SPI_REG0X3_DAT;
            //uart_printf("%d\r\n",SPI_REG0X3_DAT);
            p_spi_rxdata ++;

            rxfifo_en = SPI_REG0X2_STAT & 0x0004;

            terminal_value++;


            //break;
        }

        if(rxint&0x600)
        {
            spi_slave_status=1;
        }


        SPI_REG0X2_STAT |= 0x0600; //clear rxint
    }
}

/*************************************************************************
**函数名称:void spi_slave_test(void)
**功能描述:直接调用，可进行做从测试
*************************************************************************/
void spi_slave_test(void)
{
    /****************************************************
    **SPI slave test
    ****************************************************/
    int i;
      for(i=0;i<80;i++)
    {
        spi_buf[i]=i+0x51;//+0x56;
        spi_rxbuf[i]=i+0x20;//0x78;
    }
       p_spi_txdata =spi_buf;
       p_spi_rxdata = spi_rxbuf;
    spi_slave_init();

    //spi_init(1,3,0);

    //SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);

    //spi_slave_start();
    gpio_config(0x32,OUTPUT,PULL_NONE);

    gpio_config(0x31,OUTPUT,PULL_NONE);
    gpio_set_neg(0x32);
    gpio_set_neg(0x32);
    gpio_set_neg(0x32);
    gpio_set_neg(0x32);
    while(1)
    {
        spi_slave_done_data();
    }
}





#endif

/// @} SPI DRIVER

