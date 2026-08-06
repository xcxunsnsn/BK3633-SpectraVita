#include "dma.h"       // uart definition
#include "spi.h"
extern spi_param_t    spi_param;
extern void uart1_dma_isr_callback(uint8_t channel);

void dma_config_write(uint8_t channel,uint8_t req_mux,uint32_t src_addr,uint32_t dst_addr,uint16_t buf_len,uint8_t dest_data_width)
{

    uint32_t dma_config_temp;

    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x1*4), dst_addr); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x2*4), src_addr);
    
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x3*4), 0); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x4*4), 0); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x5*4), 0); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x6*4), 0); 
    
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x7*4), (req_mux<<POS_GENER_DMA_REG0X7_DEST_REQ_MUX));
    
    dma_config_temp = ( ((buf_len-1)<<POS_GENER_DMA_REG0X0_TRAN_LEN)|                         
                        (0<<POS_GENER_DMA_REG0X0_CHN_PRIOPRIITY)|                   
                        (0<<POS_GENER_DMA_REG0X0_DEST_ADDR_LOOP)|                   
                        (0<<POS_GENER_DMA_REG0X0_SRC_SDDR_LOOP)|                    
                        (0<<POS_GENER_DMA_REG0X0_DEST_ADDR_INC)|                    
                        (1<<POS_GENER_DMA_REG0X0_SRC_ADDR_INC)|                     
                        (dest_data_width<<POS_GENER_DMA_REG0X0_DEST_DATA_WIDTH)|                  
                        (DMA_DW_32B<<POS_GENER_DMA_REG0X0_SRC_DATA_WIDTH)|                   
                        (0<<POS_GENER_DMA_REG0X0_DMA_MODE)|                         
                        (0<<POS_GENER_DMA_REG0X0_HALF_FINISH_INTEN)|                
                        (1<<POS_GENER_DMA_REG0X0_FINISH_INTEN)|                     
                        (1<<POS_GENER_DMA_REG0X0_DMA_EN) ); 
    
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x0*4), dma_config_temp); 
    
}

void dma_config_read(uint8_t channel,uint8_t req_mux,uint32_t src_addr,uint32_t dst_addr,uint16_t buf_len,uint8_t src_data_width)
{

    uint32_t dma_config_temp;

    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x1*4), dst_addr); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x2*4), src_addr);
    
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x3*4), 0); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x4*4), 0); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x5*4), 0); 
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x6*4), 0); 
    
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x7*4), (req_mux<<POS_GENER_DMA_REG0X7_SRC_REQ_MUX));
    
    dma_config_temp = ( ((buf_len-1)<<POS_GENER_DMA_REG0X0_TRAN_LEN)|                         
                        (0<<POS_GENER_DMA_REG0X0_CHN_PRIOPRIITY)|                   
                        (0<<POS_GENER_DMA_REG0X0_DEST_ADDR_LOOP)|                   
                        (0<<POS_GENER_DMA_REG0X0_SRC_SDDR_LOOP)|                    
                        (1<<POS_GENER_DMA_REG0X0_DEST_ADDR_INC)|                    
                        (0<<POS_GENER_DMA_REG0X0_SRC_ADDR_INC)|                     
                        (DMA_DW_32B<<POS_GENER_DMA_REG0X0_DEST_DATA_WIDTH)|                  
                        (src_data_width<<POS_GENER_DMA_REG0X0_SRC_DATA_WIDTH)|                   
                        (0<<POS_GENER_DMA_REG0X0_DMA_MODE)|                         
                        (0<<POS_GENER_DMA_REG0X0_HALF_FINISH_INTEN)|                
                        (1<<POS_GENER_DMA_REG0X0_FINISH_INTEN)|                     
                        (1<<POS_GENER_DMA_REG0X0_DMA_EN) ); 
    
    REG_WRITE((BASEADDR_GENER_DMA+0x20*channel+0x0*4), dma_config_temp); 
    
}

uint32_t dma_isr_stat_get(void)
{
    return GENER_DMA_REG0XD1;
}

void dma_isr_stat_set(uint32_t val)
{
     GENER_DMA_REG0XD1 = val;
}

void dma_isr(void)
{
    uint32_t state;
    uart_printf("dmaisr:0x%x\n",GENER_DMA_REG0XD1);
    state = GENER_DMA_REG0XD1;
    switch(state)
    {
        case 1:
            if(spi_param.read_complete_cb!=NULL)
                spi_param.read_complete_cb();
            break;
        case 2:
            uart1_dma_isr_callback(1);
            break;
        case 4:
            uart1_dma_isr_callback(2);
            break;


    }

    
    GENER_DMA_REG0XD1=0XFFFFFFFF;
    
    
}

