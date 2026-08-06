/**
 ****************************************************************************************
 *
 * @file   braces_task.c
 *
 * @brief barcelet Server Role Task Implementation.
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PROV_SERVER)


#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "atts.h"
#include "co_utils.h"
#include "prov.h"
#include "prov_task.h"

#include "prf_utils.h"
#include "uart.h"




 void dataout_notify_data(uint8_t * databuffer,uint8_t len)
{
     struct prov_env_tag* prov_env = PRF_ENV_GET(PROV, prov);
     uart_printf("%s \r\n",__func__);
     struct gattc_send_evt_cmd *cmd = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                                KE_BUILD_ID(TASK_GATTC, 0), prf_src_task_get(&(prov_env->prf_env),0),
                                gattc_send_evt_cmd, sizeof(uint8_t) * 2);
    
    // Fill in the parameter structure
    cmd->operation = GATTC_NOTIFY;
    cmd->handle = prov_env->prov_start_hdl + M_IDX_PROV_DATA_OUT_VAL;
    cmd->length = 2;
    memcpy(&cmd->value[0],databuffer,len);
    ke_msg_send(cmd);
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

 //  uart_printf("oad %s \r\n",__func__);
    uint8_t status = ATT_ERR_NO_ERROR;

    int msg_status = KE_MSG_CONSUMED;

    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information

    // If the attribute has been found, status is ATT_ERR_NO_ERROR
    if(ke_state_get(dest_id) == PROV_IDLE)
    {

        struct prov_env_tag* prov_env = PRF_ENV_GET(PROV, prov);
        if(prov_env == NULL)
        {
            while(1)
            {
                uart_printf("prov_env == null\r\n");
            };
        }
        uint16_t ntf_cfg = co_read16p(&param->value[0]);
        if(((prov_env->features & 0x01) == PROV_NTF_SUP) && ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF)))
        {

            if(param->handle == (prov_env->prov_start_hdl + M_IDX_PROV_DATA_OUT_CCC))
            {    
                uart_printf("PROV_IDX_DATA_OUT_NTF_CFG ntf_cfg = %d\r\n",ntf_cfg);
                if (ntf_cfg == PRF_CLI_START_NTF)
                {
                    // Ntf cfg bit set to 1
                    prov_env->data_out_ntf_cfg[conidx] |= (PROV_NTF_SUP);
                }
                else
                {    
                    // Ntf cfg bit set to 0
                    prov_env->data_out_ntf_cfg[conidx] &= ~(PROV_NTF_SUP);
                }            
            }
            
        }
        
        if(param->handle == (prov_env->prov_start_hdl + M_IDX_PROV_DATA_IN_VAL))
        {   

                
                uint8_t senddata[2]={12,13};
                uint8_t receive[30];
                memcpy(receive,&param->value[0],param->length);
                uart_printf("receive  data from peer device\r\n");    
                
                dataout_notify_data(senddata,2);
                
        }

        //Send write response
        struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
        GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
        //Send write response
        cfm->handle = param->handle;
        cfm->status = status;
        ke_msg_send(cfm);
    }
    else if(ke_state_get(dest_id) == PROV_BUSY)
    {
        uart_printf("OADS_BUSY\r\n");
        msg_status = KE_MSG_SAVED;
    }
    return (msg_status);
}   



static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t status = ATT_ERR_NO_ERROR;
    int msg_status = KE_MSG_CONSUMED;
    struct gattc_read_cfm * cfm;
    struct prov_env_tag* prov_env = PRF_ENV_GET(PROV, prov);
    uint16_t length = 0;
    uint8_t value[20] = {0};

    if(ke_state_get(dest_id) == PROV_IDLE)
    {
        //uart_printf("oads OADS_IDLE read param->handle = 0x%x \r\n",param->handle);
        
        if(param->handle == (prov_env->prov_start_hdl + M_IDX_PROV_DATA_OUT_CCC))
        {
            length = sizeof(uint16_t);
            memcpy(value,&prov_env->data_out_ntf_cfg[0],length);
            //uart_printf("Read c2 cfg \r\n");
            uart_printf("read data_out cfg =%d\r\n",prov_env->data_out_ntf_cfg[0]);
        }
        
        cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, length);
        cfm->length = length;
        memcpy(cfm->value, value, length);
        cfm->handle = param->handle;
        cfm->status = status;

        // Send value to peer device.
        ke_msg_send(cfm);
        //uart_printf("oad read send cfm\r\n");
    }
    else if(ke_state_get(dest_id) == PROV_BUSY)
    {
        uart_printf("oads OADS_BUSY \r\n");
        msg_status = KE_MSG_SAVED;
    }
    //uart_printf("oads %s end!\r\n",__func__);
    return (msg_status);
}   

static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uart_printf("%s\r\n",__func__);
    if(param->operation == GATTC_NOTIFY)
    {
    }
    return (KE_MSG_CONSUMED);
}

/// Default State handlers definition
///const struct ke_msg_handler prov_default_state[] =
KE_MSG_HANDLER_TAB(prov)   
{
  //  {PROV_ENABLE_REQ,             (ke_msg_func_t) prov_enable_req_handler},
    {GATTC_WRITE_REQ_IND,          (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_READ_REQ_IND,          (ke_msg_func_t) gattc_read_req_ind_handler},
    {GATTC_CMP_EVT,               (ke_msg_func_t) gattc_cmp_evt_handler},
};

/// Specifies the message handlers that are common to all states.
//const struct ke_state_handler braces_default_handler = KE_STATE_HANDLER(oads_default_state);


void prov_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct prov_env_tag *srv_env = PRF_ENV_GET(PROV, prov);

    task_desc->msg_handler_tab = prov_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(prov_msg_handler_tab);
    task_desc->state           = srv_env->state;
    task_desc->idx_max         = PROV_IDX_MAX;
}



#endif /* #if (BLE_PROV_SERVER) */




 
