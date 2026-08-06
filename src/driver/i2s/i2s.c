
#include <stdint.h>        // standard integer definition
#include <string.h>        // string manipulation
#include <stddef.h>        // standard definition
#include "i2s.h"
#include "BK_HCI_Protocol.h"
#include "uart.h"
#include "gpio.h"
#include "BK3633_RegList.h"
#include "icu.h"


#if     ( I2S_DRIVER )
#define NUMBER_ROUND_UP(a,b)        ((a) / (b) + (((a) % (b)) ? 1 : 0))

volatile i2s_message        i2s_msg;
unsigned int    *p_i2s_tx0_data;
//unsigned int    *p_i2s_tx1_data;
//unsigned int    *p_i2s_tx2_data;
unsigned int    *p_i2s_rx_data;

uint8_t master_mode =0 ;
uint8_t   master_step =0 ;

I2S_TRX_FLAG  test_step0 ;
I2S_TRX_FLAG  test_step1 ;
I2S_TRX_FLAG  test_step2 ;
I2S_TRX_FLAG  test_step3 ;

I2S_TRX_FLAG  *test_step0_p ;
I2S_TRX_FLAG  *test_step1_p ;
I2S_TRX_FLAG  *test_step2_p ;
I2S_TRX_FLAG  *test_step3_p ;
I2S_TRX_FLAG  *test_stepx_p;

uint32_t        rxbuf[100] ;

I2S_PARAM        my_i2s_param ;
extern volatile uint32_t XVR_ANALOG_REG_BAK[32];

void DelayNops(volatile unsigned int nops)
{
    while (nops --)
    {
    }
}

void set_I2s_pll_clk(unsigned long smp_ratio)
{
//set xvr.0x0f,the i2s  Fsys_clk = Fvco /div,0x0f = Fvco*2^24/8M
//set div is xvr.0x0e.bit29  0 div=3*2,1 div = 4*2
// Fvco需要根据采样率和datalen来确定，并且实际的Fsys需要再除4，以下给的是按照16bits来处理的参考值。
//SMP = datalen(4的倍数)，BIT 是要填入posI2S0_Reg0x0_BITRATIO的BIT = Fsys/2/(Fs*2*(SMP+1)), Fsys=Fsys_clk/4,Fs:实际采样率
    switch(smp_ratio)
    {
        case 44100:
            //the Fvco = 361.2672,clk maybe= 11.2896M :div =1,
            XVR_ANALOG_REG_BAK[0xf] = 0x2D288CE7;
            addXVR_Reg0xf = 0x2D288CE7  ;
            XVR_ANALOG_REG_BAK[0xe] |= 1<<29  ;
            break;
        case 48000:
            //393.216,clk maybe= 48k, div =1,
            XVR_ANALOG_REG_BAK[0xf] = 0x3126E978;
            addXVR_Reg0xf = 0x3126E978  ;
            XVR_ANALOG_REG_BAK[0xe] |= 1<<29  ;
            break;
        case 16000:
        case 8000:
        default:
            //393.216,clk maybe= 32k,div=0,1
            XVR_ANALOG_REG_BAK[0xf] = 0x3126E978;
            addXVR_Reg0xf = 0x3126E978  ;
            XVR_ANALOG_REG_BAK[0xe] |= 1<<29  ;
            break;
    }
    XVR_ANALOG_REG_BAK[0xe] |= 1ul<<31;
    addXVR_Reg0xe = XVR_ANALOG_REG_BAK[0xe];

 //   addPMU_Reg0x10 |= 1<<18;
 //   addPMU_Reg0x12 &= ~(1<<18);

    XVR_ANALOG_REG_BAK[0xe] |= 1<<18;
    addXVR_Reg0xe = XVR_ANALOG_REG_BAK[0xe];

    XVR_ANALOG_REG_BAK[0xe] &= ~(1<<18);
    addXVR_Reg0xe = XVR_ANALOG_REG_BAK[0xe];
}
//=============================================================
//                      I2S-DMA
//=============================================================
void    i2s_dma_intial(void)
{

}
//=============================================================
//                      I2S
//=============================================================
void    i2s_master_param_flush (void)
{
    my_i2s_param.i2s_enable            = 0x1;
    my_i2s_param.master_nslave      = MASTER    ;
    my_i2s_param.i2s_mode            = PHILIPS;
    my_i2s_param.lrck_pol            = 0x0;
    my_i2s_param.sck_pol            = 0x0;
    my_i2s_param.lsb_first          = 0x1;
    my_i2s_param.sync_len            = 0x0;
    my_i2s_param.data_len           = I2S_DATA_LEN32;
    my_i2s_param.pcm_d_len          = 0x0;
    my_i2s_param.samp_radio            = 0x1F;
    my_i2s_param.bit_radio          = 0x01;


    my_i2s_param.txint_levl            = I2S_TXBUF_LESS_16;
    my_i2s_param.rxint_levl            = I2S_RXBUF_MORE_16;
    my_i2s_param.txint_dnov_en      = OPEN_INT;
    my_i2s_param.rxint_upov_en      = CLOSE_INT;
    my_i2s_param.txint_en           = OPEN_INT ;
    my_i2s_param.rxint_en           = CLOSE_INT;

};

void    i2s_slave_param_flush (void)
{
    my_i2s_param.i2s_enable            = 0x1;
    my_i2s_param.master_nslave      = SLAVE    ;
    my_i2s_param.i2s_mode            = PHILIPS;
    my_i2s_param.lrck_pol            = 0x0;
    my_i2s_param.sck_pol            = 0x0;
    my_i2s_param.lsb_first          = 0x1;
    my_i2s_param.sync_len            = 0x0;
    my_i2s_param.data_len           = I2S_DATA_LEN32;
    my_i2s_param.pcm_d_len          = 0x0;
    my_i2s_param.samp_radio            = 0x1F;
    my_i2s_param.bit_radio          = 0x0;

    my_i2s_param.txint_levl            = I2S_TXBUF_LESS_16;
    my_i2s_param.rxint_levl             = I2S_RXBUF_MORE_8;
    my_i2s_param.txint_dnov_en      = CLOSE_INT;
    my_i2s_param.rxint_upov_en      = OPEN_INT;
    my_i2s_param.txint_en           = CLOSE_INT ;
    my_i2s_param.rxint_en           = OPEN_INT ;

}

extern void Delay(int times);

void i2s_init(unsigned long smp_ratio, unsigned char role, unsigned char data_width,
              unsigned char mode_sel, unsigned long data_cnt)
{
    uint32_t freq_clk;
    unsigned long bitclk_div;
    unsigned long lrck_div;

    clrf_SYS_Reg0x3_i2s_pwd;
  //  XVR_ANALOG_REG_BAK[0xe] |= (1<< 31)/*|(1<< 29)*/ ;
  //  addXVR_Reg0xe = XVR_ANALOG_REG_BAK[0xe]; //open apll
    set_I2s_pll_clk(smp_ratio);
    //addXVR_Reg0xf = 0x3126E978  ;
    //XVR_ANALOG_REG_BAK[0xf] = 0x3126E978;//open apll

    setf_SYS_Reg0x4_i2smclk_en;
    Delay(2000);  //wait pll_steady
    setf_SYS_Reg0x10_int_i2s_en;

    // Enable GPIO P0.2, P0.3 peripheral function for I2C
    gpio_config(0x25,SC_FUN,PULL_NONE);
    gpio_config(0x26,SC_FUN,PULL_NONE);
    gpio_config(0x27,SC_FUN,PULL_NONE);
    gpio_config(0x30,SC_FUN,PULL_NONE);
    gpio_config(0x23,SC_FUN,PULL_NONE);

    if (smp_ratio != 8000 && smp_ratio != 16000 && smp_ratio != 44100 && smp_ratio != 48000)
    {
        return;
    }
    if (data_width < 9)
    {
        lrck_div = 7;
    }
    else if (data_width < 17)
    {
        lrck_div = 15;
    }
    else if (data_width < 21)
    {
        lrck_div = 19;
    }
    else if (data_width < 25)
    {
        lrck_div = 23;
    }
    else
    {
        lrck_div = 31;
    }

    switch(smp_ratio)
    {
        case 48000:
            freq_clk = I2s_48000_CLK;
            break;
        case 44100:
            freq_clk = I2s_44100_CLK;
            break;
        case 16000:
            freq_clk = I2s_16000_CLK;
            break;
        default:
        case 8000:
            freq_clk = I2s_8000_CLK;
            break;
    }
    bitclk_div =  NUMBER_ROUND_UP((freq_clk / 2 ), (smp_ratio * 2 * (lrck_div + 1)));
    uart_printf("bitclk_div = 0x%lx,%x,%x,%x\r\n",
    freq_clk,bitclk_div,lrck_div,smp_ratio);

    addI2S0_Reg0x0 = ((bitclk_div & 0x00FF) << posI2S0_Reg0x0_BITRATIO)
                   | ( lrck_div             << posI2S0_Reg0x0_SMPRATIO)
                   | ( data_width           << posI2S0_Reg0x0_DATALEN)
                   | ((role & 0x01)         << posI2S0_Reg0x0_MSTEN)
                   | ((mode_sel & 0x07)     << posI2S0_Reg0x0_MODESEL);


    addI2S0_Reg0x1 = (0 <<posI2S0_Reg0x1_BITRATIO_H4B)  +
                     (0 <<posI2S0_Reg0x1_SMPRATIO_H2B)  +
                    (0x01 << posI2S0_Reg0x1_RXFIFO_CLR)  +
                    (0x01 << posI2S0_Reg0x1_TXFIFO_CLR)  +
                    (I2S_TXBUF_LESS_16 <<posI2S0_Reg0x1_TXINT_LEVEL)  +
                    (I2S_TXBUF_LESS_16 <<posI2S0_Reg0x1_RXINT_LEVEL)  +
                    (1 <<posI2S0_Reg0x1_TXUDF_EN)  +
                    (1 <<posI2S0_Reg0x1_RXOVF_EN)  +
                    (1 <<posI2S0_Reg0x1_TXINT_EN)  +
                    (1 <<posI2S0_Reg0x1_RXINT_EN)  ;

    addI2S0_Reg0x2 = 0x3f;
    addI2S0_Reg0x5 = 0xfff;

    i2s_msg.rxfifo_level = 1;
    i2s_msg.txfifo0_level = 1;
    i2s_msg.remain_data_cnt = data_cnt;
    i2s_msg.trans_done = 0;
}

void i2s_disable(void)
{
    clrf_I2S0_Reg0x0_I2SPCMEN;
    setf_SYS_Reg0x3_i2s_pwd;
    clrf_SYS_Reg0x10_int_i2s_en;
}

void i2s_enable(void)
{
    setf_I2S0_Reg0x0_I2SPCMEN;
    clrf_SYS_Reg0x3_i2s_pwd;
    setf_SYS_Reg0x10_int_i2s_en;

}

void i2s_clrfifo(unsigned char fifo_idx)
{
    //[0]:rxfifo, [1]:txfifo0,
     addI2S0_Reg0x1 |= fifo_idx << 8;
}

void i2s_isr(void)
{
    unsigned long        i2s_stat;
    unsigned char        rxint, txint0;
    unsigned int        i;
    volatile unsigned int data_num;
    uart_printf("i2s_isr\r\n");
    i2s_stat = addI2S0_Reg0x2;
//    rxint  = i2s_stat & (0x01 | 0x10);
//    txint0 = i2s_stat & (0x02 | 0x20);
    rxint  = i2s_stat & 0x01; // RxINT
    txint0 = i2s_stat & 0x02; // TxINT
    //txint1 = i2s_stat & 0x04;
    //txint2 = i2s_stat & 0x08;

//    printf("REG_I2S_STATUS = 0x%lx\r\n", i2s_stat);
    if (txint0)
    {
        switch (i2s_msg.txfifo0_level)
        {
            case 0    :     data_num = 24;  break;
            case 1    :     data_num = 16;  break;
            case 2    :     data_num = 8;   break;
            default    :      data_num = 32;  break;
        }

        for (i=0; i<data_num; i++)
        {
            addI2S0_Reg0x3 = *p_i2s_tx0_data;
            p_i2s_tx0_data ++;
        }
        i2s_stat |= 0x2;
    }

    if (rxint)
    {
        switch (i2s_msg.rxfifo_level)
        {
            case 0    :     data_num = 24;  break;
            case 1    :     data_num = 16;  break;
            case 2    :     data_num = 8;   break;
            default    :      data_num = 32;  break;
        }
        if (data_num > i2s_msg.remain_data_cnt)
        {
            data_num = i2s_msg.remain_data_cnt;
        }
        for (i=0; i<data_num; i++)
        {
            *p_i2s_rx_data = addI2S0_Reg0x3;
            p_i2s_rx_data ++;
        }
        i2s_msg.remain_data_cnt -= data_num;
        if (i2s_msg.remain_data_cnt <= 0)
        {
            i2s_msg.trans_done = 1;
        }
        i2s_stat |= 0x1;
    }

    addI2S0_Reg0x2 = i2s_stat;
}

#define I2S_DATA_SIZE            150
unsigned int    i2s_tx_data[I2S_DATA_SIZE], i2s_rx_data[I2S_DATA_SIZE];
unsigned int     dbg_cnt = 0;

void I2S_Test(unsigned char role)    //0: Slave; 1: Master
{
    int             i, j;
    unsigned char    data_width;//, role;
    unsigned long    /*temp,*/ err_cnt, total_err_cnt;
    unsigned char   mode_sel_cnt, smp_ratio_cnt;
    unsigned char   mode_sel[7] = {0, 1, 2, 4, 5, 6, 7};
    unsigned long   smp_ratio[4] = {8000, 16000, 44100, 48000};
    unsigned int    i2s_rx_expected_data[I2S_DATA_SIZE];
    extern    void Delay_ms(int num);
    uart_printf("----- I2S_Test %s start -----\r\n", (role) ? ("Master") : ("Slave"));

//    err_cnt = 0;
    total_err_cnt = 0;

//    role = 0;    //0: Slave; 1: Master
    if (role)
    {
        for (i=0; i<TEST_DATA_LEN+8; i++)
        {
            i2s_tx_data[i] = ((i+1)<<24) | ((i+1)<<16)  | ((i+1)<<8) | ((i+1)<<0) | 0x80808080;
        }
        for (i=0; i<TEST_DATA_LEN+8; i++)
        {
            i2s_rx_expected_data[i] = ((i+1)<<24) | ((i+1)<<16)  | ((i+1)<<8) | ((i+1)<<0) | 0xC0C0C0C0;
        }
        uart_printf("Master:\r\n");
    }
    else
    {
        for (i=0; i<TEST_DATA_LEN+8; i++)
        {
            i2s_tx_data[i] = ((i+1)<<24) | ((i+1)<<16)  | ((i+1)<<8) | ((i+1)<<0) | 0xC0C0C0C0;
        }
        for (i=0; i<TEST_DATA_LEN+8; i++)
        {
            i2s_rx_expected_data[i] = ((i+1)<<24) | ((i+1)<<16)  | ((i+1)<<8) | ((i+1)<<0) | 0x80808080;
        }
        uart_printf("Slave:\r\n");
    }

    //if(!role) PERI_PWDS &= ~bit_PCM2_CLK_SEL;     //select xtal as pcm clock in slave mode

    for (smp_ratio_cnt=0; smp_ratio_cnt<4; smp_ratio_cnt++)
    {
//        if (smp_ratio == 8000)        // 8KHz
//         {
//             mode_sel_start = 0;
//             mode_sel_end = 3;
//             data_width = 15;
//         }
//         else    // 8KHz
//         {
//             mode_sel_start = 4;
//             mode_sel_end = 6;
//             data_width = 15;
//         }

        smp_ratio_cnt = 1;

        for (data_width=15; data_width<31; data_width+=4)
        {
            //for (mode_sel_cnt=0; mode_sel_cnt<7; mode_sel_cnt++)
             mode_sel_cnt = 0;
//             data_width = 15;
            {
                //printf("DataWidth = %d, ModeSel = %d!\r\n",data_width, mode_sel);
                mode_sel_cnt = 0;

                uart_printf("\r\nsmp_ratio_cnt=%d, role=%d, data_width=%d, mode_sel_cnt=%d\r\n",
                        smp_ratio_cnt, role, data_width, mode_sel_cnt);
                i2s_init(smp_ratio[smp_ratio_cnt], role, data_width, mode_sel[mode_sel_cnt], TEST_DATA_LEN);

//                 for (j = 0; j < 150; j ++)
//                 {
//                     temp = 65536 - (j + 1) * (smp_ratio + 1) * data_width * 100;
//                     switch (data_width)
//                     {
//                         case 7     :     temp_data = temp & 0xff;        break;
//                         case 15    :    temp_data = temp & 0xffff;      break;
//                         case 23    :    temp_data = temp & 0xffffff;    break;
//                         default    :    temp_data = temp;                break;
//                     }
//                     i2s_tx_data[j] = temp_data;
//                 }
//                if (role)
//                {
//                    memset(i2s_rx_data, 0, sizeof(i2s_rx_data));
//                }
//                else
                {
                    memset(i2s_rx_data, 0, sizeof(i2s_rx_data));
                }

                p_i2s_tx0_data = i2s_tx_data + mode_sel_cnt;
                //p_i2s_tx1_data = i2s_tx_data;
                //p_i2s_tx2_data = i2s_tx_data;
                p_i2s_rx_data = i2s_rx_data;

                if (role)
                {
                    Delay_ms(3000);
                }
                else
                {
                    Delay_ms(1000);
                }
                i2s_enable();
                //gpio_sendout(0xcd);

                while (!i2s_msg.trans_done)
                {
                    //dbg_cnt ++;
                }
                i2s_msg.trans_done = 0;

                //gpio_sendout(0xce);

                i2s_disable();
                i2s_clrfifo(3);

                //compare Rx Data
#if 0
                if (role)
                {
                    err_cnt = 0;
                    i = 0;
                    j = 0;
                    while (!i2s_rx_data[i])
                    {
                        i++;
                    }
                    while (i < TEST_DATA_LEN)
                    {
                        if (i2s_rx_data[i] != i2s_tx_data[j])
                        {
                            //printf("I2S Data Error! should be(%02X) but(%02X)!\r\n", i2s_tx_data[i], i2s_rx_data[i]);
                            err_cnt ++;
                        }
                        //else
                            //printf("I2S Data OK! should be(%02X) actual be(%02X)!\r\n", i2s_tx_data[i], i2s_rx_data[i]);
                        i ++;
                        j ++;
                    }
                    //printf("I2S Error Counter = %d!\r\n", err_cnt);
//                    gpio_sendout(0xb0);
//                    gpio_sendout(err_cnt);  //output compare result
                    DelayNops(200);
//                    gpio_sendout(0xb1);
                    total_err_cnt += err_cnt;
                }
                printf("I2S Error Counter = %d\r\n", err_cnt
);
#else
                do
                {
                    unsigned long ulTemp;
                    ulTemp = 0;
                    err_cnt = 0;
                    for (i=0; i<=data_width; i++)
                    {
                        ulTemp |= (0x01UL << i);
                    }
                    uart_printf("ulTemp = 0x%x\r\n", ulTemp);

                    for (i=0; i<TEST_DATA_LEN; i++)
                    {
                        if (i2s_rx_data[i] != 0)
                        {
                            break;
                        }
                    }
                    if (i == TEST_DATA_LEN)
                    {
                        uart_printf("i2s_rx_data search failed!\r\n");
                        break;        // 没收到数据
                    }

                    for (j=0; j<TEST_DATA_LEN; j++)
                    {
                        if (i2s_rx_data[i] == (i2s_rx_expected_data[j] & ulTemp))
                        {
                            break;
                        }
                    }
                    if (j == TEST_DATA_LEN)
                    {
                        uart_printf("i2s_rx_expected_data search failed!\r\n");
                        break;        // 没找到与第一个接收到的数据对应的i2s_rx_expected_data的数据
                    }
                    j = j-i;
                    uart_printf("i = %d, j = %d\r\n", i, j);

                    if (role)
                    {
                        for (i=0; i<TEST_DATA_LEN; i++)
                        {
                            if (i2s_rx_data[i] != (i2s_rx_expected_data[i+j] & ulTemp))
                            {
                                *((volatile unsigned long *) 0x00F50000UL + 0x00 * 4) = i2s_rx_data[i];
                                uart_printf("i2s_rx_data[%d]=0x%x, i2s_rx_expected_data[%d]=0x%x\r\n",
                                        i, i2s_rx_data[i], i+j, i2s_rx_expected_data[i+j] & ulTemp);
                                err_cnt ++;
                            }
                        }
                    }
                    else
                    {
                        *((volatile unsigned long *) 0x00F50000UL + 0x01 * 4) = i2s_tx_data[i];
                    }
                    total_err_cnt += err_cnt;
                    uart_printf("I2S Error Counter = %d\r\n", err_cnt);
                }
                while (0);
#endif
                for (i=0; i<TEST_DATA_LEN; i++)
                {
                    uart_printf("i2s_rx_data[%d]=0x%x, i2s_tx_data[%d]=0x%x\r\n", i, i2s_rx_data[i], i, i2s_tx_data[i]);
                }
                DelayNops(200);
            }
        }
    }

//    if (role)
    {
        uart_printf("total_err_cnt = %d\r\n", total_err_cnt);
//        gpio_sendout(0xe0);
//        gpio_sendout(total_err_cnt);  //output compare result
//        DelayNops(200);
//        gpio_sendout(0xa5);
    }

    uart_printf("----- I2S_Test %s over  -----\r\n", (role) ? ("Master") : ("Slave"));
}

#endif
