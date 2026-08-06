/**
 ****************************************************************************************
 *
 * @file braces.c
 *
 * @brief barcelet Server Implementation.
 *
 * Copyright (C) beken 2009-2015
 *
 *
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PROV_SERVER)
#include "attm.h"
#include "prov.h"
#include "prov_task.h"
#include "ke_mem.h"
#include "prf_utils.h"
#include "prf.h"
#include "co_utils.h"
#include "flash.h"
#include "uart.h"
#include "ke.h"
#include "icu.h"
#include "wdt.h"


/*
 * FFC0 ATTRIBUTES DEFINITION
 ****************************************************************************************
 */





/// Mesh Provisioning Database Description - Used to add attributes into the database
__STATIC const struct attm_desc prov_att_db[M_IDX_PROV_NB] =
{
    // Mesh Provisioning Service Declaration
    [M_IDX_PROV_SVC]           = {ATT_DECL_PRIMARY_SERVICE,    PERM(RD, ENABLE), 0, 0},
    // Mesh Provisioning Data In Characteristic Declaration
    [M_IDX_PROV_DATA_IN_CHAR]  = {ATT_DECL_CHARACTERISTIC,     PERM(RD, ENABLE), 0, 0},
    // Mesh Provisioning Data In Characteristic Value
    [M_IDX_PROV_DATA_IN_VAL]   = {
        ATT_CHAR_MESH_PROV_DATA_IN,  PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE),
        MAL_CON_MAX_PROV_DATA_LEN
    },
    // Mesh Provisioning Data Out Characteristic Declaration
    [M_IDX_PROV_DATA_OUT_CHAR] = {ATT_DECL_CHARACTERISTIC,     PERM(RD, ENABLE), 0, 0},
    // Mesh Provisioning Data Out Characteristic Value
    [M_IDX_PROV_DATA_OUT_VAL]  = {
        ATT_CHAR_MESH_PROV_DATA_OUT, PERM(NTF, ENABLE), PERM(RI, ENABLE),
        MAL_CON_MAX_PROV_DATA_LEN
    },
    // Mesh Provisioning Data Out Client Characteristic Configuration
    [M_IDX_PROV_DATA_OUT_CCC]  = {ATT_DESC_CLIENT_CHAR_CFG,    PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
};




static uint8_t prov_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct prov_db_cfg* params)
{
 // Service content flag
    uint32_t cfg_flag = PROV_MANDATORY_MASK;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    //-------------------- allocate memory required for the profile  ---------------------
    struct prov_env_tag* prov_env =(struct prov_env_tag* ) ke_malloc(sizeof(struct prov_env_tag), KE_MEM_ATT_DB);

    // allocate BRACE required environment variable
    env->env = (prf_env_t*) prov_env;

     status = attm_svc_create_db(start_hdl, ATT_SVC_MESH_PROVISIONING, (uint8_t *)&cfg_flag,
                                        M_IDX_PROV_NB, NULL, env->task, &prov_att_db[0],
                                        (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)));
       
    uart_printf("create_db status = %x\r\n",status);
    prov_env->prov_start_hdl     = *start_hdl;
    *start_hdl += M_IDX_PROV_NB;
                
    if(status == ATT_ERR_NO_ERROR)
    {
        //Set optional permissions
        if(params->features == PROV_NTF_SUP)
        {
            // Level characteristic value permissions
            uint16_t perm = prov_att_db[M_IDX_PROV_DATA_OUT_VAL].perm ;
            perm |= PERM(NTF, ENABLE);
            attm_att_set_permission(prov_env->prov_start_hdl + M_IDX_PROV_DATA_OUT_VAL, perm, 0);
        }

        // set start handle to first allocated service value
        *start_hdl = prov_env->prov_start_hdl;

        prov_env->features          = params->features;
        prov_env->prf_env.app_task  = app_task
        | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        prov_env->prf_env.prf_task  = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_PROXR;
        env->desc.idx_max           = PROV_IDX_MAX;
        env->desc.state             = prov_env->state;
        prov_task_init(&(env->desc));

        // service is ready, go into an Idle state
        ke_state_set(env->task, PROV_IDLE);
        //uart_printf("go into prov_IDLE");
    }
    return (status);
}



static void prov_destroy(struct prf_task_env* env)
{
    struct prov_env_tag* prov_env = (struct prov_env_tag*) env->env;

    // clear on-going operation
    if(prov_env->operation != NULL)
    {
        ke_free(prov_env->operation);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(prov_env);
}

static void prov_create(struct prf_task_env* env, uint8_t conidx)
{
    struct prov_env_tag* prov_env = (struct prov_env_tag*) env->env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // force notification config to zero when peer device is connected
    prov_env->data_out_ntf_cfg[conidx] = 0;
}




static void prov_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct prov_env_tag* prov_env = (struct prov_env_tag*) env->env;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
    prov_env->data_out_ntf_cfg[conidx] = 0;
        
}



/// BRACE Task interface required by profile manager
const struct prf_task_cbs prov_itf =
{
    (prf_init_fnct) prov_init,
    prov_destroy,
    prov_create,
    prov_cleanup,
};



const struct prf_task_cbs* prov_prf_itf_get(void)
{
   return &prov_itf;
}


#endif // (BLE_BRACELET_REPORTER)


 
