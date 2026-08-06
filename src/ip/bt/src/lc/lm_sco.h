/**
****************************************************************************************
*
* @file lm_sco.h
*
* @brief Link Manager synchronous link functions
*
* Copyright (C) RivieraWaves 2009-2015
*
*
****************************************************************************************
*/
#ifndef LM_SCO_H_
#define LM_SCO_H_
/**
****************************************************************************************
* @defgroup LMSCO SCO Functions
* @ingroup LM
* @brief LM module for synchronous functions.
* @{
****************************************************************************************
*/

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"    // stack configuration

#if (MAX_NB_SYNC > 0)

/*
 * DEFINITION
 * ***************************************************************************************
 */
/// Commands for synchronous link
#define SYNC_HL_REQ             0
#define SYNC_PEER_REQ           1
#define SYNC_HL_RSP             2
#define SYNC_PEER_RSP           3
#define SYNC_HL_MODIF           4
#define SYNC_PEER_MODIF         5
#define SYNC_UPDATE             6

/*
 * FUNCTION DECLARATIONS
 * ***************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief This function is used to initialize LM Sco Data.
 ****************************************************************************************
 */
void lm_init_sync(void);
/**
 ****************************************************************************************
 * @brief This function is used to reset LM Sco Data.
 ****************************************************************************************
 */
void lm_reset_sync(void);
/**
 ****************************************************************************************
 * @brief This function is used to check if a SCO is active.
 *
 * @return True if active (at least one), False otherwise
 ****************************************************************************************
 */
bool lm_check_active_sync(void);

/**
 ****************************************************************************************
 * @brief This function is used to check and store informations about a SCO link.
 *
 * @param[in]  sco_link_id           Synchronous link ID
 * @param[in]  RemoteFeaturesPtr     Remote device features
 * @param[in]  Role                  Master or Slave
 * @param[in]  SyncType              SCO or eSCO
 * @param[in]  SyncLtAddr            Synchronous link LT address
 * @param[out] *SyncHdlPtr           Synchronous link handle
 * @param[in]  SyncTxBw              TX bandwidth
 * @param[in]  SyncRxBw              RX bandwidth
 * @param[in]  SyncLat               Maximum allowed Latency
 * @param[in]  SyncVoice             Voice settings
 * @param[in]  SyncAirMode           Air Mode
 * @param[in]  SyncReTx              Retransmission effort
 * @param[in]  SyncPacketType        Allowed packet types
 * @param[in]  Request               Request originator
 *
 * @return Status
 ****************************************************************************************
 */
uint8_t lm_add_sync(uint8_t sco_link_id, struct features *RemoteFeaturesPtr,
                    uint8_t LinkId, uint8_t Role, uint8_t SyncType,
                    uint8_t SyncLtAddrPtr, uint8_t *SyncHdlPtr,
                    uint32_t SyncTxBw, uint32_t SyncRxBw, uint16_t SyncLat, uint16_t VoiceSetting,
                    uint8_t SyncAirMode, uint8_t SyncReTx, uint16_t SyncPacketType, uint8_t Request);

/**
 ****************************************************************************************
 * @brief This function is used to check if a Sync link can be modified.
 *
 * @param[in] sco_link_id       Synchronous link ID
 * @param[in] SyncType              SCO or eSCO
 * @param[in] SyncTxBw              TX bandwidth
 * @param[in] SyncRxBw              RX bandwidth
 * @param[in] SyncLat               Maximum allowed Latency
 * @param[in] SyncVoice             Voice settings
 * @param[in] SyncReTx              Retransmission effort
 * @param[in] SyncPacketType        Allowed packet types
 * @param[in] Request               Request originator
 *
 * @return Status
 *
 ****************************************************************************************
 */
uint8_t lm_modif_sync(uint8_t sco_link_id, uint8_t SyncType, uint32_t SyncTxBw,
                uint32_t SyncRxBw, uint16_t SyncLat, uint16_t SyncVoice,
                uint8_t SyncReTx, uint16_t SyncPacketType, uint8_t Request);

/**
 ****************************************************************************************
 * @brief This function is used to check the Accept Sync connection parameters.
 *
 * @param[in] sco_link_id       Synchronous link ID
 * @param[in] SyncTxBw          TX bandwidth
 * @param[in] SyncRxBw          RX bandwidth
 * @param[in] SyncLat           Maximum allowed Latency
 * @param[in] SyncVoice         Voice settings
 * @param[in] SyncReTx          Retransmission effort
 * @param[in] SyncPacketType    Allowed packet types
 *
 * @return Status
 *
 ****************************************************************************************
 */
uint8_t lm_check_sync_hl_rsp(uint8_t sco_link_id, uint32_t SyncTxBw,
                uint32_t SyncRxBw, uint16_t SyncLat, uint16_t SyncVoice,
                uint8_t SyncReTx, uint16_t SyncPacketType);

/**
 ****************************************************************************************
 * @brief This function is used to remove stored synchronous link information.
 *
 * @param[in] sco_link_id       Synchronous link ID
 ****************************************************************************************
 */
void lm_remove_sync(uint8_t sco_link_id);

/**
 ****************************************************************************************
 * @brief This function is used to Restore parameters for a synchronous link.
 *
 * @param[in]  sco_link_id        Synchronous link ID
 * @param[out] *SyncTypePtr       SCO or eSCO
 * @param[out] *SyncHdlPtr        Synchronous link handle
 * @param[out] *SyncLtAddrPtr     Synchronous link LT address
 * @param[out] *SyncIntervalPtr   Tsco or Tesco
 * @param[out] *SyncOffsetPtr     Dsco or Desco
 * @param[out] *SyncAirModePtr    Air mode
 * @param[out] *SyncPacketM2SPtr  Packet M->S type
 * @param[out] *SyncPacketS2MPtr  Packet S->M type
 * @param[out] *SyncLenM2SPtr     Packet M->S length for eSCO link
 * @param[out] *SyncLenS2MPtr     Packet S->M length for eSCO link
 * @param[out] *SyncWindowPtr     Retransmission window size
 * @param[out] *TimingFlagPtr     Timing flags
 * @param[out] *SyncNegoState     Negotiation state
 *
 * @return Status
 ****************************************************************************************
 */
uint8_t lm_get_sync_param(uint8_t sco_link_id, uint8_t *SyncType, uint8_t *SyncHdl,
                uint8_t *SyncLtAddr, uint8_t *SyncInterval, uint8_t *SyncOffset,
                uint8_t *SyncAirMode, uint8_t *SyncPacketM2S,
                uint8_t *SyncPacketS2M, uint16_t *SyncLenM2S, uint16_t *SyncLenS2M,
                uint8_t *SyncWindow, uint8_t *TimingFlag,uint8_t *SyncNegoState, uint8_t Request);


/**
 ****************************************************************************************
 * @brief This function is used to get the Sync Handle of a Sync link.
 *
 * @param[in] sco_link_id       Synchronous link ID
 *
 * @return Synchronous link handle
 ****************************************************************************************
 */
uint8_t lm_get_synchdl(uint8_t sco_link_id);

/**
 ****************************************************************************************
 * @brief This function is used to check if an ACL link has at least 1 synchronous link.
 *
 * @param[in] sco_link_id       Synchronous link ID
 * @param[in] type              Check SCO or eSCO specifically (ACL_TYPE if the type does not matter)
 *
 * @return True if SCO link present
 ****************************************************************************************
 */
bool lm_look_for_sync(uint8_t link_id, uint8_t type);

/**************************************************************************************
 * @brief Indicate the end of a SCO negotiation
 **************************************************************************************/
void lm_sco_nego_end(void);

/**
 ****************************************************************************************
 * @brief This function is used to update the SCO offset.
 *
 * @param[in] sco_link_id       SCO link ID
 * @param[in] sco_offset        SCO offset in slots
 *
 * @return True if SCO link present
 ****************************************************************************************
 */
void lc_sco_offset_update(uint8_t sco_link_id, uint16_t sco_offset);

#endif //(MAX_NB_SYNC > 0)

///@} LMSCO

#endif // LM_SCO_H_
