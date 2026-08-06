/**
 ****************************************************************************************
 *
 * @file lm.h
 *
 * @brief Main API file for the Link manager
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 ****************************************************************************************
 */

#ifndef LM_H_
#define LM_H_

/**
 ****************************************************************************************
 * @defgroup LM Link Manager
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
#include "rwip.h"         // Type definitions
#include "rwip_task.h"    // Task definitions
#include "ke_task.h"      // kernel task definitions
#include "co_bt.h"        // BT standard definitions
#include "ld.h"           // link driver definitions

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * MESSAGES
 ****************************************************************************************
 */

/// Message API of the LM task
/*@TRACE*/
enum lm_msg_id
{
    LM_MSG_ID_FIRST = TASK_FIRST_MSG(TASK_ID_LM),

    /*
     * ************** Msg LD->LM****************
     */
    LM_INQ_RES_IND,
    LM_INQ_END_IND,
    LM_PAGE_END_IND,
    LM_PAGE_SCAN_END_IND,
    #if CSB_SUPPORT
    LM_PAGE_RESP_TO_IND,
    #endif //CSB_SUPPORT
    #if PCA_SUPPORT
    LM_LOCAL_PCA_REQ,
    LM_PCA_END_IND,
    LM_PCA_MONITOR_INTV_TO,
    LM_PCA_SSCAN_START_REQ,
    #endif // PCA_SUPPORT
    LM_SAM_CONFIG_REQ,
    #if RW_BT_MWS_COEX
    LM_MWS_PATTERN_IND,
    #endif // RW_BT_MWS_COEX
    /*
     * ************** Msg LM->LM****************
     */
    LM_AFH_TO,
    LM_ECC_RESULT_IND,
};


/*
 * ************** API LD->LM ****************
 */

/// Inquiry result indication structure
/*@TRACE*/
struct lm_inq_res_ind
{
    /// BdAddr
    struct bd_addr  bd_addr;
    /// Page Scan Repetition Mode
    uint8_t         page_scan_rep_mode;
    /// Class of device
    struct devclass class_of_dev;
    /// Clock Offset | Bit 15 Reserved | Bits 14-0: Bits 16-2 of CLKNslave-CLK
    uint16_t        clk_off;
    /// RSSi
    uint8_t         rssi;
    /// EIR bit in FHS packet
    bool            fhs_eir_bit;
    /// Extended inquiry response length
    uint8_t         eir_len;
    /// Extended inquiry response data
    uint8_t         eir_data[__ARRAY_EMPTY];
};

/// Page end indication structure
/*@TRACE*/
struct lm_page_end_ind
{
    /// Link identifier
    uint8_t link_id;
    /// Status (BT error code)
    uint8_t status;
};

/// Page scan end indication structure
/*@TRACE*/
struct lm_page_scan_end_ind
{
    /// Link identifier
    uint8_t link_id;
    /// Peer BdAddr
    struct bd_addr peer_bd_addr;
    // Peer Class of Device
    struct devclass class_of_dev;
    // Slave timing information
    struct ld_slave_timing_info slave_timing_info;
};

#if PCA_SUPPORT
/// LM PCA local adjustment request structure
/*@TRACE*/
struct lm_local_pca_req
{
    /// Clock adjusted us
    int16_t clk_adj_us;
    /// Clock adjusted slots
    uint8_t clk_adj_slots;
    /// Clock adjusted period
    uint8_t clk_adj_period;
};

/// LM PCA adjustment complete indication structure
/*@TRACE*/
struct lm_pca_end_ind
{
    /// Status
    uint8_t status;
};

/// LM Sync Scan start request for PCA recovery mode 
/*@TRACE*/
struct lm_pca_sscan_start_req
{
    /// Link identifier
    uint8_t link_id;
    /// Status
    uint8_t status;
};

/// LM MWS pattern activation or change indication 
/*@TRACE*/
struct lm_mws_pattern_ind
{
    /// Active pattern index
    uint8_t pattern_index;
    /// Time when pattern will be activated
    rwip_time_t time;
};

#endif // PCA_SUPPORT

/// LM SAM configuration request for new connection
/*@TRACE*/
struct lm_sam_config_req
{
    /// Link identifier
    uint8_t link_id;
    /// Submap available
    bool submap_av;
    /// Patterns available
    bool pattern_av[SAM_INDEX_MAX];
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BT LM task
 *
 * This function initializes the the LM task, as well as the environment of the LM
 *
 * @param[in]  reset    True: reset | False: init
 ****************************************************************************************
 */
void lm_init(bool reset);

/**
 ****************************************************************************************
 * @brief This function is used to allocate an LT address
 *
 * @return 0x00: No LT address found | 0x01-0x07: Allocated LT address
 ****************************************************************************************
 */
uint8_t lm_lt_addr_alloc(void);

/**
 ****************************************************************************************
 * @brief This function is used to reserve an LT address
 *
 * @param[in]  lt_addr    LT address
 ****************************************************************************************
 */
bool lm_lt_addr_reserve(uint8_t lt_addr);

/**
 ****************************************************************************************
 * @brief This function is used to free a LT address
 *
 * @param[in]  lt_addr    LT address
 ****************************************************************************************
 */
void lm_lt_addr_free(uint8_t lt_addr);

/**
 ****************************************************************************************
 * @brief Returns the number of slave and/or master link(s).
 *
 * @param[in] AclFlag      Flag to tell if we need to return the number
 *                         of master and/or slave links.
 *
 * @return Nb of slave and/or master link(s).
 *
 ****************************************************************************************
 */
uint8_t lm_get_nb_acl(uint8_t acl_flag);

/**
 ****************************************************************************************
 * @brief This function indicates a Role Switch starts.
 *
 * @param[in]  link_id            Link Identifier
 * @param[out] lt_addr            LT Address allocated for new Slave (Slave->Master switch only)
 *
 * @return True if role switch is allowed, False otherwise
 ****************************************************************************************
 */
bool lm_role_switch_start(uint8_t link_id, uint8_t* lt_addr);

/**
 ****************************************************************************************
 * @brief This function indicates a Role Switch is finished.
 *
 * @param[in] link_id            Link Identifier
 * @param[in] success            Status of Role Switch
 ****************************************************************************************
 */
void lm_role_switch_finished(uint8_t link_id, bool success);

/**
 ****************************************************************************************
 * @brief Return device internal supported features.
 *
 * @param[in]  page_nb            Page feature
 * @param[out] page_nb_max        Maximum page number
 * @param[out] feats              Feature structure
 ****************************************************************************************
 */
void lm_read_features(uint8_t page_nb, uint8_t *page_nb_max, struct features *feats);

/**
 ****************************************************************************************
 * @brief This function handles ACL link disconnection.
 *
 * @param[in] link_id            Link Identifier
 *
 ****************************************************************************************
 */
void lm_acl_disc(uint8_t link_id);

/**
 ****************************************************************************************
 * @brief This function indicates if authentication is enabled or not
 *
 * @return True if authentication is enabled, False otherwise
 ****************************************************************************************
 */
bool lm_get_auth_en(void);

/**
 ****************************************************************************************
 * @brief This function indicates if simple pairing is enabled or not
 *
 * @return True if simple pairing is enabled, False otherwise
 ****************************************************************************************
 */
bool lm_get_sp_en(void);

/**
 ****************************************************************************************
 * @brief This function indicates if Host supports secure connections
 *
 * @return True if secure connections is supported by Host, False otherwise
 ****************************************************************************************
 */
bool lm_get_sec_con_host_supp(void);

/**
 ****************************************************************************************
 * @brief This function is used to Get the PIN Type.
 *
 * @return PINType
 ****************************************************************************************
 */
uint8_t LM_GetPINType(void);

/**
 ****************************************************************************************
 * @brief This function is used to Get Connection Accept Timeout.
 *
 * @return Connection accept timeout
 *
 ****************************************************************************************
 */
uint16_t LM_GetConnectionAcceptTimeout(void);

/**
 ****************************************************************************************
 * @brief This function is used to extract a segment of the local name.
 *
 * @param[out] NameSeg             Name segment
 * @param[in]  NameOffset          Offset of the name
 * @param[out] NameLen             Length of the name
 *
 ****************************************************************************************
 */
void LM_GetLocalNameSeg(struct name_vect *NameSeg, uint8_t NameOffset, uint8_t* NameLen);

/**
 ****************************************************************************************
 * @brief This function is used to get the loopback mode.
 *
 * @return loop back mode
 ****************************************************************************************
 */
uint8_t lm_get_loopback_mode(void);

/**
 ****************************************************************************************
 * @brief This function returns the public key
 * @param[out] PublicKey            Pointer on the public key data
 ****************************************************************************************
 */
void lm_get_pub_key_192(uint8_t* PublicKey);

/**
 ****************************************************************************************
 * @brief This function returns the private key
 * @param[out] PrivateKey           Pointer on the private key data
 ****************************************************************************************
 */
void lm_get_priv_key_192(uint8_t* PrivateKey);

/**
 ****************************************************************************************
 * @brief This function returns the public key
 * @param[out] PublicKey            Pointer on the public key data
 ****************************************************************************************
 */
void lm_get_pub_key_256(uint8_t* PublicKey);

/**
 ****************************************************************************************
 * @brief This function returns the private key
 * @param[out] PrivateKey           Pointer on the private key data
 ****************************************************************************************
 */
void lm_get_priv_key_256(uint8_t* PrivateKey);

/**
 ****************************************************************************************
 * @brief This function gets the local OOB data
 *
 * @param[in]  R              Randomizer value
 * @param[out] C              Hash value
 ****************************************************************************************
 */
void lm_get_oob_local_data_192(struct randomizer* R, struct hash* C);

/**
 ****************************************************************************************
 * @brief This function gets the local OOB data
 *
 * @param[in]  R              Randomizer value
 * @param[out] C              Hash value
 ****************************************************************************************
 */
void lm_get_oob_local_data_256(struct randomizer* R, struct hash* C);

/**
 ****************************************************************************************
 * @brief This function is used to Get the commitment for OOB algorithm.
 *
 * @param[out] A        Output parameter (randomizer) where commitment will be copied
 * @param[in]  IOCAP    IO capability
 ****************************************************************************************
 */
void lm_get_oob_local_commit(struct randomizer* A, struct io_capability IOCAP);

/**
 ****************************************************************************************
 * @brief This function is used to read the simple pairing debug mode.
 *
 * @return Simple pairing debug mode
 ****************************************************************************************
 */
uint8_t lm_sp_debug_mode_get(void);

/**
 ****************************************************************************************
 * @brief This function is used to check if a key is a debug one.
 *
 * @return True if key is equal to the debug key / False otherwise
 ****************************************************************************************
 */
bool lm_debug_key_compare_192(uint8_t* X, uint8_t* Y);

/**
 ****************************************************************************************
 * @brief This function is used to check if a key is a debug one.
 *
 * @return True if key is equal to the debug key / False otherwise
 ****************************************************************************************
 */
bool lm_debug_key_compare_256(uint8_t* pub_key);

/**
 ****************************************************************************************
 * @brief This function is used to check if device under test mode is enabled
 *
 * @return True if device under test mode is enabled
 ****************************************************************************************
 */
bool lm_dut_mode_en_get(void);

/**
 ****************************************************************************************
 * @brief This function is used to check if channel assessment is enabled
 *
 * @return True if channel assessment is enabled
 ****************************************************************************************
 */
bool lm_afh_ch_ass_en_get(void);

/**
 ****************************************************************************************
 * @brief This function is used to check if synchronous flow control is enabled
 *
 * @return True if synchronous flow control is enabled
 ****************************************************************************************
 */
bool lm_sync_flow_ctrl_en_get(void);

/**
 ****************************************************************************************
 * @brief This function is used to get the Host channel classification
 *
 * @return Pointer to Host channel classification
 ****************************************************************************************
 */
struct chnl_map* lm_afh_host_ch_class_get(void);

/**
 ****************************************************************************************
 * @brief This function is used to set the channel classification from a peer device
 *
 * @param[in]  link_id     Link Identifier
 * @param[in]  ch_class    Peer's channel classification
 ****************************************************************************************
 */
void lm_afh_peer_ch_class_set(uint8_t link_id, struct chnl_map* ch_class);

/**
 ****************************************************************************************
 * @brief This function is used to get the master channel map
 *
 * @return Pointer to master channel map
 ****************************************************************************************
 */
struct chnl_map* lm_afh_master_ch_map_get(void);

/**
 ****************************************************************************************
 * @brief Activates the AFH timer if not already active
 *
 ****************************************************************************************
 */
void lm_afh_activate_timer(void);

/**
 ****************************************************************************************
 * @brief This function is used to check if a link_id is in connected state
 *
 ****************************************************************************************
 */
bool lm_is_acl_con(uint8_t link_id);

/**
 ****************************************************************************************
 * @brief This function is used to check if a link_id is in connected state & role
 *
 ****************************************************************************************
 */
bool lm_is_acl_con_role(uint8_t link_id, uint8_t role);

/**
 ****************************************************************************************
 * @brief This function is used to get the local page scan repetition mode
 *
 * @return Page scan repetition mode
 ****************************************************************************************
 */
uint8_t lm_page_scan_rep_mode_get(void);

#if PCA_SUPPORT
/**
 ****************************************************************************************
 * @brief Generic handler for a PCA request on a Master device
 *
 * @param[in] clk_adj_us. Request intraslot adjustment
 * @param[in] clk_adj_slots. Request interslot adjustment
 * @param[in] clk_adj_period. Period associated with interslot adjustment
 * @param[in] tr_id. Transaction ID of the request.
 * @return status as a result of processing the request.
 ****************************************************************************************
 */
uint8_t lm_master_clk_adj_req_handler(int16_t clk_adj_us, uint8_t clk_adj_slots, uint8_t clk_adj_period, uint8_t tr_id);

/**
 ****************************************************************************************
 * @brief This function is used to check if a PCA clk_adj is pending.
 *
 ****************************************************************************************
 */
bool lm_is_clk_adj_instant_pending(void);

/**
 ****************************************************************************************
 * @brief This function is used to set if a PCA clk_adj is pending.
 *
 ****************************************************************************************
 */
void lm_clk_adj_instant_pending_set(bool instant_pending);

/**
 ****************************************************************************************
 * @brief This function is used to check if a PCA clk_adj_ack(s) is pending.
 *
 ****************************************************************************************
 */
bool lm_is_clk_adj_ack_pending(void);

/**
 ****************************************************************************************
 * @brief This function is used to set a PCA clk_adj_ack pending flag/count.
 *
 ****************************************************************************************
 */
void lm_num_clk_adj_ack_pending_set(uint8_t pending_clk_adj_acks);

/**
 ****************************************************************************************
 * @brief This function is used to clear a PCA clk_adj_ack pending flag/count.
 *
 ****************************************************************************************
 */
void lm_clk_adj_ack_pending_clear(uint8_t link_id);

/**
 ****************************************************************************************
 * @brief This function is used to return whether PCA external Frame is configured.
 *
 ****************************************************************************************
 */
bool lm_local_ext_fr_configured(void);

/**
 ****************************************************************************************
 * @brief This function is used to set lind_id for which a PCA sscan recovery relates.
 *
 ****************************************************************************************
 */
void lm_pca_sscan_link_set(uint8_t link_id);

/**
 ****************************************************************************************
 * @brief This function is used to get lind_id for which a PCA sscan recovery relates.
 *
 ****************************************************************************************
 */
uint8_t lm_pca_sscan_link_get(void);

#endif // PCA_SUPPORT

/**
****************************************************************************************
* @brief This function returns the number of Rx slots in the SAM submap to period T.
*
****************************************************************************************
*/
uint8_t lm_sam_submap_rx_slots_get(uint8_t t_sam_sm);

/**
****************************************************************************************
* @brief This function returns the number of Tx slots in the SAM submap to period T.
*
****************************************************************************************
*/
uint8_t lm_sam_submap_tx_slots_get(uint8_t t_sam_sm);

/**
****************************************************************************************
* @brief This function returns the locally configured SAM submap0
*
****************************************************************************************
*/
uint8_t* lm_sam_submap0_get(void);

/**
****************************************************************************************
* @brief This function returns the locally configured SAM pattern of specified index
*
****************************************************************************************
*/
uint8_t* lm_sam_pattern_get(uint8_t pattern_idx);

#if (MAX_NB_SYNC > 0)
/**
 ****************************************************************************************
 * @brief This function is used to check if an (e)SCO link can be moved autonomously by the controller
 *
 ****************************************************************************************
 */
bool lm_sco_move_en(void);
#endif //(MAX_NB_SYNC > 0)


/// @} LM

#endif // LM_H_
