/**
 ****************************************************************************************
 *
 * @file bt_util_lmp.h
 *
 * @brief Main API file for the Link Manager Protocol
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef BT_UTIL_LMP_H_
#define BT_UTIL_LMP_H_

/**
 ****************************************************************************************
 * @defgroup BT_UTIL_LMP Link Manager Protocol
 * @ingroup ROOT
 * @brief Link Manager Protocol
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>          // standard integer


/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum size of LMP unpacked parameters
#define LMP_MAX_UNPACKED_SIZE     (LMP_MAX_PDU_SIZE+10)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Status returned by generic packer-unpacker
enum LMP_PACK_STATUS
{
    LMP_PACK_OK,
    LMP_PACK_OVERFLOW,
    LMP_PACK_WRONG_FORMAT,
    LMP_PACK_INVALID_SIZE,
    LMP_PACK_UNKNOWN,
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Check if LMP packet is allowed at connection establishment
 *
 * @param[in] opcode      LMP opcode
 * @param[in] ext_opcode  LMP extended opcode (if applicable)
 *
 * @return  True if allowed, false otherwise
 ****************************************************************************************
 */
bool lmp_estab_allowed(uint8_t opcode, uint8_t ext_opcode);

/**
 ****************************************************************************************
 * @brief Pack LMP packet
 *
 * @param[inout] buf      Buffer containing parameters
 * @param[inout] p_len    Pointer on parameters length
 *
 * @return  Status of the packing operation
 ****************************************************************************************
 */
uint8_t lmp_pack(uint8_t* buf, uint8_t* p_len);

/**
 ****************************************************************************************
 * @brief Unpack LMP packet
 *
 * @param[in]    in       Buffer containing packed parameters
 * @param[out]   out      Buffer containing unpacked parameters
 * @param[inout] p_len    Pointer on parameters length
 *
 * @return  Status of the unpacking operation
 ****************************************************************************************
 */
uint8_t lmp_unpack(uint8_t* out, uint8_t* in, uint8_t* p_len);

/// @} BT_UTIL_LMP

#endif // BT_UTIL_LMP_H_
