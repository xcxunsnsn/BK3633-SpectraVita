/**
 ****************************************************************************************
 *
 * @file app_fee0.c
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
#ifndef APP_FEE0_H_
#define APP_FEE0_H_
/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief fee0 Application Module entry point
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
struct app_fee0_env_tag
{
    /// Connection handle
    uint8_t conidx;
    /// Current Battery Level
    uint8_t fee1_lvl;
    uint8_t fee3_lvl;
};
/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// fee0s Application environment
extern struct app_fee0_env_tag app_fee0_env;

/// Table of message handlers
extern const struct app_subtask_handlers app_fee0_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * fff0s Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize fff0s Application Module
 ****************************************************************************************
 */
void app_fee0_init(void);
/**
 ****************************************************************************************
 * @brief Add a fee0 Service instance in the DB
 ****************************************************************************************
 */
void app_fee0_add_fee0s(void);
/**
 ****************************************************************************************
 * @brief Enable the fee0 Service
 ****************************************************************************************
 */
void app_fee4_send_ntf(uint8_t conidx,uint16_t len,uint8_t* buf);
/**
 ****************************************************************************************
 * @brief Send a fee5  value
 ****************************************************************************************
 */
void app_fee5_send_ind(uint8_t conidx,uint16_t len,uint8_t* buf);


#endif // APP_FEE0_H_
