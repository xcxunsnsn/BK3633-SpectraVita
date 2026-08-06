/**
 ****************************************************************************************
 *
 * @file rf.h
 *
 * @brief Common header file for all radios.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef RF_H_
#define RF_H_

/**
 ****************************************************************************************
 * @addtogroup RF
 * @ingroup DRIVERS
 * @brief Common definitions for radio modules.
 *
 * This module declares the functions and constants that have to be defined for all RF.
 *
 * @{
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

struct rwip_rf_api;  // forward declaration to avoid including rw.h

/**
 *****************************************************************************************
 * @brief Initialization of RF.
 *
 * This function initializes the RF and fills the structure containing the function
 * pointers and parameters required by the RW BT stack.
 *
 * @param[out]  api  Pointer to the BT RF API structure
 *
 *****************************************************************************************
 */
void rf_init(struct rwip_rf_api *api);
void  xvr_reg_initial_BT(void);

void xvr_reg_initial(void) ;
void kmod_calibration(void) ;
void kmod_calibration_2M(void) ;
void  rf_debug_gpio_init(uint8_t GPIO_C_D);
void CLK32K_AutoCali_init(void);
void Delay(int num);
void Delay_us(int num);
void Delay_ms(int num);
void singleWaveCfg(uint8_t freq, uint8_t power_level);
void set_power(uint8_t power_level);
void xtal_cal_set(uint8_t cal_data);
void pll_clk_calibration(void);
void rf_debug_gpio_init(uint8_t GPIO_C_D);
void CLK32K_AutoCali_Start(void);
void CLK32K_AutoCali_End(void);
uint32_t ana_xvr_reg_get(uint8_t reg);
void ana_xvr_reg_set(uint8_t reg, uint32_t val);
#endif // RF_H_
