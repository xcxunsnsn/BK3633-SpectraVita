/**
 ****************************************************************************************
 *
 * @file otahs_task.h
 *
 * @brief Header file - OTAH Service Server Role Task.
 *
 * Copyright (C) RivieraWaves 2019-2022
 *
 *
 ****************************************************************************************
 */


#ifndef _OTAHS_TASK_H_
#define _OTAHS_TASK_H_


#include "rwprf_config.h"
#include <stdint.h>
#include "rwip_task.h" // Task definitions
/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of OTAH Server task instances
#define OTAHS_IDX_MAX     0x01
///Maximal number of OTAH that can be added in the DB

#define  OTAH_CHAR_DATA_LEN  512

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the OTAHS task
enum otahs_state
{
    /// Idle state
    OTAHS_IDLE,
    /// busy state
    OTAHS_BUSY,
    /// Number of defined states.
    OTAHS_STATE_MAX
};

/// Messages for FFF0 Server
enum otahs_msg_id
{
    /// Start the FFF0 Server - at connection used to restore bond data
    OTAHS_CREATE_DB_REQ   = TASK_FIRST_MSG(TASK_ID_OTAHS),
    
    OTAHS_DATA_WRITER_CMD_IND,
    
    OTAHS_CTRL_WRITER_CMD_IND,
    /// CTRL char Value Update Request
    OTAHS_CTRL_VALUE_UPD_REQ,
    /// Inform APP the CTRL value has been send to stack
    OTAHS_CTRL_VALUE_UPD_RSP,
    /// Inform APP that CTRL Value Notification Configuration has been changed - use to update bond data
    OTAHS_CTRL_VALUE_NTF_CFG_IND,

};

/// Features Flag Masks
enum otahs_features
{
    /// CTRL  Characteristic doesn't support notifications
    OTAH_CTRL_VLUAE_NTF_NOT_SUP,
    /// CTRL Value Characteristic support notifications
    OTAH_CTRL_VAL_NTF_SUP,

};


/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters for the database creation
struct otahs_db_cfg
{
    /// Number of OTAH to add
    uint8_t otah_nb;
    /// Features of each FEE0 instance
    uint8_t features;
};

/// Parameters of the @ref OTAHS_CREATE_DB_REQ message
struct otahs_enable_req
{
    /// connection index
    uint8_t  conidx;
    /// Notification Configuration
    uint16_t  ntf_cfg;

};


///Parameters of the @ref OTAHS_CTRL_VALUE_UPD_REQ message
struct otahs_ctrl_val_upd_req
{
    ///  instance
    uint8_t conidx;
    
    uint16_t length;
    /// char value
    uint8_t value[OTAH_CHAR_DATA_LEN];
};




///Parameters of the @ref OTAHS_CTRL_VALUE_NTF_CFG_IND message
struct otahs_ctrl_val_ntf_cfg_ind
{
    /// connection index
    uint8_t  conidx;
    ///Notification Configuration
    uint16_t  ntf_cfg;
};

///Parameters of the @ref OTAHS_CTRL_VALUE_UPD_RSP message
struct otahs_ctrl_val_upd_rsp
{
    /// connection index
    uint8_t  conidx;
    ///status
    uint8_t status;
};



/// Parameters of the @ref OTAHS_DATA_WRITER_CMD_IND message
struct otahs_data_writer_ind
{
  /// Connection index
    uint8_t conidx;
    uint16_t length;
    ///  value
    uint8_t value[OTAH_CHAR_DATA_LEN];

};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler otahs_default_handler;
#endif // BLE_OTAH_SERVER



