/**
 ****************************************************************************************
 *
 * @file otas.h
 *
 * @brief Header file - OTA Profile Server Role
 *
 * Copyright (C) beken 2009-2015
 *
 *
 ****************************************************************************************
 */
#ifndef _PROV_H_
#define _PROV_H_

/**
 ****************************************************************************************
 * @addtogroup  OTA 'Profile' Server
 * @ingroup OTAS
 * @brief OTAS 'Profile' Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "rwprf_config.h"

#if (BLE_PROV_SERVER)

#include "prov_task.h"
#include "atts.h"
#include "prf_types.h"
#include "prf.h"

/*
 * DEFINES
 ****************************************************************************************
 */
 
 
enum m_idx_prov
{
    /// Service Declaration
    M_IDX_PROV_SVC = 0,

    /// Data In Characteristic
    M_IDX_PROV_DATA_IN_CHAR,
    M_IDX_PROV_DATA_IN_VAL,

    /// Data Out Characteristic
    M_IDX_PROV_DATA_OUT_CHAR,
    M_IDX_PROV_DATA_OUT_VAL,
    M_IDX_PROV_DATA_OUT_CCC,

    M_IDX_PROV_NB
};



#define PROV_MANDATORY_MASK (0xFFFFFFFF)



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// OAD 'Profile' Server environment variable
struct prov_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// On-going operation
    struct ke_msg * operation;
    ///  Services Start Handle
    uint16_t prov_start_hdl; 
    ///  control point char
    uint8_t data_in_value[MAL_CON_MAX_PROV_DATA_LEN]; 
    /// alarm 1 char
    uint8_t data_out_value[MAL_CON_MAX_PROV_DATA_LEN];
    /// prov task state
    ke_state_t state[PROV_IDX_MAX];
    /// Notification configuration of peer devices.
    uint8_t data_out_ntf_cfg[BLE_CONNECTION_MAX];
    /// Database features
    uint8_t features;

    uint8_t cursor;

};



/**
 ****************************************************************************************
 * @brief Retrieve Bracelet service profile interface
 *
 * @return Brace service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* prov_prf_itf_get(void);

    
#endif 



#endif /*  _OTAS_H_ */





