/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "user_config.h"  

#include "rwip_config.h"             // SW configuration

#if (BLE_APP_PRESENT)
#include "rwapp_config.h"
#include <string.h>

#include "rwip.h"
#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "gap.h"                     // GAP Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API

#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition

#if (BLE_APP_SEC)
#include "app_sec.h"                 // Application security Definition
#endif // (BLE_APP_SEC)

#if (BLE_APP_FEE0S)
#include "app_fee0.h"                  // USER define FEE0S Application Definitions
#endif //(BLE_APP_FEE0S)

#if (BLE_APP_FCC0S)
#include "app_fcc0.h"                  // USER define FCC0S Application Definitions
#endif //(BLE_APP_FCC0S)

#if(BLE_APP_ANCS)
#include "app_ancsc.h"               // Application ancs Definition
#endif
#if(BLE_APP_OADS)
#include "app_oads.h"                 // Application oads Definition
#endif
#if(BLE_APP_FTMPS)
#include "app_ftmp.h"
#endif
#if (BLE_APP_HT)
#include "app_ht.h"                  // Health Thermometer Application Definitions
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h"                 // Device Information Service Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"                // Battery Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
#include "app_hid.h"                 // HID Application Definitions
#endif //(BLE_APP_HID)

#if (DISPLAY_SUPPORT)
#include "app_display.h"             // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#if (BLE_APP_AM0)
#include "app_am0.h"                 // Audio Mode 0 Application
#endif //(BLE_APP_AM0)

#if (NVDS_SUPPORT)
#include "nvds.h"                    // NVDS Definitions
#endif //(NVDS_SUPPORT)
#include "flash.h"  
#include "ke_timer.h"
#include "ke_mem.h"
/*
 * DEFINES
 ****************************************************************************************
 */

#if (BLE_APP_HID)
// HID Mouse
#define DEVICE_NAME        "Hid Mouse"
#else
#define DEVICE_NAME        "BK DEVICE"
#endif

#define DEVICE_NAME_SIZE    sizeof(DEVICE_NAME)

/**
 * UUID List part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x03 - Complete list of 16-bit UUIDs available
 * x09\x18 - Health Thermometer Service UUID
 *   or
 * x12\x18 - HID Service UUID
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_UUID        "\x03\x03\x09\x18"
#define APP_HT_ADV_DATA_UUID_LEN    (4)
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_UUID       "\x03\x03\x12\x18"
#define APP_HID_ADV_DATA_UUID_LEN   (4)
#endif //(BLE_APP_HID)

/**
 * Appearance part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x19 - Appearance
 * x03\x00 - Generic Thermometer
 *   or
 * xC2\x04 - HID Mouse
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_APPEARANCE    "\x03\x19\x00\x03"
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_APPEARANCE   "\x03\x19\xC2\x03"
#endif //(BLE_APP_HID)

#define APP_ADV_DATA_APPEARANCE_LEN  (4)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database
enum appm_svc_list
{
    #if (BLE_APP_FEE0S)
    APPM_SVC_FEE0S,
    #endif //(BLE_APP_FEE0S)
    
    #if (BLE_APP_FCC0S)
    APPM_SVC_FCC0S,
    #endif //(BLE_APP_FCC0S)
    
    #if (BLE_APP_HT)
    APPM_SVC_HTS,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    APPM_SVC_DIS,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    APPM_SVC_BATT,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    APPM_SVC_HIDS,
    #endif //(BLE_APP_HID)
    #if (BLE_APP_AM0)
    APPM_SVC_AM0_HAS,
    #endif //(BLE_APP_AM0)

    #if(BLE_APP_ANCS)
    APPM_SVC_ANCSC,
    #endif
    #if(BLE_APP_OADS)
    APPM_SVC_OADS,
    #endif
    #if(BLE_APP_FTMPS)
    APPM_SVC_FTMPS,
    #endif
    APPM_SVC_LIST_STOP,
};

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// Application Task Descriptor
extern const struct ke_task_desc TASK_DESC_APP;

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] =
{
    #if (BLE_APP_FEE0S)
    (appm_add_svc_func_t)app_fee0_add_fee0s,
    #endif //(BLE_APP_FEE0S)
    
    #if (BLE_APP_FCC0S)
    (appm_add_svc_func_t)app_fcc0_add_fcc0s,
    #endif //(BLE_APP_FCC0S)
    
    #if (BLE_APP_HT)
    (appm_add_svc_func_t)app_ht_add_hts,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    (appm_add_svc_func_t)app_dis_add_dis,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    (appm_add_svc_func_t)app_batt_add_bas,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    (appm_add_svc_func_t)app_hid_add_hids,
    #endif //(BLE_APP_HID)
    #if (BLE_APP_AM0)
    (appm_add_svc_func_t)app_am0_add_has,
    #endif //(BLE_APP_AM0)
    #if (BLE_APP_ANCS)
    (appm_add_svc_func_t)app_ancs_add_ancsc,
    #endif
    #if (BLE_APP_OADS)
    (appm_add_svc_func_t)app_oad_add_oads,
    #endif
    #if(BLE_APP_FTMPS)
    (appm_add_svc_func_t)app_ftmp_add_ftmps,
    #endif
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (!BLE_APP_AM0)
static void appm_build_adv_data(uint16_t max_length, uint16_t *p_length, uint8_t *p_buf)
{
    // Remaining Length
    uint8_t rem_len = max_length;

    //adv type flag
    p_buf[0]=0x02;
    p_buf[1]=0x01;
    p_buf[2]=0x06;
    p_buf += 3;
    *p_length += 3;
    
    #if (BLE_APP_HT)
    // Set list of UUIDs
    memcpy(p_buf, APP_HT_ADV_DATA_UUID, APP_HT_ADV_DATA_UUID_LEN);
    *p_length += APP_HT_ADV_DATA_UUID_LEN;
    p_buf += APP_HT_ADV_DATA_UUID_LEN;

    // Set appearance
    memcpy(p_buf, APP_HT_ADV_DATA_APPEARANCE, APP_ADV_DATA_APPEARANCE_LEN);
    *p_length += APP_ADV_DATA_APPEARANCE_LEN;
    p_buf += APP_ADV_DATA_APPEARANCE_LEN;
    #endif //(BLE_APP_HT)

    #if (BLE_APP_HID)
    // Set list of UUIDs
    memcpy(p_buf, APP_HID_ADV_DATA_UUID, APP_HID_ADV_DATA_UUID_LEN);
    *p_length += APP_HID_ADV_DATA_UUID_LEN;
    p_buf += APP_HID_ADV_DATA_UUID_LEN;

    // Set appearance
    memcpy(p_buf, APP_HID_ADV_DATA_APPEARANCE, APP_ADV_DATA_APPEARANCE_LEN);
    *p_length += APP_ADV_DATA_APPEARANCE_LEN;
    p_buf += APP_ADV_DATA_APPEARANCE_LEN;
    #endif //(BLE_APP_HID)

    // Sanity check
    ASSERT_ERR(rem_len >= max_length);

    // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
    rem_len -= *p_length;

    // Check if additional data can be added to the Advertising data - 2 bytes needed for type and length
    if (rem_len > 2)
    {
        uint8_t dev_name_length = co_min(app_env.dev_name_len, (rem_len - 2));

        // Device name length
        *p_buf = dev_name_length + 1;
        // Device name flag (check if device name is complete or not)
        *(p_buf + 1) = (dev_name_length == app_env.dev_name_len) ? '\x09' : '\x08';
        // Copy device name
        memcpy(p_buf + 2, app_env.dev_name, dev_name_length);

        // Update advertising data length
        *p_length += (dev_name_length + 2);
        
        #if ENABLE_EXT_ADV//add ext adv data(128bytes)
        p_buf += (dev_name_length + 2); 
        *p_buf = APP_DFLT_ADV_0XFF_LEN + 1; 
        *(p_buf + 1) = 0xff;
        memcpy(p_buf + 2, APP_DFLT_ADV_0XFF, APP_DFLT_ADV_0XFF_LEN);
        // Update advertising data length
        *p_length += (APP_DFLT_ADV_0XFF_LEN + 2);
        #endif
    }
}
#endif //(!BLE_APP_AM0)

void appm_start_advertising(void)
{
    uart_printf("%s adv_state:%x\r\n",__func__,app_env.adv_state);
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_APP,
                                                         gapm_activity_start_cmd);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = app_env.adv_actv_idx;
    #if (BLE_APP_HID)
    p_cmd->u_param.adv_add_param.duration = APP_DFLT_ADV_DURATION;
    #else //(BLE_APP_HID)
    p_cmd->u_param.adv_add_param.duration = 0;
    #endif //(BLE_APP_HID)
    p_cmd->u_param.adv_add_param.max_adv_evt = 0;

    // Send the message
    ke_msg_send(p_cmd);

    // Keep the current operation
    app_env.adv_state = APP_ADV_STATE_STARTING;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_START_ACTIVITY;

}

void apptc_cmd_encode(uint8_t cmd, uint8_t *data, uint8_t len)
{
    uint8_t send[48];
    uint8_t i, sum = 0;

    send[0] = 0xA9;
    send[1] = cmd;
    send[2] = len;
    
    for(i=0; i<len; i++)
    {
        send[3+i] = data[i];
    }
    
    for(i=0; i<len +3; i++)
    {
        sum ^= send[i];
    }

    send[3 +len] = sum;

#if 0
    UART_PRINTF("tx data: ");
    for(i=0; i<len+4; i++)
    {
        UART_PRINTF("%02x ", send[i]);
    }
    UART_PRINTF("\r\n");
#endif

    uart_send(send, len+4);
}
void appm_stop_advertising(void)
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                      TASK_GAPM, TASK_APP,
                                                      gapm_activity_stop_cmd);

    // Fill the allocated kernel message
    cmd->operation = GAPM_STOP_ACTIVITY;
    cmd->actv_idx = app_env.adv_actv_idx;

    // Send the message
    ke_msg_send(cmd);

    // Update advertising state
    app_env.adv_state = APP_ADV_STATE_STOPPING;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_STOP_ACTIVITY;
}


void appm_set_adv_data(void)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_ADV_DATA;
    p_cmd->actv_idx = app_env.adv_actv_idx;

    p_cmd->length = 0;
    // GAP will use 3 bytes for the AD Type
    #if ENABLE_EXT_ADV
    appm_build_adv_data(EXT_ADV_DATA_MAX_LEN, &p_cmd->length, &p_cmd->data[0]);
    #else
    appm_build_adv_data(ADV_DATA_LEN, &p_cmd->length, &p_cmd->data[0]);
    #endif
    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    app_env.adv_state = APP_ADV_STATE_SETTING_ADV_DATA;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_SET_ADV_DATA;
}

void appm_set_scan_rsp_data(void)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_SCAN_RSP_DATA;
    p_cmd->actv_idx = app_env.adv_actv_idx;

    #if (BLE_APP_AM0)
    app_am0_fill_scan_rsp_data(p_cmd);
    #else //(BLE_APP_AM0)
    p_cmd->length = APP_SCNRSP_DATA_LEN;
    memcpy(&p_cmd->data[0], APP_SCNRSP_DATA, APP_SCNRSP_DATA_LEN);
    #endif //(BLE_APP_AM0)

    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    app_env.adv_state = APP_ADV_STATE_SETTING_SCAN_RSP_DATA;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_SET_SCAN_RSP_DATA;
}


static void appm_send_gapm_reset_cmd(void)
{
    // Reset the stack
    struct gapm_reset_cmd *p_cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                                TASK_GAPM, TASK_APP,
                                                gapm_reset_cmd);

    p_cmd->operation = GAPM_RESET;

    ke_msg_send(p_cmd);
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void appm_init()
{
    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));
    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);
    // Initialize Task state
    ke_state_set(TASK_APP, APPM_INIT);

    flash_read_data(&app_env.dev_name[0],(flash_env.bdaddr_def_addr_abs+8),APP_DEVICE_NAME_MAX_LEN);

    for(uint8_t i=0;i<APP_DEVICE_NAME_MAX_LEN;i++)
    {
        if( (app_env.dev_name[i]!=0xff) && (app_env.dev_name[i]!=0x00) )
        {
            app_env.dev_name_len++;
        }
        else
        {
            break;
        }
    }       
    if( (app_env.dev_name[0]==0xff) && (app_env.dev_name[1]==0xff) )
    {
        // Get default Device Name (No name if not enough space)
        memcpy(app_env.dev_name, APP_DFLT_DEVICE_NAME, APP_DFLT_DEVICE_NAME_LEN);
        app_env.dev_name_len = APP_DFLT_DEVICE_NAME_LEN;

        // TODO update this value per profiles
    }
    /*------------------------------------------------------
     * INITIALIZE ALL MODULES
     *------------------------------------------------------*/
    // load device information:
    #if (DISPLAY_SUPPORT)
    app_display_init();
    #endif //(DISPLAY_SUPPORT)

    #if (BLE_APP_SEC)
    // Security Module
    app_sec_init();
    #endif // (BLE_APP_SEC)

    #if (BLE_APP_HT)
    // Health Thermometer Module
    app_ht_init();
    #endif //(BLE_APP_HT)

    #if (BLE_APP_DIS)
    // Device Information Module
    app_dis_init();
    #endif //(BLE_APP_DIS)

    #if (BLE_APP_HID)
    // HID Module
    app_hid_init();
    #endif //(BLE_APP_HID)

    #if (BLE_APP_BATT)
    // Battery Module
    app_batt_init();
    #endif //(BLE_APP_BATT)

    #if (BLE_APP_AM0)
    // Audio Mode 0 Module
    app_am0_init();
    #endif //(BLE_APP_AM0)
    
    #if (BLE_APP_ANCS)
    app_ancsc_init();
    #endif
    #if (BLE_APP_OADS)
    app_oads_init();        
    #endif

    #if (BLE_APP_FTMPS)
    app_ftmp_init();
    #endif
    // Reset the stack
    appm_send_gapm_reset_cmd();
}

bool appm_add_svc(void)
{
    // Indicate if more services need to be added in the database
    bool more_svc = false;

    // Check if another should be added in the database
    if (app_env.next_svc != APPM_SVC_LIST_STOP)
    {
        ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL, app_env.next_svc, 1);

        // Call the function used to add the required service
        appm_add_svc_func_list[app_env.next_svc]();

        // Select following service to add
        app_env.next_svc++;
        more_svc = true;
    }
    uart_printf("%s more_svc:%d\r\n",__func__,more_svc);
    return more_svc;
}

void appm_disconnect(void)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                   gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
}

void rw_ip_sleep_test(uint32_t time);
void rw_ip_deep_sleep_test(void);
void appm_create_advertising(void)
{
    uart_printf("%s adv_state:%x\r\n",__func__,app_env.adv_state);
    if (app_env.adv_state == APP_ADV_STATE_IDLE)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_adv_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;


        // Fill the allocated kernel message
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
        p_cmd->adv_param.type = GAPM_ADV_TYPE_LEGACY;
        p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
        p_cmd->adv_param.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
        p_cmd->adv_param.prim_cfg.chnl_map = APP_ADV_CHMAP;
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;

        #if (BLE_APP_HID)
        p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_LIM_DISC;

        /*
         * If the peripheral is already bonded with a central device, use the direct advertising
         * procedure (BD Address of the peer device is stored in NVDS.
         */
        if (app_sec_get_bond_status())
        {
//            // BD Address of the peer device
//            struct gap_bdaddr peer_bd_addr;
//            // Length
//            uint8_t length = NVDS_LEN_PEER_BD_ADDRESS;
//
//            // Get the BD Address of the peer device in NVDS
//            if (nvds_get(NVDS_TAG_PEER_BD_ADDRESS, &length, (uint8_t *)&peer_bd_addr) != NVDS_OK)
//            {
//                // The address of the bonded peer device should be present in the database
//                ASSERT_ERR(0);
//            }
//
//            // Set the DIRECT ADVERTISING mode
//            cmd->op.code = GAPM_ADV_DIRECT;
//            // Copy the BD address of the peer device and the type of address
//            memcpy(&cmd->info.direct, &peer_bd_addr, NVDS_LEN_PEER_BD_ADDRESS);

            p_cmd->adv_param.prim_cfg.adv_intv_min = APP_ADV_FAST_INT;
            p_cmd->adv_param.prim_cfg.adv_intv_max = APP_ADV_FAST_INT;
        }
        else
        {
            p_cmd->adv_param.prim_cfg.adv_intv_min = APP_ADV_INT_MIN;
            p_cmd->adv_param.prim_cfg.adv_intv_max = APP_ADV_INT_MAX;
        }
        #else //(BLE_APP_HID)
        p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_GEN_DISC;
        p_cmd->adv_param.prim_cfg.adv_intv_min = APP_ADV_INT_MIN;
        p_cmd->adv_param.prim_cfg.adv_intv_max = APP_ADV_INT_MAX;
        #endif //(BLE_APP_HID)
      

        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.adv_state = APP_ADV_STATE_CREATING;
        // And the next expected operation code for the command completed event
        app_env.adv_op = GAPM_CREATE_ADV_ACTIVITY;
    }
}

void Delay_us(int num);
void appm_change_adv_intv(void)
{
        rom_env.stack_printf("%s 11\n", __func__);
        appm_stop_advertising();
        uart_printf("stop adv\n");
        Delay_us(100);
        rwip_schedule();
        Delay_us(100);
        rwip_schedule();
        Delay_us(100);
        rwip_schedule();
    
        struct gapm_activity_create_adv_cmd *p_cmd  = ke_malloc(sizeof(struct gapm_activity_create_adv_cmd), KE_MEM_ENV);
    
        // Set operation code
        p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;

        // Fill the allocated kernel message
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
        p_cmd->adv_param.type = GAPM_ADV_TYPE_LEGACY;
        p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
        p_cmd->adv_param.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
        p_cmd->adv_param.prim_cfg.chnl_map = APP_ADV_CHMAP;
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;

        
        p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_GEN_DISC;
        //p_cmd->adv_param.prim_cfg.adv_intv_min = APP_ADV_INT_MIN;
        //p_cmd->adv_param.prim_cfg.adv_intv_max = APP_ADV_INT_MAX;
        p_cmd->adv_param.prim_cfg.adv_intv_min = 1600; ///1s
        p_cmd->adv_param.prim_cfg.adv_intv_max = 1600; ///1s
        
        extern uint8_t gapm_adv_update(uint8_t actv_idx, struct gapm_activity_create_adv_cmd *p_param);
        gapm_adv_update(app_env.adv_actv_idx, p_cmd);
        uart_printf("change adv intv\n");
        appm_start_advertising();
        ke_free(p_cmd);
}

void appm_create_ext_advertising(void)
{
    uart_printf("%s adv_state:%x\r\n",__func__,app_env.adv_state);
    if (app_env.adv_state == APP_ADV_STATE_IDLE)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,TASK_GAPM, TASK_APP,gapm_activity_create_adv_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;

        // see spec version 5.1 | Vol 6,PartB Table 2.3:Adv phy channel PDU header's PDU Type field encoding
        // Fill the allocated kernel message
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
        p_cmd->adv_param.type = GAPM_ADV_TYPE_EXTENDED;
        p_cmd->adv_param.prop = GAPM_EXT_ADV_PROP_UNDIR_CONN_MASK;//GAPM_ADV_PROP_UNDIR_CONN_MASK;
        p_cmd->adv_param.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
        p_cmd->adv_param.prim_cfg.chnl_map = APP_ADV_CHMAP;
        #if ENABLE_PHY_2M_LE_CODE
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_125KBPS;
        p_cmd->adv_param.second_cfg.phy = GAP_PHY_2MBPS;//GAP_PHY_2MBPS;
        #else
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;
        p_cmd->adv_param.second_cfg.phy = GAP_PHY_LE_1MBPS;//GAP_PHY_2MBPS;
        #endif

        p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_GEN_DISC;
        p_cmd->adv_param.prim_cfg.adv_intv_min = APP_ADV_INT_MIN;
        p_cmd->adv_param.prim_cfg.adv_intv_max = APP_ADV_INT_MAX;

        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.adv_state = APP_ADV_STATE_CREATING;
        // And the next expected operation code for the command completed event
        app_env.adv_op = GAPM_CREATE_ADV_ACTIVITY;
    }
}


void appm_delete_advertising(void)
{
    // Prepare the GAPM_ACTIVITY_CREATE_CMD message
    struct gapm_activity_delete_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_delete_cmd);

    // Set operation code
    p_cmd->operation = GAPM_DELETE_ALL_ACTIVITIES;

    // Send the message
    ke_msg_send(p_cmd);

    // Keep the current operation
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_DELETE_ALL_ACTIVITIES;
}
/*
void appm_adv_fsm_next(void)
{
    uart_printf("%s adv_state:%x\r\n",__func__,app_env.adv_state);
    switch (app_env.adv_state)
    {
        case (APP_ADV_STATE_IDLE):
        {
            // Create advertising
            appm_create_advertising();
        } break;

        case (APP_ADV_STATE_CREATING):
        {
            // Set advertising data
            appm_set_adv_data();
        } break;

        case (APP_ADV_STATE_SETTING_ADV_DATA):
        {
            // Set scan response data
            appm_set_scan_rsp_data();
        } break;

        case (APP_ADV_STATE_CREATED):
        case (APP_ADV_STATE_SETTING_SCAN_RSP_DATA):
        {
            // Start advertising activity
            appm_start_advertising();
        } break;

        case (APP_ADV_STATE_STARTING):
        {
            #if (DISPLAY_SUPPORT)
            // Update advertising state screen
            app_display_set_adv(true);
            #endif //(DISPLAY_SUPPORT)

            // Go to started state
            app_env.adv_state = APP_ADV_STATE_STARTED;
        } break;

        case (APP_ADV_STATE_STARTED):
        {
            // Stop advertising activity
            appm_stop_advertising();
        } break;

        case (APP_ADV_STATE_STOPPING):
        {
            #if (DISPLAY_SUPPORT)
            // Update advertising state screen
            app_display_set_adv(false);
            #endif //(DISPLAY_SUPPORT)

            // Go created state
            app_env.adv_state = APP_ADV_STATE_CREATED;
        } break;



        default:
        {
            ASSERT_ERR(0);
        } break;
    }
}
*/
void appm_update_param(struct gapc_conn_param *conn_param)
{
    uart_printf("%s adv_state:%x\r\n",__func__,app_env.adv_state);
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = conn_param->intv_min;
    cmd->intv_max   = conn_param->intv_max;
    cmd->latency    = conn_param->latency;
    cmd->time_out   = conn_param->time_out;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    ke_msg_send(cmd);
}

void appm_update_phy_param(uint8_t tx_phy,uint8_t rx_phy,uint8_t phy_opt)      
{
    /// Supported LE PHY for data reception (@see enum gap_phy) 
    /// PHY options (@see enum gapc_phy_option)
    #if(ENABLE_PHY_2M_LE_CODE) 
    uart_printf("%s \r\n",__func__);
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_set_phy_cmd *cmd = KE_MSG_ALLOC(GAPC_SET_PHY_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                     gapc_set_phy_cmd);

    cmd->operation  = GAPC_SET_PHY;
    /// Supported LE PHY for data transmission (@see enum gap_phy)
    cmd->tx_phy = tx_phy;//GAP_PHY_LE_CODED;//GAP_PHY_LE_1MBPS;//GAP_PHY_LE_2MBPS;
    /// Supported LE PHY for data reception (@see enum gap_phy)
    cmd->rx_phy = rx_phy;//GAP_PHY_LE_CODED;//GAP_PHY_LE_1MBPS;//GAP_PHY_LE_2MBPS;
    /// PHY options (@see enum gapc_phy_option)
    cmd->phy_opt = phy_opt;//GAPC_PHY_OPT_LE_CODED_125K_RATE;//GAPC_PHY_OPT_LE_CODED_500K_RATE;//GAPC_PHY_OPT_LE_CODED_ALL_RATES;//conn_param->phy_opt;
   
    // Send the message
    ke_msg_send(cmd);
    #endif
}
uint8_t appm_get_dev_name(uint8_t* name)
{
    // copy name to provided pointer
    memcpy(name, app_env.dev_name, app_env.dev_name_len);
    // return name length
    return app_env.dev_name_len;
}

#if (BLE_CENTRAL || BLE_OBSERVER)

/// Scan interval 
#define APP_SCAN_INTV         (100)
/// Scan window 
#define APP_SCAN_WD         (20)


#define APPM_SCAN_TIME_10MS   400

uint16_t BLE_SCAN_INTVALUE = 88;
uint16_t BLE_SCAN_WINDOW = 88;

uint8_t gapm_scan_type = 0;///GAPM_SCAN_


static void appm_create_scaning(void)
{
    uart_printf("func %s,scan_state:%d\r\n",__func__,app_env.scan_state);
    if (app_env.scan_state == APP_SCAN_STATE_IDLE)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_SCAN_ACTIVITY;

        // Fill the allocated kernel message
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
         
        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.scan_state = APP_SCAN_STATE_CREATING;
        // And the next expected operation code for the command completed event
        app_env.scan_op = GAPM_CREATE_SCAN_ACTIVITY;
        
        app_env.scan_intv = APP_SCAN_INTV;
        app_env.scan_wd = APP_SCAN_WD;
    }
}

static void appm_start_scaning(void)
{
    uart_printf("func %s,scan_state:%d\r\n",__func__,app_env.scan_state);
    if (app_env.scan_state == APP_SCAN_STATE_CREATED)
    {
        // Prepare the GAPM_ACTIVITY_START_CMD message
        struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_activity_start_cmd);

        p_cmd->operation = GAPM_START_ACTIVITY;
        p_cmd->actv_idx = app_env.scan_actv_idx;
        
        p_cmd->u_param.scan_param.type = GAPM_SCAN_TYPE_GEN_DISC;//GAPM_SCAN_TYPE_OBSERVER;//;
        
        p_cmd->u_param.scan_param.prop = GAPM_SCAN_PROP_PHY_1M_BIT ;//| GAPM_SCAN_PROP_ACTIVE_1M_BIT;
        
        uint8_t scan_param_len = 2;
        if(rwip_param.get(NVDS_TAG_SCAN_INTV, &scan_param_len, (uint8_t*) &app_env.scan_intv) != PARAM_OK)
        {
            uart_printf("not found:NVDS_TAG_SCAN_WD err\r\n");
        }
        if(rwip_param.get(NVDS_TAG_SCAN_WD, &scan_param_len, (uint8_t*) &app_env.scan_wd) != PARAM_OK)
        {
            uart_printf("not found:NVDS_TAG_SCAN_WD err\r\n");
        }
        uart_printf("scan_intv:%d,scan_wd:%d\r\n",app_env.scan_intv,app_env.scan_wd);
        p_cmd->u_param.scan_param.scan_param_1m.scan_intv = app_env.scan_intv;
        p_cmd->u_param.scan_param.scan_param_1m.scan_wd = app_env.scan_wd;
        
        p_cmd->u_param.scan_param.scan_param_1m.scan_intv = 4;//app_env.scan_intv;
        p_cmd->u_param.scan_param.scan_param_1m.scan_wd = 4;//app_env.scan_wd;
        
        p_cmd->u_param.scan_param.dup_filt_pol = 0;
        
        p_cmd->u_param.scan_param.duration = 0;
        
        p_cmd->u_param.scan_param.period = 10;

        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.scan_state = APP_SCAN_STATE_STARTING;
        // And the next expected operation code for the command completed event
        app_env.scan_op = GAPM_START_ACTIVITY;
    }
}

static void appm_stop_scaning(void)
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                      TASK_GAPM, TASK_APP,
                                                      gapm_activity_stop_cmd);

    // Fill the allocated kernel message
    cmd->operation = GAPM_STOP_ACTIVITY;
    cmd->actv_idx = app_env.scan_actv_idx;

    // Send the message
    ke_msg_send(cmd);

    // Update advertising state
    app_env.scan_state = APP_SCAN_STATE_STOPPING;
    // And the next expected operation code for the command completed event
    app_env.scan_op = GAPM_STOP_ACTIVITY;
}

void appm_delete_scaning(void)
{
    // Prepare the GAPM_ACTIVITY_CREATE_CMD message
    struct gapm_activity_delete_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_delete_cmd);

    // Set operation code
    p_cmd->operation = GAPM_DELETE_ACTIVITY;
    
    p_cmd->actv_idx = app_env.scan_actv_idx;

    // Send the message
    ke_msg_send(p_cmd);

    // Keep the current operation
    // And the next expected operation code for the command completed event
    app_env.scan_op = GAPM_DELETE_ACTIVITY;
}

void appm_scan_fsm_next(void)
{
  //  return;
    uart_printf("%s scan_state:%x\r\n",__func__,app_env.scan_state);
    switch (app_env.scan_state)
    {
        case (APP_SCAN_STATE_IDLE)://0
        {
            // Create advertising
            appm_create_scaning();
        } break;

        case (APP_SCAN_STATE_CREATING):  //1      
        case (APP_SCAN_STATE_CREATED):   //2    
        {
            // Start scaning activity
            appm_start_scaning();
        } break;

        case (APP_SCAN_STATE_STARTING)://3
        {
            // Go to started state
            app_env.scan_state = APP_SCAN_STATE_STARTED;
        } break;

        case (APP_SCAN_STATE_STARTED)://4
        {
            // Stop scaning activity  
             appm_stop_scaning();
        } break;

        case (APP_SCAN_STATE_STOPPING)://5
        {
            
            // Go created state
            app_env.scan_state = APP_SCAN_STATE_CREATED;
        } break;

        default:
        {
            ASSERT_ERR(0);
        } break;
    }
    uart_printf("end scan_state:%x\r\n",app_env.scan_state);
}

void appm_update_scan_state(bool start)
{
    // TODO [LT] - Check current advertising state

    // Start or stop advertising
    appm_scan_fsm_next();
}

uint8_t appm_adv_data_decode_name(uint8_t len,const uint8_t *data,uint8_t *name_str)
{
    uint8_t find = 0;
    uint8_t index;
    for(index = 0; index < len;)
    {
        switch(data[index + 1])
        {
        case GAP_AD_TYPE_FLAGS:
        {
//            UART_PRINTF("AD_TYPE : ");
//            for(uint8_t len = 0; len < data[index] - 1; len++)
//            {
//                UART_PRINTF("%02x ",data[index + 2 + len]);
//            }
//            UART_PRINTF("\r\n");
            index +=(data[index] + 1);
        }
        break;
        case GAP_AD_TYPE_SHORTENED_NAME:
        case GAP_AD_TYPE_COMPLETE_NAME:
        {
//            if(strncmp((char*)&data[index + 2],find_str,str_len) == 0 )
//            {
//                find = 1;
//            }
//            UART_PRINTF("ADV_NAME : ");
            for(uint8_t len = 0; len < data[index] - 1; len++)
            {
               // UART_PRINTF("%c",data[index + 2 + len]);
                *name_str++ = data[index + 2 + len];
            }
//            UART_PRINTF("\r\n");
            index +=(data[index] + 1);
            
            find = 1;
        }
        break;
        case GAP_AD_TYPE_MORE_16_BIT_UUID:
        {
//            UART_PRINTF("UUID : ");
//            for(uint8_t len = 0; len < data[index] - 1;)
//            {
//                UART_PRINTF("%02x%02x  ",data[index + 2 + len],data[index + 3 + len]);
//                len+=2;
//            }
//            UART_PRINTF("\r\n");
            index +=(data[index] + 1);
        }
        break;
        default:
        {
            index +=(data[index] + 1);
        }
        break;
        }
    }
    return find;
}

uint8_t appm_adv_data_decode(uint8_t len,const uint8_t *data,uint8_t *find_str,uint8_t str_len)
{
    uint8_t find = 0;
    uint16_t index;
    for(index = 0; index < len;)
    {
        if(data[index] < 2 ||(len-index) < data[index] + 1) break;
        switch(data[index + 1])
        {
        case GAP_AD_TYPE_FLAGS:
        {
            uart_printf("AD_TYPE : ");
            
            uint8_t type_data = data[index + 2];

            uart_printf("%02x ",type_data);
            
            
            index +=(data[index] + 1);
        }
        break;
        case GAP_AD_TYPE_SHORTENED_NAME:
        case GAP_AD_TYPE_COMPLETE_NAME:
        {
          //  if(strncmp((char*)&data[index + 2],(const char *)find_str,str_len) == 0 )
          //  {
                find = 1;
          //  }
            uart_printf("ADV_NAME : ");
            for(uint8_t len = 0; len < data[index] - 1; len++)
            {
                uart_printf("%c",data[index + 2 + len]);
            }
            uart_printf("\r\n");
            index +=(data[index] + 1);
        }
        break;
        case GAP_AD_TYPE_MORE_16_BIT_UUID:
        {
            uart_printf("UUID : ");
            for(uint8_t len = 0; len < data[index] - 1;)
            {
                uart_printf("%02x%02x  ",data[index + 2 + len],data[index + 3 + len]);
                len+=2;
            }
            uart_printf("\r\n");
            index +=(data[index] + 1);
        }
        break;
        default:
        {
            index +=(data[index] + 1);
        }
        break;
        }
    }
    return find;
}


#endif // (#if (BLE_CENTRAL || BLE_OBSERVER))

#endif //(BLE_APP_PRESENT)

/// @} APP
