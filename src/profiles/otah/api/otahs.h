/**
 ****************************************************************************************
 *
 * @file otahs.h
 *
 * @brief Header file - OTAH Service Server Role
 *
 * Copyright (C) beken 2019-2022
 *
 *
 ****************************************************************************************
 */
#ifndef _OTAHS_H_
#define _OTAHS_H_

/**
 ****************************************************************************************
 * @addtogroup  OTAH 'Profile' Server
 * @ingroup FC0
 * @brief OTAH 'Profile' Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "rwprf_config.h"


#include "otahs_task.h"
#include "atts.h"
#include "prf_types.h"
#include "prf.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define OTAHS_CFG_FLAG_MANDATORY_MASK       (0x1FFF)

/// OTAHS Service Attributes Indexes
enum
{
    OTAHS_IDX_SVC,

   
   
    OTAHS_IDX_CTRL_VAL_CHAR,
    OTAHS_IDX_CTRL_VAL_VAL,
    OTAHS_IDX_CTRL_VAL_IND_CFG,

     OTAHS_IDX_DATA_VAL_CHAR,
    OTAHS_IDX_DATA_VAL_VAL,
    OTAHS_IDX_NB,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// OTAH 'Profile' Server environment variable
struct otahs_env_tag
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
    uint8_t data_val[OTAH_CHAR_DATA_LEN];
    uint8_t ctrl_val[OTAH_CHAR_DATA_LEN];
   
    /// BASS task state
    ke_state_t state[OTAHS_IDX_MAX];
    /// Notification configuration of peer devices.
    uint16_t ntf_cfg[BLE_CONNECTION_MAX];
    /// Database features
    uint8_t features;

};



/**
 ****************************************************************************************
 * @brief Retrieve otah service profile interface
 *
 * @return fee0 service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* otahs_prf_itf_get(void);

uint16_t otahs_get_att_handle(uint8_t att_idx);

uint8_t  otahs_get_att_idx(uint16_t handle, uint8_t *att_idx);

void otahs_indicate_ctrl_val(uint8_t conidx,struct otahs_env_tag* env, struct otahs_ctrl_val_upd_req const *param);

void otahs_task_init(struct ke_task_desc *task_desc);




#endif /*  _FEE0_H_ */



