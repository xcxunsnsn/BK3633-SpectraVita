/**
 ****************************************************************************************
 *
 * @file   fcc0s_task.c
 *
 * @brief FCC0 Server Role Task Implementation.
 *
 * Copyright (C) Beken 2019-2022
 *
 *
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_FCC0_SERVER)

#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "atts.h"
#include "co_utils.h"
#include "fcc0s.h"
#include "fcc0s_task.h"
#include "prf_utils.h"
#include "user_config.h"




static int fcc0s_fcc2_val_upd_req_handler(ke_msg_id_t const msgid,
                                            struct fcc0s_fcc2_val_upd_req const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_SAVED;
    uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(src_id);
    uart_printf("%s,conidx:%d\r\n",__func__,conidx);
    // check state of the task
    if(state == FCC0S_IDLE)
    {
        struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);

        if(fcc0s_env->ntf_cfg[conidx] == PRF_CLI_START_IND)
        {
            // put task in a busy state
            ke_state_set(dest_id, FCC0S_BUSY);                        
            fcc0s_indicate_fcc2_val(conidx,fcc0s_env, param);
            //ke_state_set(dest_id, FCC0S_IDLE);
        }
           
        msg_status = KE_MSG_CONSUMED;    
    }

    return (msg_status);
  }
 
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    
    struct gattc_att_info_cfm * cfm;
    uint8_t  att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = fcc0s_get_att_idx(param->handle, &att_idx);
    uart_printf("%s,conidx:%d\r\n",__func__,conidx);
    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

    if(status == GAP_ERR_NO_ERROR)
    {
        // check if it's a client configuration char
        if(att_idx == FCC0S_IDX_FCC2_VAL_IND_CFG)
        {
            // CCC attribute length = 2
            cfm->length = 2;
        }        
        else if(att_idx == FCC0S_IDX_FCC2_VAL_VAL)
        {
            cfm->length = FCC0_CHAR_DATA_LEN;
        }
        // not expected request
        else
        {
            cfm->length = 0;
            status = ATT_ERR_WRITE_NOT_PERMITTED;
        }
    }

    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}



static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    
    struct gattc_write_cfm * cfm;
    uint8_t att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    uart_printf("%s,conidx:%d\r\n",__func__,conidx);
    // retrieve handle information
    uint8_t status = fcc0s_get_att_idx(param->handle,  &att_idx);
    // If the attribute has been found, status is GAP_ERR_NO_ERROR
    if (status == GAP_ERR_NO_ERROR)
    {
        struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
        // Extract value before check
        uint16_t ntf_cfg = co_read16p(&param->value[0]);

        // Only update configuration if value for stop or notification enable
        if ((att_idx == FCC0S_IDX_FCC2_VAL_IND_CFG)
                && ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND)))
        {

        
            // Conserve information in environment
            if (ntf_cfg == PRF_CLI_START_IND)
            {
                // Ntf cfg bit set to 1
                fcc0s_env->ntf_cfg[conidx] = PRF_CLI_START_IND ;
            }
            else
            {
                // Ntf cfg bit set to 0
                fcc0s_env->ntf_cfg[conidx] = PRF_CLI_STOP_NTFIND;
            }

            // Inform APP of configuration change
            struct fcc0s_fcc2_val_ntf_cfg_ind * ind = KE_MSG_ALLOC(FCC0S_FCC2_VALUE_NTF_CFG_IND,
                    prf_dst_task_get(&(fcc0s_env->prf_env), conidx), dest_id,
                    fcc0s_fcc2_val_ntf_cfg_ind);
            ind->conidx = conidx;
            ind->ntf_cfg = fcc0s_env->ntf_cfg[conidx];
                        
            ke_msg_send(ind);            
        }        
        else if (att_idx == FCC0S_IDX_FCC1_VAL_VAL)
        {
            // Allocate the alert value change indication
            struct fcc0s_fcc1_writer_ind *ind = KE_MSG_ALLOC(FCC0S_FCC1_WRITER_CMD_IND,
                    prf_dst_task_get(&(fcc0s_env->prf_env), conidx),
                    dest_id, fcc0s_fcc1_writer_ind);
            
            // Fill in the parameter structure    
            memcpy(ind->value,&param->value[0],param->length);
            ind->conidx = conidx;
            ind->length = param->length;
            if((ntf_cfg & 0x00ff) == 0xff)
            {
                fcc0s_env->ntf_cfg[conidx] = PRF_CLI_START_IND ;
            }
            // Send the message
            ke_msg_send(ind);
        }
        else
        {
            status = PRF_APP_ERROR;
        }

    }

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);


    return (KE_MSG_CONSUMED);
}   



static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    
    struct gattc_read_cfm * cfm;
    uint8_t  att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = fcc0s_get_att_idx(param->handle, &att_idx);
    uint16_t length = 0;
    struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
    uart_printf("%s conidx:%d,handle:%d,att_idx:%d\r\n",__func__,conidx,param->handle,att_idx);
    // If the attribute has been found, status is GAP_ERR_NO_ERROR
    if (status == GAP_ERR_NO_ERROR)
    {
        // read notification information
        if (att_idx == FCC0S_IDX_FCC1_VAL_VAL)
        {
            length = FCC0_CHAR_DATA_LEN * sizeof(uint8_t);
        }
        if (att_idx == FCC0S_IDX_FCC2_VAL_VAL)
        {
            length = FCC0_CHAR_DATA_LEN * sizeof(uint8_t);
        }
        // read notification information
        else if (att_idx == FCC0S_IDX_FCC2_VAL_IND_CFG)
        {
            length = sizeof(uint16_t);
        }        
        else
        {
            status = PRF_APP_ERROR;
        }
    }

    //Send write response
    cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, length);
    cfm->handle = param->handle;
    cfm->status = status;
    cfm->length = length;
   
    if (status == GAP_ERR_NO_ERROR)
    {
        // read notification information
        if (att_idx == FCC0S_IDX_FCC1_VAL_VAL)
        {
            
            memcpy(cfm->value,fcc0s_env->fcc1_val,FCC0_CHAR_DATA_LEN);
            fcc0s_env->fcc1_val[0]++;
            fcc0s_env->fcc1_val[FCC0_CHAR_DATA_LEN -1]++;
        }
        // retrieve notification config
        else if (att_idx == FCC0S_IDX_FCC2_VAL_IND_CFG)
        {
            uint16_t ntf_cfg = fcc0s_env->ntf_cfg[conidx];
            co_write16p(cfm->value, ntf_cfg);
        }          
        else
        {
            /* Not Possible */
        }
    }

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}   

extern uint16_t g_hande;
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uart_printf("%s\r\n",__func__);
    struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
    uint8_t conidx = KE_IDX_GET(src_id);
    if(param->operation == GATTC_NOTIFY)
    {               
        struct fcc0s_fcc2_val_upd_rsp *rsp = KE_MSG_ALLOC(FCC0S_FCC2_VALUE_UPD_RSP,
                                             prf_dst_task_get(&(fcc0s_env->prf_env), conidx),
                                             dest_id, fcc0s_fcc2_val_upd_rsp);
        rsp->conidx = conidx;
        rsp->status = param->status;            
        ke_msg_send(rsp);
        
    }
    // go back in to idle mode
    ke_state_set(dest_id, ke_state_get(dest_id) & ~FCC0S_BUSY);
    
    return (KE_MSG_CONSUMED);
}

/// Default State handlers definition
KE_MSG_HANDLER_TAB(fcc0s)
{
    {FCC0S_FCC2_VALUE_UPD_REQ,      (ke_msg_func_t) fcc0s_fcc2_val_upd_req_handler},    
    {GATTC_ATT_INFO_REQ_IND,        (ke_msg_func_t) gattc_att_info_req_ind_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_READ_REQ_IND,            (ke_msg_func_t) gattc_read_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
};

void fcc0s_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct fcc0s_env_tag *srv_env = PRF_ENV_GET(FCC0S, fcc0s);

    task_desc->msg_handler_tab = fcc0s_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(fcc0s_msg_handler_tab);
    task_desc->state           = srv_env->state;
    task_desc->idx_max         = FCC0S_IDX_MAX;
}

#endif /* #if (BLE_FCC0_SERVER) */


