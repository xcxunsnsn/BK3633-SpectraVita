/**
 ****************************************************************************************
 *
 * @file fee0s_task.h
 *
 * @brief Header file - Battery Service Server Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */


#ifndef _FEE0S_TASK_H_
#define _FEE0S_TASK_H_


#include "rwprf_config.h"
#if (BLE_FEE0_SERVER)
#include <stdint.h>
#include "rwip_task.h" // Task definitions
/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of FFF0 Server task instances
#define FEE0S_IDX_MAX     0x01
///Maximal number of FEE0 that can be added in the DB

#define  FEE0_CHAR_DATA_LEN  628

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the FFF0S task
enum fee0s_state
{
    /// Idle state
    FEE0S_IDLE,
    /// busy state
    FEE0S_BUSY,
    /// Number of defined states.
    FEE0S_STATE_MAX
};

/// Messages for FFF0 Server
enum fee0s_msg_id
{
    /// Start the FFF0 Server - at connection used to restore bond data
    FEE0S_CREATE_DB_REQ   = TASK_FIRST_MSG(TASK_ID_FEE0S),
    
    FEE0S_FEE1_VALUE_READ_IND,
    FEE0S_FEE2_WRITER_CMD_IND,
    FEE0S_FEE3_WRITER_REQ_IND,

    
    /// FFF1 char Value Update Request
    FEE0S_FEE4_VALUE_UPD_REQ,
    /// Inform APP the FEE4 value has been send to stack
    FEE0S_FEE4_VALUE_UPD_RSP,
    /// Inform APP that FFF1 Level Notification Configuration has been changed - use to update bond data
    FEE0S_FEE4_VALUE_NTF_CFG_IND,

    /// FEE5 Level Value Update Request
    FEE0S_FEE5_VALUE_UPD_REQ,
    /// Inform APP if FFF1 Level value has been notified or not
    FEE0S_FEE5_VALUE_UPD_RSP,

    FEE0S_FEE5_VALUE_IND_CFG_IND,
    


};

/// Features Flag Masks
enum fee0s_features
{
    /// FEEX  Characteristic doesn't support notifications
    FEE0_FEEX_VLUAE_NTF_NOT_SUP,
    /// FEE4 Value Characteristic support notifications
    FEE0_FEE4_VAL_NTF_SUP,
    /// FEE5 Value Characteristic support indication
    FEE0_FEE5_VAL_IND_SUP,
};


/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters for the database creation
struct fee0s_db_cfg
{
    /// Number of FEE0 to add
    uint8_t fee0_nb;
    /// Features of each FEE0 instance
    uint8_t features;
    
    uint16_t fee1_desc_len;
    uint8_t fee1_desc[FEE0_CHAR_DATA_LEN];
};

/// Parameters of the @ref FEE0S_CREATE_DB_REQ message
struct fee0s_enable_req
{
    /// connection index
    uint8_t  conidx;
    /// Notification Configuration
    uint16_t  ntf_cfg;

};


///Parameters of the @ref FEE0S_FEEx_VALUE_UPD_REQ message
struct fee0s_fee45_val_upd_req
{
    ///  instance
    uint8_t conidx;
    
    uint16_t length;
    /// char value
    uint8_t value[FEE0_CHAR_DATA_LEN];
};


///Parameters of the @ref FEE0S_FEE4_VALUE_UPD_RSP message
struct fee0s_fee45_val_upd_rsp
{
    /// connection index
    uint8_t  conidx;
    ///status
    uint8_t status;
};

///Parameters of the @ref FEE0S_FEE4_VALUE_NTF_CFG_IND message
struct fee0s_fee4_val_ntf_cfg_ind
{
    /// connection index
    uint8_t  conidx;
    ///Notification Configuration
    uint16_t  ntf_cfg;
};


///Parameters of the @ref FEE0S_FEE5_VALUE_IND_CFG_IND message
struct fee0s_fee5_val_ind_cfg_ind
{
    /// connection index
    uint8_t  conidx;
    ///Ind Configuration
    uint16_t  ind_cfg;
};

/// Parameters of the @ref FEE0S_FEE2_WRITER_CMD_IND message
struct fee0s_fee23_writer_ind
{
  /// Connection index
    uint8_t conidx;
    uint16_t length;
    ///  value
    uint8_t value[FEE0_CHAR_DATA_LEN];

};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler fee0s_default_handler;
#endif // BLE_FEE0_SERVER


#endif /* _FEE0S_TASK_H_ */

