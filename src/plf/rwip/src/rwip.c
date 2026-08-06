/**
****************************************************************************************
*
* @file rwip.c
*
* @brief RW IP SW main module
*
* Copyright (C) RivieraWaves 2009-2015
*
*
****************************************************************************************
*/

/**
 ****************************************************************************************
 * @addtogroup RW IP SW main module
 * @ingroup ROOT
 * @brief The RW IP SW main module.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // RW SW configuration

#include <string.h>          // for mem* functions
#include <stdio.h>
#include "arch.h"            // Platform architecture definition
#include "compiler.h"
#include "co_version.h"      // version information
#include "co_utils.h"

#include "rwip.h"            // RW definitions
#include "rwip_int.h"        // RW internal definitions

#if (NVDS_SUPPORT)
#include "nvds.h"         // NVDS definitions
#endif // NVDS_SUPPORT

#if (BT_EMB_PRESENT)
#include "rwbt.h"            // rwbt definitions
#endif //BT_EMB_PRESENT

#if (BLE_EMB_PRESENT)
#include "rwble.h"           // rwble definitions
#endif //BLE_EMB_PRESENT

#if (BLE_HOST_PRESENT)
#include "rwble_hl.h"        // BLE HL definitions
#include "gapc.h"
#include "gapm.h"
#include "gattc.h"
#include "l2cc.h"
#endif //BLE_HOST_PRESENT

#if (BLE_APP_PRESENT)
#include "app.h"             // Application definitions
#endif //BLE_APP_PRESENT

//#include "led.h"             // led definitions

#if (BT_EMB_PRESENT)
#include "ld.h"
#endif //BT_EMB_PRESENT

#if (DISPLAY_SUPPORT)
#include "display.h"         // display definitions
#include "co_utils.h"        // toolbox
#include "plf.h"             // platform definition
#if (BT_EMB_PRESENT)
#include "reg_btcore.h"
#endif // (BT_EMB_PRESENT)
#if (BLE_EMB_PRESENT)
#include "reg_blecore.h"
#endif // (BLE_EMB_PRESENT)
#if (BT_DUAL_MODE)
#include "reg_ipcore.h"
#endif // (BT_DUAL_MODE)
#endif //DISPLAY_SUPPORT
#include "reg_ipcore.h"

#if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
#include "sch_arb.h"            // Scheduling Arbiter
#include "sch_prog.h"           // Scheduling Programmer
#include "sch_plan.h"           // Scheduling Planner
#include "sch_slice.h"          // Scheduling Slicer
#include "sch_alarm.h"          // Scheduling Alarm
#endif //(BT_EMB_PRESENT || BLE_EMB_PRESENT)

#if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
#include "rf.h"              // RF definitions
#endif //BT_EMB_PRESENT || BLE_EMB_PRESENT

#if (H4TL_SUPPORT)
#include "h4tl.h"
#endif //H4TL_SUPPORT

#if (AHI_TL_SUPPORT)
#include "ahi.h"
#endif //AHI_TL_SUPPORT

#if (HCI_PRESENT)
#include "hci.h"             // HCI definition
#endif //HCI_PRESENT

#include "ke.h"              // kernel definition
#include "ke_event.h"        // kernel event
#include "ke_timer.h"        // definitions for timer
#include "ke_mem.h"          // kernel memory manager

#if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
#include "ecc_p256.h"        // ECC P256 library
#endif // (BT_EMB_PRESENT || BLE_EMB_PRESENT)

#include "aes.h"             // For AES functions

#if (BLE_EMB_PRESENT)
#include "reg_blecore.h"        // ble core registers
#endif //BLE_EMB_PRESENT

#if (BT_EMB_PRESENT)
#include "reg_btcore.h"         // bt core registers
#endif //BT_EMB_PRESENT

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#endif

#include "dbg.h"             // debug definition
#include "icu.h" 
#include "user_config.h"
#include "aes.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    /// Sleep Duration Value in periodic wake-up mode in Half slots
#define MAX_SLEEP_DURATION_PERIODIC_WAKEUP      (0x0640 * 3)  // 1.5s
    
    /// Sleep Duration Value in external wake-up mode
#define MAX_SLEEP_DURATION_EXTERNAL_WAKEUP      0x7D00*6  //10s*6
    
    
#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)



#if (DISPLAY_SUPPORT)
///Table of HW image names for display
static const char* plf_type[4] =
{
        "BK",
        "LE",
        "BT",
        "DM"
};
static const char* rf_type[6] =
{
         "NONE",
         "RPLE",
         "EXRC",
         "ICY1",
         "ICY2",
         "BTPT"
 };

/// FW type display line
#if (BT_EMB_PRESENT && BLE_EMB_PRESENT)
#define FW_TYPE_DISPLAY   "FW: BTDM split emb"
#elif (BT_EMB_PRESENT)
#define FW_TYPE_DISPLAY   "FW: BT split emb"
#elif (BLE_EMB_PRESENT && BLE_HOST_PRESENT)
#define FW_TYPE_DISPLAY   "FW: BLE full"
#elif (BLE_EMB_PRESENT)
#define FW_TYPE_DISPLAY   "FW: BLE split emb"
#else
#define FW_TYPE_DISPLAY   "FW: ROM"
#endif // BT_EMB_PRESENT / BLE_EMB_PRESENT / BLE_HOST_PRESENT
#endif //DISPLAY_SUPPORT

 // Heap header size is 12 bytes
 #define RWIP_HEAP_HEADER             (12 / sizeof(uint32_t))
 // ceil(len/sizeof(uint32_t)) + RWIP_HEAP_HEADER
 #define RWIP_CALC_HEAP_LEN(len)      ((((len) + (sizeof(uint32_t) - 1)) / sizeof(uint32_t)) + RWIP_HEAP_HEADER)
 // compute size of the heap block in bytes
 #define RWIP_CALC_HEAP_LEN_IN_BYTES(len)  (RWIP_CALC_HEAP_LEN(len) * sizeof(uint32_t))


/*
 * STRUCT DEFINITIONS
 ****************************************************************************************
 */

#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
#if (RW_WLAN_COEX || RW_MWS_COEX)
const uint8_t rwip_coex_cfg[RWIP_COEX_CFG_MAX]={
    #if (BT_EMB_PRESENT)
    [RWIP_COEX_MSSWITCH_IDX] = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_SNIFFATT_IDX] = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_PAGE_IDX]     = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTEN << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_PSCAN_IDX]    = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_INQ_IDX]      = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_INQRES_IDX]   = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_SCORSVD_IDX]  = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_BCAST_IDX]    = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_DIS << RWIP_SAMEN_POS)),
    [RWIP_COEX_CONNECT_IDX]  = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS) | (RWIP_SAM_EN << RWIP_SAMEN_POS)),
    #endif // #if (BT_EMB_PRESENT)
    #if (BLE_EMB_PRESENT)
    [RWIP_COEX_CON_IDX]     = (uint8_t)((RWIP_PTI_TXDIS << RWIP_TXBSY_POS)  | (RWIP_PTI_RXDIS << RWIP_RXBSY_POS)  | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_CON_DATA_IDX]= (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS)  | (RWIP_PTI_RXEN << RWIP_RXBSY_POS)  | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_ADV_IDX]     = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS)  | (RWIP_PTI_RXDIS << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_SCAN_IDX]    = (uint8_t)((RWIP_PTI_TXDIS << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS)  | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_INIT_IDX]    = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS)  | (RWIP_PTI_RXEN << RWIP_RXBSY_POS)  | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    #endif // #if (BLE_EMB_PRESENT)
};
#endif //(RW_WLAN_COEX || RW_MWS_COEX)
#endif //(BLE_EMB_PRESENT || BT_EMB_PRESENT)


/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

#if 1
//#define KE_HEAP                    __attribute__((section("ke_heap"),zero_init))
//Abel modified for gcc compile warning
#define KE_HEAP                    __attribute__((section("ke_heap")))
#else
#define KE_HEAP 
#endif
/// Heap definitions - use uint32 to ensure that memory blocks are 32bits aligned.
/// Memory allocated for environment variables
uint32_t  rwip_heap_env[RWIP_CALC_HEAP_LEN(RWIP_HEAP_ENV_SIZE)];
#if (BLE_HOST_PRESENT)
/// Memory allocated for Attribute database
#if (CFG_CON>1)

uint32_t rwip_heap_db[RWIP_CALC_HEAP_LEN(RWIP_HEAP_DB_SIZE)];
#else
uint32_t KE_HEAP rwip_heap_db[RWIP_CALC_HEAP_LEN(RWIP_HEAP_DB_SIZE)];
#endif
#endif // (BLE_HOST_PRESENT)
/// Memory allocated for kernel messages
uint32_t  rwip_heap_msg[RWIP_CALC_HEAP_LEN(RWIP_HEAP_MSG_SIZE)];
/// Non Retention memory block
#if (CFG_CON>1)
uint32_t rwip_heap_non_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_NON_RET_SIZE)];
#else
uint32_t KE_HEAP rwip_heap_non_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_NON_RET_SIZE)];
#endif
/// IP reset state variable
uint8_t rwip_rst_state;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (!NVDS_SUPPORT)
__STATIC uint8_t rwip_param_dummy_get(uint8_t param_id, uint8_t * lengthPtr, uint8_t *buf)
{
    //uart_printf("rwip_param_dummy_get\r\n");
    return (PARAM_FAIL);
}
__STATIC uint8_t rwip_param_dummy_set(uint8_t param_id, uint8_t length, uint8_t *buf)
{
    return (PARAM_FAIL);
}
__STATIC uint8_t rwip_param_dummy_del(uint8_t param_id)
{
    return (PARAM_FAIL);
}
#endif // (!NVDS_SUPPORT)


#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
/**
 ****************************************************************************************
 * @brief Converts a duration in lp cycles into a duration in half us.
 *
 * The function converts a duration in lp cycles into a duration is half us, according to the
 * low power clock frequency (32768Hz or 32000Hz).
 *
 * To do this the following formula are applied:
 *
 *   Tus = (x*30.517578125)*2 = (30*x + x/2 + x/64 + x/512)*2 = (61*x + (x*8 + x)/256) for a 32.768kHz clock or
 *   Tus = (x*31.25)*2        = (31*x + x/4) * 2              = (62*x + x/2)           for a 32kHz clock
 *
 * @param[in]     lpcycles    duration in lp cycles
 * @param[in|out] error_corr  Insert and retrieve error created by truncating the LP Cycle Time to a half us (32kHz: 1/2 half us | 32.768kHz: 1/256 half-us)
 *
 * @return duration in half us
 ****************************************************************************************
 */
__STATIC uint32_t rwip_lpcycles_2_hus(uint32_t lpcycles, uint32_t *error_corr)
{
    uint32_t res;

    // Sanity check: The number of lp cycles should not be too high to avoid overflow
    ASSERT_ERR(lpcycles < 2000000);

    #if (HZ32000)
    // Compute the sleep duration in us - case of a 32kHz clock and insert previous computed error
    *error_corr = lpcycles + *error_corr;
    // get the truncated value
    res = *error_corr >> 1;
    // retrieve new inserted error
    *error_corr = *error_corr - (res << 1);
    // finish computation
    res = 62 * lpcycles + res;
    #else //HZ32000
    // Compute the sleep duration in half us - case of a 32.768kHz clock and insert previous computed error
    *error_corr = (lpcycles << 3) + lpcycles + *error_corr;
    // get the truncated value
    res = *error_corr >> 8;
    // retrieve new inserted error
    *error_corr = *error_corr - (res << 8);
    // finish computation
    res = 61 * lpcycles + res;
    #endif //HZ32000

    return(res);
}

/**
 ****************************************************************************************
 * @brief Converts a duration in half slots into a number of low power clock cycles.
 * The function converts a duration in half slots into a number of low power clock cycles.
 * Sleep clock runs at either 32768Hz or 32000Hz, so this function divides the value in
 * slots by 10.24 or 10 depending on the case.
 * To do this the following formulae are applied:
 *
 *   N = x * 10.24 = (1024 * x)/100 for a 32.768kHz clock or
 *   N = x * 10                     for a 32kHz clock
 *
 * @param[in] hs_cnt    The value in half slot count
 *
 * @return The number of low power clock cycles corresponding to the slot count
 *
 ****************************************************************************************
 */
int32_t rwip_slot_2_lpcycles(int32_t hs_cnt)
{
    int32_t lpcycles;

    #if HZ32000
    // Sanity check: The number of slots should not be too high to avoid overflow
    ASSERT_ERR(hs_cnt < (0xFFFFFFFF / 10));

    // Compute the low power clock cycles - case of a 32kHz clock
    lpcycles = hs_cnt * 10;
    #else //HZ32000
    // Sanity check: The number of slots should not be too high to avoid overflow
    ASSERT_ERR(hs_cnt < (0xFFFFFFFF >> 10));

    // Compute the low power clock cycles - case of a 32.768kHz clock
    lpcycles = (hs_cnt << 10)/100;
    #endif //HZ32000

    // So reduce little bit sleep duration in order to allow fine time compensation
    // Note compensation will be in range of [1 , 2[ lp cycles if there is no external wake-up
    lpcycles--;

    return(lpcycles);
}



/**
 ****************************************************************************************
 * @brief Converts a duration in us into a duration in lp cycles.
 *
 * The function converts a duration in us into a duration is lp cycles, according to the
 * low power clock frequency (32768Hz or 32000Hz).
 *
 * @param[in] us    duration in us
 *
 * @return duration in lpcycles
 ****************************************************************************************
 */
__STATIC uint32_t rwip_us_2_lpcycles(uint32_t us)
{
    uint32_t lpcycles;

    #if (HZ32000)
    // Compute the low power clock cycles - case of a 32kHz clock
    lpcycles = ((us * 32) + (999)) / 1000;
    #else //HZ32000
    // Compute the low power clock cycles - case of a 32.768kHz clock
    lpcycles = ((us * 32768) + (999999)) / 1000000;
    #endif //HZ32000

    return(lpcycles);
}

/**
 ****************************************************************************************
 * @brief Handles the Half slot timer target
 ****************************************************************************************
 */
void rwip_timer_hs_handler(void)
{
    // disable the timer driver
    rwip_env.timer_hs_target = RWIP_INVALID_TARGET_TIME;
    ip_intcntl1_finetgtintmsk_setf(0);

    // call the default half slot call-back
    sch_alarm_timer_isr();
}

/**
 ****************************************************************************************
 * @brief Handles the 10 ms timer target
 ****************************************************************************************
 */
void rwip_timer_10ms_handler(void)
{
    // disable the timer driver
    rwip_env.timer_10ms_target = RWIP_INVALID_TARGET_TIME;

    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    ip_intcntl1_timestamptgt2intmsk_setf(0);
    #elif (BLE_HOST_PRESENT)
    // Stop timer
    timer_set_timeout(0, NULL);
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    // Mark that 10ms timer is over
    ke_event_set(KE_EVENT_KE_TIMER);
}

/**
 ****************************************************************************************
 * @brief Handles the Half slot timer target
 ****************************************************************************************
 */
void rwip_timer_hus_handler(void)
{
    // disable the timer driver
    rwip_env.timer_hus_target = RWIP_INVALID_TARGET_TIME;
    ip_intcntl1_timestamptgt1intmsk_setf(0);

    // call the default half slot call-back
    sch_arb_event_start_isr();
}

#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

/**
 ****************************************************************************************
 * @brief Handles crypto event (to provide results out of interrupt context
 ****************************************************************************************
 */
__STATIC void rwip_crypt_evt_handler(void)
{
    uint8_t aes_result[KEY_LEN];

    // Clear event
    ke_event_clear(KE_EVENT_AES_END);

    // Load AES result
    em_rd(aes_result, EM_ENC_OUT_OFFSET, KEY_LEN);
#if (BT_DUAL_MODE || BLE_STD_MODE)
    // inform AES result handler
    aes_result_handler(CO_ERROR_NO_ERROR, aes_result);
#endif //(BT_DUAL_MODE || BLE_STD_MODE)
}

/**
 ****************************************************************************************
 * @brief Handles crypto interrupt
 ****************************************************************************************
 */
void rwip_crypt_isr_handler(void)
{
    // Prevent going to deep sleep during encryption
    rwip_prevent_sleep_clear(RW_CRYPT_ONGOING);

    // Clear interrupt mask
    ip_intcntl1_cryptintmsk_setf(0);

    // mark that AES is done
    ke_event_set(KE_EVENT_AES_END);
}

void rwip_driver_init(bool reset)
{
    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    uint8_t length;
    uint8_t sleep_enable = 0;
    uint8_t ext_wakeup_enable;
    #if (BT_DUAL_MODE)
    uint8_t diag_cfg[PARAM_LEN_DIAG_DM_HW];
    #endif // (BT_DUAL_MODE)

    if(!reset)
    {
        // Register AES event
        ke_event_callback_set(KE_EVENT_AES_END, &rwip_crypt_evt_handler);
    }

    // initialize environment
    rwip_env.prevent_sleep     = 0;
    // clear target timer
    rwip_env.timer_10ms_target = RWIP_INVALID_TARGET_TIME;
    rwip_env.timer_hs_target   = RWIP_INVALID_TARGET_TIME;
    rwip_env.timer_hus_target  = RWIP_INVALID_TARGET_TIME;

    if(reset)
    {
        // Reset the IP core
        ip_rwdmcntl_master_soft_rst_setf(1);
        while(ip_rwdmcntl_master_soft_rst_getf());
    }

    // Enable default common interrupts
    ip_intcntl1_set(IP_FIFOINTMSK_BIT | IP_CRYPTINTMSK_BIT | IP_ERRORINTMSK_BIT | IP_SWINTMSK_BIT);

    #if (BT_DUAL_MODE)
    // Read diagport configuration from NVDS
    length = PARAM_LEN_DIAG_DM_HW;
    if(rwip_param.get(PARAM_ID_DIAG_DM_HW, &length, diag_cfg) == PARAM_OK)
    {
        ip_diagcntl_pack(1, diag_cfg[3], 1, diag_cfg[2], 1, diag_cfg[1], 1, diag_cfg[0]);
    }
    else
    {
        ip_diagcntl_set(0);
    }
    #endif // (BT_DUAL_MODE)

    // Activate deep sleep feature if enabled in NVDS and in reset mode
    length = PARAM_LEN_SLEEP_ENABLE;
    if(!reset || rwip_param.get(PARAM_ID_SLEEP_ENABLE, &length, &sleep_enable) != PARAM_OK)
    {
        sleep_enable = 0;
    }
    sleep_enable = 1;
    // check is sleep is enabled
    if(sleep_enable != 0)
    {
        uint16_t twext, twosc, twrm;

        // Set max sleep duration depending on wake-up mode
        if(rwip_param.get(PARAM_ID_EXT_WAKEUP_ENABLE, &length, &ext_wakeup_enable) != PARAM_OK)
        {
            ext_wakeup_enable = 0;
        }
        ext_wakeup_enable = 1;
        rwip_env.ext_wakeup_enable = (ext_wakeup_enable != 0) ? true : false;

        // Set max sleep duration depending on wake-up mode
        length = sizeof(rwip_env.sleep_algo_dur);
        if(rwip_param.get(PARAM_ID_SLEEP_ALGO_DUR, &length, (uint8_t*) &rwip_env.sleep_algo_dur) != PARAM_OK)
        {
            // set a default duration: 200 us ==> 400 half us
            rwip_env.sleep_algo_dur = 400;
        }

        // Initialize sleep parameters
        rwip_env.sleep_acc_error   = 0;

        // Get TWrm from NVDS
        length = sizeof(uint16_t);
        if (rwip_param.get(PARAM_ID_RM_WAKEUP_TIME, &length, (uint8_t*)&twrm) != PARAM_OK)
        {
            // Set default values : 625 us
            twrm = 1500;//SLEEP_RM_WAKEUP_DELAY;
        }

        // Get TWosc from NVDS
        length = sizeof(uint16_t);
        if (rwip_param.get(PARAM_ID_OSC_WAKEUP_TIME, &length, (uint8_t*)&twosc) != PARAM_OK)
        {
            // Set default values : 5 ms
            twosc = 2000;//1500;//SLEEP_OSC_NORMAL_WAKEUP_DELAY;////修改使用一些晶体跑不起来的问题
        }

        // Get TWext from NVDS
        length = sizeof(uint16_t);
        if (rwip_param.get(PARAM_ID_EXT_WAKEUP_TIME, &length, (uint8_t*)&twext) != PARAM_OK)
        {
            // Set default values : 5 ms
            twext = 1500;//SLEEP_OSC_EXT_WAKEUP_DELAY;
        }

        twrm  = rwip_us_2_lpcycles(twrm);
        twosc = rwip_us_2_lpcycles(twosc);
        twext = rwip_us_2_lpcycles(twext);

        // Program register
        ip_enbpreset_pack(twext, twosc, twext);

        // Configure wake up delay to the highest parameter
        twext = co_max(twext,twrm);
        twext = co_max(twext,twosc);

        // Store wake-up delay in lp cycles
        rwip_env.lp_cycle_wakeup_delay = twext;

        // Set the external wakeup parameter
        ip_deepslcntl_extwkupdsb_setf(rwip_env.ext_wakeup_enable ? false : true);
    }
    else
    {
        // ensure that we will never enter in deep sleep
        rwip_prevent_sleep_set(RW_PLF_DEEP_SLEEP_DISABLED);
    }

    #if BT_DUAL_MODE
    // Set BTDM arbiter
    //TODO: restore //ip_prioscharb_pack(/*blepriomode*/ 1, /*bredrpriomode*/ 1);
    #endif //BT_DUAL_MODE
    #elif (BLE_HOST_PRESENT)
    // initialize environment
    rwip_env.prevent_sleep     = 0;
    rwip_env.timer_10ms_target = RWIP_INVALID_TARGET_TIME;
    // enable timer
    timer_enable(true);
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    rwip_env.extreme_temp = 0;
}

void rwip_init(uint32_t error)
{
    // IP power up
    SET_RWBT_POWER_UP;
    // IP initialization
    rwip_rst_state = RWIP_INIT;
    #if (NVDS_SUPPORT)
    // Point to NVDS for parameters get/set
    rwip_param.get = nvds_get;
    rwip_param.set = nvds_put;
    rwip_param.del = nvds_del;
    #else // (NVDS_SUPPORT)
    // !! Need to point to some parameter configuration system
    ASSERT_WARN(0, 0, 0);
    rwip_param.get = rwip_param_dummy_get;
    rwip_param.set = rwip_param_dummy_set;
    rwip_param.del = rwip_param_dummy_del;
    #endif // (NVDS_SUPPORT)

    // Initialize kernel
    ke_init();
    // Initialize memory heap used by kernel.
    // Memory allocated for environment variables
    ke_mem_init(KE_MEM_ENV,           (uint8_t*)rwip_heap_env,     RWIP_CALC_HEAP_LEN_IN_BYTES(RWIP_HEAP_ENV_SIZE));
    #if (BLE_HOST_PRESENT)
    // Memory allocated for Attribute database
    ke_mem_init(KE_MEM_ATT_DB,        (uint8_t*)rwip_heap_db,      RWIP_CALC_HEAP_LEN_IN_BYTES(RWIP_HEAP_DB_SIZE));
    #endif // (BLE_HOST_PRESENT)
    // Memory allocated for kernel messages
    ke_mem_init(KE_MEM_KE_MSG,        (uint8_t*)rwip_heap_msg,     RWIP_CALC_HEAP_LEN_IN_BYTES(RWIP_HEAP_MSG_SIZE));
    // Non Retention memory block
    ke_mem_init(KE_MEM_NON_RETENTION, (uint8_t*)rwip_heap_non_ret, RWIP_CALC_HEAP_LEN_IN_BYTES(RWIP_HEAP_NON_RET_SIZE));

    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    #if (RW_DEBUG)
    // Initialize the debug process
    dbg_init(false);
    #endif //(RW_DEBUG)
    #endif //(BT_EMB_PRESENT || BLE_EMB_PRESENT)

    // Initialize RF
    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    rf_init(&rwip_rf);
    #endif //BT_EMB_PRESENT || BLE_EMB_PRESENT

    #if (DISPLAY_SUPPORT)
    // Initialize display module
    //display_init();

    // Add some configuration information to display
    //display_add_config();
    #endif //DISPLAY_SUPPORT

    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    // Initialize Diffie Hellman Elliptic Curve Algorithm
    ecc_init(false);
    #endif // (BT_EMB_PRESENT || BLE_EMB_PRESENT)

    // Initialize H4TL
    #if (H4TL_SUPPORT)
    #if (H4TL_NB_CHANNEL > 1)
    h4tl_init(1, rwip_eif_get(1));
    uart_printf("h4tl_init ok\r\n");
    #endif // (H4TL_NB_CHANNEL > 1)
    h4tl_init(0, rwip_eif_get(0));
    uart_printf("h4tl_init-1 ok\r\n");
    #endif //(H4TL_SUPPORT)

    #if (HCI_PRESENT)
    // Initialize the HCI
    hci_init(false);
    uart_printf("hci_init ok\r\n");
    #endif //HCI_PRESENT

    #if (AHI_TL_SUPPORT)
    // Initialize the Application Host Interface
    ahi_init();
    uart_printf("ahi_init ok\r\n");
    #endif //AHI_TL_SUPPORT

    #if (BLE_HOST_PRESENT)
    // Initialize BLE Host stack
    rwble_hl_init(false);
    uart_printf("rwble_hl_init ok\r\n");
    #endif //BLE_HOST_PRESENT

    #if (BT_EMB_PRESENT)
    // Initialize BT
    rwbt_init();
    uart_printf("rwbt_init ok\r\n");
    #endif //BT_EMB_PRESENT

    #if (BLE_EMB_PRESENT)
    // Initialize BLE
    rwble_init(false);
    uart_printf("rwble_init ok\r\n");
    #endif //BLE_EMB_PRESENT

    // Initialize AES
    #if (BT_DUAL_MODE || BLE_STD_MODE)
    aes_init(false);
    uart_printf("aes_init ok\r\n");
    #endif //(BT_DUAL_MODE || BLE_STD_MODE)

    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    // Initialize Scheduling blocks
    sch_arb_init(false);
    sch_prog_init(false);
    sch_plan_init(false);
    sch_alarm_init(false);
    sch_slice_init(false);
    #endif //(BT_EMB_PRESENT || BLE_EMB_PRESENT)

    // Initialize IP core driver
    rwip_driver_init(false);
    uart_printf("rwip_driver_init ok\r\n");

    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    #if (RW_WLAN_COEX)
    rwip_wlcoex_set(1);
    uart_printf("rwip_wlcoex_set ok\r\n");
    #endif //(RW_WLAN_COEX)
    #endif //(BT_EMB_PRESENT || BLE_EMB_PRESENT)

    #if (BT_EMB_PRESENT || (BLE_EMB_PRESENT && !BLE_HOST_PRESENT))
    // If FW initializes due to FW reset, send the message to Host
    if(error != RESET_NO_ERROR)
    {
        uart_printf("@@error(%x)\r\n",error);
        if(error == RESET_TO_ROM || error == RESET_AND_LOAD_FW)
        {
            // Send platform reset command complete if requested by user

        }
        else
        {
            // Allocate a message structure for hardware error event
            struct hci_hw_err_evt *evt = KE_MSG_ALLOC(HCI_EVENT, 0, HCI_HW_ERR_EVT_CODE, hci_hw_err_evt);

            // Fill the HW error code
            switch(error)
            {
                case RESET_MEM_ALLOC_FAIL: evt->hw_code = CO_ERROR_HW_MEM_ALLOC_FAIL; break;
                default: ASSERT_INFO(0, error, 0); break;
            }

            // Send the message
            hci_send_2_host(evt);
        }
    }
    #endif //(BT_EMB_PRESENT || (BLE_EMB_PRESENT && !BLE_HOST_PRESENT))

    /*
     ************************************************************************************
     * Application initialization
     ************************************************************************************
     */
    #if (BLE_APP_PRESENT)
    // Initialize APP
    if(get_sys_mode()==NORMAL_MODE)
    {   
        appm_init();
        uart_printf("appm_init ok\r\n");
    }
    #endif //BLE_APP_PRESENT
}

void rwip_reset(void)
{
    //uart_printf("rwip_reset\r\n");
    // Disable interrupts until reset procedure is completed
    GLOBAL_INT_DISABLE();

    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    #if (RW_DEBUG)
    // Reset dbg
    dbg_init(true);
    #endif //(RW_DEBUG)
    #endif //(BT_EMB_PRESENT || BLE_EMB_PRESENT)

    //Clear all message and timer pending
    ke_flush();

    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    // Reset Diffie Hellman Elliptic Curve Algorithm
    ecc_init(true);
    #endif // (BT_EMB_PRESENT || BLE_EMB_PRESENT)

    #if (HCI_PRESENT)
    // Reset the HCI
    hci_init(true);
    #endif //HCI_PRESENT

    #if (BLE_HOST_PRESENT)
    // Initialize BLE Host stack
    rwble_hl_init(true);
    #endif //BLE_HOST_PRESENT

    #if (BT_EMB_PRESENT)
    // Reset BT
    rwbt_reset();
    #endif //BT_EMB_PRESENT

    #if (BLE_EMB_PRESENT)
    // Reset BLE
    rwble_init(true);
    #endif //BLE_EMB_PRESENT
#if (BT_DUAL_MODE || BLE_STD_MODE)
    // Reset AES
    aes_init(true);
#endif //(BT_DUAL_MODE || BLE_STD_MODE) 
    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    // Reset Scheduling blocks
    sch_arb_init(true);
    sch_prog_init(true);
    sch_plan_init(true);
    sch_alarm_init(true);
    sch_slice_init(true);
    #endif //(BT_EMB_PRESENT || BLE_EMB_PRESENT)

    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    // Initialize IP core driver
    rwip_driver_init(true);
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    #if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
    #if (RW_WLAN_COEX)
    rwip_wlcoex_set(1);
    #endif //(RW_WLAN_COEX)

    // Reset the RF
    rwip_rf.reset();
    #endif //(BT_EMB_PRESENT || BLE_EMB_PRESENT)


    #if (DISPLAY_SUPPORT)
    // Restart display module
    display_resume();
    #endif //DISPLAY_SUPPORT

    // Restore interrupts once reset procedure is completed
    GLOBAL_INT_RESTORE();
}

void rwip_schedule(void)
{
    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    // If system is waking up, delay the handling up to the Bluetooth clock is available and corrected
    if ((rwip_env.prevent_sleep & RW_WAKE_UP_ONGOING) == 0)
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    {
        // schedule all pending events
        ke_event_schedule();
    }
}

#if (BT_EMB_PRESENT)
#if PCA_SUPPORT
bool rwip_pca_clock_dragging_only(void)
{
#if (BLE_EMB_PRESENT)
    return rwble_activity_ongoing_check();
#else
    return false;
#endif // BLE_EMB_PRESENT
}
#endif // PCA_SUPPORT
#endif // BT_EMB_PRESENT

#if (BT_EMB_PRESENT || BLE_EMB_PRESENT)
#if (RW_MWS_COEX)
void rwip_mwscoex_set(bool state)
{
#if (BT_EMB_PRESENT)
    if (state)
    {
        bt_coexifcntl0_mwswci_en_setf(0);
        bt_coexifcntl0_mwscoex_en_setf(1);
    }
    else
    {
        bt_coexifcntl0_mwswci_en_setf(0);
        bt_coexifcntl0_mwscoex_en_setf(0);
    }
#endif // BT_EMB_PRESENT
}
#endif // RW_MWS_COEX
#if (RW_WLAN_COEX)
void rwip_wlcoex_set(bool state)
{
#if (BLE_EMB_PRESENT)
    if (state)
    {
        ble_coexifcntl0_syncgen_en_setf(1);
        ble_coexifcntl0_wlancoex_en_setf(1);
    }
    else
    {
        ble_coexifcntl0_syncgen_en_setf(0);
        ble_coexifcntl0_wlancoex_en_setf(0);
    }
#endif // BLE_EMB_PRESENT
}
#endif // RW_WLAN_COEX
#endif // (BT_EMB_PRESENT || BLE_EMB_PRESENT)

#if RW_DEBUG
void rwip_assert(const char * file, int line, int param0, int param1, uint8_t type)
{
    #if (BT_EMB_PRESENT || (BLE_EMB_PRESENT && !BLE_HOST_PRESENT))
    struct hci_dbg_assert_evt *evt = KE_MSG_ALLOC_DYN(HCI_DBG_EVT, 0, 0, hci_dbg_assert_evt, sizeof(struct hci_dbg_assert_evt) + strlen(file));
    evt->subcode = HCI_DBG_ASSERT_EVT_SUBCODE;
    evt->type = type;
    evt->line = line;
    evt->param0 = param0;
    evt->param1 = param1;
    strcpy((char *) evt->file, file);
    hci_send_2_host(evt);
    #endif //(BT_EMB_PRESENT || (BLE_EMB_PRESENT && !BLE_HOST_PRESENT))
}
#endif //RW_DEBUG

void rwip_isr(void)
{
    DBG_SWDIAG(ISR, RWIP, 1);

    // Check interrupt status and call the appropriate handlers
    uint32_t irq_stat      = ip_intstat1_get();

    // General purpose timer interrupt - half slot accuracy
    if (irq_stat & IP_FINETGTINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, FINETGTINT, 1);
        // Clear the interrupt
        ip_intack1_finetgtintack_clearf(1);

        // handles half slot timer target
        rwip_timer_hs_handler();

        DBG_SWDIAG(IP_ISR, FINETGTINT, 0);
    }

    // General purpose timer interrupt - half us accuracy
    if (irq_stat & IP_TIMESTAMPTGT1INTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, TIMESTAMPINT, 1);
        // Clear the interrupt
        ip_intack1_timestamptgt1intack_clearf(1);

        // handles half slot timer target
        rwip_timer_hus_handler();

        DBG_SWDIAG(IP_ISR, TIMESTAMPINT, 0);
    }

    // Clock
    if (irq_stat & IP_CLKNINTSTAT_BIT) // clock interrupt
    {
        DBG_SWDIAG(IP_ISR, CLKNINT, 1);

        // Ack clock interrupt
        ip_intack1_clknintack_clearf(1);

        if(rwip_env.prevent_sleep & RW_WAKE_UP_ONGOING)
        {
            // Handle end of wake-up
            rwip_wakeup_end();
        }
        #if (BT_EMB_PRESENT)
        else // BT uses clock IRQ to program ACL frames
        {
            // Call Scheduling Programmer
            sch_prog_clk_isr();
        }
        #endif //BT_EMB_PRESENT

        DBG_SWDIAG(IP_ISR, CLKNINT, 0);
    }

    // FIFO
    if (irq_stat & IP_FIFOINTSTAT_BIT) // FIFO interrupt
    {
        DBG_SWDIAG(IP_ISR, FIFOINT, 1);

        // Call scheduling programmer
        sch_prog_fifo_isr();

        // Ack FIFO interrupt
        ip_intack1_fifointack_clearf(1);

        DBG_SWDIAG(IP_ISR, FIFOINT, 0);
    }

    if (irq_stat & IP_SLPINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, SLPINT, 1);

        // ack Sleep wakeup interrupt
        ip_intack1_slpintack_clearf(1);

        // Handle wake-up
        rwip_wakeup();
        rwip_prevent_sleep_clear(RW_BLE_ACTIVE_MODE);
        DBG_SWDIAG(IP_ISR, SLPINT, 0);
    }

    // General purpose timer interrupt
    if (irq_stat & IP_TIMESTAMPTGT2INTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, GROSSTGTINT, 1);

        // Clear the interrupt
        ip_intack1_timestamptgt2intack_clearf(1);

        // handles 10 ms timer target
        rwip_timer_10ms_handler();

        DBG_SWDIAG(IP_ISR, GROSSTGTINT, 0);
    }

    // Encryption interrupt
    if (irq_stat & IP_CRYPTINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, CRYPTINT, 1);

        ip_intack1_cryptintack_clearf(1);

        // call the crypto ISR handler
        rwip_crypt_isr_handler();

        DBG_SWDIAG(IP_ISR, CRYPTINT, 0);
    }

    // SW interrupt
    if (irq_stat & IP_SWINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, SWINT, 1);
        // Clear the interrupt
        ip_intack1_swintack_clearf(1);

        // call SW interrupt handler
        rwip_sw_int_handler();

        DBG_SWDIAG(IP_ISR, SWINT, 0);
    }

    DBG_SWDIAG(ISR, RWIP, 0);
}

#if OPTIMIZE_POWER_CONSUMPTION_TEMP
#define ADC_INTERVAL                (3200 * 15)

static uint32_t adc_temp_itv = ADC_INTERVAL;
static uint32_t pre_adc_time_hs = 0;

uint8_t adc_temp_expired(void)
{
    rwip_time_t curr_time;

    curr_time = rwip_time_get();

    if(curr_time.hs > pre_adc_time_hs)
    {
        if(curr_time.hs-pre_adc_time_hs > adc_temp_itv)
        {
            pre_adc_time_hs = curr_time.hs;
            return 1;
        }
    }
    else if(curr_time.hs < pre_adc_time_hs)
    {
        pre_adc_time_hs = curr_time.hs;
    }
    return 0;
}

#endif

#if CONFIG_RC32K_CALIBRATION

#define RC32K_CALI_INTERVAL_0               (3200 * 60 * 5)
#define RC32K_CALI_INTERVAL_1               (3200 * 60 * 1)

#define RC32K_CALI_STATE_COMPL            0
#define RC32K_CALI_STATE_PREPARE          1
#define RC32K_CALI_STATE_DOING            2
#define RC32K_CALI_STATE_END              3

#define RC32K_CALI_DEALY_TIME             100
#define RC32K_CALI_END_TIME               100

static uint32_t rc32k_cali_itv = RC32K_CALI_INTERVAL_0;
static uint32_t s_current_time_hs = 0;
static uint8_t s_rc32k_check_state = RC32K_CALI_STATE_COMPL;
uint8_t rc32k_cali_flag = 0;
uint8_t rwip_cali_rc32k_expired(void)
{
    rwip_time_t current_time;

    #if OPTIMIZE_POWER_CONSUMPTION_TEMP
    extern uint8_t temp_trigger;
    if (rwip_env.extreme_temp && app_env.connected) 
    {
        rc32k_cali_itv = RC32K_CALI_INTERVAL_1;
    } 
    else 
    {
        rc32k_cali_itv = RC32K_CALI_INTERVAL_0;
    }
    #endif

    current_time = rwip_time_get();

    if(current_time.hs > s_current_time_hs)
    {
        #if OPTIMIZE_POWER_CONSUMPTION_TEMP
        if(current_time.hs-s_current_time_hs > rc32k_cali_itv || temp_trigger)
        {
            if (temp_trigger) 
            {
                temp_trigger = 0;
            }
            s_current_time_hs = current_time.hs;
            return 1;
        }
        #else
        if(current_time.hs-s_current_time_hs > rc32k_cali_itv)
        {
            s_current_time_hs = current_time.hs;
            return 1;
        }
        #endif
    }
    else if(current_time.hs < s_current_time_hs)
    {
        s_current_time_hs = current_time.hs;
    }
    return 0;
}

void rwip_cali_rc32k_start(void)
{
    s_rc32k_check_state = RC32K_CALI_STATE_PREPARE;
//    uart_printf("RC32K-Cali.Start\n");
    CLK32K_AutoCali_Start();
    s_rc32k_check_state = RC32K_CALI_STATE_DOING;
//    gpio_set(0x36, 1);
}

uint8_t rwip_cali_rc32k_w4_compl(void)
{
    rwip_time_t current_time;
    if(s_rc32k_check_state == RC32K_CALI_STATE_DOING) 
    {
        current_time = rwip_time_get();
        if(current_time.hs-s_current_time_hs < RC32K_CALI_DEALY_TIME)//wait calib complete
        {
            return 0;
        }
        else
        {
//            uart_printf("RC32K-Cali.Compl\n");
            CLK32K_AutoCali_End();
            s_current_time_hs = current_time.hs;
            s_rc32k_check_state = RC32K_CALI_STATE_END;
            return 0;
        }   
    }
    else if(s_rc32k_check_state == RC32K_CALI_STATE_END)
    {
        current_time = rwip_time_get();
        if(current_time.hs-s_current_time_hs < RC32K_CALI_END_TIME)//wait calib complete
        {
            return 0;
        }
        else
        {
            s_current_time_hs = current_time.hs;
//            uart_printf("RC32K-Cali.End\n");
            s_current_time_hs = current_time.hs;
            s_rc32k_check_state = RC32K_CALI_STATE_COMPL;
//            gpio_set(0x36, 0);
            return 1;
        }   
    }
    else
    {
        s_rc32k_check_state = RC32K_CALI_STATE_COMPL; 
        return 1;       
    }
}

#endif

uint8_t rwip_sleep(void)
{
    uint8_t sleep_res = RWIP_ACTIVE;
    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    int32_t sleep_duration;
//    int32_t sleep_duration1;
    rwip_time_t current_time;
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    DBG_SWDIAG(SLEEP, FUNC, 1);

    DBG_SWDIAG(SLEEP, ALGO, 0);

    do
    {
        /************************************************************************
         **************            CHECK KERNEL EVENTS             **************
         ************************************************************************/
        // Check if some kernel processing is ongoing (during wakeup, kernel events are not processed)
        if (((rwip_env.prevent_sleep & RW_WAKE_UP_ONGOING) == 0) && !ke_sleep_check())
            break;
        //uart_printf("%x:%x\r\n",rwip_env.prevent_sleep,ke_sleep_check());
        // Processor sleep can be enabled
        sleep_res = RWIP_CPU_SLEEP;

        DBG_SWDIAG(SLEEP, ALGO, 1);
        #if CONFIG_RC32K_CALIBRATION
        if(rwip_env.prevent_sleep & RW_RC32K_CALI_ONGOING)
        {
            if(rwip_cali_rc32k_w4_compl())
            {
                sleep_res = RWIP_CPU_SLEEP;
                rwip_env.prevent_sleep &= ~RW_RC32K_CALI_ONGOING;
            }
            else
            {
                sleep_res = RWIP_ACTIVE;
                break;
            }            
        }
        #endif
        /************************************************************************
         **************              CHECK RW FLAGS                **************
         ************************************************************************/
        // First check if no pending procedure prevent from going to sleep
        if (rwip_env.prevent_sleep != 0)
            break;

        #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
        DBG_SWDIAG(SLEEP, ALGO, 2);

        /************************************************************************
         **************           Retrieve Current time            **************
         ************************************************************************/
        current_time = rwip_time_get();
        // remove the on-going slot for clock correction
        current_time.hs += 1;
        // Remove 1 more slot because next slot will be started at end of function
        if((HALF_SLOT_INV(current_time.hus)) < rwip_env.sleep_algo_dur)
        {
            current_time.hs += 1;
        }
        // Be sure that we don't exceed the clock wrapping time
        current_time.hs &= RWIP_MAX_CLOCK_TIME;
        /************************************************************************
         ******* COMPUTE SLEEP TIME ACCORDING TO 10 MS AND HALF SLOT TIMER ******
         ************************************************************************/

        // put sleep duration to maximum value
        sleep_duration = (rwip_env.ext_wakeup_enable) ? MAX_SLEEP_DURATION_EXTERNAL_WAKEUP : MAX_SLEEP_DURATION_PERIODIC_WAKEUP;

        // check if 10ms timer is active
        if(rwip_env.timer_10ms_target != RWIP_INVALID_TARGET_TIME)
        {
            int32_t duration = CLK_DIFF(current_time.hs, rwip_env.timer_10ms_target);
            // update sleep duration to minimum value
            sleep_duration = co_min_s(sleep_duration, duration);
        }

        // check if Half slot timer is active
        if(rwip_env.timer_hs_target != RWIP_INVALID_TARGET_TIME)
        {
            int32_t duration = CLK_DIFF(current_time.hs, rwip_env.timer_hs_target);
            // update sleep duration to minimum value
            sleep_duration = co_min_s(sleep_duration, duration);
        }

        // check if Half us timer is active
        if(rwip_env.timer_hus_target != RWIP_INVALID_TARGET_TIME)
        {
            int32_t duration = CLK_DIFF(current_time.hs, rwip_env.timer_hus_target);
            // update sleep duration to minimum value
            sleep_duration = co_min_s(sleep_duration, duration);
        }
        // A timer ISR is not yet handled or will be raised soon
        // note the sleep duration could be negative, that's why it's useful to check if a minimum requirement is ok
        // at least one half slot.

        if(sleep_duration <= RWIP_MINIMUM_SLEEP_TIME)
        {
            break;
        }
         
        DBG_SWDIAG(SLEEP, ALGO, 3);

        /************************************************************************
         **************           CHECK SLEEP TIME                 **************
         ************************************************************************/
        sleep_duration -= RWIP_MINIMUM_SLEEP_TIME;
        sleep_duration = rwip_slot_2_lpcycles(sleep_duration);

        // check if sleep duration is sufficient according to wake-up delay
        // HW issue, if sleep duration = max(twosc,twext) + 1 the HW never wakes up, so we have to ensure that at least
        // sleep duration > max(twosc,twext) + 1 (all in lp clk cycle)
        if(sleep_duration < rwip_env.lp_cycle_wakeup_delay * 2)
        {
            break;
        }
        DBG_SWDIAG(SLEEP, ALGO, 4);
        #if CONFIG_RC32K_CALIBRATION
        if(rwip_cali_rc32k_expired())
        {
            rwip_env.prevent_sleep |= RW_RC32K_CALI_ONGOING;
            sleep_res = RWIP_ACTIVE;
            /* xtal temperature compensation */
            //xtal_tempe_compensation();

            rwip_cali_rc32k_start();
            rc32k_cali_flag = 1;
            break;
        }
        #endif

        #if (H4TL_SUPPORT)
        /************************************************************************
         **************                 CHECK TL                   **************
         ************************************************************************/
        // Try to switch off TL
        if (!h4tl_stop())
        {
            sleep_res = RWIP_ACTIVE;
            break;
        }
        #endif //H4TL_SUPPORT
        DBG_SWDIAG(SLEEP, FUNC, 0);
        sleep_res = RWIP_DEEP_SLEEP;

        TRC_REQ_SLEEP();

        /************************************************************************
         **************          PROGRAM CORE DEEP SLEEP           **************
         ************************************************************************/
        #if (BT_EMB_PRESENT)
        // Put BT core into deep sleep
        rwbt_sleep_enter();
        #endif // (BT_EMB_PRESENT)

        #if (BLE_EMB_PRESENT)
        // Put BLE core into deep sleep
        rwble_sleep_enter();
        #endif // (BLE_EMB_PRESENT)

        // Program wake-up time
        ip_deepslwkup_set(sleep_duration);

        // Mask all interrupts except sleep IRQ
        ip_intcntl1_set(IP_SLPINTMSK_BIT);

        // Clear possible pending IRQs
        ip_intack1_clear(0xFFFFFFFF);
      //  uart_printf("sleep:%d:%d\r\n",sleep_duration1,current_time.hs);


        if(!rwip_env.ext_wakeup_enable)
        {
            ip_deepslcntl_extwkupdsb_setf(1);
        }


        DBG_SWDIAG(SLEEP, SLEEP, 1);

        /************************************************************************
         **************               SWITCH OFF RF                **************
         ************************************************************************/
        rwip_rf.sleep();
        
        rwip_prevent_sleep_set(RW_BLE_ACTIVE_MODE) ;
        #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    } while(0);

    if(sleep_res != RWIP_DEEP_SLEEP)
    {
        DBG_SWDIAG(SLEEP, FUNC, 0);
    }
    return sleep_res;
}

/**
 ****************************************************************************************
 * @brief Wake-up from Core sleep.
 *
 * Compute and apply the clock correction according to duration of the deep sleep.
 ****************************************************************************************
 */
void rwip_wakeup(void)
{
    uint16_t fintetime_correction;
    // duration in half us
    uint32_t dur_hus;
    // duration in half slot
    uint32_t dur_hslot;
    // Get the number of low power sleep period
    uint32_t slp_period = ip_deepslstat_get();

    DBG_SWDIAG(SLEEP, SLEEP, 0);

    // Prevent going to deep sleep until a slot interrupt is received
    rwip_prevent_sleep_set(RW_WAKE_UP_ONGOING);

    // Re-enable external wake-up by default
    ip_deepslcntl_extwkupdsb_setf(0);

    // Compensate the base time counter and fine time counter by the number of slept periods
    dur_hus = rwip_lpcycles_2_hus(slp_period, &(rwip_env.sleep_acc_error));
    // Compute the sleep duration (based on number of low power clock cycles)
    dur_hslot = dur_hus / HALF_SLOT_SIZE;

    // retrieve halfslot sleep duration
    fintetime_correction = (HALF_SLOT_SIZE-1) - (dur_hus - dur_hslot*HALF_SLOT_SIZE);

    // The correction values are then deduced from the sleep duration in us
    ip_clkncntcorr_pack(/*absdelta*/ 1, /*clkncntcorr*/ dur_hus / HALF_SLOT_SIZE);

    // The correction values are then deduced from the sleep duration in us
    ip_finecntcorr_setf(fintetime_correction);

    // Start the correction
    ip_deepslcntl_deep_sleep_corr_en_setf(1);

    // Enable the RWBT slot interrupt
    ip_intcntl1_clknintsrmsk_setf(0);
    ip_intcntl1_set(IP_CLKNINTMSK_BIT);
    ip_intack1_clear(0xFFFFFFFF);

    #if (H4TL_SUPPORT)
    // Restart the flow on the TL
    h4tl_start();
    #endif //H4TL_SUPPORT

    TRC_REQ_WAKEUP();
}

/**
 ****************************************************************************************
 * @brief Restore the core processing after the clock correction
 *
 * Enable the core and check if some timer target has been reached.
 ****************************************************************************************
 */
void rwip_wakeup_end(void)
{
    // get current time
    rwip_time_t current_time = rwip_time_get();

    // Clear slot interrupt, not needed anymore
    ip_intcntl1_clknintmsk_setf(0);

    #if (BT_EMB_PRESENT)
    // Wake-up BT core
    rwbt_sleep_wakeup_end();
    #endif // (BT_EMB_PRESENT)

    #if (BLE_EMB_PRESENT)
    // Wake-up BLE core
    
    #if    (ROM_REGISTER_CALLBACK)
    if(rom_env.rwble_sleep_wakeup_end != NULL)
    {
        rom_env.rwble_sleep_wakeup_end();
    }
    #else
    rwble_sleep_wakeup_end();
    #endif //(ROM_REGISTER_CALLBACK)
    #endif // (BLE_EMB_PRESENT)
  //  uart_printf("wakeup_end:%d:%d:%d\r\n",current_time.hs,rwip_env.timer_hs_target,rwip_env.timer_hus_target);
    // Re-enable default common interrupts
    ip_intcntl1_set(IP_FIFOINTMSK_BIT | IP_CRYPTINTMSK_BIT | IP_ERRORINTMSK_BIT | IP_SWINTMSK_BIT);

    if(rwip_env.timer_hs_target != RWIP_INVALID_TARGET_TIME)
    {
        // check if half slot timer target is reach
        if(1)//((current_time.hs == rwip_env.timer_hs_target))
        {
            rwip_timer_hs_handler();
        }
        // enable half slot timer interrupt
        else
        {
            ip_intcntl1_finetgtintmsk_setf(1);
        }
    }

    if(rwip_env.timer_hus_target != RWIP_INVALID_TARGET_TIME)
    {
        // check if half us timer target is reach
        if(1)//((current_time.hs == rwip_env.timer_hus_target))
        {
            rwip_timer_hus_handler();
        }
        // enable half us timer interrupt
        else
        {
            ip_intcntl1_timestamptgt1intmsk_setf(1);
        }
    }

    if(rwip_env.timer_10ms_target != RWIP_INVALID_TARGET_TIME)
    {
        // check if 10ms target is reach
        if((current_time.hs >= rwip_env.timer_10ms_target))
        {
            rwip_timer_10ms_handler();
        }
        // enable 10ms timer interrupt
        else
        {
            ip_intcntl1_timestamptgt2intmsk_setf(1);
        }
    }

    // Wake up is complete now, so we allow the deep sleep again
    rwip_prevent_sleep_clear(RW_WAKE_UP_ONGOING);

//    led_reset(6);
}
///@} RW

void event_null_handle(uint32_t ev)
{
    (void)ev;
}

#ifdef USE_FREERTOS
static EventGroupHandle_t rw_event_group;

extern uint32_t isIRQorFIQContext(void);

static void rwble_setEvent(uint32_t ev)
{
    uint32_t ctx = isIRQorFIQContext();
    if(ctx)
    {
        BaseType_t xResult;
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(rw_event_group,ev,&higherPriorityTaskWoken);
        if( xResult == pdPASS )
        {
            portEXIT_SWITCHING_ISR(higherPriorityTaskWoken);
        }
    }
    else
    {
        xEventGroupSetBits(rw_event_group,ev);
    }
}

static void rwble_clearEvent(uint32_t ev)
{
    if(isIRQorFIQContext())
    {
        xEventGroupClearBitsFromISR(rw_event_group,ev);
    }
    else
    {
        xEventGroupClearBits(rw_event_group,ev);
    }
}

static void rwble_task( void * arg )
{    
    rw_event_group = xEventGroupCreate();
    
    vPortEnterCritical();
    rom_env.setEvent = rwble_setEvent;
    rom_env.clearEvent = rwble_clearEvent;
    xEventGroupSetBits(rw_event_group,ke_event_get_all());
    vPortExitCritical();
    
    while(1)
    {
        xEventGroupWaitBits(rw_event_group,0xFFFFFF,pdFALSE,pdFALSE,portMAX_DELAY);
        rwip_schedule();
    }
}

void create_rwble_task(uint32_t stack_depth)
{
    xTaskCreate(rwble_task, "rwble_task", stack_depth, NULL, RTOS_BK_PRIORITY, NULL);
}

#endif
