/**
 ****************************************************************************************
 *
 * @file lb.h
 *
 * @brief Main API file for the Link Broadcaster
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 ****************************************************************************************
 */

#ifndef LB_H_
#define LB_H_

/**
 ****************************************************************************************
 * @defgroup LB Link Broadcaster
 * @ingroup ROOT
 * @brief BT Lower Layers
 *
 * The CONTROLLER contains the modules allowing the physical link establishment,
 * maintenance and management.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_task.h"    // Task definitions
#include "ke_task.h"      // kernel task definitions


/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the LB task
/*@TRACE*/
enum lb_msg_id
{
    LB_MSG_ID_FIRST = TASK_FIRST_MSG(TASK_ID_LB),

    /*
     * ************** Msg LC -> LB****************
     */
    LB_MST_KEY_CFM,
    LB_ENC_RESTART_CFM,
    LB_ENC_START_IND,
    LB_ENC_STOP_IND,

    /*
     * ************** Msg LD -> LB****************
     */
    LB_ACL_TX_CFM,
    LB_CSB_TX_END_IND,
    LB_CSB_RX_IND,
    LB_CSB_RX_END_IND,
    LB_AFH_UPDATE_IND,
    LB_AFH_REPORT_TO,
    LB_SSCAN_END_IND,
    LB_STRAIN_END_IND,

#if PCA_SUPPORT
    /*
     * ************** Msg LB->LB****************
     */
    LB_PCA_TX_INTV_TO,
#endif // PCA_SUPPORT
};

/// LB master key confirmation
/*@TRACE*/
struct lb_mst_key_cfm
{
    ///Status
    uint8_t status;
    ///Encryption mode
    uint8_t enc_mode;
    ///Key size mask
    uint16_t key_sz_msk;
};

/// LB encryption restart confirmation
/*@TRACE*/
struct lb_enc_restart_cfm
{
    ///Status
    uint8_t status;
};

/// LB encryption start indication
/*@TRACE*/
struct lb_enc_start_ind
{
    ///Key flag
    uint8_t key_flag;
};

/// LB ACL transmission confirmation structure
/*@TRACE*/
struct lb_acl_tx_cfm
{
    /// EM buffer containing PDU
    uint16_t em_buf;
};

/// LB CSB TX end indication
/*@TRACE*/
struct lb_csb_tx_end_ind
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// LT_ADDR
    uint8_t            lt_addr;
};

/// LB CST RX indication
/*@TRACE*/
struct lb_csb_rx_ind
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// LT_ADDR
    uint8_t            lt_addr;
    /// CLK when CSB data was received
    uint32_t clk;
    /// (CLKNslave - CLK) modulo 2^28
    uint32_t offset;
    /// receive status
    uint8_t rx_status;
    /// PDU length
    uint16_t length;
    /// PDU (packed)
    uint8_t pdu[__ARRAY_EMPTY];
};

/// LB CSB RX end indication
/*@TRACE*/
struct lb_csb_rx_end_ind
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// LT_ADDR
    uint8_t            lt_addr;
};

/// LB CSB AFH update indication
/*@TRACE*/
struct lb_afh_update_ind
{
    /// AFH_Channel_Map
    struct chnl_map    afh_ch_map;
};

/// LB CSB AFH report timeout
/*@TRACE*/
struct lb_afh_report_to
{
    /// BD_ADDR
    struct bd_addr     bd_addr;
};

/// LB Sync Scan end indication
/*@TRACE*/
struct lb_sscan_end_ind
{
    /// Status
    uint8_t            status;
    /// BD_ADDR
    struct bd_addr     bd_addr;
    /// Clock_Offset (28 bits) - (CLKNslave - CLK) modulo 2^28
    uint32_t           clock_offset;
#if PCA_SUPPORT
    /// Bit offset (CLKNslave - CLK) modulo 2^28
    int16_t            bit_offset;
#endif // PCA_SUPPORT
    /// AFH_Channel_Map
    struct chnl_map    afh_ch_map;
    /// LT_ADDR
    uint8_t            lt_addr;
    /// Next_Broadcast_Instant (28 bits)
    uint32_t next_bcst_instant;
    /// Connectionless_Slave_Broadcast_Interval (in slots)
    uint16_t csb_int;
    /// Service_Data
    uint8_t service_data;
    ///Coarse clock adjust use
    bool coarse_clk_adj;
};

/// LB Sync Train end indication
/*@TRACE*/
struct lb_strain_end_ind
{
    ///Status
    uint8_t status;
    ///Coarse clock adjust use
    bool coarse_clk_adj;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BT LB task
 *
 * This function initializes the the LB task, as well as the environment of the LB
 ****************************************************************************************
 */
void lb_init(void);

/**
 ****************************************************************************************
 * @brief Reset the BT LB task
 ****************************************************************************************
 */
void lb_reset(void);

/**
 ****************************************************************************************
 * @brief Get num broadcast retransmissions
 ****************************************************************************************
 */
uint8_t lb_util_get_nb_broadcast(void);

/**
 ****************************************************************************************
 * @brief Set num broadcast retransmissions
 ****************************************************************************************
 */
void lb_util_set_nb_broadcast(uint8_t num_bcst_ret);

/**
 ****************************************************************************************
 * @brief Get reserved lt_addr
 ****************************************************************************************
 */
uint8_t lb_util_get_res_lt_addr(void);

/**
 ****************************************************************************************
 * @brief Get CSB mode (active/inactive)
 ****************************************************************************************
 */
bool lb_util_get_csb_mode(void);

/**
******************************************************************************************
* @brief Function called to pack and broadcast an LMP PDU.
 ****************************************************************************************
 */
void lb_send_lmp(void *param, uint32_t clock, uint8_t num_bcst);

//Functions dedicated to send broadcast LMP
void lb_send_pdu_clk_adj(uint32_t clk_adj_instant, uint8_t clk_adj_id, int16_t clk_adj_us, uint8_t clk_adj_slots, uint8_t clk_adj_mode, uint32_t clk_adj_clk,  uint8_t tr_id);

/**
 ****************************************************************************************
 * @brief This function is used to actiavte  the coarse clock adjustment procedure
 *
 ****************************************************************************************
 */
void lb_clk_adj_activate(int16_t clk_adj_us, uint8_t clk_adj_slots, uint8_t clk_adj_period, uint8_t tr_id);


/**
 ****************************************************************************************
 * @brief This function updates the remote clock adjust period of slave devices
 *
 ****************************************************************************************
 */
void lb_clk_adj_period_update(uint8_t clk_adj_period);

/**
 ****************************************************************************************
 * @brief This function is used to get the activce clk_adj ID.
 *
 ****************************************************************************************
 */
uint8_t lb_clk_adj_id_get(void);

/**
 ****************************************************************************************
 * @brief This function processes the coarse clock adjustment acknowledgements
 *
 ****************************************************************************************
 */
void lb_clk_adj_ack(int idx, uint8_t clk_adj_id);

/**
 ****************************************************************************************
 * @brief This function read the current master key.
 *
 * @param[out] MasterKey       Master key with LTK size
 ****************************************************************************************
 */
void LM_GetMasterKey(struct ltk *MasterKey);

/**
 ****************************************************************************************
 * @brief This function read the current master key random.
 *
 * @param[out] MasterRand      Master key random
 ****************************************************************************************
 */
void LM_GetMasterKeyRand(struct ltk *MasterRand);

/**
 ****************************************************************************************
 * @brief This function read the current Random vector for broadcast encryption.
 *
 * @param[out] MasterRand          Random Vector
 ****************************************************************************************
 */
void LM_GetMasterEncRand(struct ltk *MasterRand);

/**
 ****************************************************************************************
 * @brief This function read Broadcast Encryption Key Size.
 *
 * @return Encryption Key Size
 ****************************************************************************************
 */
uint8_t LM_GetMasterEncKeySize(void);

/// @} LB

#endif // LB_H_
