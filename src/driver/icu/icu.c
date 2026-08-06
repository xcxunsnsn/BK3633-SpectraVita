/**
****************************************************************************************
*
* @file icu.c
*
* @brief icu initialization and specific functions
*
* Copyright (C) Beken 2009-2016
*
* $Rev: $
*
****************************************************************************************
*/
#include <stddef.h>     // standard definition
#include "rwip_config.h"
#include "rwip.h"
#include "reg_ipcore.h"
#include "icu.h"      // timer definition
#include "rf.h"
#include "icu.h"      // timer definition
#include "uart.h"
#include "flash.h"
#include "BK3633_RegList.h"
#include "rf.h"
#include "gpio.h"


#if 1
static volatile uint8_t reduce_voltage_set=0;
static uint8_t default_sleep_mode = 0;  //0:降压休眠   1:普通idle
static uint8_t system_clk=0;
//uint8_t system_sleep_flag;
#endif
extern void CLK32K_AutoCali_init(void);
extern volatile uint32_t XVR_ANALOG_REG_BAK[32];
uint8_t system_mode;
void sys_mode_init(uint8_t mode)
{
    system_mode = mode;
}
uint8_t get_sys_mode(void)
{
    return system_mode;
}

void icu_init(void)
{
#ifndef CFG_JTAG_DEBUG    
    clrf_SYS_Reg0x0_jtag_mode;   ///close JTAG
#endif
    if(get_sys_mode() !=DUT_FCC_MODE )
    {
        sys_mode_init(NORMAL_MODE);
    }
    
    gpio_config(0x00,INPUT,PULL_HIGH);
    Delay_ms(1);
    if(0==gpio_get_input(0x00))
    {
      Delay_ms(10);
      if(0==gpio_get_input(0x00))
          sys_mode_init(DUT_FCC_MODE);
    }

    addPMU_Reg0x1 &= ~(1<<11);

    set_SYS_Reg0x2_core_sel(0x01);
    set_SYS_Reg0x2_core_div(0x0);///16M CLK

    //setf_SYS_Reg0xb_pwd_on_boostsel;
    addSYS_Reg0x17 = 0x80;
    setf_SYS_Reg0xd_PLL_PWR_sel;
    addPMU_Reg0x14=0x6666;

#if(LDO_MODE)
    addPMU_Reg0x13 |= ((1<<12)+(1<<28));
#else
    if( get_sys_mode()==DUT_FCC_MODE )    // dut is in LDO mode
    {
        addPMU_Reg0x13 |= ((1<<12)+(1<<28));
    }

#endif    
}

uint8_t icu_get_sleep_mode(void)
{
    return default_sleep_mode;
}

void icu_set_sleep_mode(uint8_t v)
{
    default_sleep_mode = v;
}


void cpu_reduce_voltage_sleep()
{
    uint32_t tmp_reg;
    volatile uint8_t delay_nop=0;

    do
    {
        char *text;
        __attribute__((unused)) volatile char temp;
        int i;
        text = (char *)cpu_reduce_voltage_sleep;
        for (i = 0; i < 0x160; i ++)
        {
            temp = text[i];
        }
    } while(0);
    
    //set_flash_clk(0x08);
    set_SYS_Reg0x2_core_sel(0x01);
    set_SYS_Reg0x2_core_div(0x0);
    
#if(LDO_MODE==0)
#if(LDO_MODE_IN_SLEEP)
    if( get_sys_mode()!=DUT_FCC_MODE )
    {
        ///to ldo mode
        addXVR_Reg0x6 = 0x8487CC00;//0x80B7CE20  ;
        XVR_ANALOG_REG_BAK[6] = 0x8487CC00;//0x80B7CE20;
        addXVR_Reg0xa = 0x9C03F86B;//0x9C27785B  ;
        XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86B;//0x9C27785B;
    }
#endif
#endif
    setf_SYS_Reg0x17_enb_busrt_sel; 
    setf_SYS_Reg0x17_CLK96M_PWD;
    setf_SYS_Reg0x17_HP_LDO_PWD;
    setf_SYS_Reg0x17_cb_bias_pwd;


    tmp_reg = addSYS_Reg0x17 | 0x08;


#if(!LDO_MODE)
    if( get_sys_mode()!=DUT_FCC_MODE )    // dut is in LDO mode
    {
        set_PMU_Reg0x14_voltage_ctrl_work_aon(0x05);
        set_PMU_Reg0x14_voltage_ctrl_work_core(0x05);
    }
#endif
    set_SYS_Reg0x2_core_sel(0x00);

    addSYS_Reg0x17 = tmp_reg;

    setf_SYS_Reg0x1_CPU_PWD;  ////sleep

    delay_nop++;
    addSYS_Reg0x17 = 0x80;
    set_SYS_Reg0x2_core_sel(0x01);

    delay_nop++;
    delay_nop++;
    addPMU_Reg0x14=0x6666;
    delay_nop++;
    delay_nop++;
    delay_nop++;
    //set_SYS_Reg0x2_core_sel(0x01);

#if(LDO_MODE==0)
#if(LDO_MODE_IN_SLEEP)
    ///to buck mode
    if( get_sys_mode()!=DUT_FCC_MODE )    // dut is in LDO mode
    {
        addXVR_Reg0x6 = 0x84A7CC00;//0x8097CE20  ;
        XVR_ANALOG_REG_BAK[6] = 0x84A7CC00;//0x8097CE20;
        addXVR_Reg0xa = 0x9C03F86F;//0x9C27785B  ;
        XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86F;//0x9C27785B;
    }
#endif
#endif
}

void cpu_wakeup(void)
{   
    addSYS_Reg0x17 = 0x80;
    
    switch(system_clk)
    {
        case MCU_CLK_16M:
        {   
            Delay_us(100);///if mcu clk=16M wait for PLL clk for spi
            set_SYS_Reg0x2_core_sel(0x01);
            set_SYS_Reg0x2_core_div(0x0);
          
        }break;
        case MCU_CLK_32M:
        case MCU_CLK_40M:
        {
            set_SYS_Reg0x2_core_div(0x2);         
            set_SYS_Reg0x2_core_sel(0x03);               
        }break;
        case MCU_CLK_64M:
        case MCU_CLK_80M:
        {  
            set_SYS_Reg0x2_core_div(0x1);            
            set_SYS_Reg0x2_core_sel(0x03);               
        
        }break;
    
        default:
        {   
            Delay_us(100);///if mcu clk=16M wait for PLL clk for spi
            set_SYS_Reg0x2_core_sel(0x01);
            set_SYS_Reg0x2_core_div(0x0);
          
        }break;
    }
    
    //set_flash_clk(0x01);         
}

void cpu_idle_sleep(void)
{
    clrf_PMU_Reg0x14_sleep_sel; 
    setf_SYS_Reg0x1_CPU_PWD;            
}


void mcu_clk_switch(uint8_t clk)
{
    if( (system_clk==MCU_CLK_80M) && (clk==MCU_CLK_64M) )
        return;
    if( (system_clk==MCU_CLK_40M) && (clk==MCU_CLK_64M) )
        return;
    if( (system_clk==MCU_CLK_64M) && (clk==MCU_CLK_80M) )
        return;
    if( (system_clk==MCU_CLK_64M) && (clk==MCU_CLK_40M) )
        return;
    if( (system_clk==MCU_CLK_80M) && (clk==MCU_CLK_32M) )
        return;
    if( (system_clk==MCU_CLK_40M) && (clk==MCU_CLK_32M) )
        return;
    if( (system_clk==MCU_CLK_32M) && (clk==MCU_CLK_80M) )
        return;
    if( (system_clk==MCU_CLK_32M) && (clk==MCU_CLK_40M) )
        return;
    
    system_clk = clk;
    switch(clk)
    {
        case MCU_CLK_16M:
        {   
            set_SYS_Reg0x2_core_div(0x1);
            set_SYS_Reg0x2_core_sel(0x01);
            
        }break;
        case MCU_CLK_40M:
        {
            ///cpu 40M,spi 80M时钟源      
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 20);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 17);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 16);
            addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
            
            clrf_SYS_Reg0x17_CLK96M_PWD;
            //setf_SYS_Reg0xd_PLL_PWR_sel;
            set_SYS_Reg0x2_core_div(0x2);         
            set_SYS_Reg0x2_core_sel(0x03);
        
        }break;
        case MCU_CLK_80M:
        {
            ///cpu 80M,spi 80M时钟源
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 20);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 17);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 16);
            addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
            
            clrf_SYS_Reg0x17_CLK96M_PWD;
            //setf_SYS_Reg0xd_PLL_PWR_sel;
            set_SYS_Reg0x2_core_div(0x1);            
            set_SYS_Reg0x2_core_sel(0x03);
        
        }break;        
        case MCU_CLK_32M:
        {            
            ///cpu 64M,spi 64M时钟源
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 20);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 17);
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 16);
            addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
            Delay_ms(1);
            clrf_SYS_Reg0x17_CLK96M_PWD;           
            set_SYS_Reg0x2_core_div(0x2);            
            set_SYS_Reg0x2_core_sel(0x03);
        
        }break; 
        case MCU_CLK_64M:
        {            
            ///cpu 64M,spi 64M时钟源
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 20);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 17);
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 16);
            addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
            Delay_ms(1);
            clrf_SYS_Reg0x17_CLK96M_PWD;           
            set_SYS_Reg0x2_core_div(0x1);            
            set_SYS_Reg0x2_core_sel(0x03);
        
        }break; 
        default:break;
    }
    
}
void deep_sleep_wakeup_set(uint8_t gpio)
{
    gpio_config(gpio,INPUT,PULL_HIGH);
    gpio_wakeup_config(gpio);///set wakeup gpio,set your need gpio

}

void cpu_delay( volatile unsigned int times)
{
    while(times--) ;
}

void deep_sleep(void)
{   
    system_set_reset_reson(C_DEEPSLEEP_RESET);
    cpu_delay( 1000 ) ;

    addXVR_Reg0x6 = 0x85A7CC00;

    cpu_delay( 1000 ) ;

    addXVR_Reg0x7 = 0xAA023FC0;
    cpu_delay( 1000 ) ;
    
    #if (LDO_MODE)
    addXVR_Reg0xa = 0x9C04785b;
    addPMU_Reg0x1 |= (1<<11);
    #else
    
    if( get_sys_mode()==DUT_FCC_MODE )    // dut is in LDO mode
    {
    
        addXVR_Reg0xa = 0x9C04785b;
        addPMU_Reg0x1 |= (1<<11);
    }
    else
    {
        addXVR_Reg0xa = 0x9C04785F;
    }
    #endif

    //addXVR_Reg0xa = 0x9C04785F;
    cpu_delay( 1000 ) ;

    addXVR_Reg0x1c= 0x919CDDC5;//addXVR_Reg0x1c = 0x999CDDC5 
    cpu_delay( 1000 ) ;

    set_PMU_Reg0x4_gotosleep(0x3633);
    while(1);
}

void cpu_reset(void)
{
    setf_PMU_Reg0x1_reg0_w_en;
    //clrf_PMU_Reg0x1_fast_boot;
    Delay_us(2000);
    //setf_PMU_Reg0x0_digital_reset;
    setf_PMU_Reg0x0_all_reset;
}


uint8_t system_reset_reson(void)
{
// 在reset
// 之前需要调用system_set_reset_reson函数，写明reset 原因。
    uint32_t datatmp,datatmp1;

    uart_printf("reset reason=%x,%x\r\n",addPMU_Reg0x0,addPMU_Reg0x2);
    datatmp = get_PMU_Reg0x0_reset_reason;
    datatmp1 = addPMU_Reg0x2&0xffff; // in deepsleep,the high  = 0x2000XXXX

    set_PMU_Reg0x0_reset_reason(0);
    switch(datatmp)
    {
        case 0x01:
            switch(datatmp1)
            {
                case C_WDT_RESET:
                    uart_printf("wdt reset\r\n");
                    return 1;
                case C_FORCE_ALL_RESET:
                    uart_printf("force all reset\r\n");
                    return 2;
            }
            uart_printf("reset 1 err\r\n");
            return 0;

        case 0x03:
            switch(datatmp1)
            {
                case C_DEEPSLEEP_RESET:
                    addPMU_Reg0x1 |= (1<<8);
                    uart_printf("deep sleep reset\r\n");
                    return 3;
                case C_FORCE_DIGITAL_RESET:
                    uart_printf("force digital reset\r\n");
                    return 4;
                case C_POWERON_RESET:
                    uart_printf("power on reset\r\n");
                    return 5;
            }
            uart_printf("reset 3 err\r\n");
            return 0;

    }
    uart_printf("reset 0 err\r\n");
    return 0;
}



void system_set_reset_reson(uint32_t reson_data)
    // only poweron,wdt(PMU.reg1.bit3=0),force reset all  ,this reg =0
{
    setf_PMU_Reg0x1_wdt_reset_ctrl;
    addPMU_Reg0x2 = reson_data;

    uart_printf("set reset reason=%x\r\n",addPMU_Reg0x2);
}


void test_reset_reason(void)
{
    //system_set_reset_reson(C_DEEPSLEEP_RESET);

    gpio_config(0x31,INPUT,PULL_HIGH);
    deep_sleep_wakeup_set(0x31);
    deep_sleep();

}


uint8_t system_sleep_status = 0;

#define REG_AHB10_RW_DEEPSLCNTL     (*((volatile unsigned long *)   0x00820030))
#define REG_AHB10_RW_DEEPSLTIME     (*((volatile unsigned long *)   0x00820034))
#define REG_AHB10_RW_DEEPSLDUR     (*((volatile unsigned long *)    0x00820038))
#define REG_AHB10_RW_ENBPRESET     (*((volatile unsigned long *)    0x0082003C))
#define REG_AHB10_RW_FINECNTCORR     (*((volatile unsigned long *)  0x00820040))
#define REG_AHB10_RW_BASETIMECNTCORR    (*((volatile unsigned long *)   0x00820044))

#define RW_ENBPRESET_TWEXT_bit     21
#define RW_ENBPRESET_TWOSC_bit     10
#define RW_ENBPRESET_TWRW_bit     0
void rw_sleep(void)
{

 //goto sleep

    clrf_SYS_Reg0x3_rwbt_pwd;//open rw clk
    setf_SYS_Reg0xd_OSC_en_sel;
    /*   rw_sleep_tmp_buf.rfu = get_SYS_Reg0x1e_rfu;
    set_SYS_Reg0x1e_rfu(0xf);
    */
    /*     rw_sleep_tmp_buf.deepsleeptime = REG_AHB10_RW_DEEPSLTIME;
    REG_AHB10_RW_DEEPSLTIME=0x00000;//sleep time ;finite
    rw_sleep_tmp_buf.enbprset = REG_AHB10_RW_ENBPRESET;*/
    REG_AHB10_RW_ENBPRESET = (0x40<<RW_ENBPRESET_TWEXT_bit)|
    (0x40<<RW_ENBPRESET_TWOSC_bit)|(0x40<<RW_ENBPRESET_TWRW_bit);

    /*           rw_sleep_tmp_buf.deepslcntl = REG_AHB10_RW_DEEPSLCNTL;
    REG_AHB10_RW_DEEPSLCNTL=0x00000007;//BLE sleep*/
}
void rw_wakeup(void)
{
    clrf_SYS_Reg0xd_OSC_en_sel;

}

//osc 32k idle
void cpu_usb_reduce_voltage_sleep()
{
    uint32_t tmp_reg;
    
    do
    {
        char *text;
        __attribute__((unused)) volatile char temp;
        int i;
        text = (char *)cpu_usb_reduce_voltage_sleep;
        for (i = 0; i < 0x100; i ++)
        {
            temp = text[i];
        }
    } while(0);

    set_flash_clk(0x08);

    XVR_ANALOG_REG_BAK[3] |= 1ul<<31;
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3] ;

  /*  addPMU_Reg0x10 |= (1<<21)|(1<<18)|(1<<4)|(1<<3);
    Delay_ms(1);

    addPMU_Reg0x12 &= ~((1<<21)|(1<<18)|(1<<4)|(1<<3));
    Delay_ms(1);
    addPMU_Reg0x10 |= (1<<21)|(1<<18)|(1<<4)|(1<<3);
    Delay_ms(1);
    addPMU_Reg0x12 |= ((1<<21)|(1<<18)|(1<<4)|(1<<3));
    Delay_ms(1);

*/
    rw_sleep();

    set_SYS_Reg0x2_core_sel(0x01);
    set_SYS_Reg0x2_core_div(0x0);

    setf_SYS_Reg0x17_enb_busrt_sel;
    setf_SYS_Reg0x17_CLK96M_PWD;
    setf_SYS_Reg0x17_HP_LDO_PWD;
    setf_SYS_Reg0x17_cb_bias_pwd;
    tmp_reg = addSYS_Reg0x17 | 0x08;
#if(!LDO_MODE)
    if( get_sys_mode()!=DUT_FCC_MODE )    // dut is in LDO mode
    {
        set_PMU_Reg0x14_voltage_ctrl_work_aon(0x05);
        set_PMU_Reg0x14_voltage_ctrl_work_core(0x05);
    }
#endif


    set_SYS_Reg0x2_core_sel(0x00);

    addSYS_Reg0x17 = tmp_reg;

    setf_SYS_Reg0x1_CPU_PWD;  ////sleep

    extern void __nop(void);
    __nop();
    addSYS_Reg0x17 = 0x80;

    __nop();
    __nop();
    addPMU_Reg0x14=0x6666;
    __nop();
    __nop();
    __nop();
    set_SYS_Reg0x2_core_sel(0x01);

/*
    addPMU_Reg0x10 |= (1<<21)|(1<<18)|(1<<4)|(1<<3);
    Delay_ms(1);

    addPMU_Reg0x12 &= ~((1<<21)|(1<<18)|(1<<4)|(1<<3));
    Delay_ms(10);
*/

}


void cpu_24_reduce_voltage_sleep()
{
    uint32_t tmp_reg;

    do
    {
        char *text;
        __attribute__((unused)) volatile char temp;
        int i;
        text = (char *)cpu_24_reduce_voltage_sleep;
        for (i = 0; i < 0x100; i ++)
        {
            temp = text[i];
        }
    } while(0);
    
    set_flash_clk(0x08);
    set_SYS_Reg0x2_core_sel(0x01);
    set_SYS_Reg0x2_core_div(0x0);
    rw_sleep();

    setf_SYS_Reg0x17_enb_busrt_sel;
    setf_SYS_Reg0x17_CLK96M_PWD;
    setf_SYS_Reg0x17_HP_LDO_PWD;
    setf_SYS_Reg0x17_cb_bias_pwd;


    tmp_reg = addSYS_Reg0x17 | 0x08;
    system_sleep_status = 1;
    #if(!LDO_MODE)
    
    if( get_sys_mode()!=DUT_FCC_MODE )    // dut is in LDO mode
    {
        set_PMU_Reg0x14_voltage_ctrl_work_aon(0x05);
        set_PMU_Reg0x14_voltage_ctrl_work_core(0x05);
    }
    #endif

    set_SYS_Reg0x2_core_sel(0x00);

    addSYS_Reg0x17 = tmp_reg;

    setf_SYS_Reg0x1_CPU_PWD;

  //    don't used 96M
  //  addSYS_Reg0x17 = 0x82;
  // used 96M
    addSYS_Reg0x17 = 0x80;
    addPMU_Reg0x14=0x6666;
    set_SYS_Reg0x2_core_sel(0x01);
    
    rw_wakeup();

}

void cpu_24_wakeup(void)
{
    if(system_sleep_status == 1)
    {
  //    don't used 96M
  //  addSYS_Reg0x17 = 0x82;
  // used 96M
        addSYS_Reg0x17 = 0x80;
        addPMU_Reg0x14=0x6666;

        switch(system_clk)
        {
            case MCU_CLK_16M:
            {
                set_SYS_Reg0x2_core_sel(0x01);
                set_SYS_Reg0x2_core_div(0x0);

            }break;

            case MCU_CLK_32M:
            {
                set_SYS_Reg0x2_core_div(0x2);
                set_SYS_Reg0x2_core_sel(0x03);
            }break;


            case MCU_CLK_64M:
            {
                set_SYS_Reg0x2_core_div(0x1);
                set_SYS_Reg0x2_core_sel(0x03);

            }break;

            case MCU_CLK_80M:
            {
                set_SYS_Reg0x2_core_div(0x1);
                set_SYS_Reg0x2_core_sel(0x03);

            }break;

            default:break;
        }
        set_flash_clk(0x01);
        system_sleep_status = 0;
    }
}
#if 0
static void usb_suspend_idle(void)   //2021.02.22-1
{
    uint32_t tmp_reg;

    set_flash_clk(0x08);

    XVR_ANALOG_REG_BAK[3] |= (uint32_t)1<<31;
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3] ;

/*
    addPMU_Reg0x10 |= (1<<21)|(1<<18)|(1<<4)|(1<<3);
    Delay_ms(1);

    addPMU_Reg0x12 &= ~((1<<21)|(1<<18)|(1<<4)|(1<<3));
    Delay_ms(1);
    addPMU_Reg0x10 |= (1<<21)|(1<<18)|(1<<4)|(1<<3);
    Delay_ms(1);
    addPMU_Reg0x12 |= ((1<<21)|(1<<18)|(1<<4)|(1<<3));
     Delay_ms(1);
*/
     rw_sleep();

     set_SYS_Reg0x2_core_sel(0x01);
     set_SYS_Reg0x2_core_div(0x0);

     setf_SYS_Reg0x17_enb_busrt_sel;
     setf_SYS_Reg0x17_CLK96M_PWD;
     setf_SYS_Reg0x17_HP_LDO_PWD;
     setf_SYS_Reg0x17_cb_bias_pwd;
     tmp_reg = addSYS_Reg0x17 | 0x08;

#if(!LDO_MODE)
     set_PMU_Reg0x14_voltage_ctrl_work_aon(0x05);
     set_PMU_Reg0x14_voltage_ctrl_work_core(0x05);
#endif

     set_SYS_Reg0x2_core_sel(0x00);

     addSYS_Reg0x17 = tmp_reg;

     setf_SYS_Reg0x1_CPU_PWD;  ////sleep

     __nop();
     addSYS_Reg0x17 = 0x80;

     __nop();
     __nop();
     addPMU_Reg0x14=0x6666;
     __nop();
     __nop();
     __nop();
     set_SYS_Reg0x2_core_sel(0x01);
/*
     addPMU_Reg0x10 |= (1<<21)|(1<<18)|(1<<4)|(1<<3);
     Delay_ms(1);

     addPMU_Reg0x12 &= ~((1<<21)|(1<<18)|(1<<4)|(1<<3));
     Delay_ms(10);
*/
}
#endif

