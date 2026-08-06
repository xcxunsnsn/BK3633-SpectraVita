/**
 ****************************************************************************************
 *
 * @file fcc0s.h
 *
 * @brief Header file - FCC0 Service Server Role
 *
 * Copyright (C) beken 2019-2022
 *
 *
 ****************************************************************************************
 */
#ifndef _FCC0S_H_
#define _FCC0S_H_

/**
 ****************************************************************************************
 * @addtogroup  FCC0 'Profile' Server
 * @ingroup FC0
 * @brief FCC0 'Profile' Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "rwprf_config.h"

#if (BLE_FCC0_SERVER)

#include "fcc0s_task.h"
#include "atts.h"
#include "prf_types.h"
#include "prf.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define FCC0S_CFG_FLAG_MANDATORY_MASK       (0x1FFF)

/// FCC0S Service Attributes Indexes
enum
{
    FCC0S_IDX_SVC,

    FCC0S_IDX_FCC1_VAL_CHAR,
    FCC0S_IDX_FCC1_VAL_VAL,
   
    FCC0S_IDX_FCC2_VAL_CHAR,
    FCC0S_IDX_FCC2_VAL_VAL,
    FCC0S_IDX_FCC2_VAL_IND_CFG,


    FCC0S_IDX_NB,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// FCC0 'Profile' Server environment variable
struct fcc0s_env_tag
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
    uint8_t fcc1_val[FCC0_CHAR_DATA_LEN];
    uint8_t fcc2_val[FCC0_CHAR_DATA_LEN];
   
    /// BASS task state
    ke_state_t state[FCC0S_IDX_MAX];
    /// Notification configuration of peer devices.
    uint16_t ntf_cfg[BLE_CONNECTION_MAX];
    /// Database features
    uint8_t features;

};



/**
 ****************************************************************************************
 * @brief Retrieve fcc0 service profile interface
 *
 * @return fee0 service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* fcc0s_prf_itf_get(void);

uint16_t fcc0s_get_att_handle(uint8_t att_idx);

uint8_t  fcc0s_get_att_idx(uint16_t handle, uint8_t *att_idx);

void fcc0s_indicate_fcc2_val(uint8_t conidx,struct fcc0s_env_tag* env, struct fcc0s_fcc2_val_upd_req const *param);

void fcc0s_task_init(struct ke_task_desc *task_desc);

#endif /* #if (BLE_FCC0_SERVERs) */



#endif /*  _FEE0_H_ */



