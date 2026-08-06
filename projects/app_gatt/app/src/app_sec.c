/**
 ****************************************************************************************
 *
 * @file app_sec.c
 *
 * @brief Application Security Entry Point
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

#include "rwip_config.h"
#include "rwapp_config.h"
#if (BLE_APP_SEC)

#include <string.h>
#include "co_utils.h"
#include "co_math.h"
#include "gapc_task.h"      // GAP Controller Task API Definition
#include "gap.h"            // GAP Definition
#include "gapc.h"           // GAPC Definition
#include "prf_types.h"
#include "ke_timer.h"       // Kernel timer

#include "app.h"            // Application API Definition
#include "app_sec.h"        // Application Security API Definition
#include "app_task.h"       // Application Manager API Definition

#if (DISPLAY_SUPPORT)
#include "app_display.h"    // Display Application Definitions
#endif //(DISPLAY_SUPPORT)

#if (NVDS_SUPPORT)
#include "nvds.h"           // NVDS API Definitions
#endif //(NVDS_SUPPORT)

#if (BLE_APP_AM0)
#include "app_am0.h"
#endif //(BLE_APP_AM0)
#include "user_config.h"
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Security Environment Structure
struct app_sec_env_tag app_sec_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_sec_init()
{
    /*------------------------------------------------------
     * RETRIEVE BOND STATUS
     *------------------------------------------------------*/
    #if (NVDS_SUPPORT)
    uint8_t length = NVDS_LEN_PERIPH_BONDED;

    // Get bond status from NVDS
    if (nvds_get(NVDS_TAG_PERIPH_BONDED, &length, (uint8_t *)&app_sec_env.bonded) != NVDS_OK)
    {
        // If read value is invalid, set status to not bonded
        app_sec_env.bonded = false;
    }
    
 #if MULTI_BOND
    
    uint8_t ltk_buff[36];
    length = NVDS_LEN_BOND_INDEX;
    if (nvds_get(NVDS_TAG_BOND_INDEX, &length, (uint8_t *)&app_sec_env.bond_index) != NVDS_OK)
    {
        // If read value is invalid, set status to not bonded
        app_sec_env.bond_index= 0;
    }
    app_sec_env.ltk_buff_statu = false;    
    app_sec_env.bond_num=0;
    length = NVDS_LEN_LTK;
    for(uint8_t i=0;i<MAX_BOND_DEVICE;i++)
    {
        if (nvds_get(NVDS_TAG_LTK0+i, &length, ltk_buff) == NVDS_OK)
        {
             app_sec_env.bond_num++;
        }
    }  
    
 #endif/* MULTI_BOND */

    if ((app_sec_env.bonded != true) && (app_sec_env.bonded != false))
    {
        app_sec_env.bonded = false;
    }
    app_sec_env.peer_pairing = false;
    app_sec_env.peer_encrypt = false;    
    app_sec_env.bond_lost = false;

    #if (DISPLAY_SUPPORT)
    // Update the bond status screen value
    app_display_set_bond(app_sec_env.bonded);
    #endif //(DISPLAY_SUPPORT)
    #endif //(NVDS_SUPPORT)
}

bool app_sec_get_bond_status(void)
{
    return app_sec_env.bonded;
}


#if (NVDS_SUPPORT)
void app_sec_remove_bond(void)
{
    #if (BLE_APP_HID)
    uint16_t ntf_cfg = PRF_CLI_STOP_NTFIND;
    #endif //(BLE_APP_HID)

    // Check if we are well bonded
    if (app_sec_env.bonded == true)
    {
        // Update the environment variable
        app_sec_env.bonded = false;

        if (nvds_put(NVDS_TAG_PERIPH_BONDED, NVDS_LEN_PERIPH_BONDED,
                     (uint8_t *)&app_sec_env.bonded) != NVDS_OK)
        {
            ASSERT_ERR(0);
        }

        #if (BLE_APP_HT)
        #if MULTI_BOND
        for(uint8_t i=0;i<app_sec_env.bond_num;i++)
        {
            if (nvds_del(NVDS_TAG_LTK0+i) != NVDS_OK)
            {
                ASSERT_ERR(0);
            }
        }
        app_sec_env.bond_num=0;
        #else
        if (nvds_del(NVDS_TAG_LTK) != NVDS_OK)
        {
            ASSERT_ERR(0);
        }
        #endif /* MULTI_BOND */


        if (nvds_del(NVDS_TAG_PEER_BD_ADDRESS) != NVDS_OK)
        {
            ASSERT_ERR(0);
        }
        #endif //(BLE_APP_HT)

        #if (BLE_APP_HID)
        if (nvds_put(NVDS_TAG_MOUSE_NTF_CFG, NVDS_LEN_MOUSE_NTF_CFG,
                     (uint8_t *)&ntf_cfg) != NVDS_OK)
        {
            ASSERT_ERR(0);
        }
        #endif //(BLE_APP_HID)
    }
    app_sec_env.bonded = false;
    app_sec_env.peer_pairing = false;
    app_sec_env.peer_encrypt = false;
}
#endif //(NVDS_SUPPORT)

void app_sec_send_security_req(uint8_t conidx)
{
    uart_printf("%s\r\n",__func__);

    if( (!app_sec_env.peer_encrypt) && (!app_sec_env.peer_pairing) )
    {
        // Send security request
        struct gapc_security_cmd *cmd = KE_MSG_ALLOC(GAPC_SECURITY_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                                     gapc_security_cmd);

        cmd->operation = GAPC_SECURITY_REQ;

        #if (BLE_APP_HID || BLE_APP_HT)
        cmd->auth      = GAP_AUTH_REQ_MITM_BOND;
        #elif defined(BLE_APP_AM0)
        cmd->auth      = GAP_AUTH_REQ_NO_MITM_BOND;
        #else
        cmd->auth      = GAP_AUTH_REQ_NO_MITM_NO_BOND;
        #endif //(BLE_APP_HID || BLE_APP_HT)

        #if BLE_APP_SEC_CON
        cmd->auth      = GAP_AUTH_REQ_SEC_CON_BOND;
        #else
        cmd->auth      = GAP_AUTH_REQ_NO_MITM_BOND;
        #endif /*BLE_APP_SEC_CON*/
        
        // Send the message
        ke_msg_send(cmd);
    }
    else
    {
        ke_timer_clear(APP_SEND_SECURITY_REQ,TASK_APP); 
    }
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */
static char* const Bond_event_type_str[] =
{

    /// Bond Pairing request
    "GAPC_PAIRING_REQ",
    /// Respond to Pairing request
    "GAPC_PAIRING_RSP",

    /// Pairing Finished information
    "GAPC_PAIRING_SUCCEED",
    /// Pairing Failed information
    "GAPC_PAIRING_FAILED",

    /// Used to retrieve pairing Temporary Key
    "GAPC_TK_EXCH",
    /// Used for Identity Resolving Key exchange
    "GAPC_IRK_EXCH",
    /// Used for Connection Signature Resolving Key exchange
    "GAPC_CSRK_EXCH",
    /// Used for Long Term Key exchange
    "GAPC_LTK_EXCH",

    /// Bond Pairing request issue, Repeated attempt
    "GAPC_REPEATED_ATTEMPT",

    /// Out of Band - exchange of confirm and rand.
    "GAPC_OOB_EXCH",

    /// Numeric Comparison - Exchange of Numeric Value -
    "GAPC_NC_EXCH",
};
static int gapc_bond_req_ind_handler(ke_msg_id_t const msgid,
                                     struct gapc_bond_req_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Prepare the GAPC_BOND_CFM message
    struct gapc_bond_cfm *cfm = KE_MSG_ALLOC(GAPC_BOND_CFM,
                                             src_id, TASK_APP,
                                             gapc_bond_cfm);
    uart_printf("gapc_bond_req_ind request:%d :%s\r\n",param->request,Bond_event_type_str[param->request]);
    switch (param->request)
    {
        case (GAPC_PAIRING_REQ):
        {
                        app_sec_env.peer_pairing = true;
            cfm->request = GAPC_PAIRING_RSP;

            #if ((!defined(BLE_APP_AM0)) && (!MULTI_BOND))
            cfm->accept  = false;
            // Check if we are already bonded (Only one bonded connection is supported)
            if (!app_sec_env.bonded)
            #endif // !BLE_APP_AM0
            {
                cfm->accept  = true;

                #if (BLE_APP_HID || BLE_APP_HT)
                // Pairing Features
                cfm->data.pairing_feat.auth      = GAP_AUTH_REQ_MITM_BOND;
                #elif defined(BLE_APP_AM0)
                #if (BLE_APP_SEC_CON == 1)
                if (param->data.auth_req & GAP_AUTH_SEC_CON)
                {
                    cfm->data.pairing_feat.auth      = GAP_AUTH_REQ_SEC_CON_BOND;
                    app_env.sec_con_enabled = true;
                }
                else
                {
                    cfm->data.pairing_feat.auth      = GAP_AUTH_REQ_NO_MITM_BOND;
                    app_env.sec_con_enabled = false;
                }
                #else  // !(BLE_APP_SEC_CON)
                cfm->data.pairing_feat.auth      = GAP_AUTH_REQ_NO_MITM_BOND;
                app_env.sec_con_enabled = false;
                #endif // (BLE_APP_SEC_CON)
                #else
                cfm->data.pairing_feat.auth      = GAP_AUTH_REQ_NO_MITM_NO_BOND;
                #endif //(BLE_APP_HID || BLE_APP_HT)

                #if (BLE_APP_HT)
                cfm->data.pairing_feat.iocap     = GAP_IO_CAP_DISPLAY_ONLY;
                #else
                cfm->data.pairing_feat.iocap     = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
                #endif //(BLE_APP_HT)

                cfm->data.pairing_feat.key_size  = 16;
                cfm->data.pairing_feat.oob       = GAP_OOB_AUTH_DATA_NOT_PRESENT;
                cfm->data.pairing_feat.sec_req   = GAP_NO_SEC;
                #if (defined(BLE_APP_AM0))
                cfm->data.pairing_feat.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
                #if BLE_APP_AM0
                // No Keys required from the Initiator
                cfm->data.pairing_feat.ikey_dist = 0;
                #else
                cfm->data.pairing_feat.ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
                #endif
                #else
                cfm->data.pairing_feat.ikey_dist = GAP_KDIST_NONE;
                cfm->data.pairing_feat.rkey_dist = GAP_KDIST_ENCKEY;
                #endif // (defined(BLE_APP_AM0))
            }
        } break;

        case (GAPC_LTK_EXCH):
        {
            // Counter
            uint8_t counter;

            cfm->accept  = true;
            cfm->request = GAPC_LTK_EXCH;

            // Generate all the values
            cfm->data.ltk.ediv = (uint16_t)co_rand_word();

            for (counter = 0; counter < RAND_NB_LEN; counter++)
            {
                cfm->data.ltk.ltk.key[counter]    = (uint8_t)co_rand_word();
                cfm->data.ltk.randnb.nb[counter] = (uint8_t)co_rand_word();
            }

            for (counter = RAND_NB_LEN; counter < KEY_LEN; counter++)
            {
                cfm->data.ltk.ltk.key[counter]    = (uint8_t)co_rand_word();
            }

            #if (NVDS_SUPPORT)
            // Store the generated value in NVDS
            #if MULTI_BOND
            uart_printf("%s,%d\r\n",__FUNCTION__,__LINE__);
            if (nvds_put(NVDS_TAG_LTK0+app_sec_env.bond_index, NVDS_LEN_LTK,(uint8_t *)&cfm->data.ltk) != NVDS_OK)
            {
                    uart_printf("%s,%d\r\n",__FUNCTION__,__LINE__);
                ASSERT_ERR(0);
            }
            #else
            if (nvds_put(NVDS_TAG_LTK, NVDS_LEN_LTK,(uint8_t *)&cfm->data.ltk) != NVDS_OK)
            {
                ASSERT_ERR(0);
            }
            #endif /* MULTI_BOND */

            #endif // #if (NVDS_SUPPORT)
        } break;


        case (GAPC_IRK_EXCH):
        {
            #if (NVDS_SUPPORT)
            uint8_t addr_len = BD_ADDR_LEN;
            #endif //(NVDS_SUPPORT)

            cfm->accept  = true;
            cfm->request = GAPC_IRK_EXCH;

            // Load IRK
            memcpy(cfm->data.irk.irk.key, app_env.loc_irk, KEY_LEN);

            #if (NVDS_SUPPORT)
            if (nvds_get(NVDS_TAG_BD_ADDRESS, &addr_len, cfm->data.irk.addr.addr.addr) != NVDS_OK)
            #endif //(NVDS_SUPPORT)
            {
                ASSERT_ERR(0);
            }
            // load device address
            cfm->data.irk.addr.addr_type = (cfm->data.irk.addr.addr.addr[5] & 0xC0) ? ADDR_RAND : ADDR_PUBLIC;
        } break;
        case (GAPC_CSRK_EXCH):
        {
            // Generate a PIN Code- (Between 100000 and 999999)
            uint32_t pin_code = (100000 + (co_rand_word()%900000));

            #if DISPLAY_SUPPORT
            // Display the PIN Code
            app_display_pin_code(pin_code);
            #endif //DISPLAY_SUPPORT

            cfm->accept  = true;
            cfm->request = GAPC_CSRK_EXCH;

            // Set the TK value
            memset(cfm->data.csrk.key, 0, KEY_LEN);

            cfm->data.csrk.key[0] = (uint8_t)((pin_code & 0x000000FF) >>  0);
            cfm->data.csrk.key[1] = (uint8_t)((pin_code & 0x0000FF00) >>  8);
            cfm->data.csrk.key[2] = (uint8_t)((pin_code & 0x00FF0000) >> 16);
            cfm->data.csrk.key[3] = (uint8_t)((pin_code & 0xFF000000) >> 24);
        }
        break;

        #if (BLE_APP_HT)
        case (GAPC_TK_EXCH):
        {
            // Generate a PIN Code- (Between 100000 and 999999)
            uint32_t pin_code = (100000 + (co_rand_word()%900000));

            #if DISPLAY_SUPPORT
            // Display the PIN Code
            app_display_pin_code(pin_code);
            #endif //DISPLAY_SUPPORT

            cfm->accept  = true;
            cfm->request = GAPC_TK_EXCH;

            // Set the TK value
            memset(cfm->data.tk.key, 0, KEY_LEN);

            cfm->data.tk.key[0] = (uint8_t)((pin_code & 0x000000FF) >>  0);
            cfm->data.tk.key[1] = (uint8_t)((pin_code & 0x0000FF00) >>  8);
            cfm->data.tk.key[2] = (uint8_t)((pin_code & 0x00FF0000) >> 16);
            cfm->data.tk.key[3] = (uint8_t)((pin_code & 0xFF000000) >> 24);
        } break;
        #endif //(BLE_APP_HT)

        default:
        {
            ASSERT_ERR(0);
        } break;
    }

    // Send the message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int gapc_bond_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_bond_ind const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    uart_printf("gapc_bond_ind info:%d,%s\r\n",param->info,Bond_event_type_str[param->info]);
    switch (param->info)
    {
        case (GAPC_PAIRING_SUCCEED):
        {
            // Update the bonding status in the environment
            app_sec_env.bond_lost = false;
            app_sec_env.pairing_fail = false;
            app_sec_env.bonded = true;

            uart_printf("PAIRING_SUCCEED\r\n");
            // Update the bonding status in the environment
            #if MULTI_BOND

            app_sec_env.bond_index++;
            if(app_sec_env.bond_index >= MAX_BOND_DEVICE)
            {
                app_sec_env.bond_index= 0;
            }
            if(app_sec_env.bond_num<MAX_BOND_DEVICE)
            app_sec_env.bond_num++;

            #if (NVDS_SUPPORT)
            if (nvds_put(NVDS_TAG_BOND_INDEX, NVDS_LEN_BOND_INDEX,
                         (uint8_t *)&app_sec_env.bond_index) != NVDS_OK)
            {
                // An error has occurred during access to the NVDS
                ASSERT_ERR(0);
            }
            #endif
            #endif /* MULTI_BOND */            

            #if (NVDS_SUPPORT)
            if (nvds_put(NVDS_TAG_PERIPH_BONDED, NVDS_LEN_PERIPH_BONDED,
                         (uint8_t *)&app_sec_env.bonded) != NVDS_OK)
            {
                // An error has occurred during access to the NVDS
                ASSERT_ERR(0);
            }

            // Set the BD Address of the peer device in NVDS
            if (nvds_put(NVDS_TAG_PEER_BD_ADDRESS, NVDS_LEN_PEER_BD_ADDRESS,
                         (uint8_t *)gapc_get_bdaddr(0, GAPC_SMP_INFO_PEER)) != NVDS_OK)
            {
                // An error has occurred during access to the NVDS
                ASSERT_ERR(0);
            }

            #if (DISPLAY_SUPPORT)
            // Update the bond status screen value
            app_display_set_bond(app_sec_env.bonded);
            #endif //(DISPLAY_SUPPORT)
            #endif 

            #if (BLE_APP_AM0)
            app_am0_send_audio_init(KE_IDX_GET(src_id));
            #endif //(BLE_APP_AM0)
            
            #if (BLE_APP_ANCS)
            ke_timer_set(APP_ANCS_REQ_IND,TASK_APP,3); 
            #endif
        } break;

        case (GAPC_REPEATED_ATTEMPT):
        {
            appm_disconnect();
        } break;

        case (GAPC_IRK_EXCH):
        {
           #if (NVDS_SUPPORT)
           // Store peer identity in NVDS
           #if MULTI_BOND
           if (nvds_put(NVDS_TAG_PEER_IRK0+app_sec_env.bond_index, NVDS_LEN_PEER_IRK, (uint8_t *)&param->data.irk) != NVDS_OK)
           {
               ASSERT_ERR(0);
           }
           #else
           if (nvds_put(NVDS_TAG_PEER_IRK, NVDS_LEN_PEER_IRK, (uint8_t *)&param->data.irk) != NVDS_OK)
           {
               ASSERT_ERR(0);
           }
           #endif /* MULTI_BOND */
           
           #endif // (NVDS_SUPPORT)
            
            uart_printf("irk.key = ");
            for(int i = 0;i<sizeof(struct gap_sec_key);i++)
            {
                uart_printf("0x%x ",param->data.irk.irk.key[i]);
            }
            uart_printf("\r\n");
                    
            uart_printf("addr.type = %x\r\n",param->data.irk.addr.addr_type);
            uart_printf("addr.addr = ");
            for(int i = 0;i<sizeof(struct bd_addr);i++)
            {
                uart_printf("0x%x ",param->data.irk.addr.addr.addr[i]);
            }
            uart_printf("\r\n");
        } break;

        case (GAPC_PAIRING_FAILED):
        {
            app_sec_send_security_req(0);
        } break;

        // In Secure Connections we get BOND_IND with SMPC calculated LTK
        case (GAPC_LTK_EXCH) :
        {
            #if (BLE_APP_SEC_CON)
            if (app_env.sec_con_enabled == true)
            {
                #if (NVDS_SUPPORT)
                // Store LTK in NVDS
                #if MULTI_BOND
                uart_printf("%s,%d\r\n",__FUNCTION__,__LINE__);
                if (nvds_put(NVDS_TAG_LTK0+app_sec_env.bond_index, NVDS_LEN_LTK,(uint8_t *)&param->data.ltk.ltk.key[0]) != NVDS_OK)
                {
                    uart_printf("%s,%d\r\n",__FUNCTION__,__LINE__);
                    ASSERT_ERR(0);
                }
                #else
                if (nvds_put(NVDS_TAG_LTK, NVDS_LEN_LTK,(uint8_t *)&param->data.ltk.ltk.key[0]) != NVDS_OK)
                {
                    ASSERT_ERR(0);
                }
                #endif /* MULTI_BOND */

                #endif // (NVDS_SUPPORT)
                
                uart_printf("ltk key:");
                for(uint8_t i = 0; i < 16; i++)
                {
                    uart_printf("%02x", param->data.ltk.ltk.key[i]);
                }
                uart_printf("\n");
                
                uart_printf("rand nb:");
                for(uint8_t i = 0; i < 16; i++)
                {
                    uart_printf("%02x", param->data.ltk.randnb.nb[i]);
                }
                uart_printf("\n");
                
                    
            }
            #endif // (BLE_APP_SEC_CON)
        }
        break;

        default:
        {
            ASSERT_ERR(0);
        } break;
    }

    return (KE_MSG_CONSUMED);
}

static int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_encrypt_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uart_printf("encrypt_req_ind\r\n");
    app_sec_env.peer_encrypt = true;
    #if (NVDS_SUPPORT)
    // LTK value
    struct gapc_ltk ltk;
    // Length
    uint8_t length = NVDS_LEN_LTK;
    #endif // #if (NVDS_SUPPORT)

    // Prepare the GAPC_ENCRYPT_CFM message
    struct gapc_encrypt_cfm *cfm = KE_MSG_ALLOC(GAPC_ENCRYPT_CFM,
                                                src_id, TASK_APP,
                                                gapc_encrypt_cfm);

    cfm->found    = false;

    
    #if MULTI_BOND
        if(app_sec_env.ltk_buff_statu == true)
        {
                cfm->found    = true;
                cfm->key_size = 16;
                memcpy(&cfm->ltk, app_sec_env.ltk_buff, sizeof(struct gap_sec_key));
        }
    #else
        #if (NVDS_SUPPORT)
        // Retrieve the required informations from NVDS
        if (nvds_get(NVDS_TAG_LTK, &length, (uint8_t *)&ltk) == NVDS_OK)
        {
            // Check if the provided EDIV and Rand Nb values match with the stored values
            if ((param->ediv == ltk.ediv) &&
                !memcmp(&param->rand_nb.nb[0], &ltk.randnb.nb[0], sizeof(struct rand_nb)))
            {
                cfm->found    = true;
                cfm->key_size = 16;
                memcpy(&cfm->ltk, &ltk.ltk, sizeof(struct gap_sec_key));
                uart_printf("encrypt_req_ind121\n");
            }
            
            uart_printf("ediv=%d\n",ltk.ediv);
            for(int i=0;i<sizeof(struct rand_nb);i++)
            {
                uart_printf("%d ",param->rand_nb.nb[i]);
                }
            /*
             * else we are bonded with another device, disconnect the link
             */
        }
        else
        {
            ASSERT_ERR(0);
        }
        #endif // #if (NVDS_SUPPORT)
    #endif /* MULTI_BOND */
    /*
     * else the peer device is not known, an error should trigger a new pairing procedure.
     */

    // Send the message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


static int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
                                    struct gapc_encrypt_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uart_printf("encrypt_ind auth:%d\r\n",param->auth);
    // encryption/ re-encryption succeeded

    #if (BLE_APP_AM0)
    // Need to Setup Authenicated Payload TO for the connection.
    struct gapc_set_le_ping_to_cmd *cmd = KE_MSG_ALLOC(GAPC_SET_LE_PING_TO_CMD,
                                                       KE_BUILD_ID(TASK_GAPC, KE_IDX_GET(src_id)), TASK_APP,
                                                       gapc_set_le_ping_to_cmd);

    // encryption/ re-encryption succeeded

    app_am0_send_audio_init(KE_IDX_GET(src_id));

    cmd->operation = GAPC_SET_LE_PING_TO;
    cmd->timeout = 1000; // 10 Sec

    // Send the message
    ke_msg_send(cmd);
    #endif //(BLE_APP_AM0)
    return (KE_MSG_CONSUMED);
}

static int app_sec_msg_dflt_handler(ke_msg_id_t const msgid,
                                    void *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Drop the message

    return (KE_MSG_CONSUMED);
}

 /*
  * LOCAL VARIABLE DEFINITIONS
  ****************************************************************************************
  */

/// Default State handlers definition
const struct ke_msg_handler app_sec_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,  (ke_msg_func_t)app_sec_msg_dflt_handler},

    {GAPC_BOND_REQ_IND,       (ke_msg_func_t)gapc_bond_req_ind_handler},
    {GAPC_BOND_IND,           (ke_msg_func_t)gapc_bond_ind_handler},

    {GAPC_ENCRYPT_REQ_IND,    (ke_msg_func_t)gapc_encrypt_req_ind_handler},
    {GAPC_ENCRYPT_IND,        (ke_msg_func_t)gapc_encrypt_ind_handler},
};

const struct app_subtask_handlers app_sec_handlers = {&app_sec_msg_handler_list[0], ARRAY_LEN(app_sec_msg_handler_list)};

#endif //(BLE_APP_SEC)

/// @} APP
