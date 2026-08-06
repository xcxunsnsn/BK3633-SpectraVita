#ifndef _REG_EM_BT_TXLMBUF_H_
#define _REG_EM_BT_TXLMBUF_H_

#include <stdint.h>
#include "_reg_em_bt_txlmbuf.h"
#include "compiler.h"
#include "arch.h"
#include "em_map.h"
#include "reg_access.h"

#define REG_EM_BT_TXLMBUF_COUNT 1

#define REG_EM_BT_TXLMBUF_DECODING_MASK 0x00000000

#define REG_EM_BT_TXLMBUF_ADDR_GET(idx) (EM_BT_TXLMBUFFER_OFFSET + (idx) * REG_EM_BT_TXLMBUF_SIZE)

/**
 * @brief TXLMBUF register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00              TXLMBUF   0x0
 * </pre>
 */
#define EM_BT_TXLMBUF_ADDR   (0x00810000 + EM_BT_TXLMBUFFER_OFFSET)
#define EM_BT_TXLMBUF_INDEX  0x00000000
#define EM_BT_TXLMBUF_RESET  0x00000000
#define EM_BT_TXLMBUF_COUNT  14

__INLINE uint16_t em_bt_txlmbuf_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 13);
    return EM_BT_RD(EM_BT_TXLMBUF_ADDR + elt_idx * REG_EM_BT_TXLMBUF_SIZE + reg_idx * 2);
}

__INLINE void em_bt_txlmbuf_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 13);
    EM_BT_WR(EM_BT_TXLMBUF_ADDR + elt_idx * REG_EM_BT_TXLMBUF_SIZE + reg_idx * 2, value);
}

// field definitions
#define EM_BT_TXLMBUF_MASK   ((uint16_t)0x0000FFFF)
#define EM_BT_TXLMBUF_LSB    0
#define EM_BT_TXLMBUF_WIDTH  ((uint16_t)0x00000010)

#define EM_BT_TXLMBUF_RST    0x0

__INLINE uint16_t em_bt_txlmbuf_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 13);
    uint16_t localVal = EM_BT_RD(EM_BT_TXLMBUF_ADDR + elt_idx * REG_EM_BT_TXLMBUF_SIZE + reg_idx * 2);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void em_bt_txlmbuf_setf(int elt_idx, int reg_idx, uint16_t txlmbuf)
{
    ASSERT_ERR(reg_idx <= 13);
    ASSERT_ERR((((uint16_t)txlmbuf << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BT_WR(EM_BT_TXLMBUF_ADDR + elt_idx * REG_EM_BT_TXLMBUF_SIZE + reg_idx * 2, (uint16_t)txlmbuf << 0);
}


#endif // _REG_EM_BT_TXLMBUF_H_

