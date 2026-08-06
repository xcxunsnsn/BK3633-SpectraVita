/**
 ****************************************************************************************
 *
 * @file fee0s.h
 *
 * @brief Header file - FEE0 Service Server Role
 *
 * Copyright (C) beken 2019-2022
 *
 *
 ****************************************************************************************
 */
#ifndef _FEE0S_H_
#define _FEE0S_H_

/**
 ****************************************************************************************
 * @addtogroup  FFF0 'Profile' Server
 * @ingroup FFF0
 * @brief FFF0 'Profile' Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "rwprf_config.h"

#if (BLE_FEE0_SERVER)

#include "fee0s_task.h"
#include "atts.h"
#include "prf_types.h"
#include "prf.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define FEE0S_CFG_FLAG_MANDATORY_MASK       (0x3FFF)

enum
{        
        ATT_USER_SERVER_FEE0                         = ATT_UUID_16(0xFEE0),    // service 
        ATT_USER_SERVER_CHAR_FEE1                   = ATT_UUID_16(0xFEE1), // read        
        ATT_USER_SERVER_CHAR_FEE2                    = ATT_UUID_16(0xFEE2),// write cmd
        ATT_USER_SERVER_CHAR_FEE3                    = ATT_UUID_16(0xFEE3),// write req
        ATT_USER_SERVER_CHAR_FEE4                    = ATT_UUID_16(0xFEE4), // ntf
        ATT_USER_SERVER_CHAR_FEE5                    = ATT_UUID_16(0xFEE5), // ind
        
};

/// FEE0S Service Attributes Indexes
enum
{
    FEE0S_IDX_SVC,

    FEE0S_IDX_FEE1_VAL_CHAR,
    FEE0S_IDX_FEE1_VAL_VAL,
    FEE0S_IDX_FEE1_USER_DESC,
    
    FEE0S_IDX_FEE2_VAL_CHAR,
    FEE0S_IDX_FEE2_VAL_VAL,

    FEE0S_IDX_FEE3_VAL_CHAR,
    FEE0S_IDX_FEE3_VAL_VAL,
    
    FEE0S_IDX_FEE4_VAL_CHAR,
    FEE0S_IDX_FEE4_VAL_VAL,
    FEE0S_IDX_FEE4_VAL_NTF_CFG,

    FEE0S_IDX_FEE5_VAL_CHAR,
    FEE0S_IDX_FEE5_VAL_VAL,
    FEE0S_IDX_FEE5_VAL_IND_CFG,

    FEE0S_IDX_NB,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// FEE0 'Profile' Server environment variable
struct fee0s_env_tag
{
    /// profile environment
    prf_env_t prf_env;
   
    /// On-going operation
    struct ke_msg * operation;   
    /// List of values set by application
    struct co_list values;
    /// FFF0 Services Start Handle
    uint16_t start_hdl;
    /// value of the Feex
    uint8_t fee1_val[FEE0_CHAR_DATA_LEN];
    uint8_t fee2_val[FEE0_CHAR_DATA_LEN];
    uint8_t fee3_val[FEE0_CHAR_DATA_LEN];
    uint8_t fee4_val[FEE0_CHAR_DATA_LEN];
    uint8_t fee5_val[FEE0_CHAR_DATA_LEN];
    uint8_t fee1_desc[FEE0_CHAR_DATA_LEN];
    uint8_t fee1_desc_len;
    /// BASS task state
    ke_state_t state[FEE0S_IDX_MAX];
    /// Notification configuration of peer devices.
    uint16_t ntf_cfg[BLE_CONNECTION_MAX];
    /// Notification configuration of peer devices.
    uint16_t ind_cfg[BLE_CONNECTION_MAX];
    /// Database features
    uint8_t features;

};



/**
 ****************************************************************************************
 * @brief Retrieve fee0 service profile interface
 *
 * @return fee0 service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* fee0s_prf_itf_get(void);

uint16_t fee0s_get_att_handle(uint8_t att_idx);

uint8_t  fee0s_get_att_idx(uint16_t handle, uint8_t *att_idx);

void fee0s_notify_fee4_val(uint8_t conidx,struct fee0s_env_tag* fee0s_env, struct fee0s_fee45_val_upd_req const *param);

void fee0s_indicate_fee5_val(uint8_t conidx,struct fee0s_env_tag* fee0s_env, struct fee0s_fee45_val_upd_req const *param);
void fee0s_task_init(struct ke_task_desc *task_desc);

#endif /* #if (BLE_FEE0_SERVERs) */



#endif /*  _FEE0_H_ */



