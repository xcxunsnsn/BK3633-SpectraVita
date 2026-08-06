/**
 ****************************************************************************************
 *
 * @file app_fcc0.h
 *
 * @brief fee0 Application Module entry point
 *
 * @auth  gang.cheng
 *
 * @date  2020.03.17
 *
 * Copyright (C) Beken 2020-2022
 *
 *
 ****************************************************************************************
 */
#ifndef APP_FCC0_H_
#define APP_FCC0_H_
/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief fcc0 Application Module entry point
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration


#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"         // Kernel Task Definition

/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */

/// fee0s Application Module Environment Structure
struct app_fcc0_env_tag
{
    /// Connection handle
    uint8_t conidx;
    
};
/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// fee0s Application environment
extern struct app_fcc0_env_tag app_fcc0_env;

/// Table of message handlers
extern const struct app_subtask_handlers app_fcc0_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * fcc0s Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize fff0s Application Module
 ****************************************************************************************
 */
void app_fcc0_init(void);
/**
 ****************************************************************************************
 * @brief Add a fee0 Service instance in the DB
 ****************************************************************************************
 */
void app_fcc0_add_fcc0s(void);


/**
 ****************************************************************************************
 * @brief Send a fcc2  value
 ****************************************************************************************
 */
void app_fcc2_send_ntf(uint8_t conidx,uint16_t len,uint8_t* buf);

#endif // APP_FCC0_H_
