#ifndef _REG_EM_BT_TXACLBUF_H_
#define _REG_EM_BT_TXACLBUF_H_

#include <stdint.h>
#include "_reg_em_bt_txaclbuf.h"
#include "compiler.h"
#include "arch.h"
#include "em_map.h"
#include "reg_access.h"

#define REG_EM_BT_TXACLBUF_COUNT 1

#define REG_EM_BT_TXACLBUF_DECODING_MASK 0x00000000

#define REG_EM_BT_TXACLBUF_ADDR_GET(idx) (EM_BT_TXACLBUFFER_OFFSET + (idx) * REG_EM_BT_TXACLBUF_SIZE)

/**
 * @brief TXACLBUF register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             TXACLBUF   0x0
 * </pre>
 */
#define EM_BT_TXACLBUF_ADDR   (0x00810000 + EM_BT_TXACLBUFFER_OFFSET)
#define EM_BT_TXACLBUF_INDEX  0x00000000
#define EM_BT_TXACLBUF_RESET  0x00000000
#define EM_BT_TXACLBUF_COUNT  511

__INLINE uint16_t em_bt_txaclbuf_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 510);
    return EM_BT_RD(EM_BT_TXACLBUF_ADDR + elt_idx * REG_EM_BT_TXACLBUF_SIZE + reg_idx * 2);
}

__INLINE void em_bt_txaclbuf_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 510);
    EM_BT_WR(EM_BT_TXACLBUF_ADDR + elt_idx * REG_EM_BT_TXACLBUF_SIZE + reg_idx * 2, value);
}

// field definitions
#define EM_BT_TXACLBUF_MASK   ((uint16_t)0x0000FFFF)
#define EM_BT_TXACLBUF_LSB    0
#define EM_BT_TXACLBUF_WIDTH  ((uint16_t)0x00000010)

#define EM_BT_TXACLBUF_RST    0x0

__INLINE void em_bt_txaclbuf_setf(int elt_idx, int reg_idx, uint16_t txaclbuf)
{
    ASSERT_ERR(reg_idx <= 510);
    ASSERT_ERR((((uint16_t)txaclbuf << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BT_WR(EM_BT_TXACLBUF_ADDR + elt_idx * REG_EM_BT_TXACLBUF_SIZE + reg_idx * 2, (uint16_t)txaclbuf << 0);
}


#endif // _REG_EM_BT_TXACLBUF_H_

