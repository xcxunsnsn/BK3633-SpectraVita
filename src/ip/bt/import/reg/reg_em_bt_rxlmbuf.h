#ifndef _REG_EM_BT_RXLMBUF_H_
#define _REG_EM_BT_RXLMBUF_H_

#include <stdint.h>
#include "_reg_em_bt_rxlmbuf.h"
#include "compiler.h"
#include "arch.h"
#include "em_map.h"
#include "reg_access.h"

#define REG_EM_BT_RXLMBUF_COUNT 1

#define REG_EM_BT_RXLMBUF_DECODING_MASK 0x00000000

#define REG_EM_BT_RXLMBUF_ADDR_GET(idx) (EM_BT_RXLMBUF_OFFSET + (idx) * REG_EM_BT_RXLMBUF_SIZE)

/**
 * @brief RXLMBUF register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00              RXLMBUF   0x0
 * </pre>
 */
#define EM_BT_RXLMBUF_ADDR   (0x00810000 + EM_BT_RXLMBUF_OFFSET)
#define EM_BT_RXLMBUF_INDEX  0x00000000
#define EM_BT_RXLMBUF_RESET  0x00000000
#define EM_BT_RXLMBUF_COUNT  14

__INLINE uint16_t em_bt_rxlmbuf_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 13);
    return EM_BT_RD(EM_BT_RXLMBUF_ADDR + elt_idx * REG_EM_BT_RXLMBUF_SIZE + reg_idx * 2);
}

__INLINE void em_bt_rxlmbuf_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 13);
    EM_BT_WR(EM_BT_RXLMBUF_ADDR + elt_idx * REG_EM_BT_RXLMBUF_SIZE + reg_idx * 2, value);
}

// field definitions
#define EM_BT_RXLMBUF_MASK   ((uint16_t)0x0000FFFF)
#define EM_BT_RXLMBUF_LSB    0
#define EM_BT_RXLMBUF_WIDTH  ((uint16_t)0x00000010)

#define EM_BT_RXLMBUF_RST    0x0

__INLINE uint16_t em_bt_rxlmbuf_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 13);
    uint16_t localVal = EM_BT_RD(EM_BT_RXLMBUF_ADDR + elt_idx * REG_EM_BT_RXLMBUF_SIZE + reg_idx * 2);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}


#endif // _REG_EM_BT_RXLMBUF_H_

