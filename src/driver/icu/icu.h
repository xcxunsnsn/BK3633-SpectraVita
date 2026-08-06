#ifndef _ICU_H_
#define _ICU_H_
#include "stdint.h"
#define BASEADDR_SYS                                            0x00800000
#define SYS_REG0X0                                           *((volatile unsigned long *) (0x00800000+0x0*4))
#define POS_SYS_REG0X0_JTAG_MODE                             9
#define POS_SYS_REG0X0_RAM_RENT                              1
#define POS_SYS_REG0X0_PWR_SAVE                              0

#define SYS_REG0X1                                           *((volatile unsigned long *) (0x00800000+0x1*4))
#define POS_SYS_REG0X1_GOTO_SLEEP                                1
#define POS_SYS_REG0X1_CPU_PWD                                   0

#define SYS_REG0X2                                           *((volatile unsigned long *) (0x00800000+0x2*4))
#define POS_SYS_REG0X2_CORE_DIV                                 0
#define POS_SYS_REG0X2_CORE_SEL                                 7
#define POS_SYS_REG0X2_DSP_DIV2                                 9


#define SYS_REG0X3_POWER                   *((volatile unsigned long *) (0x00800000+0x3*4))
#define POS_SYS_REG0X3_UART0_PWD           0
#define SET_UART_POWER_UP                 SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_UART0_PWD)
#define SET_UART_POWER_DOWN               SYS_REG0X3_POWER |= (1<<POS_SYS_REG0X3_UART0_PWD)
#define POS_SYS_REG0X3_UART2_PWD           1
#define SET_UART2_POWER_UP                 SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_UART2_PWD)               
#define POS_SYS_REG0X3_TIME0_PWD           2
#define SET_TIME0_POWER_UP                 SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_TIME0_PWD)               
#define POS_SYS_REG0X3_TIME1_PWD           3
#define SET_TIME1_POWER_UP                 SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_TIME1_PWD)               
#define POS_SYS_REG0X3_PWM0_PWD            4
#define SET_PWM0_POWER_UP                  SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_PWM0_PWD)               
#define POS_SYS_REG0X3_PWM1_PWD            6
#define SET_PWM1_POWER_UP                  SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_PWM1_PWD)               
#define POS_SYS_REG0X3_RTC_PWD             8
#define SET_RTC_POWER_UP                   SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_RTC_PWD)               
#define POS_SYS_REG0X3_SADC_PWD            10
#define SET_SADC_POWER_UP                  SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_SADC_PWD)               
#define SET_SADC_POWER_DOWN                SYS_REG0X3_POWER |= (1<<POS_SYS_REG0X3_SADC_PWD)
#define POS_SYS_REG0X3_USB_PWD             11
#define SET_USB_POWER_UP                   SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_I2C_PWD)               
#define POS_SYS_REG0X3_I2C_PWD             12
#define SET_I2C_POWER_UP                   SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_I2C_PWD)               
#define POS_SYS_REG0X3_SPI_PWD             13
#define SET_SPI_POWER_UP                   SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_SPI_PWD)               
#define SET_SPI_POWER_DOWN                 SYS_REG0X3_POWER |= (1<<POS_SYS_REG0X3_SPI_PWD)
#define POS_SYS_REG0X3_EFUSE_PWD           14
#define SET_EFUSE_POWER_UP                 SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_EFUSE_PWD)               
#define POS_SYS_REG0X3_WDT_PWD             15
#define SET_WDT_POWER_UP                   SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_WDT_PWD)               
#define POS_SYS_REG0X3_BK24_PWD            16
#define SET_BK24_POWER_UP                  SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_BK24_PWD)               
#define POS_SYS_REG0X3_RWBT_PWD            17
#define SET_RWBT_POWER_UP                  SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_RWBT_PWD) 
#define SET_RWBT_POWER_DOWN                SYS_REG0X3_POWER |= (1<<POS_SYS_REG0X3_RWBT_PWD)

#define POS_SYS_REG0X3_DMA_PWD             18
#define SET_DMA_POWER_UP                   SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_DMA_PWD)               
#define POS_SYS_REG0X3_I2S_PWD             19
#define SET_I2S_POWER_UP                   SYS_REG0X3_POWER &= ~(1<<POS_SYS_REG0X3_I2S_PWD)               
#define SET_I2S_POWER_DOWN                 SYS_REG0X3_POWER |= (1<<POS_SYS_REG0X3_I2S_PWD)

#define SYS_REG0X4_CLK_SEL                                      *((volatile unsigned long *) (0x00800000+0x4*4))
#define POS_SYS_REG0X4_WDTCLK_SEL                                0
#define POS_SYS_REG0X4_TIME0CLK_SEL                              2
#define POS_SYS_REG0X4_TIME1CLK_SEL                              3
#define POS_SYS_REG0X4_PWM0CLK_SEL                               4
#define POS_SYS_REG0X4_PWM1CLK_SEL                               6
#define POS_SYS_REG0X4_RTCCLK_SEL                                8
#define POS_SYS_REG0X4_SADCCLK_SEL                               10
#define POS_SYS_REG0X4_SPICLK_SEL                                11
#define POS_SYS_REG0X4_I2SMCLK_EN                                13
#define POS_SYS_REG0X4_I2SMCLK_DIV                               14
#define POS_SYS_REG0X4_PLL_OUT_EN                                19
#define POS_SYS_REG0X4_PLL_OUT_SEL                               20
#define POS_SYS_REG0X4_PLL_OUT_DIV                               21
#define POS_SYS_REG0X4_XTAL_OUT_EN                               24
#define POS_SYS_REG0X4_XTAL_OUT_SEL                              25
#define POS_SYS_REG0X4_XTAL_OUT_DIV                              26

#define SYS_REG0X10_INT_EN                                       *((volatile unsigned long *) (0x00800000+0x10*4))
#define POS_SYS_REG0X10_INT_EN_PWM0                              0
#define POS_SYS_REG0X10_INT_EN_PWM1                              1
#define POS_SYS_REG0X10_INT_EN_TIMER0                            2
#define POS_SYS_REG0X10_INT_EN_TIMER1                            3
#define POS_SYS_REG0X10_INT_EN_UART                             4
#define POS_SYS_REG0X10_INT_EN_UART2                             5
#define POS_SYS_REG0X10_INT_EN_SPI                               6
#define POS_SYS_REG0X10_INT_EN_I2C                               7
#define POS_SYS_REG0X10_INT_EN_ADC                               8
#define POS_SYS_REG0X10_INT_EN_GPIO                              9
#define POS_SYS_REG0X10_INT_EN_RTC                               10
#define POS_SYS_REG0X10_INT_EN_I2S                               11
#define POS_SYS_REG0X10_INT_EN_AON_RTC                           12
#define POS_SYS_REG0X10_INT_EN_USB                               17
#define POS_SYS_REG0X10_INT_EN_DMA                               18
#define POS_SYS_REG0X10_INT_EN_BK24                              19
#define POS_SYS_REG0X10_INT_EN_RWBLE                             20
#define POS_SYS_REG0X10_INT_EN_RWBT                              21
#define POS_SYS_REG0X10_INT_EN_RWDM                              22
#define SYS_REG0X10_INT_FLAG                                     *((volatile unsigned long *) (0x00800000+0x12*4))


#define SYS_REG0X0C                                           *((volatile unsigned long *) (0x00800000+0xc*4))
#define SYS_REG0X0D                                           *((volatile unsigned long *) (0x00800000+0xd*4))
#define POS_SYS_REG0X0D_EN_SEL                                7
#define POS_SYS_REG0X0D_PLL_PWR_SEL                           6

#define SYS_REG0X17                                           *((volatile unsigned long *) (0x00800000+0x17*4))
#define POS_SYS_REG0X17_PWM23_SEL                                11
#define POS_SYS_REG0X17_PWM12_SEL                                10
#define POS_SYS_REG0X17_ID_GND                                   9
#define POS_SYS_REG0X17_ADC_Q_SEL                                8
#define POS_SYS_REG0X17_ENB_BUSRT_SEL                            7
#define POS_SYS_REG0X17_ENB_BUSRT_REG                            6
#define POS_SYS_REG0X17_ANA_TEST_PIN                             5
#define POS_SYS_REG0X17_BB_TEST_PIN                              4
#define POS_SYS_REG0X17_CLK16M_PWD                               3
#define POS_SYS_REG0X17_HP_LDO_PWD                               2
#define POS_SYS_REG0X17_CLK96M_PWD                               1
#define POS_SYS_REG0X17_CB_BIAS_PWD                              0


enum sleep_type
{
    NO_SLEEP = 0,
    MCU_IDLE_SLEEP = 1, //cpu idle,clk run    
    MCU_REDUCE_VO_SLEEP = 2, //cpu idle,16M,PLL STOP,reduce cpu voltage
    MCU_DEEP_SLEEP = 3, //cpu idle,16M,PLL STOP
    
};


enum reduce_vol_sleep_level{
    REDUCE_VOL_V1_SLEEP = 0x06, // 1.2v
    REDUCE_VOL_V2_SLEEP = 0x05, //
    REDUCE_VOL_V3_SLEEP = 0x04, 
    REDUCE_VOL_V4_SLEEP = 0x03,
    
};


enum system_run_mode
{
    NORMAL_MODE = 0,
    DUT_FCC_MODE = (0x01 << 0),

};

#define MCU_CLK_16M   1
#define MCU_CLK_40M   2
#define MCU_CLK_80M   3
#define MCU_CLK_32M   4
#define MCU_CLK_64M   5


#define REDUCE_VOL_SLEEP        1
#define REDUCE_VOL_SLEEP_LEVEL        REDUCE_VOL_V1_SLEEP

#define C_WDT_RESET             0x1234
#define C_DEEPSLEEP_RESET       0x2345
#define C_FORCE_DIGITAL_RESET   0x3456
#define C_FORCE_ALL_RESET       0x0000
#define C_POWERON_RESET         0x0000

extern uint8_t system_mode;

void system_sleep_init(void);

void icu_init(void);

uint8_t get_sleep_mode(void);

void cpu_reduce_voltage_sleep(void);

void cpu_idle_sleep(void);

uint8_t icu_get_sleep_mode(void);

void icu_set_sleep_mode(uint8_t v);

void cpu_wakeup(void);

void mcu_clk_config(void);

void udi_wdt_enable(uint16_t wdt_cnt);

void mcu_clk_switch(uint8_t clk);

void rw_ip_sleep_test(uint32_t time);

uint8_t get_rw_ip_sleep_test_flag(void);
void deep_sleep(void);
void cpu_reset(void);
uint8_t system_reset_reson(void);
void system_set_reset_reson(uint32_t reson_data);
void cpu_24_reduce_voltage_sleep(void);
void cpu_24_wakeup(void);
void cpu_usb_reduce_voltage_sleep(void);
void sys_mode_init(uint8_t mode);
uint8_t get_sys_mode(void);
void deep_sleep_wakeup_set(uint8_t gpio);
void rw_wakeup(void);

#endif

