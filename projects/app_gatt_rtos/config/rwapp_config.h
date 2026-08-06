/**
 ****************************************************************************************
 *
 * @file rwapp_config.h
 *
 * @brief Application configuration definition
 *
 * Copyright (C) RivieraWaves 2009-2016
 *
 ****************************************************************************************
 */


#ifndef _RWAPP_CONFIG_H_
#define _RWAPP_CONFIG_H_

/**
 ****************************************************************************************
 * @addtogroup app
 * @brief Application configuration definition
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */
 
#define CFG_APP_DIS
#define CFG_APP_BATT
#define CFG_APP_SEC
#define CFG_APP_FEE0S
#define CFG_APP_FCC0S
//#define CFG_APP_FTMPS
#define CFG_APP_OADS




/******************************************************************************************/
/* -------------------------   BLE APPLICATION SETTINGS      -----------------------------*/
/******************************************************************************************/

/// User define Application
#if defined(CFG_APP_FEE0S)
#define BLE_APP_FEE0S           1
#else // defined(CFG_APP_FEE0S)
#define BLE_APP_FEE0S           0
#endif // defined(CFG_APP_FEE0S)

/// User define Application
#if defined(CFG_APP_FCC0S)
#define BLE_APP_FCC0S           1
#else 
#define BLE_APP_FCC0S           0
#endif // defined(CFG_APP_FCC0S)


/// Health Thermometer Application
#if defined(CFG_APP_HT)
#define BLE_APP_HT           1
#else // defined(CFG_APP_HT)
#define BLE_APP_HT           0
#endif // defined(CFG_APP_HT)

/// HID Application
#if defined(CFG_APP_HID)
#define BLE_APP_HID          1
#else // defined(CFG_APP_HID)
#define BLE_APP_HID          0
#endif // defined(CFG_APP_HID)

/// DIS Application
#if defined(CFG_APP_DIS)
#define BLE_APP_DIS          1
#else // defined(CFG_APP_DIS)
#define BLE_APP_DIS          0
#endif // defined(CFG_APP_DIS)

#if defined(CFG_APP_ANCS)
#define BLE_APP_ANCS          1
#else 
#define BLE_APP_ANCS          0
#endif 

#if defined(CFG_APP_OADS)
#define BLE_APP_OADS          1
#else 
#define BLE_APP_OADS          0
#endif 

/// Audio Application
#if defined(CFG_APP_AM0)
#define BLE_APP_AM0          1
#else // defined(CFG_APP_AM0)
#define BLE_APP_AM0          0
#endif // defined(CFG_APP_AM0)

#if defined(CFG_APP_FTMPS)
#define BLE_APP_FTMPS          1
#else
#define BLE_APP_FTMPS          0
#endif

/// Battery Service Application
#if (BLE_APP_HID)
    #define BLE_APP_BATT         1
#else
    /// BATT Application
    #if defined(CFG_APP_BATT)
        #define BLE_APP_BATT          1
    #else // defined(CFG_APP_BATT)
        #define BLE_APP_BATT         0
    #endif
#endif //(BLE_APP_HID)

/// Security Application
#if (defined(CFG_APP_SEC) || BLE_APP_HID || BLE_APP_AM0)
#define BLE_APP_SEC          1
#else //(defined(CFG_APP_SEC) || BLE_APP_HID || BLE_APP_AM0)
#define BLE_APP_SEC          0
#endif //(defined(CFG_APP_SEC) || BLE_APP_HID || BLE_APP_AM0)

/// Secure Connection
#if (BLE_APP_AM0)
#define BLE_APP_SEC_CON      1
#else //(BLE_APP_AM0)
//#define BLE_APP_SEC_CON      1
#define BLE_APP_SEC_CON      0
#endif ////(BLE_APP_AM0)

/// Hearing Aid Service Configuration
#if (BLE_APP_AM0)
#define AM0_APP_OPTIONAL_CHARACTERISTICS        0
/// Default Settings for Optional Characteristics
/// Specification Default Value
#define AM0_APP_DEFAULT_TREBLE                  0
#define AM0_APP_DEFAULT_BASS                    0
/// Following have no specification Defaults - as application dependent.
/// Please change to suit product.
#define AM0_APP_DEFAULT_MIXED_VOL_STEP          5
#define AM0_APP_DEFAULT_MIXED_VOL               10
#define AM0_APP_DEFAULT_MIC_VOL_STEP            8
#define AM0_APP_DEFAULT_STREAM_VOL_STEP         7
#define AM0_APP_DEFAULT_MIC_SENSITIVITY         127
#define AM0_APP_DEFAULT_SENSITIVITY_STEP        12
/// Default is to have no active streaming program id.
#define AM0_APP_DEFAULT_ACT_STREAM_PROG_ID      0
#else
#define AM0_APP_OPTIONAL_CHARACTERISTICS        0
#endif //BLE_APP_AM0

/// @} rwapp_config

#endif /* _RWAPP_CONFIG_H_ */
