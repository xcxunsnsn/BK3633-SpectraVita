/**
****************************************************************************************
*
* @file lc_sco.h
*
* @brief Definitions for LC SCO procedures
*
* Copyright (C) RivieraWaves 2009-2015
*
*
****************************************************************************************
*/

#ifndef LC_SCO_H_
#define LC_SCO_H_

/**
 ****************************************************************************************
 * @defgroup LCSCO Synchronous Connection Oriented connection
 * @ingroup LC
 * @brief Responsible for handling SCO procedures.
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"    // stack configuration

#if (MAX_NB_SYNC > 0)

#include <stdint.h>         // integer
#include <stdbool.h>        // boolean
#include "ke_task.h"        // kernel task

/*
 * CONSTANT DEFINITIONS
 ****************************************************************************************
 */


/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */

struct lc_sco_air_params_tag
{
    ///Transaction ID
    uint8_t  tr_id;
    ///eSCO handle
    uint8_t  esco_hdl;
    ///eSCo LT Address
    uint8_t  esco_lt_addr;
    ///timing control flags
    uint8_t  flags;
    ///Desco
    uint8_t  d_esco;
    ///t_esco
    uint8_t  t_esco;
    ///Wesco
    uint8_t  w_esco;
    ///eSCO packet type M2S
    uint8_t  m2s_pkt_type;
    ///eSCO packet type S2M
    uint8_t  s2m_pkt_type;
    ///Packet Length M2S
    uint16_t m2s_pkt_len;
    ///Packet Length S2m
    uint16_t s2m_pkt_len;
    ///Air Mode
    uint8_t  air_mode;
    ///Negotiation state
    uint8_t  negt_st;
};

struct lc_sco_host_params_tag
{
    uint32_t      tx_bw;                // Transmit Bandwidth (in B/sec)
    uint32_t      rx_bw;                // Receive Bandwidth (in B/sec)
    uint8_t       tx_cod_fmt[5];        // Transmit Coding Format
    uint8_t       rx_cod_fmt[5];        // Receive Coding Format
    uint16_t      tx_cod_fr_sz;         // Transmit Codec Frame Size (in B)
    uint16_t      rx_cod_fr_sz;         // Receive Codec Frame Size (in B)
    uint32_t      in_bw;                // Input Bandwidth (in B/sec)
    uint32_t      out_bw;               // Output Bandwidth (in B/sec)
    uint8_t       in_cod_fmt[5];        // Input Coding Format
    uint8_t       out_cod_fmt[5];       // Output Coding Format
    uint16_t      in_cod_data_sz;       // Input Coded Data Size (in bits)
    uint16_t      out_cod_data_sz;      // Output Coded Data Size (in bits)
    uint8_t       in_data_fmt;          // Input PCM Data Format
    uint8_t       out_data_fmt;         // Output PCM Data Format
    uint8_t       in_msb_pos;           // Input PCM Sample Payload MSB Position (in bits)
    uint8_t       out_msb_pos;          // Output PCM Sample Payload MSB Position (in bits)
    uint8_t       in_data_path;         // Input Data Path (VoHCI / PCM / Other ...)
    uint8_t       out_data_path;        // Output Data Path
    uint8_t       in_tr_unit_sz;        // Input Transport Unit Size (in bits)
    uint8_t       out_tr_unit_sz;       // Output Transport Unit Size (in bits)
    uint16_t      max_lat;              // Max Latency (in ms)
    uint16_t      packet_type;          // Packet Type (HV1 / HV2 / HV3 / EV3 / EV4 / EV5 / 2EV3 / 3EV3 / 2EV5 / 3EV5)
    uint8_t       retx_eff;             // Retransmission Effort (No, opt power, opt quality, don't care)
};


/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the LC_SCO module
 ****************************************************************************************
 */
void lc_sco_init(void);

/**
 ****************************************************************************************
 * @brief Reset LC_SCO module
 ****************************************************************************************
 */
void lc_sco_reset(void);

/**
 ****************************************************************************************
 * @brief Detach all SCO connections on a ACL link
 *
 * This function starts the disconnection of all SCO links on a ACL connection.
 *
 * When the full disconnection procedure is requested, each disconnection procedure
 * loops to this function to restart the next disconnection until no more SCO on the ACL
 * is present.
 *
 * Host is NOT notified for each SCO link disconnection.
 *
 * param[in]    pid      Process identifier (task / instance)
 * param[in]    reason   Detach reason (error code)
 *
 * return status
 ****************************************************************************************
 */
uint8_t lc_sco_detach(ke_task_id_t pid, uint8_t reason);

/**
 ****************************************************************************************
 * @brief Release all SCO connections on a ACL link
 *
 * This function releases all SCO links on a ACL connection, without notification to the
 * peer device. However, Host is notified for each SCO link disconnection.
 *
 * param[in]    pid      Process identifier (task / instance)
 * param[in]    reason   Detach reason (error code)
 ****************************************************************************************
 */
void lc_sco_release(ke_task_id_t pid, uint8_t reason);

uint8_t lc_sco_host_request(ke_task_id_t pid, bool old_style, bool host_initiated, uint16_t conhdl, struct lc_sco_host_params_tag * p_host_params);
uint8_t lc_sco_host_accept(ke_task_id_t pid, bool old_style, bool very_old_style, struct lc_sco_host_params_tag * p_host_params);
void lc_sco_host_reject(ke_task_id_t pid, bool very_old_style, uint8_t status);
uint8_t lc_sco_host_request_disc(ke_task_id_t pid, uint16_t conhdl, uint8_t reason);

void lc_sco_peer_request(ke_task_id_t pid, uint8_t req_type, struct lc_sco_air_params_tag * p_peer_params);
void lc_sco_peer_accept(ke_task_id_t pid);
void lc_sco_peer_reject(ke_task_id_t pid, uint8_t reason);

void lc_sco_peer_request_disc(ke_task_id_t pid, uint8_t req_type, uint8_t scohdl, uint8_t reason);
void lc_sco_peer_accept_disc(ke_task_id_t pid);
void lc_sco_peer_reject_disc(ke_task_id_t pid, uint8_t reason);

void lc_sco_baseband_ack(ke_task_id_t pid, uint8_t lmp_opcode);
void lc_sco_timeout(ke_task_id_t pid);

#endif // (MAX_NB_SYNC > 0)

/// @} LCSCO
#endif // LC_SCO_H_
