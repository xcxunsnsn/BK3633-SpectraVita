/**
****************************************************************************************
*
* @file rwbt.c
*
* @brief RWBT core interrupt handler
*
* Copyright (C) RivieraWaves 2009-2015
*
*
****************************************************************************************
*/

/**
 ****************************************************************************************
 * @addtogroup RWBTINT Interrupt Handler
 * @ingroup ROOT
 * @brief The RWBT Interrupt controller.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"          // stack configuration
#if (BT_DUAL_MODE || BT_STD_MODE) 

#include <string.h>          // for mem* functions
#include "co_version.h"
#include "co_math.h"
#include "rwbt.h"            // rwbt interrupt definitions
#include "rwip.h"           // stack main module

#include "bt_util_buf.h"       // BT EM buffer management
#include "ld.h"              // link driver definition

#include "lb.h"              // link broadcast definition
#include "lc.h"              // link controller definition
#include "lm.h"              // link manager definition

#include "ke_event.h"        // kernel event definition

#include "sch_arb.h"         // Scheduling Arbiter
#include "sch_prog.h"        // Scheduling Programmer

#include "dbg.h"             // debug definitions

#include "reg_btcore.h"      // BT Core registers



/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void rwbt_init(void)
{
    uint32_t seed;
   
    uart_printf("rwbt_init entry\r\n");
    seed = ld_read_clock();
    seed += bt_finetimecnt_get(); 
    uart_printf("seed %x\r\n",seed);  
    
    //Init the random seed
    co_random_init(seed);
    
    // Initialize buffer management system
    bt_util_buf_init(false);

    // Initialize the Link Layer Driver
    ld_init();

    // Initialize the Link Layer Broadcaster
    lb_init();  

    // Initialize the Link Layer Controller
    lc_init();
    uart_printf("lc_init ok\r\n");  

    // Initialize the Link Layer Manager
    lm_init(false);
    uart_printf("lm_init ok\r\n");  
}

void rwbt_reset(void)
{
    // Initialize buffer management system
    bt_util_buf_init(true);

    // Reset the BT core
    ld_reset();

    // Initialize the Link Layer Broadcaster
    lb_reset();

    // Reset the Link Layer Controller
    lc_reset();

    // Reset the Link Layer Manager
    lm_init(true);

    // Turn on BT Core
    bt_rwbtcntl_rwbten_setf(1);
}

__BTIRQ void rwbt_isr(void)
{
    DBG_SWDIAG(ISR, BT, 1);
    // Loop until no more interrupts have to be handled
    while (1)
    {
        // Check BT interrupt status and call the appropriate handlers
        uint32_t irq_stat = bt_intstat0_get();
        
        uart_printf("%s irq_stat(%x) \r\n",__func__, irq_stat );          
        
        if (irq_stat == 0)
            break;

        #if VOICE_OVER_HCI
        // Audio interrupt 0
        if (irq_stat & BT_AUDIOINT0STAT_BIT)
        {
            // Clear the interrupt
            bt_intack0_audioint0ack_clearf(1);

            // Handle audio IRQ
            ld_sco_audio_isr(0);
        }

        // Audio interrupt 1
        if (irq_stat & BT_AUDIOINT1STAT_BIT)
        {
            // Clear the interrupt
            bt_intack0_audioint1ack_clearf(1);

            // Handle audio IRQ
            ld_sco_audio_isr(1);
        }
        #endif //VOICE_OVER_HCI

        #if RW_BT_MWS_COEX
        // MWS FRAME_SYNC interrupt pending.
        if (irq_stat & BT_FRSYNCINTSTAT_BIT)
        {
            bt_intack0_frsyncintack_clearf(1);
            // Handle the IRQ
            ld_pca_mws_frame_sync();
        }
        #endif // RW_BT_MWS_COEX
        #if PCA_SUPPORT
        // Momentary offset interrupt pending. The newly calculated momentary offset is greater than the correction step.
        if (irq_stat & BT_MTOFFINT0STAT_BIT)
        {
            // Clear the interrupt
            bt_intack0_mtoffint0ack_clearf(1);
            // Handle the IRQ
            ld_pca_mws_moment_offset_gt();
        }
        // Momentary offset interrupt pending. The newly calculated momentary offset is lower than the correction step.
        if (irq_stat & BT_MTOFFINT1STAT_BIT)
        {
            // Clear the interrupt
            bt_intack0_mtoffint1ack_clearf(1);
            // Handle the IRQ
            ld_pca_mws_moment_offset_lt();
        }
        #endif // PCA_SUPPORT

        // Error interrupt
        if (irq_stat & BT_ERRORINTSTAT_BIT)
        {
            // Clear the interrupt
            bt_intack0_errorintack_clearf(1);

            ASSERT_INFO(0, bt_errortypestat_get(), 0);
        }
    }

    DBG_SWDIAG(ISR, BT, 0);
}

void rwbt_sleep_enter(void)
{
    // Mask all interrupts
    bt_intcntl0_set(0);

    // Clear possible pending IRQs
    bt_intack0_clear(0xFFFFFFFF);
}

void rwbt_sleep_wakeup_end(void)
{
    // Re-enable the default RWBT interrupts
    bt_intcntl0_set(BT_SKIPFRMINTMSK_BIT | BT_ENDFRMINTMSK_BIT | BT_RXINTMSK_BIT | BT_ERRORINTMSK_BIT | BT_FRSYNCINTMSK_BIT);
    bt_intack0_clear(0xFFFFFFFF);
}

#endif //(BT_DUAL_MODE || BT_STD_MODE)

///@} RWBTINT
