#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "uart2.h"
#include "user_config.h"
#include "gpio.h"
#include "icu.h"
#include "BK3633_RegList.h"



extern volatile uint32_t XVR_ANALOG_REG_BAK[32];
volatile uint8_t  uart2_rx_done = 0;
volatile uint32_t uart2_rx_index = 0;

uint8_t uart2_rx_buf[UART2_FIFO_MAX_COUNT];
uint8_t uart2_tx_buf[UART2_FIFO_MAX_COUNT];


//==============================UART_INITIAL===================

void uart2_init(uint32_t baudrate)
{
    uint32_t    uart_clk_div;
    
    SET_UART2_POWER_UP ;   //open periph
    uart_clk_div = 16000000/baudrate -1 ;
    
    gpio_config(0x16,SC_FUN,PULL_HIGH);
    gpio_config(0x17,SC_FUN,PULL_HIGH);
    
    UART2_REG0X0 = (uart_clk_div << POS_UART2_REG0X0_CLK_DIVID) |
                                (0x0          << POS_UART2_REG0X0_STOP_LEN ) |
                                (0x0          << POS_UART2_REG0X0_PAR_MODE ) |
                                (0x0          << POS_UART2_REG0X0_PAR_EN   ) |
                                (0x3          << POS_UART2_REG0X0_LEN      ) |
                                (0x0          << POS_UART2_REG0X0_IRDA     ) |
                                (0x1          << POS_UART2_REG0X0_RX_ENABLE) |
                                (0x1          << POS_UART2_REG0X0_TX_ENABLE) ;
    
    UART2_REG0X1 = 0x00004010;
    UART2_REG0X4 = 0x42;
    UART2_REG0X6 = 0x0;
    UART2_REG0X7 = 0x0;

    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_UART2);

    uart2_rx_done = 0;
    uart2_rx_index = 0;

}




static void uart2_send_byte(unsigned char data)
{
    while( 0==(UART2_REG0X2&(1<<POS_UART2_REG0X2_FIFO_WR_READY)) );
    UART2_REG0X3 = data;
}

void uart2_send(void *buff, int len)
{
    uint8_t *tmpbuf = (uint8_t *)buff;
    while (len--)
        uart2_send_byte(*tmpbuf++);
}


void clear_uart2_buffer(void)
{
    uart2_rx_index = 0;
    uart2_rx_done = 0;
    memset(uart2_rx_buf, 0, sizeof(uart2_rx_buf)); /**< Clear the RX buffer */
    memset(uart2_tx_buf, 0, sizeof(uart2_tx_buf)); /**< Clear the TX buffer */
}

 
int uart2_printf(const char *fmt,...)
{
#if (UART_PRINTF_ENABLE)    
    va_list ap;
    int n;
    char send_buf[128];
    va_start(ap, fmt);
    n=vsprintf(send_buf, fmt, ap);
    va_end(ap);
    uart2_send(send_buf, n);
    while(0==(addUART1_Reg0x2&(1<<posUART1_Reg0x2_TX_FIFO_EMPTY)));
    return n;
#else
    return 0;
#endif
}   

int uart2_printf_null(const char *fmt,...)
{
    return 0;

}
//==========================UART_INTERRUPT==========================
void uart2_rx_handler(uint8_t value );
void uart2_isr(void)
{
    unsigned long uart_int_status;
    unsigned char uart_fifo_rdata;    
    uart_int_status = UART2_REG0X5;
    if ( uart_int_status & ( (1<<POS_UART2_REG0X5_RX_FIFO_NEED_READ)|(1<<POS_UART2_REG0X5_UART_RX_STOP_END ) ) )
    {    
        while(UART2_REG0X2 & (1<<POS_UART2_REG0X2_FIFO_RD_READY))
        {
            uart_fifo_rdata = (UART2_REG0X3>>8);

            uart2_rx_buf[uart2_rx_index] = uart_fifo_rdata;         
            uart2_rx_handler(uart2_rx_buf[uart2_rx_index]);
            uart2_rx_index++;
            if (uart2_rx_index == UART2_FIFO_MAX_COUNT)
            {
                uart2_rx_index = 0;
            }
        }

        if ( uart_int_status & (1<<POS_UART2_REG0X5_UART_RX_STOP_END ) )
        uart2_rx_done = 1;    // 此处应该先判断是否满足RX STOP
    }

    UART2_REG0X5 = uart_int_status;
}


void uart2_rx_cmd_respone(uint8_t *buff,uint8_t len )
{
    uint8_t respone_buff[32];
    
    respone_buff[0]=0x04;
    respone_buff[1]=0x0e;
    respone_buff[2]=4+len;
    respone_buff[3]=0x01;
    respone_buff[4]=0xe0;
    respone_buff[5]=0xfc;
    respone_buff[6]=len;
    
    memcpy(&respone_buff[7],buff,len);

    uart2_send(respone_buff,7+len);


}
void uart2_rx_cmd_handler(uint8_t *buff,uint8_t len )
{      
    uint32_t rx_command;
    uint8_t length;
    static uint8_t rx_cmd_count=0;
    length=len;
    switch(buff[0])
    {
        case UART_LINK_CHECK:
        {
            if(length==1)
            {   
                if(rx_cmd_count++>2)
                    icu_set_sleep_mode(NO_SLEEP);   
                buff[1]=0;
            }
        }            
        break;
        case CMD_SYS_RESET:
        {    
            if(length==5)
            {
                rx_command = (buff[1]<<24|buff[2]<<16|buff[3]<<8|buff[4]);
                if(rx_command==0x95279527)
                {
                    cpu_reset();
                    
                }
            }
            length -=1;
        }
        break;
        #if(DEBUG_RF_REG)          
        case REG_WRITE_CMD:
        {
            uint32_t reg_index;
            uint32_t reg_addr;
            uint32_t reg_value; 

            reg_addr=buff[1]+(buff[2]<<8)+(buff[3]<<16)+(buff[4]<<24); 
            reg_value=buff[5]+(buff[6]<<8)+(buff[7]<<16)+(buff[8]<<24);
            reg_index = (reg_addr-BASEADDR_XVR)/4;

            *(volatile uint32_t *)reg_addr = reg_value;
            
            if(reg_index<=0x1f)
            {
                XVR_ANALOG_REG_BAK[reg_index] = reg_value;
            }
            
        }       
        break;
        case REG_READ_CMD:
        {
            uint32_t reg_index;
            uint32_t reg_addr;
            uint32_t reg_value; 

            reg_addr=buff[1]+(buff[2]<<8)+(buff[3]<<16)+(buff[4]<<24);  
            
            reg_index = (reg_addr-BASEADDR_XVR)/4;

            if (reg_index<=0x0f)
            {
                reg_value  = XVR_ANALOG_REG_BAK[reg_index];    
            }
            else if( (reg_index>0x1b) && (reg_index<=0x1f) )
            {
                reg_value  = XVR_ANALOG_REG_BAK[reg_index];    
            }
            else
            {
                reg_value  = *(volatile uint32_t *)reg_addr;              
            }
            
            memcpy(&buff[5],&reg_value,4);
            length += 3;
            
        }
        break;
        #endif        
    }
  
    #if(DEBUG_RF_REG)       
    uart2_rx_cmd_respone(&buff[1],length);
    #endif

}

void uart2_rx_handler(uint8_t value )
{

    static uint8_t cmd_status=0;
    static uint16_t index=0;
    static uint16_t length;
    static uint8_t uart_cmd[32];

                      
    switch(cmd_status)
    {        
        case UART_CMD_STATE_HEAD:
        {
            if(value==0x01)
            {
                cmd_status=UART_CMD_STATE_OPCODE_ONE;              
            }
            else
                cmd_status=UART_CMD_STATE_HEAD;
        }
        break;
        case UART_CMD_STATE_OPCODE_ONE:
        {
            if( value==0xe0 )                    
                cmd_status=UART_CMD_STATE_OPCODE_TWO;
            else
                cmd_status=UART_CMD_STATE_HEAD;     
        }
        break;
        case UART_CMD_STATE_OPCODE_TWO:
        {
            if( value==0xfc )                    
                cmd_status=UART_CMD_STATE_LENGTH;
            else
                cmd_status=UART_CMD_STATE_HEAD;      
        }
        break;
        case UART_CMD_STATE_LENGTH:
        {
            length=value;           
            if( length>0 )                    
            {
                cmd_status=UART_CMD_STATE_CMD;
                index=0;   
            }
            else
                cmd_status=UART_CMD_STATE_HEAD;                         
        }
        break;       
        case UART_CMD_STATE_CMD:
        {
            uart_cmd[index++]=value;           
            if(index==length)
            {              
                uart2_rx_cmd_handler(uart_cmd,length);
                cmd_status=UART_CMD_STATE_HEAD;
            }        
        }
        break;           
    }         
}



