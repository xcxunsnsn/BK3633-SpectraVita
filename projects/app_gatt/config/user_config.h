/**
 ****************************************************************************************
 *
 * @file user_config.h
 *
 * @brief Configuration of the BT function
 *
 * Copyright (C) Beken 2019
 *
 ****************************************************************************************
 */
#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_
#include "uart2.h"
#include "uart.h"

#define ITEST_ENABLE               1
#define RF_FREQ_OFFSET_CALI        0

#define VIRTUAL_UART_H4TL          1
#define UART_PRINTF_ENABLE         1///串口打印开关
#define DEBUG_HW                   0///硬件调试
#define DEBUG_HW_DIGITAL           0///数字硬件调试
#define GPIO_DBG_MSG               0///DEBUG信息可以通过DEBUG_MSG函数输出到GPIO
#define DEBUG_RF_REG               0///RF调试，为1可以通过串口读写RF寄存器
#define LDO_MODE                   0///LDO工作模式
#define LDO_MODE_IN_SLEEP          1

#define OPTIMIZE_POWER_CONSUMPTION      0

#if(OPTIMIZE_POWER_CONSUMPTION)
#undef LDO_MODE
#define LDO_MODE                     0
#undef LDO_MODE_IN_SLEEP
#define LDO_MODE_IN_SLEEP            0

#define OPTIMIZE_POWER_CONSUMPTION_TEMP 0
#if(OPTIMIZE_POWER_CONSUMPTION_TEMP)
#define CONFIG_RC32K_CALIBRATION     1
#define ADC_DRIVER                   1
#endif// OPTIMIZE_POWER_CONSUMPTION_TEMP
#endif // OPTIMIZE_POWER_CONSUMPTION


//DRIVER CONFIG
#define UART0_DRIVER                1
#define UART2_DRIVER                0
#define GPIO_DRIVER                 1
#define ADC_DRIVER                  1

#define I2C_DRIVER                  1
#define PWM_DRIVER                  0
#define USB_DRIVER                  0 
#define SPI_DRIVER                  1 
#define AON_RTC_DRIVER              0
#define I2S_DRIVER                    0

/* 1: only read AS7331/AS7341/ADXL345 IDs, then stop before BLE startup. */
#define SENSOR_ID_ONLY_TEST           0

/*
 * Default sensor settings used after boot and before the APP overrides them
 * through protocol V4 commands 0x42/0x44/0x46/0x48.
 */
#define SENSOR_DEFAULT_MSS_ATIME      29
#define SENSOR_DEFAULT_MSS_ASTEP      599
#define SENSOR_DEFAULT_MSS_GAIN       10
#define SENSOR_DEFAULT_FIFO_TIME      720
#define SENSOR_DEFAULT_FIFO_GAIN      5
#define SENSOR_DEFAULT_UV_GAIN        9
#define SENSOR_DEFAULT_DIST_GAIN      0

#define ADC_CALIB                    0//////У׼ADC��Ҫ��оƬһ���ȶ��Ĺ���ѹ��Ȼ����ADC�ο���ѹ

#define ENABLE_PHY_2M_LE_CODE       0 ////2M֧��2M�շ�   

/******************************************************/

#define uart_printf              uart0_printf

/// Default Device Name
#define APP_DFLT_DEVICE_NAME            ("SpectraVita")

#define APP_SCNRSP_DATA         "\x09\xFF\x00\x60\x42\x4B\x2D\x42\x4C\x45"
#define APP_SCNRSP_DATA_LEN     (10)


#define APP_DFLT_ADV_0XFF            ("BK3633_01234567890_01234567890_01234567890_01234567890_01234567890_01234567890_01234567890_01234567890_01234567890_01234567890")
#define APP_DFLT_ADV_0XFF_LEN        (sizeof(APP_DFLT_ADV_0XFF))



/// Advertising channel map - 37, 38, 39
#define APP_ADV_CHMAP           (0x07)
/// Advertising minimum interval - 40ms (64*0.625ms)
#define APP_ADV_INT_MIN         (3200)//160
/// Advertising maximum interval - 40ms (64*0.625ms)
#define APP_ADV_INT_MAX         (3200)//160
/// Fast advertising interval
#define APP_ADV_FAST_INT        (32)


//��С���Ӽ��
#define BLE_UAPDATA_MIN_INTVALUE        24 /* 30 ms, unit 1.25 ms */
//������Ӽ�� 
#define BLE_UAPDATA_MAX_INTVALUE        40 /* 50 ms, unit 1.25 ms */
//����Latency
#define BLE_UAPDATA_LATENCY                0
//���ӳ�ʱ
#define BLE_UAPDATA_TIMEOUT               1000 /* 10 s, unit 10 ms */

// If you want to enable MULTI BOND, make sure CFG_BLE_SMP, BLE_APP_SEC are enabled
#define MULTI_BOND              0

#ifndef CONFIG_RC32K_CALIBRATION
#define CONFIG_RC32K_CALIBRATION     1
#endif

//ADC CONFIG
#define BAT_ADC 7
#define UV_ADC 	4

//GPIO CONFIG
#define BLE_LED_PIN	    0x14
#define VDD_1P8_PIN     0x30
#define VDD_SENSOR_PIN  0x17
#define DC_DET_PIN   0x32
#define CHG_OK_PIN   0x11
#define BAT_POWER_PIN   0x35
#define W25QXX_CS_PIN   0x07
#define W25QXX_SCL_PIN   0x04
#define W25QXX_SDA_PIN   0x05
#define W25QXX_SDO_PIN   0x06

#define ADXL_CS_PIN   0x12
/* XZ: ADXL345 INT1 -> P20; INT2 -> P37. */
#define ADXL_IN1_PIN   0x20
#define ADXL_IN2_PIN   0x37
#define WH4530_INT_PIN   0x21
#define UV_READY_PIN   0x10
#define UV_SYN_PIN   0x34


//#define __APP_ATTR_ARM __attribute__((section("APP_BLE_FIQ")))
#define __APP_ATTR_ARM 
#endif // USER_CONFIG_H_
