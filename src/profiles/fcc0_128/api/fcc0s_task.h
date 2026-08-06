/**
 ****************************************************************************************
 *
 * @file fcc0s_task.h
 *
 * @brief Header file - FCC0 Service Server Role Task.
 *
 * Copyright (C) RivieraWaves 2019-2022
 *
 *
 ****************************************************************************************
 */


#ifndef _FCC0S_TASK_H_
#define _FCC0S_TASK_H_


#include "rwprf_config.h"
#if (BLE_FCC0_SERVER)
#include <stdint.h>
#include "rwip_task.h" // Task definitions
/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of FCC0 Server task instances
#define FCC0S_IDX_MAX     0x01
///Maximal number of FCC0 that can be added in the DB

#define  FCC0_CHAR_DATA_LEN  512

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the FCC0S task
enum fcc0s_state
{
    /// Idle state
    FCC0S_IDLE,
    /// busy state
    FCC0S_BUSY,
    /// Number of defined states.
    FCC0S_STATE_MAX
};

/// Messages for FFF0 Server
enum fcc0s_msg_id
{
    /// Start the FFF0 Server - at connection used to restore bond data
    FCC0S_CREATE_DB_REQ   = TASK_FIRST_MSG(TASK_ID_FCC0S),
    
    FCC0S_FCC1_WRITER_CMD_IND,

    /// FCC2 char Value Update Request
    FCC0S_FCC2_VALUE_UPD_REQ,
    /// Inform APP the FCC2 value has been send to stack
    FCC0S_FCC2_VALUE_UPD_RSP,
    /// Inform APP that FCC2 Value Notification Configuration has been changed - use to update bond data
    FCC0S_FCC2_VALUE_NTF_CFG_IND,

};

/// Features Flag Masks
enum fcc0s_features
{
    /// FCC2  Characteristic doesn't support notifications
    FCC0_FCC2_VLUAE_NTF_NOT_SUP,
    /// FCC2 Value Characteristic support notifications
    FCC0_FCC2_VAL_NTF_SUP,

};


/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters for the database creation
struct fcc0s_db_cfg
{
    /// Number of FCC0 to add
    uint8_t fcc0_nb;
    /// Features of each FEE0 instance
    uint8_t features;
};

/// Parameters of the @ref FCC0S_CREATE_DB_REQ message
struct fcc0s_enable_req
{
    /// connection index
    uint8_t  conidx;
    /// Notification Configuration
    uint16_t  ntf_cfg;

};


///Parameters of the @ref FCC0S_FCC2_VALUE_UPD_REQ message
struct fcc0s_fcc2_val_upd_req
{
    ///  instance
    uint8_t conidx;
    
    uint16_t length;
    /// char value
    uint8_t value[FCC0_CHAR_DATA_LEN];
};




///Parameters of the @ref FCC0S_FCC2_VALUE_NTF_CFG_IND message
struct fcc0s_fcc2_val_ntf_cfg_ind
{
    /// connection index
    uint8_t  conidx;
    ///Notification Configuration
    uint16_t  ntf_cfg;
};

///Parameters of the @ref FCC0S_FCC2_VALUE_UPD_RSP message
struct fcc0s_fcc2_val_upd_rsp
{
    /// connection index
    uint8_t  conidx;
    ///status
    uint8_t status;
};



/// Parameters of the @ref FCC0S_FCC1_WRITER_CMD_IND message
struct fcc0s_fcc1_writer_ind
{
  /// Connection index
    uint8_t conidx;
    uint16_t length;
    ///  value
    uint8_t value[FCC0_CHAR_DATA_LEN];

};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler fcc0s_default_handler;
#endif // BLE_FCC0_SERVER


#endif /* _FCC0S_TASK_H_ */

