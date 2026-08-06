/**
 ****************************************************************************************
 *
 * @file otahs.c
 *
 * @brief otah Server Implementation.
 *
 * Copyright (C) beken 2019-2022
 *
 *
 ****************************************************************************************
 */

#include "rwip_config.h"

#include "attm.h"
#include "otahs.h"
#include "otahs_task.h"
#include "prf_utils.h"
#include "prf.h"
#include "ke_mem.h"

#include "uart.h"



/*
 * OTAH ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

#define ATT_DECL_PRIMARY_SERVICE_ARRAY  {0x00,0x28} //ATT_DECL_PRIMARY_SERVICE
#define ATT_DECL_CHARACTERISTIC_ARRAY  {0x03,0x28} //ATT_DECL_CHARACTERISTIC
#define ATT_DESC_CLIENT_CHAR_CFG_ARRAY  {0x02,0x29} //ATT_DESC_CLIENT_CHAR_CFG 


//0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01
//15f1e610-a277-43fca484-dd39ef8a9100 
const uint8_t USER_OTAH_SERV_UUID_128[] =   { \
0x0, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4,0xfc, 0x43, 0x77, 0xa2, 0x10, 0xe6, 0xf1, 0x15
}; //hilink service uuid
//15f1e611-a277- 43fca484-dd39ef8a9100
#define USER_CTRL_CHAR_UUID_128   {0x0, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc,\
0x43, 0x77, 0xa2,0x11, 0xe6, 0xf1, 0x15}  //readdata indicate/read CTRL-CTRL

//15f1e612-a277- 43fca484-dd39ef8a9100
#define USER_DATA_CHAR_UUID_128   {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4,\
0xfc, 0x43, 0x77, 0xa2, 0x12, 0xe6, 0xf1, 0x15}  //writedata write DATA-DATA

                                  
/// Full OTAH Database Description - Used to add attributes into the database
                                  
const struct attm_desc_128 otah_att_db[OTAHS_IDX_NB] =
{
    // OTAH Service Declaration
    [OTAHS_IDX_SVC]            =   {ATT_DECL_PRIMARY_SERVICE_ARRAY, PERM(RD, ENABLE), 0, 0},

    
    // ctrl Value Characteristic Declaration
    [OTAHS_IDX_CTRL_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC_ARRAY, PERM(RD, ENABLE), 0, 0},
    // ctrl Value Characteristic Value
    [OTAHS_IDX_CTRL_VAL_VAL]   =   {USER_CTRL_CHAR_UUID_128, PERM(WRITE_REQ, ENABLE) | PERM(RD, ENABLE), PERM(RI, ENABLE)|PERM(UUID_LEN, UUID_128), OTAH_CHAR_DATA_LEN * sizeof(uint8_t)},

    // ctrl Value Characteristic - Client Characteristic Configuration Descriptor
    [OTAHS_IDX_CTRL_VAL_IND_CFG] = {ATT_DESC_CLIENT_CHAR_CFG_ARRAY,  PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
    
    [OTAHS_IDX_DATA_VAL_CHAR]  =   {ATT_DECL_CHARACTERISTIC_ARRAY, PERM(RD, ENABLE), 0, 0}, 
    //  Characteristic Value
    [OTAHS_IDX_DATA_VAL_VAL]   =   {USER_DATA_CHAR_UUID_128, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE)|PERM(UUID_LEN, UUID_128), OTAH_CHAR_DATA_LEN *sizeof(uint8_t)},

};/// Macro used to retrieve permission value from access and rights on attribute.


static uint8_t otahs_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct otahs_db_cfg* params)
{
    uint16_t shdl;
    struct otahs_env_tag* otahs_env = NULL;
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    
    //-------------------- allocate memory required for the profile  ---------------------
    otahs_env = (struct otahs_env_tag* ) ke_malloc(sizeof(struct otahs_env_tag), KE_MEM_ATT_DB);
    memset(otahs_env, 0 , sizeof(struct otahs_env_tag));

    // Service content flag
    uint16_t cfg_flag = 0xFFFF;

    // Save database configuration
    otahs_env->features |= (params->features) ;

    shdl = *start_hdl;
    
    uart_printf("otahs_init------------- \r\n");
    //Create OTAH in the DB
    //------------------ create the attribute database for the profile -------------------
    status = attm_svc_create_db_128(&(shdl), USER_OTAH_SERV_UUID_128, (uint8_t *)&cfg_flag,
            OTAHS_IDX_NB, NULL, env->task, &otah_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))|PERM(SVC_UUID_LEN, UUID_128));

    //Set optional permissions
    if (status == GAP_ERR_NO_ERROR)
    {
        //Set optional permissions
       // if((params->features & 0x01) == OTAH_CTRL_VAL_NTF_SUP)
        {
            // Battery Level characteristic value permissions
            uint16_t perm = otah_att_db[OTAHS_IDX_CTRL_VAL_VAL].perm ;
            
            perm = perm|PERM(IND, ENABLE);
            
            attm_att_set_permission(shdl + OTAHS_IDX_CTRL_VAL_VAL, perm, 0);
        }
        
    }
    
    //-------------------- Update profile task information  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {

        // allocate  required environment variable
        env->env = (prf_env_t*) otahs_env;
        *start_hdl = shdl;
        otahs_env->start_hdl = *start_hdl;
        otahs_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        otahs_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_OTAHS;
        env->desc.idx_max           = OTAHS_IDX_MAX;
        env->desc.state             = otahs_env->state;
        
        otahs_task_init(&(env->desc));
        co_list_init(&(otahs_env->values));



        // service is ready, go into an Idle state
        ke_state_set(env->task, OTAHS_IDLE);
    }
    else if(otahs_env != NULL)
    {
        ke_free(otahs_env);
    }
     
    return (status);
}


static void otahs_destroy(struct prf_task_env* env)
{
    struct otahs_env_tag* otahs_env = (struct otahs_env_tag*) env->env;

    // clear on-going operation
    if(otahs_env->operation != NULL)
    {
        ke_free(otahs_env->operation);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(otahs_env);
}

static void otahs_create(struct prf_task_env* env, uint8_t conidx)
{
    struct otahs_env_tag* otahs_env = (struct otahs_env_tag*) env->env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // force notification config to zero when peer device is connected
    otahs_env->ntf_cfg[conidx] = 0;
}


static void otahs_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct otahs_env_tag* otahs_env = (struct otahs_env_tag*) env->env;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
    otahs_env->ntf_cfg[conidx] = 0;
}

void otahs_indicate_ctrl_val(uint8_t conidx,struct otahs_env_tag* otahs_env, struct otahs_ctrl_val_upd_req const *param)
{
    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(otahs_env->prf_env),0),
            gattc_send_evt_cmd, sizeof(uint8_t)* (param->length));

    // Fill in the parameter structure
    val->operation = GATTC_INDICATE;
    val->handle = otahs_get_att_handle(OTAHS_IDX_CTRL_VAL_VAL);

    // pack measured value in database
    val->length = param->length;
    memcpy(&val->value[0],&param->value[0],param->length);
    // send notification to peer device
    ke_msg_send(val);
}




/// BASS Task interface required by profile manager
const struct prf_task_cbs otahs_itf =
{
        (prf_init_fnct) otahs_init,
        otahs_destroy,
        otahs_create,
        otahs_cleanup,
};


const struct prf_task_cbs* otahs_prf_itf_get(void)
{
   return &otahs_itf;
}


uint16_t otahs_get_att_handle( uint8_t att_idx)
{
        
    struct otahs_env_tag *otahs_env = PRF_ENV_GET(OTAHS, otahs);
    uint16_t handle = ATT_INVALID_HDL;
   
    handle = otahs_env->start_hdl;

    // increment index according to expected index
    if(att_idx <= OTAHS_IDX_CTRL_VAL_IND_CFG)
    {
        handle += att_idx;
    }
    else
    {
        handle = ATT_INVALID_HDL;
    }
    

    return handle;
}

uint8_t otahs_get_att_idx(uint16_t handle, uint8_t *att_idx)
{
    struct otahs_env_tag* otahs_env = PRF_ENV_GET(OTAHS, otahs);
    uint16_t hdl_cursor1 = otahs_env->start_hdl;

    uint8_t status = PRF_APP_ERROR;

    // Browse list of services
    // handle must be greater than current index 
    // check if it's a mandatory index
    if(handle <= (hdl_cursor1 + OTAHS_IDX_DATA_VAL_VAL))
    {
        *att_idx = handle -hdl_cursor1;
        status = GAP_ERR_NO_ERROR;
        
    }
    
    return (status);
}




 
