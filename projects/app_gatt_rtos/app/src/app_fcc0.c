/**
 ****************************************************************************************
 *
 * @file app_fcc0.c
 *
 * @brief fcc0 Application Module entry point
 *
 * @auth  gang.cheng
 *
 * @date  2020.03.17
 *
 * Copyright (C) Beken 2019-2022
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

#include "rwip_config.h"     // SW configuration

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <string.h>
#include "ke_task.h"                 // Kernel
#include "app_fcc0.h"              // Battery Application Module Definitions
#include "app.h"                   // Application Definitions
#include "app_task.h"              // application task definitions
#include "fcc0s_task.h"            // health thermometer functions
#include "co_bt.h"
#include "co_utils.h"
#include "prf_types.h"             // Profile common types definition
#include "arch.h"                  // Platform Definitions
#include "prf.h"
#include "fcc0s.h"
#include "ke_timer.h"
#include "uart.h"
#include "prf_utils.h"
#include "user_config.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// fcc0 Application Module Environment Structure
struct app_fcc0_env_tag app_fcc0_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */


void app_fcc0_init(void)
{

    // Reset the environment
    memset(&app_fcc0_env, 0, sizeof(struct app_fcc0_env_tag));
}

void app_fcc0_add_fcc0s(void)
{

   struct fcc0s_db_cfg *db_cfg;
        
   struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct fcc0s_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl =   0;
    req->prf_task_id = TASK_ID_FCC0S;
    req->app_task = TASK_APP;
    req->start_hdl = 0; //req->start_hdl = 0; dynamically allocated

     
    // Set parameters
    db_cfg = (struct fcc0s_db_cfg* ) req->param;
   
    db_cfg->fcc0_nb = 1;
     
    // Sending of notifications is supported
    // Send the message
    ke_msg_send(req);
}


void app_fcc2_send_ntf(uint8_t conidx,uint16_t len,uint8_t* buf)
{
    if( ke_state_get(prf_get_task_from_id(TASK_ID_FCC0S))==FCC0S_IDLE )
    {
        struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);

        if(fcc0s_env->ntf_cfg[conidx] == PRF_CLI_START_NTF)
        {
            // Allocate the message
            struct fcc0s_fcc2_val_upd_req * req = KE_MSG_ALLOC(FCC0S_FCC2_VALUE_UPD_REQ,
                                                                prf_get_task_from_id(TASK_ID_FCC0S),
                                                                KE_BUILD_ID(TASK_APP, conidx),
                                                                fcc0s_fcc2_val_upd_req);
            // Fill in the parameter structure    
            req->length = len;
            memcpy(req->value, buf, len);

            // Send the message
            ke_msg_send(req);
        }
    }
}


static int fcc0s_fcc2_val_ntf_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct fcc0s_fcc2_val_ntf_cfg_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    uart_printf("fcc2->param->ntf_cfg = %x\r\n",param->ntf_cfg);
    if(param->ntf_cfg == PRF_CLI_STOP_NTFIND)
    {
        
    }else
    {
    
    }
    
    return (KE_MSG_CONSUMED);
}


static int fcc2_val_upd_rsp_handler(ke_msg_id_t const msgid,
                                      struct fcc0s_fcc2_val_upd_rsp const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uart_printf("%s,status:%x\r\n", __func__,param->status);
    
    if(param->status == GAP_ERR_NO_ERROR)
    {
        ke_state_set(dest_id, FCC0S_IDLE);    
    }
    
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_fcc0_msg_dflt_handler(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uart_printf("%s\r\n", __func__);
    
    // Drop the message
    return (KE_MSG_CONSUMED);
}


static int fcc1_writer_cmd_handler(ke_msg_id_t const msgid,
                                     struct fcc0s_fcc1_writer_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t buf[20];
    // Drop the message
    uart_printf("FCC1 conidx:%d,param->value = 0x ",param->conidx);
    
    for(uint16_t i = 0;i < param->length;i++)
    {
        uart_printf("%02x ",param->value[i]);
    }
    
    uart_printf("\r\n");
    
    
    if(param->value[0] == 0x55)
    {
        for(uint8_t i = 0;i < 20;i++)
        {
            buf[i] = i;
        }
        app_fcc2_send_ntf(param->conidx,20,buf);
    }
            
    return (KE_MSG_CONSUMED);
}



/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_fcc0_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_fcc0_msg_dflt_handler},
    {FCC0S_FCC2_VALUE_NTF_CFG_IND,  (ke_msg_func_t)fcc0s_fcc2_val_ntf_cfg_ind_handler},

    {FCC0S_FCC2_VALUE_UPD_RSP,      (ke_msg_func_t)fcc2_val_upd_rsp_handler},
    {FCC0S_FCC1_WRITER_CMD_IND,        (ke_msg_func_t)fcc1_writer_cmd_handler},
   
};

const struct app_subtask_handlers app_fcc0_handler = APP_HANDLERS(app_fcc0);



