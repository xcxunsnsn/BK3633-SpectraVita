/**
****************************************************************************************
*
* @file rf_xvras.c
*
* @brief Atlas radio initialization and specific functions
*
* Copyright (C) Beken 2009-2015
*
* $Rev: $
*
****************************************************************************************
*/

/**
****************************************************************************************
* @addtogroup RF_XVR
* @ingroup RF
* @brief Radio Driver
*
* This is the driver block for radio
* @{
****************************************************************************************
*/

/**
 *****************************************************************************************
 * INCLUDE FILES
 *****************************************************************************************
 */
#include "reg_ipcore.h"        // DM core registers
#include "rwip_config.h"        // RW SW configuration
#include <string.h>             // for memcpy
#include "co_utils.h"           // common utility definition
#include "co_math.h"            // common math functions
#include "rf.h"                 // RF interface
#include "plf.h"                // Platform functions
#include "rwip.h"               // for RF API structure definition
#include "em_map.h"

#if (BLE_EMB_PRESENT)
#include "reg_blecore.h"        // ble core registers
#include "reg_em_ble_cs.h"      // control structure definitions
#endif //BLE_EMB_PRESENT
#if (BT_EMB_PRESENT)
#include "reg_btcore.h"         // bt core registers
#include "reg_em_bt_cs.h"       // control structure definitions
#endif //BT_EMB_PRESENT

#include "reg_access.h"            
#include "uart2.h"        
#include "reg_btcore.h"
#include "BK3633_RegList.h"
#include "user_config.h"
#include "icu.h"
#include "app.h"

/**
 ****************************************************************************************
 * DEFINES
 ****************************************************************************************
 **/
#define RPL_GAIN_TBL_SIZE           0x0F


#define RPL_SPIRD                   0x00
#define RPL_SPIWR                   0x80
#define RPL_RFPLL_TBL_SIZE          0x50
#define RPL_PWR_TBL_SIZE            0x0F

/* The offset value given below is the offset to add to the frequency table index to
   get the value to be programmed in the radio for each channel                      */
#define RPL_FREQTAB_OFFSET          0   // Offset for Ripple radio

/// Radio skew compensation (round trip delay)
#define RPL_RADIO_SKEW              12L

#define RFLOIF                      0x00

#define RPL_RSSI_20dB_THRHLD        -20
#define RPL_RSSI_45dB_THRHLD        -45
#define RPL_RSSI_48dB_THRHLD        -48
#define RPL_RSSI_55dB_THRHLD        -55
#define RPL_RSSI_60dB_THRHLD        -60
#define RPL_RSSI_70dB_THRHLD        -70

// EDR Control value
#define RPL_EDRCNTL                 18 // Default value is set to 18us


#define RPL_POWER_MAX               0x0c    //// ????
////#define RPL_POWER_MAX               0x06
#define RPL_POWER_MIN               0x01    //// ????
#define RPL_POWER_MSK               0x07    //// ????

// Generic RSSI Threshold
#define RF_RPL_RSSI_THR             0x29

/**
 ****************************************************************************************
 * GLOBAL VARIABLE DEFINITIONS
 *****************************************************************************************
 **/
// Power table
static const int8_t RF_RPL_TX_PW_CONV_TBL[RPL_PWR_TBL_SIZE] = 
{
    [0] = -23,
    [1] = -20,
    [2] = -17,
    [3] = -14,
    [4] = -11,
    [5] = -8,
    [6] = -5,
    [7] = -2
};



void CLK32K_AutoCali_init(void);

static void rf_reset(void)
{
}



//// IP
/**
 *****************************************************************************************
 * @brief Get the TX power as control structure TX power field from a value in dBm.
 *
 * @param[in] txpwr_dbm   TX power in dBm
 * @param[in] high        If true, return index equal to or higher than requested
 *                        If false, return index equal to or lower than requested
 *
 * @return The index of the TX power
 *
 *****************************************************************************************
 */
static uint8_t rf_txpwr_cs_get (int8_t txpwr_dbm, bool high)
{
    uint8_t i;

    for (i = RPL_POWER_MIN; i <= RPL_POWER_MAX; i++)
    {
        // Loop until we find a power just higher or equal to the requested one
        if (RF_RPL_TX_PW_CONV_TBL[i] >= txpwr_dbm)
            break;
    }

    // If equal to value requested, do nothing
    // Else if 'high' is false and index higher than the minimum one, decrement by one
    if ((RF_RPL_TX_PW_CONV_TBL[i] != txpwr_dbm) && (!high) && (i > RPL_POWER_MIN))
    {
        i--;
    }

    return(i);
}


/**
 *****************************************************************************************
 * @brief Get TX power in dBm from the index in the control structure
 *
 * @param[in] txpwr_idx  Index of the TX power in the control structure
 * @param[in] modulation Modulation: 1 or 2 or 3 MBPS
 *
 * @return The TX power in dBm
 *
 *****************************************************************************************
 */

static uint8_t rf_txpwr_dbm_get(uint8_t txpwr_idx, uint8_t modulation)
{
    // Power table should be provided
    return(0);
}

/**
 *****************************************************************************************
 * @brief Sleep function for  RF.
 *****************************************************************************************
 */

static void rf_sleep(void)
{
#if 1
    ip_deepslcntl_set(ip_deepslcntl_get()|0x07);
#else    
    ip_deepslcntl_set(ip_deepslcntl_get()   |
                      IP_DEEP_SLEEP_ON_BIT  |    // RW BT Core sleep
                      IP_RADIO_SLEEP_EN_BIT |   // Radio sleep
                      IP_OSC_SLEEP_EN_BIT);     // Oscillator sleep
#endif    
    
}


/**
 *****************************************************************************************
 * @brief Convert RSSI to dBm
 *
 * @param[in] rssi_reg RSSI read from the HW registers
 *
 * @return The converted RSSI
 *
 *****************************************************************************************
 */

static int8_t rf_rssi_convert (uint8_t rssi_reg)
{
    uint8_t RssidBm = 0;

    RssidBm = ((rssi_reg) >> 1) - 118;

    return(RssidBm);
}


static uint32_t rf_rpl_reg_rd (uint32_t addr)
{
  uint32_t ret;

    ret = REG_PL_RD(addr);

    return ret;
}

static void rf_rpl_reg_wr (uint32_t addr, uint32_t value)
{
        REG_PL_WR(addr, value);
}

static void rf_em_init(void)
{
    uint8_t idx = 0;
    uint8_t temp_freq_tbl[EM_RF_FREQ_TABLE_LEN];

    #if BT_EMB_PRESENT
    // First half part of frequency table is for the even frequencies
    while(idx < EM_RF_FREQ_TABLE_LEN/2)
    {
      temp_freq_tbl[idx] = 2*idx + RPL_FREQTAB_OFFSET;
      idx++;
    }
    while(idx < EM_RF_FREQ_TABLE_LEN)
    {
      temp_freq_tbl[idx] = 2*(idx-(EM_RF_FREQ_TABLE_LEN/2)) + 1 + RPL_FREQTAB_OFFSET;
      idx++;
    }
   // em_wr(&temp_freq_tbl[0], EM_FT_OFFSET, EM_RF_FREQ_TABLE_LEN);
    #elif BLE_EMB_PRESENT
    while(idx < EM_RF_FREQ_TABLE_LEN)
    {
      temp_freq_tbl[idx] = 2*idx + RPL_FREQTAB_OFFSET;
      idx++;
    }
    em_wr(&temp_freq_tbl[0], EM_FT_OFFSET, EM_RF_FREQ_TABLE_LEN);
    #endif // BT_EMB_PRESENT/BLE_EMB_PRESENT
}



/**
 ****************************************************************************************
 * RADIO FUNCTION INTERFACE
 ****************************************************************************************
 **/

#if defined(CFG_BT)        //// IP
/**
 *****************************************************************************************
 * @brief Decrease the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be decreased
 *
 * @return true when minimum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_dec(uint8_t link_id)
{
    bool boMinpow = true;
    uint8_t tx_pwr = em_bt_pwrcntl_txpwr_getf(EM_BT_CS_ACL_INDEX(link_id)) & RPL_POWER_MSK;

    if (tx_pwr > RPL_POWER_MIN)
    {
        //Increase the TX power value
        em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), tx_pwr-1);
        boMinpow = false;
    }

    return(boMinpow);
}

/**
 *****************************************************************************************
 * @brief Increase the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be increased
 *
 * @return true when maximum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_inc(uint8_t link_id)
{
    bool boMaxpow = true;
    uint8_t tx_pwr = em_bt_pwrcntl_txpwr_getf(EM_BT_CS_ACL_INDEX(link_id)) & RPL_POWER_MSK;

    if (tx_pwr < RPL_POWER_MAX)
    {
        //Increase the TX power value
        em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), tx_pwr+1);
        boMaxpow = false;
    }

    return(boMaxpow);
}

/**
 ****************************************************************************************
 * @brief Set the TX power to max
 *
 * @param[in] link_id     Link Identifier
 ****************************************************************************************
 */
static void txpwr_max_set(uint8_t link_id)
{
    //Increase the TX power value
    em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), RPL_POWER_MAX);
}
#endif // CFG_BT



#if defined(CFG_BLE)

static void rf_force_agc_enable(bool en)
{
    #if defined(CFG_BT)
    ip_radiocntl1_forceagc_en_setf(en);
    #else
    ip_radiocntl1_forceagc_en_setf(en);
    #endif //CFG_BLE

 //   ble_forceagc_en_setf(en);
    ble_radiocntl1_forceagc_en_setf(en);

}
#endif //CFG_BLE


volatile uint32_t XVR_ANALOG_REG_BAK[32] = {0};

void  xvr_reg_initial_24(void)
{

    XVR_ANALOG_REG_BAK[2] &= ~(1<<27);
    XVR_ANALOG_REG_BAK[2] |= (1<<14);
    addXVR_Reg0x2 = XVR_ANALOG_REG_BAK[2];

    addXVR_Reg0x2c = 0x10004040;//0x106A4C71;
    addXVR_Reg0x2d = 0x082ac441;//0x082CC446;// REG_2D 0x082CC444

    addXVR_Reg0x3a |= (1<<14);// REG_3A
    addXVR_Reg0x3b &= ~(3<<2);// REG_3B 0x22341048

}

void  xvr_reg_initial_BT(void)
{

    XVR_ANALOG_REG_BAK[2] |= (1<<27);
    XVR_ANALOG_REG_BAK[2] &= ~(1<<14);
    addXVR_Reg0x2 = XVR_ANALOG_REG_BAK[2];

    addXVR_Reg0x2c = 0x106A4C71;//0x106a5c71;//0x0a6a5c71;
    addXVR_Reg0x2d = 0x082CC446;//0x082CC446;// REG_2D 0x082CC444

    addXVR_Reg0x3a &= ~(1<<14);// REG_3A
    addXVR_Reg0x3b |= (2<<2);// REG_3B 0x22341048

}

uint8_t rfcali_value = 0;
void xvr_reg_initial(void) 
{
    addXVR_Reg0x0 = 0xC4B0323F  ;
    XVR_ANALOG_REG_BAK[0] = 0xC4B0323F;

    addXVR_Reg0x1 = 0x8295C200  ;
    XVR_ANALOG_REG_BAK[1] = 0x8295C200;
    
    addXVR_Reg0x2 = 0x2F42A000  ;
    XVR_ANALOG_REG_BAK[2] = 0x2F42A000;
    addXVR_Reg0x3 = 0x60035C62  ;
    XVR_ANALOG_REG_BAK[3] = 0x60035C62;

    if( get_sys_mode()!= DUT_FCC_MODE )
    {
        //addXVR_Reg0x4 = 0xFF56AACF  ;
        //XVR_ANALOG_REG_BAK[4] = 0xFF56AACF;//0xFFD6BBCC
        addXVR_Reg0x4 = 0xFF566FCF  ;
        XVR_ANALOG_REG_BAK[4] = 0xFF566FCF;//0xFFD6BBCC
    }
    else
    {
        ///uart_printf("DUT set xvr reg4\n");
        addXVR_Reg0x4 = 0xFFD771CF  ;
        XVR_ANALOG_REG_BAK[4] = 0xFFD771CF;
    }


    addXVR_Reg0x5 = 0x4620501F  ;
    XVR_ANALOG_REG_BAK[5] = 0x4620501F; //0x4620501F 03.31 // 0x4420501F 04.01
  
    #if(LDO_MODE)    
    addXVR_Reg0x6 = 0x8487CC00;//0x80B7CE20  ;
    XVR_ANALOG_REG_BAK[6] = 0x8487CC00;//0x80B7CE20;
    #else
    addXVR_Reg0x6 = 0x84A7CC00;//0x8097CE20  ;
    XVR_ANALOG_REG_BAK[6] = 0x84A7CC00;//0x8097CE20; 
    #endif
    if(get_sys_mode() == DUT_FCC_MODE)
    {
        addXVR_Reg0x6 = 0x84B7CC00;//0x80B7CE20  ;0x8097CE20
        XVR_ANALOG_REG_BAK[6] = 0x84B7CC00;//0x80B7CE20;
    }

    addXVR_Reg0x7 = 0xAA023FC0;//0xAA023DC0  ;
    XVR_ANALOG_REG_BAK[7] = 0xAA023FC0;//0xAA023DC0;

    #if(OPTIMIZE_POWER_CONSUMPTION)
    addXVR_Reg0x8 = 0x07B0402F;//0x0FB0C02F  ;
    XVR_ANALOG_REG_BAK[8] = 0x07B0402F;//0x0FB0C02F;
    #else
    addXVR_Reg0x8 = 0x0FB0C02F  ;
    XVR_ANALOG_REG_BAK[8] = 0x0FB0C02F;
    #endif

    addXVR_Reg0x9 = 0x7080224C;////0x7080220C  ;
    XVR_ANALOG_REG_BAK[9] = 0x7080224C;

    #if(LDO_MODE)
    addXVR_Reg0xa = 0x9C03F86B;//0x9C27785B  
    XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86B;//0x9C27785B;
    #else
    if(get_sys_mode() == DUT_FCC_MODE)    // dut is in LDO mode
    {
        XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86B;//0x9C27785B;
    }
    else
    {
        #if(OPTIMIZE_POWER_CONSUMPTION)
        XVR_ANALOG_REG_BAK[0xa] = 0x9C03F807;//0x9C03F86F;//0x9C27785B;
        #else
        XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86F;
        #endif
    }

    addXVR_Reg0xa = XVR_ANALOG_REG_BAK[0xa];//0x9C27785B  ;
    //addXVR_Reg0xa = 0x9E27F85F;//0x9C27785B  ;
    //XVR_ANALOG_REG_BAK[0xa] = 0x9E27F85F;//0x9C27785B;
    #endif

    //addXVR_Reg0xb = 0x0FD93F23  ;
    //XVR_ANALOG_REG_BAK[0xb] = 0x0FD93F23;
    addXVR_Reg0xb = 0x0FF93F23  ;
    XVR_ANALOG_REG_BAK[0xb] = 0x0FF93F23;
    addXVR_Reg0xc = 0x80001008  ;
    XVR_ANALOG_REG_BAK[0xc] = 0x80001008;
    addXVR_Reg0xd = 0xCC42BF23  ;
    XVR_ANALOG_REG_BAK[0xd] = 0xCC42BF23;
    addXVR_Reg0xe = 0x00309350  ;
    XVR_ANALOG_REG_BAK[0xe] = 0x00309350;
    addXVR_Reg0xf = 0x3126E978  ;
    XVR_ANALOG_REG_BAK[0xf] = 0x3126E978;


    addXVR_Reg0x1c = 0x999CDDC5  ;
    XVR_ANALOG_REG_BAK[0x1c] = 0x999CDDC5;
    addXVR_Reg0x1d = 0xEA8501C0  ;
    XVR_ANALOG_REG_BAK[0x1d] = 0xEA8501C0;
    addXVR_Reg0x1e = 0x80010180  ;
    XVR_ANALOG_REG_BAK[0x1e] = 0x80010180;
    addXVR_Reg0x1f = 0x00000000  ;
    XVR_ANALOG_REG_BAK[0x1f] = 0x00000000;

    #if(RF_FREQ_OFFSET_CALI)   
    uint8_t length = NVDS_LEN_RFCALI_VALUE;
    // Get bond status from NVDS
    if (nvds_get(NVDS_TAG_RFCALI_VALUE, &length, (uint8_t *)&rfcali_value) != NVDS_OK)
    {
        uart_printf("xtal_cal_set default:0x4e\n");
        rfcali_value = 0x4e;
        xtal_cal_set(0x4e);
    }
    else
    {
        uart_printf("xtal_cal_set rfcali_value:0x%x\n", rfcali_value);
        xtal_cal_set(rfcali_value);
    }
    #else
        xtal_cal_set(0x4e);
    #endif

    addXVR_Reg0x20 = 0x8E89BED6;// REG_20
    addXVR_Reg0x21 = 0x96000000;//0x96000000;// REG_21
    addXVR_Reg0x22 = 0x78000000;// REG_22
    addXVR_Reg0x23 = 0xA0000000;// REG_23
    addXVR_Reg0x24 = 0x000A0382;//0x000A0782;// REG_24
    addXVR_Reg0x25 = 0X00200000;// REG_25
    addXVR_Reg0x26 = 0x10200502;// REG_26 0x10200502 0x14a40505
    addXVR_Reg0x27 = 0x0008C900;// REG_27
    addXVR_Reg0x28 = 0x01011010;// REG_28
    addXVR_Reg0x29 = 0x3C104E00;// REG_29
    addXVR_Reg0x2a = 0x0e103830;//0x0e10404d;//0x0e103D68;// REG_2A
    addXVR_Reg0x2b = 0x00000408;// REG_2B
    //addXVR_Reg0x2c = 0x006A404d;// REG_2C   //0x006a404d
    addXVR_Reg0x2d = 0x082CC446;// REG_2D 0x082CC444
    addXVR_Reg0x2e = 0x00000100;//0x00000000;// REG_2E
    addXVR_Reg0x2f = 0X00000000;// REG_2F

    addXVR_Reg0x30 = 0x10010011;// REG_30//0x10010001
    addXVR_Reg0x31 = 0X00000000;// REG_31
    addXVR_Reg0x32 = 0X00000000;// REG_32
    addXVR_Reg0x33 = 0X00000000;// REG_33
    addXVR_Reg0x34 = 0X00000000;// REG_34
    addXVR_Reg0x35 = 0X00000000;// REG_35
    addXVR_Reg0x36 = 0X00000000;// REG_36
    addXVR_Reg0x37 = 0X00000000;// REG_37
    addXVR_Reg0x38 = 0X00000000;// REG_38
    addXVR_Reg0x39 = 0X00000000;// REG_39
    addXVR_Reg0x3a = 0x00128000;// REG_3A
    if(get_sys_mode() == DUT_FCC_MODE)
    {
        addXVR_Reg0x3b = 0x36341448;//0x36341048;// REG_3B 0x22341048
    }
    else
    {
        addXVR_Reg0x3b = 0x36341048;
    }
    addXVR_Reg0x3c = 0x01FF1c80;// REG_3C
    addXVR_Reg0x3d = 0x00000000;// REG_3D
    addXVR_Reg0x3e = 0X0000D940;// REG_3E
    addXVR_Reg0x3f = 0X00000000;// REG_3F

    addXVR_Reg0x40 = 0x01000000;// REG_40
    addXVR_Reg0x41 = 0x07050402;// REG_41
    addXVR_Reg0x42 = 0x120F0C0A;// REG_42
    addXVR_Reg0x43 = 0x221E1A16;// REG_43
    addXVR_Reg0x44 = 0x35302B26;// REG_44
    addXVR_Reg0x45 = 0x4B45403A;// REG_45
    addXVR_Reg0x46 = 0x635D5751;// REG_46
    addXVR_Reg0x47 = 0x7C767069;// REG_47
    addXVR_Reg0x48 = 0x968F8983;// REG_48
    addXVR_Reg0x49 = 0xAEA8A29C;// REG_49
    addXVR_Reg0x4a = 0xC5BFBAB4;// REG_4A
    addXVR_Reg0x4b = 0xD9D4CFCA;// REG_4B
    addXVR_Reg0x4c = 0xE9E5E1DD;// REG_4C
    addXVR_Reg0x4d = 0xF5F3F0ED;// REG_4D
    addXVR_Reg0x4e = 0xFDFBFAF8;// REG_4E
    addXVR_Reg0x4f = 0xFFFFFFFE;// REG_4F
    
    

    addPMU_Reg0x10 |= (0X1 << 8);
    addPMU_Reg0x12 &= ~(0X1 << 8);
    
    addPMU_Reg0x13 = 0XFFFFFF80;
    
    kmod_calibration();
    
   // kmod_calibration_2M();
    
    pll_clk_calibration();
    
#if (INTER_RC32K)
    XVR_ANALOG_REG_BAK[9] &= ~(0x01 << 26);
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];

    XVR_ANALOG_REG_BAK[0x1e]  |= 0x80000000;
    addXVR_Reg0x1e = XVR_ANALOG_REG_BAK[0x1e];
    CLK32K_AutoCali_init(); 
    
#else
    XVR_ANALOG_REG_BAK[9] |= (0x01 << 26);
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
    addXVR_Reg0x1e = 0x00010180  ;
    XVR_ANALOG_REG_BAK[0x1e] = 0x00010180;
#endif


}
void rf_init(struct rwip_rf_api *api)
{
    ////IP
    #if defined(CFG_BT)
        uint8_t length = PARAM_LEN_RSSI_THR;
    #endif //CFG_BT

        // Initialize the RF driver API structure
        api->reg_rd = rf_rpl_reg_rd;
        api->reg_wr = rf_rpl_reg_wr;
        api->txpwr_dbm_get = rf_txpwr_dbm_get;
        api->txpwr_min = RPL_POWER_MIN;
        api->txpwr_max = RPL_POWER_MAX;
        api->sleep = rf_sleep;
        api->reset = rf_reset;

    #if defined(CFG_BLE)
            api->force_agc_enable = rf_force_agc_enable;
    #endif //CFG_BLE

        api->rssi_convert = rf_rssi_convert;
        api->txpwr_cs_get = rf_txpwr_cs_get;

    #if defined(CFG_BT)
        api->txpwr_dec = rf_txpwr_dec;
        api->txpwr_inc = rf_txpwr_inc;
        api->txpwr_max_set = txpwr_max_set;
        // Initialize the RSSI thresholds (high, low, interference)
        // These are 'real' signed values in dBm
        if (   (rwip_param.get(PARAM_ID_RSSI_HIGH_THR, &length, (uint8_t*)&api->rssi_high_thr) != PARAM_OK)
            || (rwip_param.get(PARAM_ID_RSSI_LOW_THR, &length, (uint8_t*)&api->rssi_low_thr) != PARAM_OK)
            || (rwip_param.get(PARAM_ID_RSSI_INTERF_THR, &length, (uint8_t*)&api->rssi_interf_thr) != PARAM_OK) )
        {
            api->rssi_high_thr = (int8_t)RPL_RSSI_20dB_THRHLD;
            api->rssi_low_thr = (int8_t)RPL_RSSI_60dB_THRHLD;
            api->rssi_interf_thr = (int8_t)RPL_RSSI_70dB_THRHLD;
        }
    #endif //CFG_BT

        /// Initialize Exchange Memory
        rf_em_init();
        xvr_reg_initial();        //// ????

        //uart_printf("xvr_reg_init ok\r\n");
        
//        ip_radiocntl0_pack(/*uint16_t spiptr*/     (EM_RF_SW_SPI_OFFSET >> 2),
//                            /*uint8_t  spicfg*/   0,
//                            /*uint8_t  spifreq*/  1,
//                            /*uint8_t  spigo*/      0);

        /* ip RADIOCNTL1 */
        //uart_printf("RW common reg init\r\n");
        #if 1    //// Beken Version
        ip_radiocntl1_pack(/*uint8_t  forceagcen*/         0,
                            /*uint8_t  forceiq*/         0,
                            /*uint8_t  rxdnsl*/          0,
                            /*uint8_t  txdnsl*/          0,
                            /*uint16_t forceagclength*/  0,
                            /*uint8_t  syncpulsemode*/     0,
                            /*uint8_t  syncpulsesrc*/     0,
                            /*uint8_t  dpcorren*/         0,
                            /*uint8_t  jefselect*/         0,
                            /*uint8_t  xrfsel*/          0x02,
                            /*uint8_t  subversion*/      0x0);
                            
        
        ip_radiocntl1_set(0x00000020);
        //uart_printf("ip RADIOCNTL1 addr:0x%08x,val:0x%08x\r\n",ip_RADIOCNTL1_ADDR,ip_radiocntl1_get());
        if(get_sys_mode() != DUT_FCC_MODE){
            ip_timgencntl_set(0x01df0120);        ////Beken,
        }else{
            ip_timgencntl_set(0x01df00f0);
        }

        
    
        //uart_printf("ip_TIMGENCNTL addr:0x%08x,val:0x%08x\r\n",ip_TIMGENCNTL_ADDR,ip_timgencntl_get());
    #endif


   
    #if defined(CFG_BLE)
        //uart_printf("RW BLE reg init\r\n");
        /* BLE RADIOCNTL2 */
        ble_radiocntl2_pack(/*uint8_t  lrsynccompmode*/ 0x0,
                            /*uint8_t  rxcitermbypass*/ 0x0,
                            /*uint8_t  lrvtbflush*/     0x8,
                            /*uint8_t  phymsk*/         0x2, // mark that Coded phy are supported
                            /*uint8_t  lrsyncerr*/        0,
                            /*uint8_t  syncerr*/        0,
                            /*uint16_t freqtableptr*/    (EM_FT_OFFSET >> 2));
        ble_radiocntl2_set(0x00C000C0);
        //uart_printf("BLE_RADIOCNTL2 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOCNTL2_ADDR,ble_radiocntl2_get());

        /* BLE RADIOCNTL3 */
        #if 0
        ble_radiocntl3_pack(/*uint8_t rxrate3cfg*/      0x1, // map on 1 Mbps
                            /*uint8_t rxrate2cfg*/      0x1, // map on 1 Mbps
                            /*uint8_t rxrate1cfg*/      0x0,
                            /*uint8_t rxrate0cfg*/      0x1,
                            /*uint8_t rxsyncrouting*/ 0x0,
                            /*uint8_t rxvalidbeh*/      0x0,
                            /*uint8_t txrate3cfg*/      0x1, // map on 1 Mbps
                            /*uint8_t txrate2cfg*/      0x1, // map on 1 Mbps
                            /*uint8_t txrate1cfg*/      0x0,
                            /*uint8_t txrate0cfg*/      0x1,
                            /*uint8_t txvalidbeh*/      0x0);
        ble_radiocntl3_set(ble_radiocntl2_get());                    
        uart_printf("BLE_RADIOCNTL3 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOCNTL2_ADDR,ble_radiocntl2_get());
        #endif

        /* BLE RADIOPWRUPDN0 */
        // ble_radiopwrupdn0_pack(/*uint8_t syncposition0*/ 0,
        //                        /*uint8_t rxpwrup0*/      0x50,
        //                        /*uint8_t txpwrdn0*/      0x07,
        //                        /*uint8_t txpwrup0*/      0x55);
        if(get_sys_mode() != DUT_FCC_MODE){
            ble_radiopwrupdn0_set(0x00650065);
        }else{
            ble_radiopwrupdn0_pack(/*uint8_t syncposition0*/ 0,
                                    /*uint8_t rxpwrup0*/      0x20,
                                    /*uint8_t txpwrdn0*/      0x00,
                                    /*uint8_t txpwrup0*/      0x65);
        }
        //uart_printf("BLE_RADIOPWRUPDN0 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN0_ADDR,ble_radiopwrupdn0_get());
        
        /* BLE RADIOPWRUPDN1 */
        ble_radiopwrupdn1_pack(/*uint8_t syncposition1*/ 0,
                               /*uint8_t rxpwrup1*/      0x70,
                               /*uint8_t txpwrdn0*/      0x00,
                               /*uint8_t txpwrup1*/      0x65);
        ble_radiopwrupdn1_set(0x00700065);
       // uart_printf("BLE_RADIOPWRUPDN1 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN1_ADDR,ble_radiopwrupdn1_get());
        
        /* BLE RADIOPWRUPDN2 */      
        ble_radiopwrupdn2_pack(/*uint8_t syncposition2*/ 0,
                               /*uint8_t rxpwrup2*/      0x50, // 50
                               /*uint8_t txpwrdn2*/      0x07,
                               /*uint8_t txpwrup2*/      0x55);
        ble_radiopwrupdn2_set(0x00650065);
        //uart_printf("BLE_RADIOPWRUPDN2 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN2_ADDR,ble_radiopwrupdn2_get());
        
        
        /* BLE RADIOPWRUPDN3 */
        ble_radiopwrupdn3_pack(/*uint8_t txpwrdn3*/      0x07,
                               /*uint8_t txpwrup3*/      0x55);
        ble_radiopwrupdn3_set(0x00000065);
        //uart_printf("BLE_RADIOPWRUPDN3 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN3_ADDR,ble_radiopwrupdn3_get());
        
        /* BLE RADIOTXRXTIM0 */
        ble_radiotxrxtim0_pack(/*uint8_t rfrxtmda0*/   0,
                               /*uint8_t rxpathdly0*/  0x6,
                               /*uint8_t txpathdly0*/  0x6);
        if(get_sys_mode() != DUT_FCC_MODE){
            ble_radiotxrxtim0_set(0x00000608);
        }else{
            ble_radiotxrxtim0_set(0x00000805);  //0x00001007 0x00000503
        }
        //uart_printf("BLE_RADIOTXRXTIM0 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM0_ADDR,ble_radiotxrxtim0_get());

#if(BLE_CENTRAL    )


        /* BLE RADIOTXRXTIM1 */
        ble_radiotxrxtim1_pack(/*uint8_t rfrxtmda1*/ 0x00,
                               /*uint8_t rxpathdly1*/       0x04,
                               /*uint8_t txpathdly1*/       0x04);
        ble_radiotxrxtim1_set(0x00000402);
        //uart_printf("BLE_RADIOTXRXTIM1 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM1_ADDR,ble_radiotxrxtim1_get());
#else
        /* BLE RADIOTXRXTIM1 */
        ble_radiotxrxtim1_pack(/*uint8_t rfrxtmda1*/ 0x00,
                               /*uint8_t rxpathdly1*/       0x04,
                               /*uint8_t txpathdly1*/       0x0b);
        ble_radiotxrxtim1_set(0x00000408);
       
        //uart_printf("BLE_RADIOTXRXTIM1 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM1_ADDR,ble_radiotxrxtim1_get());
#endif

        /* BLE RADIOTXRXTIM2 */
       // uart_printf("# 09\r\n");
        ble_radiotxrxtim2_pack(/*uint8_t rxflushpathdly2*/ 0x10,
                               /*uint8_t rfrxtmda2*/       0x00,
                               /*uint8_t rxpathdly2*/       0x49,
                               /*uint8_t txpathdly2*/       0x03);
        ble_radiotxrxtim2_set(0x00002020);
        //uart_printf("BLE_RADIOTXRXTIM2 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM2_ADDR,ble_radiotxrxtim2_get());

        /* BLE RADIOTXRXTIM3 */
        ble_radiotxrxtim3_pack(/*uint8_t rxflushpathdly3*/ 0x10,
                               /*uint8_t rfrxtmda3*/       0x00,
                               /*uint8_t txpathdly3*/       0x03);
        ble_radiotxrxtim3_set(0x00000020);                       
        //uart_printf("BLE_RADIOTXRXTIM3 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM3_ADDR,ble_radiotxrxtim3_get());
        
      #if (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
        // Init the DF CNTL
        ble_dfcntl0_1us_pack(/*uint8_t rxsampstinst01us*/ 0x08, /*uint8_t rxswstinst01us*/ 0x18, /*uint8_t txswstinst01us*/ 0x19);
        ble_dfcntl0_2us_pack(/*uint8_t rxsampstinst02us*/ 0x08, /*uint8_t rxswstinst02us*/ 0x18, /*uint8_t txswstinst02us*/ 0x19);
        ble_dfcntl1_1us_pack(/*uint8_t rxsampstinst11us*/ 0x08, /*uint8_t rxswstinst11us*/ 0x18, /*uint8_t txswstinst11us*/ 0x19);
        ble_dfcntl1_2us_pack(/*uint8_t rxsampstinst12us*/ 0x08, /*uint8_t rxswstinst12us*/ 0x18, /*uint8_t txswstinst12us*/ 0x19);
        ble_dfantcntl_pack(/*uint8_t rxprimidcntlen*/ 1, /*uint8_t rxprimantid*/ 0, /*uint8_t txprimidcntlen*/ 1, /*uint8_t txprimantid*/ 0);
      #endif // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
    #endif // defined CFG_BLE

    #if defined(CFG_BT)
    
        /* EDRCNTL */
      #if 1        ////BEKEN
        bt_rwbtcntl_set(0x0000010A);
      #else
        bt_rwbtcntl_nwinsize_setf(0);
      #endif
        bt_edrcntl_rxgrd_timeout_setf(RPL_EDRCNTL);

        /* BT RADIOPWRUPDN */
        #if 1        //// BEKEN
        bt_radiopwrupdn_set(0x00710271);
        #else
        bt_radiopwrupdn_rxpwrupct_setf(0x42);
        bt_radiopwrupdn_txpwrdnct_setf(0x07);
        bt_radiopwrupdn_txpwrupct_setf(0x56);
        #endif
        /* BT RADIOCNTL 2 */
        #if 1        ////BEKEN
        //uart_printf("# 14\r\n");
        bt_radiocntl2_set(0x04070100);
        #else
        bt_radiocntl2_freqtable_ptr_setf((EM_FT_OFFSET >> 2));
        bt_radiocntl2_syncerr_setf(0x7);
        #endif
        /* BT RADIOTXRXTIM */
        #define PRL_TX_PATH_DLY 4
        #define PRL_RX_PATH_DLY (RPL_RADIO_SKEW - PRL_TX_PATH_DLY)
        #if 1        ////BEKEN
        //uart_printf("# 15\r\n");
            bt_radiotxrxtim_set(0x00000403);
        #else
            bt_radiotxrxtim_rxpathdly_setf(PRL_RX_PATH_DLY);
            bt_radiotxrxtim_txpathdly_setf(PRL_TX_PATH_DLY);
            bt_radiotxrxtim_sync_position_setf(0x38); // Default is 0x10
        #endif
        /* BT RADIOCNTL 3*/
        #if 1        ////BEKEN
        //uart_printf("# 16\r\n");
        bt_radiocntl3_set(0x39003900);
        #else
        bt_radiocntl3_pack( /*uint8_t rxrate2cfg*/      3,
                            /*uint8_t rxrate1cfg*/      2,
                            /*uint8_t rxrate0cfg*/      1,
                            /*uint8_t rxserparif*/      0,
                            /*uint8_t rxsyncrouting*/ 0,
                            /*uint8_t rxvalidbeh*/      0,
                            /*uint8_t txrate2cfg*/      3,
                            /*uint8_t txrate1cfg*/      2,
                            /*uint8_t txrate0cfg*/      1,
                            /*uint8_t txserparif*/      0,
                            /*uint8_t txvalidbeh*/      0);
        #endif
    #endif //CFG_BT


        // Settings for proper reception
    #if defined(CFG_BLE)
        ip_radiocntl1_forceiq_setf(1);
        ip_radiocntl1_dpcorr_en_setf(0x0);
        ASSERT_ERR(ip_radiocntl1_dpcorr_en_getf() == 0x0);
    #endif // CFG_BLE

    #if defined(CFG_BT)
        ip_radiocntl1_dpcorr_en_setf(0x1);
        ASSERT_ERR(ip_radiocntl1_dpcorr_en_getf() == 0x1);
    #endif // CFG_BT

    #if defined(CFG_BLE)
        // Force IQ mode for BLE only
        ip_radiocntl1_forceiq_setf(1);
    #endif //CFG_BLE
//    rf_debug_gpio_init(2);
}

void Delay_us(int num)
{
   volatile int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 10; x++);
    }
}

void Delay(int num)
{
    volatile int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 50; x++);
    }
}

void Delay_ms(int num) //sync from svn revision 18
{
    volatile int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 3260; x++);
    }

}


uint32_t value_kcal_result_1M;
uint32_t value_kcal_result_2M;
void kmod_calibration(void) 
{

/* 1���ڳ�ʼ��0X26�� [16:28] = 0x1084 
            2����0X30��BT ���ó� BT = 1ȥУ׼
    
        3��У׼��ɺ���0X30��BT ���ó� BT = 0.5
    
    
    */

    uint32_t value;
    uint32_t value_kcal_result;

// start
    addXVR_Reg0x30 &= ~(0x3 << 0);
    addXVR_Reg0x30 |= (0x1 << 3);
      
    
    addXVR_Reg0x24 |= (0x1<< 30);
    
    addXVR_Reg0x24 |= 0x80000000;//(0x1<< 31);
    
    
    addXVR_Reg0x24 &= ~(0x1 << 17);
    Delay_ms(10);
    addXVR_Reg0x24 &= ~(0x7f);
    addXVR_Reg0x24 |= (0x2 << 0);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<12);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<13);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<11);
    Delay_ms(10);
    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);
    XVR_ANALOG_REG_BAK[3] |= (0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<16);
    Delay_ms(50);
    value = addXVR_Reg0x12;

    value = ((value >> 16) & 0x1fff);

    value_kcal_result =  ((500*256/value)&0x1ff) ; 
    uart_printf("value_kcal_result=%x\r\n",value_kcal_result);
    addXVR_Reg0x30 &= (~(0x1ff<<8));
    addXVR_Reg0x30 |= (value_kcal_result<<8);
    
    value_kcal_result_1M = value_kcal_result;
    Delay_ms(50);

    addXVR_Reg0x25 &= ~(1<<16);
    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);

    XVR_ANALOG_REG_BAK[3] |= (0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    addXVR_Reg0x25 &= ~(1<<11);

    addXVR_Reg0x25 &= ~(1<<13);
    addXVR_Reg0x25 &= ~(1<<12); 
    addXVR_Reg0x24 |= (0x1 << 17);
    
    addXVR_Reg0x30 |= (0x1 << 0);
    
    addXVR_Reg0x30  &= ~(0x1<< 3);
      
    
    addXVR_Reg0x24 &= ~(0x1<< 30);
    
    addXVR_Reg0x24 &= ~(0x80000000);
    
}


void kmod_calibration_2M(void) 
{

    

    uart_printf("%s\r\n",__func__);
    uint32_t value;
    uint32_t value_kcal_result;

// start
    
    addXVR_Reg0x30 |= (0x1 << 3);
    addXVR_Reg0x30 &= ~(0x3 << 0);  
    addXVR_Reg0x30 &= ~(0x1FF << 8);  
    addXVR_Reg0x30 |= (0x100 << 8);
    addXVR_Reg0x30 &= ~(0x1FF << 20);  
    addXVR_Reg0x30 |= (0x100 << 20);

    addXVR_Reg0x24 |= (0x1<< 30); // 2M MODE
    addXVR_Reg0x24 |= (0x1ul<< 31);
    
    
    addXVR_Reg0x24 &= ~(0x1 << 17);
    
    addXVR_Reg0x24 &= ~(0x7f);
   
    
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<12);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<13);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<11);
    
    Delay_ms(10);

    XVR_ANALOG_REG_BAK[3] |= (0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];

    Delay_ms(10);
    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<16);
    
    ///////////////////////////start end
    Delay_ms(200);
    value = addXVR_Reg0x12;
    uart_printf("read xvr_reg0x12:0x%x\r\n",value);
    value = ((value >> 16) & 0x1fff);
    uart_printf("cal value:0x%x\r\n",value);
    value_kcal_result =  ((500*256/value)&0x1ff) ; 
    addXVR_Reg0x30 &= ~(0x1ff<<8);
    addXVR_Reg0x30 |= (value_kcal_result<<8);
    uart_printf("value_kcal_resulte:0x%x\r\n",value_kcal_result);
    value_kcal_result_2M = value_kcal_result;
    /// end   

    addXVR_Reg0x25 &= ~(1<<16);

    addXVR_Reg0x25 &= ~(0X7 <<11); // 
    
    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];

    XVR_ANALOG_REG_BAK[3] |= (0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    
    ////////////////////////
    
    addXVR_Reg0x30 &= ~(0x1 <<3);
    
    addXVR_Reg0x30  &= ~(0x3 << 0);  
    addXVR_Reg0x30  |= (0x1<< 0);
      
    
    addXVR_Reg0x24 &= ~(0x1ul<< 30);
    
    addXVR_Reg0x24 &= ~(0x1ul<< 31);
    addXVR_Reg0x24 |= (0x1<< 17);

}


void kmod_fm_gain_set_1M(void)
{
  addXVR_Reg0x30 &= ~(0x1ff<<8);
  addXVR_Reg0x30 |= (value_kcal_result_1M<<8);

}
void kmod_fm_gain_set_2M(void)
{
  addXVR_Reg0x30 &= ~(0x1ff<<8);
  addXVR_Reg0x30 |= (value_kcal_result_2M<<8);

}

void CLK32K_AutoCali_init(void)
{
    #if CONFIG_RC32K_CALIBRATION
    volatile uint32_t temp_reg = 0;
    uart_printf("start 32k cali_first\n");
    XVR_ANALOG_REG_BAK[0x7] &= ~(0x01 << 9);
    addXVR_Reg0x7 = XVR_ANALOG_REG_BAK[0x7];
    Delay_us(10);
    XVR_ANALOG_REG_BAK[0x9] &= ~(0x01 << 6);
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[0x9]; 

    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc]; 

    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];
    Delay_ms(30);   ///Delay_ms(100);
    temp_reg = addPMU_Reg0x5;
    uart_printf("addPMU_Reg0x5 = 0x%x\n", temp_reg);
    ///backset 1
    XVR_ANALOG_REG_BAK[0xc] &= ~(0x1ff << 4);
    XVR_ANALOG_REG_BAK[0xc] |= ((temp_reg >> 9) & 0x1ff) << 4;

    ///backset 2
    XVR_ANALOG_REG_BAK[0xc] &= ~(0x0f);
    XVR_ANALOG_REG_BAK[0xc] |= ((temp_reg >> 18) & 0x0f);

    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];

    ///backset 3
    XVR_ANALOG_REG_BAK[0x9] &= ~(0x07 << 3);
    XVR_ANALOG_REG_BAK[0x9] |= ((temp_reg >> 22) & 0x7) << 3;
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[0x9];

    //manu set
    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 13);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];

    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc]; 

    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];
    uart_printf("XVR_ANALOG_REG_BAK[0x9] = 0x%x\n", XVR_ANALOG_REG_BAK[0x9]);
    uart_printf("XVR_ANALOG_REG_BAK[0xc] = 0x%x\n", XVR_ANALOG_REG_BAK[0xc]);
    Delay_ms(50);
    XVR_ANALOG_REG_BAK[0x7] |= (0x01 << 9);
    addXVR_Reg0x7 = XVR_ANALOG_REG_BAK[0x7];
    #else
    #if 0//enable HW cali one time
    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];
    Delay_ms(2);
    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];
    Delay_ms(30);
    #else //enable HW auto cali
    XVR_ANALOG_REG_BAK[0xc] = 0x3a98d004;
    addXVR_Reg0xc = 0x3a98d004;
    #endif
    #endif
}

#if (CONFIG_RC32K_CALIBRATION)
#include <stdio.h>

// remove the maximum and minimum values from an array and return the average of the remaining elements
static uint32_t remove_min_max_getavg(uint32_t *arr, uint32_t size) 
{
    uint32_t sum = 0;
    uint32_t avg;
    uint32_t min = arr[0];
    uint32_t max = arr[0];

    for (int i = 0; i < size; ++i) 
    {
        if (arr[i] < min) 
        {
            min = arr[i];
        }
        if (arr[i] > max) 
        {
            max = arr[i];
        }
        sum += arr[i];
    }
    avg=(sum - min - max) / (size - 2);
    return avg;
}



void CLK32K_AutoCali_Start(void)
{
// Note:from Start to End,must selay 30ms at least
 //   uart_printf("start 32k cali\n");
    XVR_ANALOG_REG_BAK[0x7] &= ~(0x01 << 9);
    addXVR_Reg0x7 = XVR_ANALOG_REG_BAK[0x7];
    Delay_us(10);
    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 13);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];

    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc]; 
    XVR_ANALOG_REG_BAK[0x9] &= ~(0x01 << 6);
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[0x9]; 

    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc]; 

    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];
 //   Delay_ms(30);   ///Delay_ms(100);
    
}

void CLK32K_AutoCali_End(void)
{
  // Note:from Start to End,must selay 30ms at least

    uint32_t temp_reg[4] = {0};
    volatile uint32_t avg = 0;
    volatile uint32_t avg_temp = 0;
    static volatile uint32_t last_avg = 0;

    for(uint32_t i=0;i<4;i++)
    {
        temp_reg[i] = (addPMU_Reg0x5&0x01ffffff)>>9;
        
    }
    avg=remove_min_max_getavg(temp_reg,4);

    avg<<=9;

    if( (last_avg>>18) > (avg>>18) )
    {
        avg_temp=(last_avg>>18) - (avg>>18);
    }
    else
    {
        avg_temp=(avg>>18)-(last_avg>>18) ;
    }
    if( avg_temp>1 && last_avg != 0) 
    {
        uart_printf("invalid avg = 0x%x\n", avg);
        return;
    }
    last_avg=avg;

    uart_printf("addPMU_Reg0x5 avg = 0x%x\n", avg);
    ///backset 1
    XVR_ANALOG_REG_BAK[0xc] &= ~(0x1ff << 4);
    XVR_ANALOG_REG_BAK[0xc] |= ((avg >> 9) & 0x1ff) << 4;

    ///backset 2
    XVR_ANALOG_REG_BAK[0xc] &= ~(0x0f);
    XVR_ANALOG_REG_BAK[0xc] |= ((avg >> 18) & 0x0f);


    ///backset 3
    XVR_ANALOG_REG_BAK[0x9] &= ~(0x07 << 3);
    XVR_ANALOG_REG_BAK[0x9] |= ((avg >> 22) & 0x7) << 3;
//    uart_printf("addPMU_Reg0xc,9 = 0x%x,0x%x\n", XVR_ANALOG_REG_BAK[0xc],XVR_ANALOG_REG_BAK[0x9]);

    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[0x9];

    //manu set
    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 13);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];

    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc]; 

    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];
    XVR_ANALOG_REG_BAK[0x7] |= (0x01 << 9);
    addXVR_Reg0x7 = XVR_ANALOG_REG_BAK[0x7];
 //   uart_printf("XVR_ANALOG_REG_BAK[0x9] = 0x%x\n", XVR_ANALOG_REG_BAK[0x9]);
  //  uart_printf("XVR_ANALOG_REG_BAK[0xc] = 0x%x\n", XVR_ANALOG_REG_BAK[0xc]);
}
#endif


//���õ��ز�����
//freq:Ƶ�����ã�˫Ƶ��(2-80)
//power:���ʵȼ�(0x1-0xf)
void singleWaveCfg(uint8_t freq, uint8_t power_level)
{
    uint32_t val = 0;
    uint32_t reg = XVR_ANALOG_REG_BAK[0x04];
    
    addXVR_Reg0x4 = reg | (0x1 << 29);
    
    val |= freq;
    val |= (power_level<< 7);
    addXVR_Reg0x24 = val;
    addXVR_Reg0x25 |= (0x1<<12) |(0x1<<13);

    while(1);
}


//�޸ķ��书��,Ĭ��Ϊ�ȼ�7
//power_level:���ʵȼ�(0x0-0xf) :4->0db,7->4.5db f->9db
void set_power(uint8_t power_level)
{
    uint32_t val = 0;
    uint32_t reg = XVR_ANALOG_REG_BAK[0x04];

    addXVR_Reg0x24 &= ~(0x1 << 20);
    addXVR_Reg0x4 = reg | (0x1 << 29);
    val |= (power_level << 7);
    addXVR_Reg0x24 &= ~(0xf << 7);
    addXVR_Reg0x24 |= val;
}

///����Ƶƫ����
///cal_dataĬ��Ϊ0x35,��Χ0x00~0x7f
void xtal_cal_set(uint8_t cal_data)
{
    if(cal_data>0x7f)
        cal_data=0x7f;
    XVR_ANALOG_REG_BAK[3] = 0x60000C62|(cal_data<<12);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3] ;
}

void pll_clk_calibration(void)
{
    XVR_ANALOG_REG_BAK[0x1e]  |= 0x20000;
    addXVR_Reg0x1e = XVR_ANALOG_REG_BAK[0x1e];
    Delay_ms(2);
    XVR_ANALOG_REG_BAK[0x1e]  &= ~0x20000;
    addXVR_Reg0x1e = XVR_ANALOG_REG_BAK[0x1e];
}

void rf_debug_gpio_init(uint8_t GPIO_C_D)
{
    
    addSYS_Reg0xc |= (0x01 << 4);
    
    switch(GPIO_C_D)
    {
        case 1:
        {
            addSYS_Reg0xc |= (0x01 << 5);

            set_AON_GPIO_Reg0x10_GPIO16_Config(0X40);
            set_AON_GPIO_Reg0x11_GPIO17_Config(0X40);
            set_AON_GPIO_Reg0x12_GPIO18_Config(0X40);
            set_AON_GPIO_Reg0x13_GPIO19_Config(0X40);
            set_AON_GPIO_Reg0x14_GPIO20_Config(0X40);
            set_AON_GPIO_Reg0x15_GPIO21_Config(0X40);
            set_AON_GPIO_Reg0x16_GPIO22_Config(0X40);
            set_AON_GPIO_Reg0x17_GPIO23_Config(0X40);

            addSYS_Reg0xc |= (0x01 << 3);

            addSYS_Reg0xc &= ~(0x07 << 0);
            addSYS_Reg0xc |= (0x07 << 0);
        }break;
        
        case 0:
        {
            addSYS_Reg0xc &= ~(0x01 << 5);
            set_AON_GPIO_Reg0x18_GPIO24_Config(0X40);
            set_AON_GPIO_Reg0x19_GPIO25_Config(0X40);
            set_AON_GPIO_Reg0x1a_GPIO26_Config(0X40);
            set_AON_GPIO_Reg0x1b_GPIO27_Config(0X40);
            set_AON_GPIO_Reg0x1c_GPIO28_Config(0X40);
            set_AON_GPIO_Reg0x1d_GPIO29_Config(0X40);
            set_AON_GPIO_Reg0x1e_GPIO30_Config(0X40);
            set_AON_GPIO_Reg0x1f_GPIO31_Config(0X40);

            addSYS_Reg0xc |= (0x01 << 3);

            addSYS_Reg0xc &= ~(0x07 << 0);
            addSYS_Reg0xc |= (0x07 << 0);
        }break;
        
        case 2:
        {
            addSYS_Reg0xc = (0x01 << 5) + (0x01 << 4) + (4);
            set_AON_GPIO_Reg0x10_GPIO16_Config(0X40);
            set_AON_GPIO_Reg0x11_GPIO17_Config(0X40);
            set_AON_GPIO_Reg0x12_GPIO18_Config(0X40);
            set_AON_GPIO_Reg0x13_GPIO19_Config(0X40);
            set_AON_GPIO_Reg0x14_GPIO20_Config(0X40);
            set_AON_GPIO_Reg0x15_GPIO21_Config(0X40);
            set_AON_GPIO_Reg0x16_GPIO22_Config(0X40);
            set_AON_GPIO_Reg0x17_GPIO23_Config(0X40);
        }break;
    }
   
}

uint32_t rwip_get_xvr_analog_reg_bk_count(void)
{
    return sizeof(XVR_ANALOG_REG_BAK) / sizeof(XVR_ANALOG_REG_BAK[0]);
}

uint32_t ana_xvr_reg_get(uint8_t reg)
{
    //uart_printf("%s reg:%x\n", __func__, reg);
    if(reg > rwip_get_xvr_analog_reg_bk_count())
    {
        return 0;
    }else
    {
        return XVR_ANALOG_REG_BAK[reg];
    } 
}

void ana_xvr_reg_set(uint8_t reg, uint32_t val)
{
    //uart_printf("%s reg:0x%x, val:0x%x\n", __func__, reg, val);
    if(reg > rwip_get_xvr_analog_reg_bk_count())
    {
        return;
    }else
    {
        XVR_ANALOG_REG_BAK[reg] = val;
        addXVR_Reg0xX(reg) = XVR_ANALOG_REG_BAK[reg];
    } 
}

