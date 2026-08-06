#ifndef _REG_EM_BT_FT_H_
#define _REG_EM_BT_FT_H_

#include <stdint.h>
#include "_reg_em_bt_ft.h"
#include "compiler.h"
#include "arch.h"
#include "em_map.h"
#include "reg_access.h"

#define REG_EM_BT_FT_COUNT 21

#define REG_EM_BT_FT_DECODING_MASK 0x0000003F

#define REG_EM_BT_FT_ADDR_GET(idx) (EM_BT_FT_OFFSET + (idx) * REG_EM_BT_FT_SIZE)

/**
 * @brief FREQTABEVEN register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:08            FREQEVENH   0x0
 *  07:00            FREQEVENL   0x0
 * </pre>
 */
#define EM_BT_FREQTABEVEN_ADDR   (0x00810000 + EM_BT_FT_OFFSET)
#define EM_BT_FREQTABEVEN_INDEX  0x00000000
#define EM_BT_FREQTABEVEN_RESET  0x00000000
#define EM_BT_FREQTABEVEN_COUNT  20

__INLINE uint16_t em_bt_freqtabeven_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 19);
    return EM_BT_RD(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);
}

__INLINE void em_bt_freqtabeven_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 19);
    EM_BT_WR(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2, value);
}

// field definitions
#define EM_BT_FREQEVENH_MASK   ((uint16_t)0x0000FF00)
#define EM_BT_FREQEVENH_LSB    8
#define EM_BT_FREQEVENH_WIDTH  ((uint16_t)0x00000008)
#define EM_BT_FREQEVENL_MASK   ((uint16_t)0x000000FF)
#define EM_BT_FREQEVENL_LSB    0
#define EM_BT_FREQEVENL_WIDTH  ((uint16_t)0x00000008)

#define EM_BT_FREQEVENH_RST    0x0
#define EM_BT_FREQEVENL_RST    0x0

__INLINE void em_bt_freqtabeven_pack(int elt_idx, int reg_idx, uint8_t freqevenh, uint8_t freqevenl)
{
    ASSERT_ERR(reg_idx <= 19);
    ASSERT_ERR((((uint16_t)freqevenh << 8) & ~((uint16_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint16_t)freqevenl << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BT_WR(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2,  ((uint16_t)freqevenh << 8) | ((uint16_t)freqevenl << 0));
}

__INLINE void em_bt_freqtabeven_unpack(int elt_idx, int reg_idx, uint8_t* freqevenh, uint8_t* freqevenl)
{
    ASSERT_ERR(reg_idx <= 19);
    uint16_t localVal = EM_BT_RD(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);

    *freqevenh = (localVal & ((uint16_t)0x0000FF00)) >> 8;
    *freqevenl = (localVal & ((uint16_t)0x000000FF)) >> 0;
}

__INLINE uint8_t em_bt_freqtabeven_freqevenh_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 19);
    uint16_t localVal = EM_BT_RD(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);
    return ((localVal & ((uint16_t)0x0000FF00)) >> 8);
}

__INLINE void em_bt_freqtabeven_freqevenh_setf(int elt_idx, int reg_idx, uint8_t freqevenh)
{
    ASSERT_ERR(reg_idx <= 19);
    ASSERT_ERR((((uint16_t)freqevenh << 8) & ~((uint16_t)0x0000FF00)) == 0);
    EM_BT_WR(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2, (EM_BT_RD(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2) & ~((uint16_t)0x0000FF00)) | ((uint16_t)freqevenh << 8));
}

__INLINE uint8_t em_bt_freqtabeven_freqevenl_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 19);
    uint16_t localVal = EM_BT_RD(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);
    return ((localVal & ((uint16_t)0x000000FF)) >> 0);
}

__INLINE void em_bt_freqtabeven_freqevenl_setf(int elt_idx, int reg_idx, uint8_t freqevenl)
{
    ASSERT_ERR(reg_idx <= 19);
    ASSERT_ERR((((uint16_t)freqevenl << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BT_WR(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2, (EM_BT_RD(EM_BT_FREQTABEVEN_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2) & ~((uint16_t)0x000000FF)) | ((uint16_t)freqevenl << 0));
}

/**
 * @brief FREQTABODD register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:08             FREQODDH   0x0
 *  07:00             FREQODDL   0x0
 * </pre>
 */
#define EM_BT_FREQTABODD_ADDR   (0x00810028 + EM_BT_FT_OFFSET)
#define EM_BT_FREQTABODD_INDEX  0x00000014
#define EM_BT_FREQTABODD_RESET  0x00000000
#define EM_BT_FREQTABODD_COUNT  20

__INLINE uint16_t em_bt_freqtabodd_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 19);
    return EM_BT_RD(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);
}

__INLINE void em_bt_freqtabodd_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 19);
    EM_BT_WR(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2, value);
}

// field definitions
#define EM_BT_FREQODDH_MASK   ((uint16_t)0x0000FF00)
#define EM_BT_FREQODDH_LSB    8
#define EM_BT_FREQODDH_WIDTH  ((uint16_t)0x00000008)
#define EM_BT_FREQODDL_MASK   ((uint16_t)0x000000FF)
#define EM_BT_FREQODDL_LSB    0
#define EM_BT_FREQODDL_WIDTH  ((uint16_t)0x00000008)

#define EM_BT_FREQODDH_RST    0x0
#define EM_BT_FREQODDL_RST    0x0

__INLINE void em_bt_freqtabodd_pack(int elt_idx, int reg_idx, uint8_t freqoddh, uint8_t freqoddl)
{
    ASSERT_ERR(reg_idx <= 19);
    ASSERT_ERR((((uint16_t)freqoddh << 8) & ~((uint16_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint16_t)freqoddl << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BT_WR(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2,  ((uint16_t)freqoddh << 8) | ((uint16_t)freqoddl << 0));
}

__INLINE void em_bt_freqtabodd_unpack(int elt_idx, int reg_idx, uint8_t* freqoddh, uint8_t* freqoddl)
{
    ASSERT_ERR(reg_idx <= 19);
    uint16_t localVal = EM_BT_RD(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);

    *freqoddh = (localVal & ((uint16_t)0x0000FF00)) >> 8;
    *freqoddl = (localVal & ((uint16_t)0x000000FF)) >> 0;
}

__INLINE uint8_t em_bt_freqtabodd_freqoddh_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 19);
    uint16_t localVal = EM_BT_RD(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);
    return ((localVal & ((uint16_t)0x0000FF00)) >> 8);
}

__INLINE void em_bt_freqtabodd_freqoddh_setf(int elt_idx, int reg_idx, uint8_t freqoddh)
{
    ASSERT_ERR(reg_idx <= 19);
    ASSERT_ERR((((uint16_t)freqoddh << 8) & ~((uint16_t)0x0000FF00)) == 0);
    EM_BT_WR(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2, (EM_BT_RD(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2) & ~((uint16_t)0x0000FF00)) | ((uint16_t)freqoddh << 8));
}

__INLINE uint8_t em_bt_freqtabodd_freqoddl_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 19);
    uint16_t localVal = EM_BT_RD(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2);
    return ((localVal & ((uint16_t)0x000000FF)) >> 0);
}

__INLINE void em_bt_freqtabodd_freqoddl_setf(int elt_idx, int reg_idx, uint8_t freqoddl)
{
    ASSERT_ERR(reg_idx <= 19);
    ASSERT_ERR((((uint16_t)freqoddl << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BT_WR(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2, (EM_BT_RD(EM_BT_FREQTABODD_ADDR + elt_idx * REG_EM_BT_FT_SIZE + reg_idx * 2) & ~((uint16_t)0x000000FF)) | ((uint16_t)freqoddl << 0));
}


#endif // _REG_EM_BT_FT_H_

