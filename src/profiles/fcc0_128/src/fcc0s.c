/**
 ****************************************************************************************
 *
 * @file fcc0s.c
 *
 * @brief fcc0 Server Implementation.
 *
 * Copyright (C) beken 2019-2022
 *
 *
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_FCC0_SERVER)
#include "attm.h"
#include "fcc0s.h"
#include "fcc0s_task.h"
#include "prf_utils.h"
#include "prf.h"
#include "ke_mem.h"

#include "uart.h"



/*
 * FCC0 ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

#define ATT_DECL_PRIMARY_SERVICE_ARRAY  {0x00,0x28} //ATT_DECL_PRIMARY_SERVICE
#define ATT_DECL_CHARACTERISTIC_ARRAY  {0x03,0x28} //ATT_DECL_CHARACTERISTIC
#define ATT_DESC_CLIENT_CHAR_CFG_ARRAY  {0x02,0x29} //ATT_DESC_CLIENT_CHAR_CFG 

//0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01
const uint8_t USER_FCC0_SERV_UUID_128[] =   {0x0, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, \
0xfc, 0x43, 0x77, 0xa2, 0x0, 0xe6, 0xf1, 0x15
}; //hilink service uuid

#define USER_FCC2_CHAR_UUID_128   {0x0, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc,\
0x43, 0x77, 0xa2,0x1, 0xe6, 0xf1, 0x15}  //readdata indicate/read

#define USER_FCC1_CHAR_UUID_128   {0x0, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4,\
0xfc, 0x43, 0x77, 0xa2, 0x2, 0xe6, 0xf1, 0x15}  //writedata write

                                  
/// Full FCC0 Database Description - Used to add attributes into the database
                                  
const struct attm_desc_128 fcc0_att_db[FCC0S_IDX_NB] =
{
    // FCC0 Service Declaration
    [FCC0S_IDX_SVC]            =   {ATT_DECL_PRIMARY_SERVICE_ARRAY, PERM(RD, ENABLE), 0, 0},

    [FCC0S_IDX_FCC1_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC_ARRAY, PERM(RD, ENABLE), 0, 0}, 
    //  Characteristic Value
    [FCC0S_IDX_FCC1_VAL_VAL]   =   {USER_FCC1_CHAR_UUID_128,PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE)|PERM(UUID_LEN, UUID_128), FCC0_CHAR_DATA_LEN *sizeof(uint8_t)},

    // fcc2 Value Characteristic Declaration
    [FCC0S_IDX_FCC2_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC_ARRAY, PERM(RD, ENABLE), 0, 0},
    // fcc2 Value Characteristic Value
    [FCC0S_IDX_FCC2_VAL_VAL]   =   {USER_FCC2_CHAR_UUID_128, PERM(RD, ENABLE), PERM(RI, ENABLE)|PERM(UUID_LEN, UUID_128), FCC0_CHAR_DATA_LEN * sizeof(uint8_t)},

    // fcc2 Value Characteristic - Client Characteristic Configuration Descriptor
    [FCC0S_IDX_FCC2_VAL_IND_CFG] = {ATT_DESC_CLIENT_CHAR_CFG_ARRAY,  PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
    
    
};/// Macro used to retrieve permission value from access and rights on attribute.


static uint8_t fcc0s_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct fcc0s_db_cfg* params)
{
    uint16_t shdl;
    struct fcc0s_env_tag* fcc0s_env = NULL;
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    
    //-------------------- allocate memory required for the profile  ---------------------
    fcc0s_env = (struct fcc0s_env_tag* ) ke_malloc(sizeof(struct fcc0s_env_tag), KE_MEM_ATT_DB);
    memset(fcc0s_env, 0 , sizeof(struct fcc0s_env_tag));

    // Service content flag
    uint16_t cfg_flag = 0xff;

    // Save database configuration
    fcc0s_env->features |= (params->features) ;

    shdl = *start_hdl;

    //Create FCC0 in the DB
    //------------------ create the attribute database for the profile -------------------
    status = attm_svc_create_db_128(&(shdl), USER_FCC0_SERV_UUID_128, (uint8_t *)&cfg_flag,
            FCC0S_IDX_NB, NULL, env->task, &fcc0_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))|PERM(SVC_UUID_LEN, UUID_128));

    //Set optional permissions
    if (status == GAP_ERR_NO_ERROR)
    {
        //Set optional permissions
       // if((params->features & 0x01) == FCC0_FCC2_VAL_NTF_SUP)
        {
            // Battery Level characteristic value permissions
            uint16_t perm = PERM(IND, ENABLE);//PERM(RD, ENABLE) | 

            attm_att_set_permission(shdl + FCC0S_IDX_FCC2_VAL_VAL, perm, 0);
        }
        
    }
    
    //-------------------- Update profile task information  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {

        // allocate  required environment variable
        env->env = (prf_env_t*) fcc0s_env;
        *start_hdl = shdl;
        fcc0s_env->start_hdl = *start_hdl;
        fcc0s_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        fcc0s_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_FCC0S;
        env->desc.idx_max           = FCC0S_IDX_MAX;
        env->desc.state             = fcc0s_env->state;
        
        fcc0s_task_init(&(env->desc));
        co_list_init(&(fcc0s_env->values));



        // service is ready, go into an Idle state
        ke_state_set(env->task, FCC0S_IDLE);
    }
    else if(fcc0s_env != NULL)
    {
        ke_free(fcc0s_env);
    }
     
    return (status);
}


static void fcc0s_destroy(struct prf_task_env* env)
{
    struct fcc0s_env_tag* fcc0s_env = (struct fcc0s_env_tag*) env->env;

    // clear on-going operation
    if(fcc0s_env->operation != NULL)
    {
        ke_free(fcc0s_env->operation);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(fcc0s_env);
}

static void fcc0s_create(struct prf_task_env* env, uint8_t conidx)
{
    struct fcc0s_env_tag* fcc0s_env = (struct fcc0s_env_tag*) env->env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // force notification config to zero when peer device is connected
    fcc0s_env->ntf_cfg[conidx] = 0;
}


static void fcc0s_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct fcc0s_env_tag* fcc0s_env = (struct fcc0s_env_tag*) env->env;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
    fcc0s_env->ntf_cfg[conidx] = 0;
}

void fcc0s_indicate_fcc2_val(uint8_t conidx,struct fcc0s_env_tag* fcc0s_env, struct fcc0s_fcc2_val_upd_req const *param)
{
    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(fcc0s_env->prf_env),0),
            gattc_send_evt_cmd, sizeof(uint8_t)* (param->length));

    // Fill in the parameter structure
    val->operation = GATTC_INDICATE;
    val->handle = fcc0s_get_att_handle(FCC0S_IDX_FCC2_VAL_VAL);

    // pack measured value in database
    val->length = param->length;
    memcpy(&val->value[0],&param->value[0],param->length);
    // send notification to peer device
    ke_msg_send(val);
}




/// BASS Task interface required by profile manager
const struct prf_task_cbs fcc0s_itf =
{
        (prf_init_fnct) fcc0s_init,
        fcc0s_destroy,
        fcc0s_create,
        fcc0s_cleanup,
};


const struct prf_task_cbs* fcc0s_prf_itf_get(void)
{
   return &fcc0s_itf;
}


uint16_t fcc0s_get_att_handle( uint8_t att_idx)
{
        
    struct fcc0s_env_tag *fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
    uint16_t handle = ATT_INVALID_HDL;
   
    handle = fcc0s_env->start_hdl;

    // increment index according to expected index
    if(att_idx <= FCC0S_IDX_FCC2_VAL_IND_CFG)
    {
        handle += att_idx;
    }
    else
    {
        handle = ATT_INVALID_HDL;
    }
    

    return handle;
}

uint8_t fcc0s_get_att_idx(uint16_t handle, uint8_t *att_idx)
{
    struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
    uint16_t hdl_cursor1 = fcc0s_env->start_hdl;

    uint8_t status = PRF_APP_ERROR;

    // Browse list of services
    // handle must be greater than current index 
    // check if it's a mandatory index
    if(handle <= (hdl_cursor1 + FCC0S_IDX_FCC2_VAL_IND_CFG))
    {
        *att_idx = handle -hdl_cursor1;
        status = GAP_ERR_NO_ERROR;
        
    }
    
    return (status);
}


#endif // (BLE_FCC0_SERVER)


 
