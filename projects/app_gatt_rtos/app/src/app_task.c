/**
 ****************************************************************************************
 *
 * @file appm_task.c
 *
 * @brief RW APP Task implementation
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"          // SW configuration


#if (BLE_APP_PRESENT)

#include "rwapp_config.h"
#include "app_task.h"             // Application Manager Task API
#include "app.h"                  // Application Manager Definition
#include "gapc_task.h"            // GAP Controller Task API
#include "gapm_task.h"            // GAP Manager Task API
#include "arch.h"                 // Platform Definitions
#include <string.h>
#include "co_utils.h"
#include "ke_timer.h"             // Kernel timer

#include "gpio.h"

#if (BLE_APP_FEE0S)
#include "app_fee0.h"              //  Module Definition
#endif //(BLE_APP_FEE0S)

#if (BLE_APP_FCC0S)
#include "app_fcc0.h"              //  Module Definition
#endif //(BLE_APP_FCC0S)

#if (BLE_APP_OADS)
#include "app_oads.h"             
#include "oads_task.h"   
#endif 

#if (BLE_APP_FTMPS)
#include "app_ftmp.h"
#include "ftmps_task.h"
#endif

#if (BLE_APP_ANCS)
#include "app_ancsc.h"
#include "ancsc_task.h"
#endif 

#if (BLE_APP_SEC)
#include "app_sec.h"              // Security Module Definition
#endif //(BLE_APP_SEC)

#if (BLE_APP_HT)
#include "app_ht.h"               // Health Thermometer Module Definition
#include "htpt_task.h"
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h"              // Device Information Module Definition
#include "diss_task.h"
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"             // Battery Module Definition
#include "bass_task.h"
#endif //(BLE_APP_BATT)

#if (BLE_APP_HID)
#include "app_hid.h"              // HID Module Definition
#include "hogpd_task.h"
#endif //(BLE_APP_HID)

#if (BLE_APP_AM0)
#include "app_am0.h"             // Audio Mode 0 Application
#endif //(BLE_APP_AM0)

#if (DISPLAY_SUPPORT)
#include "app_display.h"          // Application Display Definition
#endif //(DISPLAY_SUPPORT)
#include "user_config.h" 

static uint8_t phy_change_state=0;
/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static uint8_t app_get_handler(const struct app_subtask_handlers *handler_list_desc,
                               ke_msg_id_t msgid,
                               void *param,
                               ke_task_id_t src_id)
{
    // Counter
    uint8_t counter;

    // Get the message handler function by parsing the message table
    for (counter = handler_list_desc->msg_cnt; 0 < counter; counter--)
    {
        struct ke_msg_handler handler
                = /*(struct ke_msg_handler)*/(*(handler_list_desc->p_msg_handler_tab + counter - 1));

        if ((handler.id == msgid) ||
            (handler.id == KE_MSG_DEFAULT_HANDLER))
        {
            // If handler is NULL, message should not have been received in this state
            ASSERT_ERR(handler.func);

            return (uint8_t)(handler.func(msgid, param, TASK_APP, src_id));
        }
    }

    // If we are here no handler has been found, drop the message
    return (KE_MSG_CONSUMED);
}

#if MULTI_BOND
void appm_find_ltk(bd_addr_t addr)
{
      uint8_t length;
      struct gapc_irk irk;

      length = NVDS_LEN_PEER_IRK;
      for(uint8_t i=0;i<app_sec_env.bond_num;i++)
      {
          if (nvds_get(NVDS_TAG_PEER_IRK0+i, &length, (uint8_t *)&irk) == NVDS_OK)
          {
                if(!memcmp(addr.addr,&irk.addr.addr,GAP_BD_ADDR_LEN))
                {
                    uart_printf("get ltk ok\r\n",i);
                    nvds_get(NVDS_TAG_LTK0+i, &length,app_sec_env.ltk_buff) ;
                    app_sec_env.ltk_buff_statu = true;
                    break;    
                }
          }
      }  


}
void appm_gapm_resolv_dev_addr(bd_addr_t addr)
{
      uint8_t length;
      struct gapc_irk irk;
      
      uart_printf("%s,%d\r\n",__func__,app_sec_env.bond_num);
      struct gapm_resolv_addr_cmd* cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                    TASK_GAPM, TASK_APP, gapm_resolv_addr_cmd,sizeof(struct gap_sec_key)*app_sec_env.bond_num);
     
      cmd->operation = GAPM_RESOLV_ADDR;
      cmd->nb_key = 0;
     
      memcpy(cmd->addr.addr,addr.addr,sizeof(bd_addr_t));
      
      length = NVDS_LEN_PEER_IRK;
      for(uint8_t i=0;i<app_sec_env.bond_num;i++)
      {
          if (nvds_get(NVDS_TAG_PEER_IRK0+i, &length, (uint8_t *)&irk) == NVDS_OK)
          {
                uart_printf("get irk%d ok\r\n",i);
                memcpy(cmd->irk[cmd->nb_key++].key,irk.irk.key,sizeof(struct gap_sec_key));         
          }
      }  
     
      ke_msg_send(cmd);
}
static int gapm_addr_solved_ind_handler(ke_msg_id_t const msgid,
                                 struct gapm_addr_solved_ind const *p_ind,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{

    uint8_t length;
        
    uart_printf("%s\r\n",__func__);

    uart_printf("index:%d,addr ",p_ind->index);
    for(int i = 0;i < 6;i++)
    {
        uart_printf(":%02x ",p_ind->addr.addr[i]);
        
    }uart_printf("\r\n");

        length = NVDS_LEN_LTK;

     if (nvds_get(NVDS_TAG_LTK0+p_ind->index, &length,app_sec_env.ltk_buff) == NVDS_OK)  
     {
         uart_printf("get ltk ok\r\n");
         app_sec_env.ltk_buff_statu = true;
      }      

    return (KE_MSG_CONSUMED);  
}
#endif /* MULTI_BOND */
/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_CREATED_IND event
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_created_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_created_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    if (app_env.adv_state == APP_ADV_STATE_CREATING)
    {
        // Store the advertising activity index
        app_env.adv_actv_idx = p_param->actv_idx;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_STOPPED_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_stopped_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_stopped_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    if (app_env.adv_state == APP_ADV_STATE_STARTED)
    {
        // Act as if activity had been stopped by the application
        // Go created state
        app_env.adv_state = APP_ADV_STATE_CREATED;     
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAPM_PROFILE_ADDED_IND event
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_profile_added_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_profile_added_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{ 
    // Current State
    ke_state_t state = ke_state_get(dest_id);
    uart_printf("%s prf_task_id:%d,prf_task_nb:%d,start_hdl:%d\r\n",__func__,param->prf_task_id, param->prf_task_nb,param->start_hdl);
    if (state == APPM_CREATE_DB)
    {
        switch (param->prf_task_id)
        {
            #if (BLE_APP_AM0)
            case (TASK_ID_AM0_HAS):
            {
                app_am0_set_prf_task(param->prf_task_nb);
            } break;
            #endif //(BLE_APP_AM0)

            default: /* Nothing to do */ break;
        }
    }
    else
    {
        ASSERT_INFO(0, state, src_id);
    }

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    #if (NVDS_SUPPORT)
    uint8_t key_len = KEY_LEN;
    #endif //(NVDS_SUPPORT)
    uart_printf("%s operation:%x,status:%x\r\n",__func__,param->operation,param->status);
    switch(param->operation)
    {
        // Reset completed
        case (GAPM_RESET)://0
        {

            if(param->status == GAP_ERR_NO_ERROR)
            {
                #if (NVDS_SUPPORT)
                //nvds_tag_len_t len;
                #endif //(NVDS_SUPPORT)
                #if (BLE_APP_HID)
                app_hid_start_mouse();
                #endif //(BLE_APP_HID)

                // Set Device configuration
                struct gapm_set_dev_config_cmd* cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
                                                                   TASK_GAPM, TASK_APP,
                                                                   gapm_set_dev_config_cmd);
                // Set the operation
                cmd->operation = GAPM_SET_DEV_CONFIG;
                // Set the device role - Peripheral
                cmd->role      = GAP_ROLE_PERIPHERAL;
                
                #if (BLE_APP_SEC_CON)
                // The Max MTU is increased to support the Public Key exchange
                // HOWEVER, with secure connections enabled you cannot sniff the 
                // LEAP and LEAS protocols
                cmd->max_mtu = 160;
                cmd->pairing_mode = GAPM_PAIRING_SEC_CON | GAPM_PAIRING_LEGACY;
                #else // !(BLE_APP_SEC_CON)
                // Do not support secure connections
                cmd->pairing_mode = GAPM_PAIRING_LEGACY;
                #endif //(BLE_APP_SEC_CON)
                
                // Set Data length parameters
                cmd->sugg_max_tx_octets = LE_MAX_OCTETS;
                cmd->sugg_max_tx_time   = LE_MAX_TIME;
                
                cmd->max_mtu = 527;
                
                #if (BLE_APP_HID)
                // Enable Slave Preferred Connection Parameters present 
                cmd->att_cfg = GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN;
                #endif //(BLE_APP_HID)

                // Host privacy enabled by default
                cmd->privacy_cfg = 0;
                

                #if 0//(NVDS_SUPPORT)
                if (nvds_get(NVDS_TAG_BD_ADDRESS, &len, &cmd->addr.addr[0]) == NVDS_OK)
                {
                    // Check if address is a static random address
                    if ((cmd->addr.addr[5] & 0xc0) == 0xc0)
                    {
                        // Host privacy enabled by default
                        cmd->privacy_cfg |= GAPM_PRIV_CFG_PRIV_ADDR_BIT;
                    }
                }
                else
                {
                    memcpy(&cmd->addr.addr[0],&co_default_bdaddr.addr[0],BD_ADDR_LEN);
                    if ((cmd->addr.addr[5] & 0xc0) == 0xc0)
                    {
                        // Host privacy enabled by default
                        cmd->privacy_cfg |= GAPM_PRIV_CFG_PRIV_ADDR_BIT;
                    }
                }
                #endif //(NVDS_SUPPORT)

                #if (BLE_APP_AM0)
                cmd->audio_cfg   = GAPM_MASK_AUDIO_AM0_SUP;
                #endif //(BLE_APP_AM0)


                #if (NVDS_SUPPORT)
                if ((app_sec_get_bond_status()==true) &&
                    (nvds_get(NVDS_TAG_LOC_IRK, &key_len, app_env.loc_irk) == NVDS_OK))
                {
                    memcpy(cmd->irk.key, app_env.loc_irk, 16);
                }
                else
                #endif //(NVDS_SUPPORT)
                {
                    memset((void *)&cmd->irk.key[0], 0x00, KEY_LEN);
                }
                // Send message
                ke_msg_send(cmd);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;

        case (GAPM_PROFILE_TASK_ADD)://0x1b
        {
            // Add the next requested service
            if (!appm_add_svc())
            {
                // Go to the ready state
                ke_state_set(TASK_APP, APPM_READY);

                // No more service to add, start advertising
                #if (BLE_PERIPHERAL)               
                 // No more service to add, create advertising                
                if(app_env.adv_state == APP_ADV_STATE_IDLE)
                {
                    #if ENABLE_EXT_ADV
                    appm_create_ext_advertising();
                    #else
                    appm_create_advertising();
                    #endif
                }
                #endif
                
                ke_timer_set(APP_PERIOD_TIMER,TASK_APP,100);
            }
        }
        break;

        case (GAPM_GEN_RAND_NB) ://0x1a
        {
            if (app_env.rand_cnt == 1)
            {
                // Generate a second random number
                app_env.rand_cnt++;
                struct gapm_gen_rand_nb_cmd *cmd = KE_MSG_ALLOC(GAPM_GEN_RAND_NB_CMD,
                                                                TASK_GAPM, TASK_APP,
                                                                gapm_gen_rand_nb_cmd);
                cmd->operation = GAPM_GEN_RAND_NB;
                ke_msg_send(cmd);
            }
            else
            {
                struct gapm_set_irk_cmd *cmd = KE_MSG_ALLOC(GAPM_SET_IRK_CMD,
                                                        TASK_GAPM, TASK_APP,
                                                        gapm_set_irk_cmd);
                app_env.rand_cnt=0;
                ///  - GAPM_SET_IRK
                cmd->operation = GAPM_SET_IRK;
                memcpy(&cmd->irk.key[0], &app_env.loc_irk[0], KEY_LEN);
                ke_msg_send(cmd);
            }
        }
        break;

        case (GAPM_SET_IRK):
        {
            // ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            #if (BLE_APP_SEC)
            #if (NVDS_SUPPORT)
            //if (app_sec_get_bond_status()==false)
            if (nvds_get(NVDS_TAG_LOC_IRK, &key_len, app_env.loc_irk) != NVDS_OK)
            {               
                if (nvds_put(NVDS_TAG_LOC_IRK, KEY_LEN, (uint8_t *)&app_env.loc_irk) != NVDS_OK)
                {
                    ASSERT_INFO(0, 0, 0);
                }
            }
            #endif 
            #endif //(BLE_APP_SEC)
            app_env.rand_cnt = 0;


             // Go to the create db state
            ke_state_set(TASK_APP, APPM_CREATE_DB);

            // Add the first required service in the database
            // and wait for the PROFILE_ADDED_IND
            appm_add_svc();
            
        }
        break;

        // Device Configuration updated
        case (GAPM_SET_DEV_CONFIG):
        {
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            #if (BLE_APP_SEC)
            if (app_sec_get_bond_status()==true) 
            {
                #if (NVDS_SUPPORT)
                // If Bonded retrieve the local IRK from NVDS
                if (nvds_get(NVDS_TAG_LOC_IRK, &key_len, app_env.loc_irk) == NVDS_OK)
                {
                    // Set the IRK in the GAP
                    struct gapm_set_irk_cmd *cmd = KE_MSG_ALLOC(GAPM_SET_IRK_CMD,
                                                                TASK_GAPM, TASK_APP,
                                                                gapm_set_irk_cmd);
                    ///  - GAPM_SET_IRK: 
                    cmd->operation = GAPM_SET_IRK;
                    memcpy(&cmd->irk.key[0], &app_env.loc_irk[0], KEY_LEN);
                    ke_msg_send(cmd);
                }
                else
                #endif //(NVDS_SUPPORT)
               
                {
                     // If cannot read IRK from NVDS ASSERT
                     ASSERT_ERR(0);
                }
            }
            else // Need to start the generation of new IRK
            #endif //(BLE_APP_SEC)
            {
                struct gapm_gen_rand_nb_cmd *cmd = KE_MSG_ALLOC(GAPM_GEN_RAND_NB_CMD,
                                                                TASK_GAPM, TASK_APP,
                                                                gapm_gen_rand_nb_cmd);
                cmd->operation   = GAPM_GEN_RAND_NB;
                app_env.rand_cnt = 1;
                ke_msg_send(cmd);
            }
        }
        break;

        case (GAPM_CREATE_ADV_ACTIVITY):
        {
            appm_set_adv_data();  
        }break;
        case (GAPM_STOP_ACTIVITY):
        {   
            // Go created state
            app_env.adv_state = APP_ADV_STATE_CREATED;    
        }break;
        case (GAPM_START_ACTIVITY):
        {
            // Go to started state
            app_env.adv_state = APP_ADV_STATE_STARTED;
        }break;
        case (GAPM_DELETE_ACTIVITY):
        {
             app_env.adv_state = APP_ADV_STATE_IDLE;
        }break;
        case (GAPM_SET_ADV_DATA):
        {
            #if ENABLE_EXT_ADV
            appm_start_advertising();
            #else
            appm_set_scan_rsp_data();
            #endif
            
        }break;
        case (GAPM_SET_SCAN_RSP_DATA):
        {
            // Sanity checks
            ASSERT_INFO(app_env.adv_op == param->operation, app_env.adv_op, param->operation);
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->status, app_env.adv_op);

            // Perform next operation
           // Start advertising activity
            appm_start_advertising();
            
        } break;

        case (GAPM_DELETE_ALL_ACTIVITIES) :
        {
            // Re-Invoke Advertising
            app_env.adv_state = APP_ADV_STATE_IDLE;
        } break;


        default:
        {
            // Drop the message
        }
        break;
    }

    return (KE_MSG_CONSUMED);
}

static int gapc_get_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_get_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    switch(param->req)
    {
        case GAPC_DEV_NAME:
        {
            struct gapc_get_dev_info_cfm * cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                    src_id, dest_id,
                                                    gapc_get_dev_info_cfm, APP_DEVICE_NAME_MAX_LEN);
            cfm->req = param->req;
            cfm->info.name.length = appm_get_dev_name(cfm->info.name.value);

            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_APPEARANCE:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                             src_id, dest_id,
                                                             gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Set the device appearance
            #if (BLE_APP_HT)
            // Generic Thermometer - TODO: Use a flag
            cfm->info.appearance = 728;
            #elif (BLE_APP_HID)
            // HID Mouse
            cfm->info.appearance = 962;
            #else
            // No appearance
            cfm->info.appearance = 0;
            #endif

            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                    src_id, dest_id,
                                                            gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Slave preferred Connection interval Min
            cfm->info.slv_pref_params.con_intv_min = BLE_UAPDATA_MIN_INTVALUE;
            // Slave preferred Connection interval Max
            cfm->info.slv_pref_params.con_intv_max = BLE_UAPDATA_MAX_INTVALUE;
            // Slave preferred Connection latency
            cfm->info.slv_pref_params.slave_latency  = BLE_UAPDATA_LATENCY;
            // Slave preferred Link supervision timeout
            cfm->info.slv_pref_params.conn_timeout    = BLE_UAPDATA_TIMEOUT;  // 2s (500*10ms)

            // Send message
            ke_msg_send(cfm);
        } break;

        default: /* Do Nothing */ break;
    }


    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles GAPC_SET_DEV_INFO_REQ_IND message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_set_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_set_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Set Device configuration
    struct gapc_set_dev_info_cfm* cfm = KE_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM, src_id, dest_id,
                                                     gapc_set_dev_info_cfm);
    // Reject to change parameters
    cfm->status = GAP_ERR_REJECTED;
    cfm->req = param->req;
    // Send message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_connection_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_connection_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    app_env.conidx = KE_IDX_GET(src_id);
    uart_printf("%s \r\n",__func__);
    
    // Check if the received Connection Handle was valid
    if (app_env.conidx != GAP_INVALID_CONIDX)
    {
        // Retrieve the connection info from the parameters
        app_env.conhdl = param->conhdl;
        
        /// Connection handle
        uart_printf("conhdl:%d \r\n",param->conhdl);
        uart_printf("con_interval:%d \r\n",param->con_interval);
        uart_printf("con_latency:%d \r\n",param->con_latency );
        uart_printf("sup_to:%d \r\n",param->sup_to );
        uart_printf("peer_addr_type:%d \r\n",param->peer_addr_type);
        {
            uart_printf("peer_addr:0x");
            for(uint8_t i = 0;i < GAP_BD_ADDR_LEN;i++)
            {
                uart_printf("%02x",param->peer_addr.addr[i]);
            }uart_printf("\r\n");
        }
        uart_printf("role:%s \r\n",param->role ? "Slave" : "Master");
        
        // Send connection confirmation
        struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
                KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                gapc_connection_cfm);

        #if(BLE_APP_SEC)
        cfm->auth      = app_sec_get_bond_status() ? GAP_AUTH_REQ_NO_MITM_BOND : GAP_AUTH_REQ_NO_MITM_NO_BOND; // TODO [FBE] restore valid data
        #else // !(BLE_APP_SEC)
        cfm->auth      = GAP_AUTH_REQ_NO_MITM_NO_BOND;
        #endif // (BLE_APP_SEC)
        // Send the message
        ke_msg_send(cfm);

        #if DISPLAY_SUPPORT
        // Update displayed information
        app_display_set_adv(false);
        app_display_set_con(true);
        #endif //(DISPLAY_SUPPORT)

        /*--------------------------------------------------------------
         * ENABLE REQUIRED PROFILES
         *--------------------------------------------------------------*/

        #if (BLE_APP_BATT)
        // Enable Battery Service
        app_batt_enable_prf(app_env.conhdl);
        #endif //(BLE_APP_BATT)

        #if (BLE_APP_HID)
        // Enable HID Service
        app_hid_enable_prf(app_env.conhdl);
        #endif //(BLE_APP_HID)

        // We are now in connected State
        ke_state_set(dest_id, APPM_CONNECTED);
   #if MULTI_BOND
        app_sec_env.peer_pairing = false;
        app_sec_env.peer_encrypt = false;
        app_sec_env.ltk_buff_statu = false;
   
        if(app_sec_env.bonded)
        {
            
            if(param->peer_addr_type == 1)
            {
                appm_gapm_resolv_dev_addr(param->peer_addr);//resolv addr
            }else
            {
                uart_printf("%s\r\n",__func__);
                appm_find_ltk(param->peer_addr);   // static addr
            }
         }
         ke_timer_set(APP_SEND_SECURITY_REQ,TASK_APP,60);
   #else
   app_sec_env.bonded = false;
   app_sec_env.peer_pairing = false;
   app_sec_env.peer_encrypt = false;        
   #endif /* MULTI_BOND */
        
        phy_change_state=0;

        ke_timer_set(APP_GATTC_EXC_MTU_CMD,TASK_APP,20);
        
        #if (BLE_APP_ANCS)
        ke_timer_set(APP_ANCS_REQ_IND,TASK_APP,30); 
        #endif
        
        ke_timer_set(APP_PARAM_UPDATE_REQ_IND,TASK_APP,350);    

        app_env.connected = 1;
    }
    else
    {
        // No connection has been established, restart advertising
        appm_start_advertising();
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_update_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_param_update_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    app_env.conidx = KE_IDX_GET(src_id);
    uart_printf("gapc_param_update_req_ind_handler\n");

    // Check if the received Connection Handle was valid
    if (app_env.conidx != GAP_INVALID_CONIDX)
    {

        // Send connection confirmation
        struct gapc_param_update_cfm *cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM,
                KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                gapc_param_update_cfm);

        cfm->accept = true;
        cfm->ce_len_min = 2;
        cfm->ce_len_max = 4;

        // Send message
        ke_msg_send(cfm);

    }
    else
    {
        // No connection has been established, restart advertising
        appm_start_advertising();
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
   
    uart_printf("gapc_cmp_evt_handler operation = %x\r\n",param->operation);
    switch(param->operation)
    {
        case (GAPC_UPDATE_PARAMS):  //0x09
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                uart_printf("gapc update params fail !\r\n");
            }
            else
            {
                uart_printf("gapc update params ok !\r\n");
            }
            
        } break;
        case (GAPC_DISCONNECT): //0x01
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {
                uart_printf("pairing_fail = 0x%x\r\n",app_sec_env.pairing_fail);
                //bonding info lost and pairing fail
                if(app_sec_env.pairing_fail)
                {
                    app_sec_env.pairing_fail = false;
                    
                    uart_printf("restart advertising\r\n");
                    
                    //ke_state_set(TASK_APP, APPM_READY);
                    
                    //restart advertising
                    //appm_start_advertising();
                }
            }
        }break;
        case (GAPC_SECURITY_REQ): //0x0c
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                uart_printf("gapc security req fail !\r\n");
            }
            else
            {
                uart_printf("gapc security req ok !\r\n");
            }
        }break;
        case (GAPC_BOND): // 0xa
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                uart_printf("gapc bond fail !\r\n");
            }
            else
            {
                uart_printf("gapc bond ok !\r\n");
                //app_ancsc_enable_prf(app_env.conhdl); ///190917
            }
        }break;
        
        case (GAPC_ENCRYPT): // 0xb
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                uart_printf("gapc encrypt start fail !\r\n");
            }
            else
            {
                uart_printf("gapc encrypt start ok !\r\n");
                //app_ancsc_enable_prf(app_env.conhdl); ///190917
            }
        }
        break;
        

        default:
          break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uart_printf("gapc_disconnect_ind_handler,reason=%x\n",param->reason);
    // Go to the ready state
    ke_state_set(TASK_APP, APPM_READY);
    

    #if (BLE_APP_HT)
    // Stop interval timer
    app_stop_timer();
    #endif //(BLE_APP_HT)

    #if (DISPLAY_SUPPORT)
    // Update Connection State screen
    app_display_set_con(false);
    #endif //(DISPLAY_SUPPORT)

    #if (BLE_ISO_MODE_0_PROTOCOL)
    app_env.adv_state = APP_ADV_STATE_CREATING;
    #endif //(BLE_ISO_MODE_0_PROTOCOL)

    #if (!BLE_APP_HID)
    // Restart Advertising
    appm_start_advertising(); 
    #endif //(!BLE_APP_HID)

    app_env.connected = 0;

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of all messages sent from the lower layers to the application
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int appm_msg_handler(ke_msg_id_t const msgid,
                            void *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id)
{
    // Retrieve identifier of the task from received message
    ke_task_id_t src_task_id = MSG_T(msgid);
    // Message policy
    uint8_t msg_pol = KE_MSG_CONSUMED;

    switch (src_task_id)
    {
        case (TASK_ID_GAPC):
        {
            #if (BLE_APP_SEC)
            if ((msgid >= GAPC_BOND_CMD) &&
                (msgid <= GAPC_SECURITY_IND))
            {
                // Call the Security Module
                msg_pol = app_get_handler(&app_sec_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_SEC)
            // else drop the message
        } break;

        case (TASK_ID_GATTC):
        {
            // Service Changed - Drop
        } break;

        #if (BLE_APP_FEE0S)
        case (TASK_ID_FEE0S):
        {
            // Call the app fee0s Module
            msg_pol = app_get_handler(&app_fee0_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_FEE0S)
        
        #if (BLE_APP_FCC0S)
        case (TASK_ID_FCC0S):
        {
            // Call the app fee0s Module
            msg_pol = app_get_handler(&app_fcc0_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_FCC0S)
        
        #if (BLE_APP_HT)
        case (TASK_ID_HTPT):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HT)

        #if (BLE_APP_DIS)
        case (TASK_ID_DISS):
        {
            // Call the Device Information Module
            msg_pol = app_get_handler(&app_dis_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_DIS)

        #if (BLE_APP_HID)
        case (TASK_ID_HOGPD):
        {
            // Call the HID Module
            msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HID)

        #if (BLE_APP_BATT)
        case (TASK_ID_BASS):
        {
            // Call the Battery Module
            msg_pol = app_get_handler(&app_batt_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_BATT)

        #if (BLE_APP_AM0)
        case (TASK_ID_AM0):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_handlers, msgid, param, src_id);
        } break;

        case (TASK_ID_AM0_HAS):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_has_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_AM0)
        #if(BLE_APP_ANCS)
        case (TASK_ID_ANCSC):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_ancsc_handler, msgid, param, src_id);
        } break;
        #endif
        #if(BLE_APP_OADS)
        case (TASK_ID_OADS):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_oads_handler, msgid, param, src_id);
        } break;
        #endif
        #if(BLE_APP_FTMPS)
        case (TASK_ID_FTMPS):
        {
            msg_pol = app_get_handler(&app_ftmp_table_handler, msgid, param, src_id);
        }break;
        #endif
        default:
        {
            #if (BLE_APP_HT)
            if (msgid == APP_HT_MEAS_INTV_TIMER)
            {
                msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_HT)

            #if (BLE_APP_HID)
            if (msgid == APP_HID_MOUSE_TIMEOUT_TIMER)
            {
                msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_HID)
        } break;
    }

    return (msg_pol);
}

/**
 ****************************************************************************************
 * @brief Handles reception of random number generated message
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_gen_rand_nb_ind_handler(ke_msg_id_t const msgid, struct gapm_gen_rand_nb_ind *param,
                                        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if (app_env.rand_cnt==1)      // First part of IRK
    {
        memcpy(&app_env.loc_irk[0], &param->randnb.nb[0], 8);
    }
    else if (app_env.rand_cnt==2) // Second part of IRK
    {
        memcpy(&app_env.loc_irk[8], &param->randnb.nb[0], 8);
    }

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief  GAPC_PARAM_UPDATED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_updated_ind_handler (ke_msg_id_t const msgid, 
                                        const struct gapc_param_updated_ind  *param,
                                         ke_task_id_t const dest_id, 
                                         ke_task_id_t const src_id)
{
    uart_printf("%s \r\n", __func__);
    uart_printf("con_interval = %d\r\n",param->con_interval);
    uart_printf("con_latency = %d\r\n",param->con_latency);
    uart_printf("sup_to = %d\r\n",param->sup_to);
    
    return KE_MSG_CONSUMED;
}
/**
 ****************************************************************************************
 * @brief  GATTC_MTU_CHANGED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                     struct gattc_mtu_changed_ind const *ind,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uart_printf("%s \r\n",__func__);
    uart_printf("ind->mtu = %d,seq = %d\r\n",ind->mtu,ind->seq_num);
    ke_timer_clear(APP_GATTC_EXC_MTU_CMD,TASK_APP);
    
    if(phy_change_state==0)
    {
        appm_update_phy_param(GAP_PHY_LE_2MBPS,GAP_PHY_LE_2MBPS,GAP_PHY_LE_2MBPS);
    }
    
     return (KE_MSG_CONSUMED);
}


/*******************************************************************************
 * Function: gapc_update_conn_param_req_ind_handler
 * Description: Update request command processing from slaver connection parameters
 * Input: msgid   -Id of the message received.
 *          param   -Pointer to the parameters of the message.
 *          dest_id -ID of the receiving task instance
 *          src_id  -ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int app_update_conn_param_req_ind_handler (ke_msg_id_t const msgid, 
                                    const struct gapc_param_update_req_ind  *param,
                                     ke_task_id_t const dest_id, 
                                     ke_task_id_t const src_id)
{

    uart_printf("slave send param_update_req\r\n");
    struct gapc_conn_param  up_param;
    
    up_param.intv_min   = BLE_UAPDATA_MIN_INTVALUE;
    up_param.intv_max   = BLE_UAPDATA_MAX_INTVALUE; 
    up_param.latency    = BLE_UAPDATA_LATENCY;  
    up_param.time_out   = BLE_UAPDATA_TIMEOUT; 
    
    appm_update_param(&up_param);
    
    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief  APP_SEND_SECURITY_REQ
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
extern unsigned char ancs_flag;
static int app_send_security_req_handler(ke_msg_id_t const msgid, 
                                        void const *param,
                                        ke_task_id_t const dest_id, 
                                        ke_task_id_t const src_id)
{   
    app_sec_send_security_req(app_env.conidx);
    
    return KE_MSG_CONSUMED;
}

static int app_ancs_req_handler(ke_msg_id_t const msgid, void const *param,
        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    #if (BLE_APP_ANCS)
    app_ancsc_enable_prf(app_env.conhdl);
    #endif
    return KE_MSG_CONSUMED;
}
/*******************************************************************************
 * Function: app_period_timer_handler
 * Description: app period timer process
 * Input: msgid -Id of the message received.
 *          param -Pointer to the parameters of the message.
 *          dest_id -ID of the receiving task instance (TASK_GAP).
 *          ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int app_period_timer_handler(ke_msg_id_t const msgid,
                                          void *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
   
    return KE_MSG_CONSUMED;
}

/** 
*******************************************************************************

 * @brief  GATTC_EXC_MTU_CMD
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 
*******************************************************************************
**/
static int app_mtu_exchange_req_handler(ke_msg_id_t const msgid,
        struct gattc_exc_mtu_cmd const *req,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    uart_printf("%s \r\n", __func__);
    
    struct gattc_exc_mtu_cmd *cmd = KE_MSG_ALLOC(GATTC_EXC_MTU_CMD,
                                    KE_BUILD_ID(TASK_GATTC, app_env.conidx),
                                    TASK_APP,gattc_exc_mtu_cmd);
    cmd->operation = GATTC_MTU_EXCH;
    cmd->seq_num = 0;
    ke_msg_send(cmd);
    
    return (KE_MSG_CONSUMED);
}

static int gapc_le_phy_ind_handler(ke_msg_id_t const msgid,
                                     struct gapc_le_phy_ind const *ind,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{   
    uart_printf("%s\r\n",__func__); 

    uart_printf("tx_phy:%x,rx_phy:%x\r\n",ind->tx_phy,ind->rx_phy);     
    phy_change_state=1;                         
    return (KE_MSG_CONSUMED);
}



/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(appm)
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)appm_msg_handler},

    // GAPM messages
    {GAPM_PROFILE_ADDED_IND,    (ke_msg_func_t)gapm_profile_added_ind_handler},
    {GAPM_ACTIVITY_CREATED_IND, (ke_msg_func_t)gapm_activity_created_ind_handler},
    {GAPM_ACTIVITY_STOPPED_IND, (ke_msg_func_t)gapm_activity_stopped_ind_handler},
    {GAPM_CMP_EVT,              (ke_msg_func_t)gapm_cmp_evt_handler},
    {GAPM_GEN_RAND_NB_IND,      (ke_msg_func_t)gapm_gen_rand_nb_ind_handler},
 #if MULTI_BOND
    {GAPM_ADDR_SOLVED_IND,       (ke_msg_func_t)gapm_addr_solved_ind_handler},
 #endif /* MULTI_BOND */
    // GAPC messages
    {GAPC_GET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_set_dev_info_req_ind_handler},
    {GAPC_CONNECTION_REQ_IND,   (ke_msg_func_t)gapc_connection_req_ind_handler},
    {GAPC_PARAM_UPDATE_REQ_IND, (ke_msg_func_t)gapc_param_update_req_ind_handler},
    {GAPC_CMP_EVT,              (ke_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_DISCONNECT_IND,       (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GAPC_PARAM_UPDATED_IND,    (ke_msg_func_t)gapc_param_updated_ind_handler}, 
    
    {GATTC_MTU_CHANGED_IND,        (ke_msg_func_t)gattc_mtu_changed_ind_handler},
    
    
    {APP_PARAM_UPDATE_REQ_IND,     (ke_msg_func_t)app_update_conn_param_req_ind_handler},
    {APP_SEND_SECURITY_REQ,     (ke_msg_func_t)app_send_security_req_handler},
    {APP_ANCS_REQ_IND,            (ke_msg_func_t)app_ancs_req_handler},
    {APP_PERIOD_TIMER,            (ke_msg_func_t)app_period_timer_handler},
    {APP_GATTC_EXC_MTU_CMD,        (ke_msg_func_t)app_mtu_exchange_req_handler},
    {GAPC_LE_PHY_IND,           (ke_msg_func_t)gapc_le_phy_ind_handler},

};

/* Defines the place holder for the states of all the task instances. */
ke_state_t appm_state[APP_IDX_MAX];

// Application task descriptor
const struct ke_task_desc TASK_DESC_APP = {appm_msg_handler_tab, appm_state, APP_IDX_MAX, ARRAY_LEN(appm_msg_handler_tab)};

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
