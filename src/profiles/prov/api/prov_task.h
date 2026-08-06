/**
 ****************************************************************************************
 *
 * @file otas_task.h
 *
 * @brief Header file - ota profile Server Role Task.
 *
 * Copyright (C) BeKen 2009-2015
 *
 *
 ****************************************************************************************
 */


#ifndef _PROV_TASK_H_
#define _PROV_TASK_H_


#include "rwprf_config.h"
#if (BLE_PROV_SERVER)
#include <stdint.h>
#include "prf_types.h"

#include "rwip_task.h" // Task definitions
//#include "ke_task.h"


/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of  Server task instances
#define PROV_IDX_MAX     0x01
///Maximal number of BRACES that can be added in the DB



#define MAL_CON_MAX_PROV_DATA_LEN       (69)



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
 

/// Possible states of the braces task
enum PROV_state
{
    /// Idle state
    PROV_IDLE,
    /// busy state
    PROV_BUSY,
    /// Number of defined states.
    PROV_STATE_MAX
};



/// Features Flag Masks
enum PROV_features
{
    ///  Characteristic doesn't support notifications
    PROV_NTF_NOT_SUP,
    
    PROV_NTF_SUP,
};

/*
 * APIs Structures
 ****************************************************************************************
 */
/// Parameters for the database creation
struct prov_db_cfg
{
    /// Number of  to add
    uint8_t prov_nb;
    /// Features of each  instance
    uint8_t features;
   };

/// Parameters of the @ref BRACES_ENABLE_REQ message
struct prov_enable_req
{
    /// connection index
    uint8_t  conidx;
    /// Notification Configuration
    uint16_t  dataout_ntf_cfg;

};



/// Parameters of the @ref PROV_ENABLE_RSP message
struct prov_enable_rsp
{
    /// connection index
    uint8_t conidx;
    ///status
    uint8_t status;
};

///Parameters of the @ref PROV_FFC1_WRITER_REQ_IND message
struct prov_data_in_writer_ind
{
    /// instance
    uint8_t conidx;
    
    uint8_t length;
    /// data
    uint8_t data[MAL_CON_MAX_PROV_DATA_LEN];
};




/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler prov_default_handler;
void prov_task_init(struct ke_task_desc *task_desc);

#endif // BLE_BRACELET_REPORTER


#endif /* _BRACES_TASK_H_ */

