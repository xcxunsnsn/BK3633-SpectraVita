#ifndef _REG_EM_BT_RXACLBUF_H_
#define _REG_EM_BT_RXACLBUF_H_

#include <stdint.h>
#include "_reg_em_bt_rxaclbuf.h"
#include "compiler.h"
#include "arch.h"
#include "em_map.h"
#include "reg_access.h"

#define REG_EM_BT_RXACLBUF_COUNT 1

#define REG_EM_BT_RXACLBUF_DECODING_MASK 0x00000000

#define REG_EM_BT_RXACLBUF_ADDR_GET(idx) (EM_BT_RXACLBUFFER_OFFSET + (idx) * REG_EM_BT_RXACLBUF_SIZE)

/**
 * @brief RXACLBUF register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             RXACLBUF   0x0
 * </pre>
 */
#define EM_BT_RXACLBUF_ADDR   (0x00810000 + EM_BT_RXACLBUFFER_OFFSET)
#define EM_BT_RXACLBUF_INDEX  0x00000000
#define EM_BT_RXACLBUF_RESET  0x00000000
#define EM_BT_RXACLBUF_COUNT  511

__INLINE uint16_t em_bt_rxaclbuf_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 510);
    return EM_BT_RD(EM_BT_RXACLBUF_ADDR + elt_idx * REG_EM_BT_RXACLBUF_SIZE + reg_idx * 2);
}

__INLINE void em_bt_rxaclbuf_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 510);
    EM_BT_WR(EM_BT_RXACLBUF_ADDR + elt_idx * REG_EM_BT_RXACLBUF_SIZE + reg_idx * 2, value);
}

// field definitions
#define EM_BT_RXACLBUF_MASK   ((uint16_t)0x0000FFFF)
#define EM_BT_RXACLBUF_LSB    0
#define EM_BT_RXACLBUF_WIDTH  ((uint16_t)0x00000010)

#define EM_BT_RXACLBUF_RST    0x0

__INLINE uint16_t em_bt_rxaclbuf_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 510);
    uint16_t localVal = EM_BT_RD(EM_BT_RXACLBUF_ADDR + elt_idx * REG_EM_BT_RXACLBUF_SIZE + reg_idx * 2);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}


#endif // _REG_EM_BT_RXACLBUF_H_

