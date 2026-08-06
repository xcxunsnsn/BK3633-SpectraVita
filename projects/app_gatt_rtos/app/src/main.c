/**
 ****************************************************************************************
 *
 * @file arch_main.c
 *
 * @brief Main loop of the application.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */
#include "rwip_config.h" // RW SW configuration
#include "arch.h"      // architectural platform definitions
#include <stdlib.h>    // standard lib functions
#include <stddef.h>    // standard definitions
#include <stdint.h>    // standard integer definition
#include <stdbool.h>   // boolean definition
#include <string.h>   // boolean definition
#include "rwip.h"      // RW SW initialization
#include "prf.h"      // RW SW initialization
#include "rwble.h"

#include "intc.h"      // Interrupt initialization
#if PLF_UART
#include "uart.h"      // UART initialization
#if PLF_UART2
#include "uart2.h"      // UART2 initialization
#endif // PLF_UART2
#endif //PLF_UART
#include "flash.h"     // Flash initialization
//#include "led.h"       // Led initialization
#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
//#include "rf.h"        // RF initialization
#endif // BLE_EMB_PRESENT || BT_EMB_PRESENT

#if (BLE_APP_PRESENT)
#include "app.h"       // application functions
#endif

#if (NVDS_SUPPORT)
#include "nvds.h"         // NVDS definitions
#endif
#include "reg_access.h"
#include "boot.h"
#include "dbg.h"
#include "icu.h"
#include "user_config.h"
#include "co_utils.h" 
#include "gpio.h"
#include "icu.h"
#include "wdt.h"
#include "spi.h"
#include "adc.h"
#include "uart2.h"
#include "aon_rtc.h"

#include "oads.h"
#include "rf.h"

#include "rwip_int.h"

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

#if OPTIMIZE_POWER_CONSUMPTION_TEMP
extern uint8_t adc_temp_expired(void);
extern uint8_t rc32k_cali_flag;
uint8_t temp_trigger = 0;
uint16_t pre_cali_temp = 0;
#endif

static void stack_integrity_check(void)
{
    if ((REG_PL_RD(STACK_BASE_UNUSED)!= BOOT_PATTERN_UNUSED))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_UNUSED fail!\r\n");
        }
    }

    if ((REG_PL_RD(STACK_BASE_SVC)!= BOOT_PATTERN_SVC))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_SVC fail!\r\n");
        }
    }

    if ((REG_PL_RD(STACK_BASE_FIQ)!= BOOT_PATTERN_FIQ))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_FIQ fail!\r\n");
        }
    }

    if ((REG_PL_RD(STACK_BASE_IRQ)!= BOOT_PATTERN_IRQ))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_IRQ fail!\r\n");
        }
    }

}

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                        char * pcTaskName )
{
    uart_printf("StackOverflow! Task name:%s\r\n",pcTaskName);
    while(1);
}

static void rom_env_init(struct rom_env_tag *api)
{
    memset(&rom_env,0,sizeof(struct rom_env_tag));
    rom_env.prf_get_id_from_task = prf_get_id_from_task;
    rom_env.prf_get_task_from_id = prf_get_task_from_id;
    rom_env.prf_init = prf_init;
    rom_env.prf_create = prf_create;
    rom_env.prf_cleanup = prf_cleanup;
    rom_env.prf_add_profile = prf_add_profile;
    
    rom_env.rwip_reset = rwip_reset;
    rom_env.platform_reset = platform_reset;
    rom_env.rwble_sleep_wakeup_end = rwble_sleep_wakeup_end;
    rom_env.stack_printf=uart2_printf;
    //rom_env.assert_err = assert_err;
    //rom_env.assert_param = assert_param;
    //rom_env.Read_Uart_Buf = Read_Uart_Buf;
    //rom_env.uart_clear_rxfifo = uart_clear_rxfifo;
    rom_env.ana_xvr_reg_get = ana_xvr_reg_get;
    rom_env.ana_xvr_reg_set = ana_xvr_reg_set;

    rom_env.setEvent = event_null_handle;
    rom_env.clearEvent = event_null_handle;
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
#if PLF_UART        ////Added
// Creation of uart external interface api
const struct rwip_eif_api uart_api =
{
    uart_read,
    uart_write,
    uart_flow_on,
    uart_flow_off,
};
#endif
extern uint8_t g_dut_flg;

void bdaddr_env_init(void)
{
    struct bd_addr co_bdaddr;
    uart_printf("bdaddr_def_addr_abs=%x\r\n",flash_env.bdaddr_def_addr_abs);
    flash_read_data(&co_bdaddr.addr[0],flash_env.bdaddr_def_addr_abs,6);

    if(co_bdaddr.addr[0]!=0xff ||co_bdaddr.addr[1]!=0xff||
    co_bdaddr.addr[2]!=0xff||co_bdaddr.addr[3]!=0xff||
    co_bdaddr.addr[4]!=0xff||co_bdaddr.addr[5]!=0xff )
    {
        memcpy(&co_default_bdaddr,&co_bdaddr,6);
    }
}

#if (PLF_DEBUG)
void assert_err(const char *condition, const char * file, int line)
{
    uart_printf("%s,condition %s,file %s,line = %d\r\n",__func__,condition,file,line);
}

void assert_param(int param0, int param1, const char * file, int line)
{
    uart_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
}

void assert_warn(int param0, int param1, const char * file, int line)
{
    uart_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
}

void dump_data(uint8_t* data, uint16_t length)
{
    uart_printf("%s,data = %d,length = %d,file = %s,line = %d\r\n",__func__,data,length);
}

#endif //PLF_DEBUG


void platform_reset(uint32_t error)
{   
    uart_printf("reset error = %x\r\n", error);
    // Disable interrupts
    GLOBAL_INT_STOP();
    
    // Restart FW
    cpu_reset();    
}

void enable_phy_2mbps(void)
{
    extern uint8_t enable_phy_2m;   
    enable_phy_2m=1;

}

#ifdef USE_FREERTOS
void user_task( void * arg )
{
    int i = 0;
    while(1)
    {
        #if OPTIMIZE_POWER_CONSUMPTION_TEMP
        uint32_t adc_temp;
        if(adc_temp_expired()) 
        {
            adc_temp = get_temperature_value();
            if ((pre_cali_temp != 0) && ((pre_cali_temp > adc_temp && pre_cali_temp - adc_temp > 3)
                || (adc_temp > pre_cali_temp && adc_temp - pre_cali_temp > 3)))
            {
                temp_trigger = 1;
                uart_printf("temp cali trigger\n");
            }
            if (adc_temp < 0x63 || adc_temp > 0x87) // -10~10 || 41~52
            {
                rwip_env.extreme_temp = 1;
            }
            else 
            {
                rwip_env.extreme_temp = 0;
            }
        }
        #endif
        uart_printf("user task:%d\r\n",i++);
        vTaskDelay(pdMS_TO_TICKS(1000));
        #if OPTIMIZE_POWER_CONSUMPTION_TEMP
        if (rc32k_cali_flag) 
        {
            rc32k_cali_flag = 0;
            pre_cali_temp = get_temperature_value();
            //uart_printf("pre cali temp=0x%x\n", pre_cali_temp);
        }
        #endif

        stack_integrity_check();
    }
}

void dut_task( void * arg )
{
    while(1)
    {
        #if (VIRTUAL_UART_H4TL == 1)
        uart_h4tl_data_switch();
        #endif

        #if(RF_FREQ_OFFSET_CALI)
        if(xtalcali_value_en == 1)
        {
                if(nvds_put(NVDS_TAG_RFCALI_VALUE, NVDS_LEN_RFCALI_VALUE, (uint8_t *)&rfcali_value) == NVDS_OK) //NVDS_TAG_RFCALI_IDX
                {
                        xtalcali_value_en = 0;
                }
                else
                {
                        uart_printf("xtalcali value save error\n");
                }
        }
        #endif
    }
}

void rtos_sleep_process(uint32_t ticks)
{
    // Checks for sleep have to be done with interrupt disabled
    GLOBAL_INT_DISABLE();

    // Check if the processor clock can be gated
    #if 1  
    if( NO_SLEEP!=icu_get_sleep_mode() )
    {
        switch(rwip_sleep())
        {
            case RWIP_DEEP_SLEEP:
            {   
                // add here platform specific deep sleep code  
                aon_rtc_set_sleep_tick(ticks);
                if(MCU_REDUCE_VO_SLEEP==icu_get_sleep_mode())
                {
                    cpu_reduce_voltage_sleep();
                    cpu_wakeup();
                }
                else if(MCU_IDLE_SLEEP==icu_get_sleep_mode())
                {
                    cpu_idle_sleep();
                }
                else if(MCU_DEEP_SLEEP==icu_get_sleep_mode())
                {
                    ////before deepsleep use (deep_sleep_wakeup_set)  config gpio wakeup mcu
                    deep_sleep_wakeup_set(0X10);
                    deep_sleep();
                }
                aon_restore_ticktimer(ticks);
            }
            break;
            case RWIP_CPU_SLEEP:
            
            {
                // Wait for interrupt
                cpu_idle_sleep(); 
            }
            break;
            case RWIP_ACTIVE:
            default:
            {
                // nothing to do.
            } break;
        }
    }
    #endif        
    // Checks for sleep have to be done with interrupt disabled
    GLOBAL_INT_RESTORE();
}
#endif

extern uint8_t rfcali_value;
extern uint8_t xtalcali_value_en;
void enter_dut_fcc_mode(void)
{
#ifndef USE_FREERTOS
    while(1)
    {
        // schedule all pending events
        rwip_schedule();
        #if (VIRTUAL_UART_H4TL == 1)
        uart_h4tl_data_switch();
        #endif
        
        #if(RF_FREQ_OFFSET_CALI)
        if(xtalcali_value_en == 1)
        {
            if(nvds_put(NVDS_TAG_RFCALI_VALUE, NVDS_LEN_RFCALI_VALUE, (uint8_t *)&rfcali_value) == NVDS_OK) //NVDS_TAG_RFCALI_IDX
            {
                xtalcali_value_en = 0;
            }
            else
            {
                uart_printf("xtalcali value save error\n");
            }
        }
        #endif
    }
#else
        create_rwble_task(512);
        xTaskCreate(dut_task, "dut_task", 256, NULL, RTOS_APP_PRIORITY, NULL);
        vTaskStartScheduler();
#endif
}


void enter_normal_app_mode(void)
{
#ifndef USE_FREERTOS
    while(1)
    {
        // schedule all pending events
        rwip_schedule();
        //app_fee4_send_ntf(0,512,data_buf);
        oad_updating_user_section_pro();

        #if OPTIMIZE_POWER_CONSUMPTION_TEMP
        uint32_t adc_temp;
        if(adc_temp_expired()) 
        {
            adc_temp = get_temperature_value();
            if ((pre_cali_temp != 0) && ((pre_cali_temp > adc_temp && pre_cali_temp - adc_temp > 3)
                || (adc_temp > pre_cali_temp && adc_temp - pre_cali_temp > 3)))
            {
                temp_trigger = 1;
                uart_printf("temp cali trigger\n");
            }
            if (adc_temp < 0x63 || adc_temp > 0x87) // -10~10 || 41~52
            {
                rwip_env.extreme_temp = 1;
            } 
            else 
            {
                rwip_env.extreme_temp = 0;
            }
        }
        #endif
        // Checks for sleep have to be done with interrupt disabled
        GLOBAL_INT_DISABLE();

        // Check if the processor clock can be gated
        #if 1  
        if( NO_SLEEP!=icu_get_sleep_mode() )
        {
            switch(rwip_sleep())
            {
                case RWIP_DEEP_SLEEP:
                {   
                    // add here platform specific deep sleep code  
                    if(MCU_REDUCE_VO_SLEEP==icu_get_sleep_mode())
                    {
                        cpu_reduce_voltage_sleep();
                        cpu_wakeup();
                    }
                    else if(MCU_IDLE_SLEEP==icu_get_sleep_mode())
                    {
                        cpu_idle_sleep();
                    }
                    else if(MCU_DEEP_SLEEP==icu_get_sleep_mode())
                    {
                        ////before deepsleep use (deep_sleep_wakeup_set)  config gpio wakeup mcu
                        deep_sleep_wakeup_set(0X10);
                        deep_sleep();
                    }
                }
                break;
                case RWIP_CPU_SLEEP:
                
                {
                    // Wait for interrupt
                    cpu_idle_sleep(); 
                }
                break;
                case RWIP_ACTIVE:
                default:
                {
                    // nothing to do.
                } break;
            }
        }
        #endif        
        // Checks for sleep have to be done with interrupt disabled
        GLOBAL_INT_RESTORE();

        #if OPTIMIZE_POWER_CONSUMPTION_TEMP
        if (rc32k_cali_flag) 
        {
            rc32k_cali_flag = 0;
            pre_cali_temp = get_temperature_value();
            //uart_printf("pre cali temp=0x%x\n", pre_cali_temp);
        }
        #endif

        stack_integrity_check();
    }
#else
        create_rwble_task(512);
        xTaskCreate(user_task, "user_task", 256, NULL, RTOS_APP_PRIORITY, NULL);
        vTaskStartScheduler();
#endif
}
   
int main(void)
{

    icu_init();  
    wdt_disable();
    intc_init();    
    uart_init(115200);
    uart2_init(115200);
    gpio_init(); 

    system_reset_reson();
    system_set_reset_reson(C_WDT_RESET);
    //timer_init();
    
    
    flash_init();
    bdaddr_env_init();
    srand(co_default_bdaddr.addr[0]+co_default_bdaddr.addr[5]);

#if (NVDS_SUPPORT)
    nvds_init();
#endif 
    
        rom_env_init(&rom_env);
    // Initialize RW SW stack
    rwip_init(0);

    uart_printf("rw_main start!!!=%x \r\n",EM_BLE_END);
    #if(OPTIMIZE_POWER_CONSUMPTION)
    set_power(0x02);
    #else
    set_power(0x07);
    #endif

    #if(ENABLE_PHY_2M_LE_CODE)
    enable_phy_2mbps();
    #endif    

    if((system_mode & DUT_FCC_MODE) == DUT_FCC_MODE) //dut mode
    {
        mcu_clk_switch(MCU_CLK_80M);
        uart_printf("enter_dut_fcc_mode \r\n");
        GLOBAL_INT_START();
        enter_dut_fcc_mode();
    }
    else //normal mode
    {
        uart_printf("enter_normal_app_mode00000000000 \r\n");

        icu_set_sleep_mode(MCU_REDUCE_VO_SLEEP);

        #if(ENABLE_PHY_2M_LE_CODE)
        mcu_clk_switch(MCU_CLK_64M);
        #else
        mcu_clk_switch(MCU_CLK_16M);
        #endif
        //flash_test();
        #if(AON_RTC_DRIVER)   
        aon_rtc_init();
        #endif
        #if(SPI_DRIVER)    
        spi_init(0,1,0);
        #endif

        #if(ADC_DRIVER) 
        adc_init(1,1);
        #endif

        #if(PWM_DRIVER)
        pwm_pwmmode_test();
        #endif
                
                GLOBAL_INT_START();

        #if(ADC_CALIB)
        calib_adc();
        #endif   

        //add for test adc
        //check_low_volt_sleep();
        enter_normal_app_mode();
    }
    
}


const struct rwip_eif_api* rwip_eif_get(uint8_t idx)
{
    const struct rwip_eif_api* ret = NULL;
    switch(idx)
    {
        #if PLF_UART////
        case 0:
        {
            ret = &uart_api;
        }
        break;
        #endif
        default:
        {
            ASSERT_INFO(0, idx, 0);
        }
        break;
    }
    return ret;
}




