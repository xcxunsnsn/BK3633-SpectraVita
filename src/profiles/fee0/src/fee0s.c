/**
 ****************************************************************************************
 *
 * @file fee0s.c
 *
 * @brief fee0 Server Implementation.
 *
 * Copyright (C) beken 2019-2022
 *
 *
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_FEE0_SERVER)
#include "attm.h"
#include "fee0s.h"
#include "fee0s_task.h"
#include "prf_utils.h"
#include "prf.h"
#include "ke_mem.h"

#include "uart.h"



/*
 * FEE0 ATTRIBUTES DEFINITION
 ****************************************************************************************
 */
 
/// Full FEE0 Database Description - Used to add attributes into the database
const struct attm_desc fee0_att_db[FEE0S_IDX_NB] =
{
    // FEE0 Service Declaration
    [FEE0S_IDX_SVC]            =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    [FEE0S_IDX_FEE1_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0}, 
    //  Characteristic Value
    [FEE0S_IDX_FEE1_VAL_VAL]   =   {ATT_USER_SERVER_CHAR_FEE1,PERM(RD, ENABLE), PERM(RI, ENABLE), FEE0_CHAR_DATA_LEN *sizeof(uint8_t)},

    [FEE0S_IDX_FEE1_USER_DESC] =   {ATT_DESC_CHAR_USER_DESCRIPTION,PERM(RD, ENABLE), PERM(RI, ENABLE), FEE0_CHAR_DATA_LEN *sizeof(uint8_t)},

    
    // fee2 value Characteristic Declaration
    [FEE0S_IDX_FEE2_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // fee1 Level Characteristic Value
    [FEE0S_IDX_FEE2_VAL_VAL]   =   {ATT_USER_SERVER_CHAR_FEE2, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE), FEE0_CHAR_DATA_LEN * sizeof(uint8_t)},

        // fee2 value Characteristic Declaration
    [FEE0S_IDX_FEE3_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // fee1 Level Characteristic Value
    [FEE0S_IDX_FEE3_VAL_VAL]   =   {ATT_USER_SERVER_CHAR_FEE3, PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), FEE0_CHAR_DATA_LEN * sizeof(uint8_t)},
    
    // fee1 Level Characteristic Declaration
    [FEE0S_IDX_FEE4_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // fee1 Level Characteristic Value
    [FEE0S_IDX_FEE4_VAL_VAL]   =   {ATT_USER_SERVER_CHAR_FEE4, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE), FEE0_CHAR_DATA_LEN * sizeof(uint8_t)},

    // fee1 Level Characteristic - Client Characteristic Configuration Descriptor
    [FEE0S_IDX_FEE4_VAL_NTF_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,  PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
    
        // fee1 Level Characteristic Declaration
    [FEE0S_IDX_FEE5_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // fee1 Level Characteristic Value
    [FEE0S_IDX_FEE5_VAL_VAL]   =   {ATT_USER_SERVER_CHAR_FEE5, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE), FEE0_CHAR_DATA_LEN * sizeof(uint8_t)},

    // fee1 Level Characteristic - Client Characteristic Configuration Descriptor
    [FEE0S_IDX_FEE5_VAL_IND_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,  PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
    
    
};/// Macro used to retrieve permission value from access and rights on attribute.


static uint8_t fee0s_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct fee0s_db_cfg* params)
{
    uint16_t shdl;
    struct fee0s_env_tag* fee0s_env = NULL;
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    
    //-------------------- allocate memory required for the profile  ---------------------
    fee0s_env = (struct fee0s_env_tag* ) ke_malloc(sizeof(struct fee0s_env_tag), KE_MEM_ATT_DB);
    memset(fee0s_env, 0 , sizeof(struct fee0s_env_tag));

    // Service content flag
    uint16_t cfg_flag = FEE0S_CFG_FLAG_MANDATORY_MASK;

    // Save database configuration
    fee0s_env->features |= (params->features) ;
    
    fee0s_env->fee1_desc_len = params->fee1_desc_len;
    memcpy(fee0s_env->fee1_desc,params->fee1_desc,params->fee1_desc_len);
    shdl = *start_hdl;

    //Create FEE0 in the DB
    //------------------ create the attribute database for the profile -------------------
    status = attm_svc_create_db(&(shdl), ATT_USER_SERVER_FEE0, (uint8_t *)&cfg_flag,
            FEE0S_IDX_NB, NULL, env->task, &fee0_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)));

    //Set optional permissions
    if (status == GAP_ERR_NO_ERROR)
    {
        //Set optional permissions
       // if((params->features & 0x01) == FEE0_FEE4_VAL_NTF_SUP)
        {
            // Battery Level characteristic value permissions
            uint16_t perm = PERM(NTF, ENABLE);//PERM(RD, ENABLE) | 

            attm_att_set_permission(shdl + FEE0S_IDX_FEE4_VAL_VAL, perm, 0);
        }
        //if((params->features & 0x02) == FEE0_FEE3_LVL_NTF_SUP)
        {
            uint16_t perm = PERM(IND, ENABLE);//PERM(RD, ENABLE) | 

            attm_att_set_permission(shdl + FEE0S_IDX_FEE5_VAL_VAL, perm, 0);
        }
    }
    
    //-------------------- Update profile task information  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {

        // allocate  required environment variable
        env->env = (prf_env_t*) fee0s_env;
        *start_hdl = shdl;
        fee0s_env->start_hdl = *start_hdl;
        fee0s_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        fee0s_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_FEE0S;
        env->desc.idx_max           = FEE0S_IDX_MAX;
        env->desc.state             = fee0s_env->state;
        
        fee0s_task_init(&(env->desc));
        co_list_init(&(fee0s_env->values));



        // service is ready, go into an Idle state
        ke_state_set(env->task, FEE0S_IDLE);
    }
    else if(fee0s_env != NULL)
    {
        ke_free(fee0s_env);
    }
     
    return (status);
}


static void fee0s_destroy(struct prf_task_env* env)
{
    struct fee0s_env_tag* fee0s_env = (struct fee0s_env_tag*) env->env;

    // clear on-going operation
    if(fee0s_env->operation != NULL)
    {
        ke_free(fee0s_env->operation);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(fee0s_env);
}

static void fee0s_create(struct prf_task_env* env, uint8_t conidx)
{
    struct fee0s_env_tag* fee0s_env = (struct fee0s_env_tag*) env->env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // force notification config to zero when peer device is connected
    fee0s_env->ntf_cfg[conidx] = 0;
}


static void fee0s_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct fee0s_env_tag* fee0s_env = (struct fee0s_env_tag*) env->env;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
    fee0s_env->ntf_cfg[conidx] = 0;
}

void fee0s_notify_fee4_val(uint8_t conidx,struct fee0s_env_tag* fee0s_env, struct fee0s_fee45_val_upd_req const *param)
{
    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(fee0s_env->prf_env),0),
            gattc_send_evt_cmd, sizeof(uint8_t)* (param->length));

    // Fill in the parameter structure
    val->operation = GATTC_NOTIFY;
    val->handle = fee0s_get_att_handle(FEE0S_IDX_FEE4_VAL_VAL);

    // pack measured value in database
    val->length = param->length;
    memcpy(&val->value[0],&param->value[0],param->length);
    // send notification to peer device
    ke_msg_send(val);
}


void fee0s_indicate_fee5_val(uint8_t conidx,struct fee0s_env_tag* fee0s_env, struct fee0s_fee45_val_upd_req const *param)
{
    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(fee0s_env->prf_env),0),
            gattc_send_evt_cmd, sizeof(uint8_t)* (param->length));

    // Fill in the parameter structure
    val->operation = GATTC_INDICATE;
    val->handle = fee0s_get_att_handle(FEE0S_IDX_FEE5_VAL_VAL);
    // pack measured value in database
    val->length = param->length;
    memcpy(&val->value[0],&param->value[0],param->length);
    // send notification to peer device
    ke_msg_send(val);
}



/// BASS Task interface required by profile manager
const struct prf_task_cbs fee0s_itf =
{
        (prf_init_fnct) fee0s_init,
        fee0s_destroy,
        fee0s_create,
        fee0s_cleanup,
};


const struct prf_task_cbs* fee0s_prf_itf_get(void)
{
   return &fee0s_itf;
}


uint16_t fee0s_get_att_handle( uint8_t att_idx)
{
        
    struct fee0s_env_tag *fee0s_env = PRF_ENV_GET(FEE0S, fee0s);
    uint16_t handle = ATT_INVALID_HDL;
   
    handle = fee0s_env->start_hdl;

    // increment index according to expected index
    if(att_idx <= FEE0S_IDX_FEE5_VAL_IND_CFG)
    {
        handle += att_idx;
    }
    else
    {
        handle = ATT_INVALID_HDL;
    }
    

    return handle;
}

uint8_t fee0s_get_att_idx(uint16_t handle, uint8_t *att_idx)
{
    struct fee0s_env_tag* fee0s_env = PRF_ENV_GET(FEE0S, fee0s);
    uint16_t hdl_cursor1 = fee0s_env->start_hdl;    
    uint8_t status = PRF_APP_ERROR;

    // Browse list of services
    // handle must be greater than current index 
    // check if it's a mandatory index
    if(handle <= (hdl_cursor1 + FEE0S_IDX_FEE5_VAL_IND_CFG))
    {
        *att_idx = handle -hdl_cursor1;
        status = GAP_ERR_NO_ERROR;
        
    }
    
    return (status);
}


#endif // (BLE_FEE0_SERVER)


 
