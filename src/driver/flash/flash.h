/**
 ****************************************************************************************
 *
 * @file flash.h
 *
 * @brief Flash driver interface
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef FLASH_H_
#define FLASH_H_

//#include <stdint.h>               // standard integer functions

#define AHB_FLASH_BASE                     0x00802000
#define REG_FLASH_OPERATE_SW            (*((volatile unsigned long *)   (AHB_FLASH_BASE+0*4)))
#define REG_FLASH_DATA_SW_FLASH         (*((volatile unsigned long *)   (AHB_FLASH_BASE+1*4)))
#define REG_FLASH_DATA_FLASH_SW         (*((volatile unsigned long *)   (AHB_FLASH_BASE+2*4)))
#define REG_FLASH_RDID_DATA_FLASH       (*((volatile unsigned long *)   (AHB_FLASH_BASE+4*4)))
#define REG_FLASH_SR_DATA_CRC_CNT       (*((volatile unsigned long *)   (AHB_FLASH_BASE+5*4)))
#define REG_FLASH_CONF                  (*((volatile unsigned long *)   (AHB_FLASH_BASE+7*4)))
#define flash_200k_ADDR                 (*((volatile unsigned long *)    (0x00032000)))
#define BIT_ADDRESS_SW                  0
#define BIT_OP_TYPE_SW                  24
#define BIT_OP_SW                       29
#define BIT_WP_VALUE                    30
#define BIT_BUSY_SW                     31

#define SET_ADDRESS_SW                  0xFFFFFF << BIT_ADDRESS_SW
#define SET_OP_TYPE_SW                  0x1F     << BIT_OP_TYPE_SW
#define SET_OP_SW                       0x1      << BIT_OP_SW
#define SET_WP_VALUE                    0x1      << BIT_WP_VALUE
#define SET_BUSY_SW                     0x1      << BIT_BUSY_SW

#define BIT_FLASH_CLK_CONF              0
#define BIT_MODE_SEL                    4
#define BIT_FWREN_FLASH_CPU             9
#define BIT_WRSR_DATA                   10
#define BIT_CRC_EN                      26

#define SET_FLASH_CLK_CONF              0xF      << BIT_FLASH_CLK_CONF
#define SET_MODE_SEL                    0x1F     << BIT_MODE_SEL
#define SET_FWREN_FLASH_CPU             0x1      << BIT_FWREN_FLASH_CPU
#define SET_WRSR_DATA                   0xFFFF   << BIT_WRSR_DATA
#define SET_CRC_EN                      0x1      << BIT_CRC_EN

#define BIT_SR_DATA_FLASH               0
#define BIT_CRC_ERR_COUNTER             8
#define BIT_DATA_FLASH_SW_SEL           16
#define BIT_DATA_SW_FLASH_SEL           19

#define SET_SR_DATA_FLASH               0xFF     << BIT_SR_DATA_FLASH
#define SET_CRC_ERR_COUNTER             0xFF     << BIT_CRC_ERR_COUNTER
#define SET_DATA_FLASH_SW_SEL           0x7      << BIT_DATA_FLASH_SW_SEL
#define SET_DATA_SW_FLASH_SEL           0x7      << BIT_DATA_SW_FLASH_SEL


///Flash type code used to select the correct erasing and programming algorithm
#define FLASH_TYPE_UNKNOWN             0
#define FLASH_SPACE_TYPE_MAIN                     0x3435
#define FLASH_TYPE_EMBEN_BK3435        FLASH_SPACE_TYPE_MAIN


///Base address of Flash on system bus
#define FLASH_MAIN_BASE_ADDR         0x00000000



#define FLASH_MAIN_SIZE              0x00040000

/// Common for EMBED BEKEN FLASH Family

#define FLASH_SECT_SIZE2         0x00001000  //  4 KByte
#define FLASH_BOUNDARY256B       0x00000100  // 256 Byte

#define FLASH_PAGE_SIZE                                    (256)
#define FLASH_PAGE_MASK                                   (FLASH_PAGE_SIZE - 1)
#define FLASH_ERASE_SECTOR_SIZE_RSL_BIT_CNT               (12)
#define FLASH_ERASE_SECTOR_SIZE                           (4096)
#define FLASH_ERASE_SECTOR_SIZE_MASK                      (FLASH_ERASE_SECTOR_SIZE - 1)
#define UPDATE_CHUNK_SIZE                                 (32)

#define GD_MD25D40   0x514013
#define GD_GD25WD40  0xc86413
#define GD_25WD80E   0xc86414
#define P25Q40U      0X856013
#define TH25D40HB    0xcd6013
#define GT25Q40      0xc44013

#define GD_FLASH_1   0XC84013
#define XTX_FLASH_1  0X1C3113
#define BY25Q80      0xe04014
#define PN25f04      0xe04013



#define FLASH_LINE_1  1
#define FLASH_LINE_2  2
#define FLASH_LINE_4  4

#define DEFAULT_LINE_MODE  FLASH_LINE_4

#define FLASH_ADDR_FIX  0X7D000
#define FLASH_WRITE_ENABLE1  0XA6
#define FLASH_WRITE_ENABLE2  0XB3
#define FLASH_WRITE_ENABLE3  0XC2
#define FLASH_WRITE_ENABLE4  0XD1
#define FLASH_ERASE_ENABLE1  0XAB
#define FLASH_ERASE_ENABLE2  0XBC

#define FLASH_ENV_OTA_APP_DEF_ADDR_4M_ABS             0x52000
#define FLASH_ENV_OTA_APP_STACK_DEF_ADDR_4M_ABS       0x40000

#define FLASH_ENV_BDADDR_DEF_ADDR_4M_ABS              0x7E000
#define FLASH_ENV_NVDS_DEF_ADDR_4M_ABS                0x7F000

#define FLASH_ENV_OTA_APP_DEF_ADDR_8M_ABS             0x80000
#define FLASH_ENV_OTA_APP_STACK_DEF_ADDR_8M_ABS       0x80000
#define FLASH_ENV_BDADDR_DEF_ADDR_8M_ABS              0xFE000
#define FLASH_ENV_NVDS_DEF_ADDR_8M_ABS                0xFF000


/// flash operation command type(decimal)
typedef enum {
    FLASH_OPCODE_WREN    = 1,
    FLASH_OPCODE_WRDI    = 2,
    FLASH_OPCODE_RDSR    = 3,
    FLASH_OPCODE_WRSR    = 4,
    FLASH_OPCODE_READ    = 5,
    FLASH_OPCODE_RDSR2   = 6,
    FLASH_OPCODE_WRSR2   = 7,
    FLASH_OPCODE_PP      = 12,
    FLASH_OPCODE_SE      = 13,
    FLASH_OPCODE_BE1     = 14,
    FLASH_OPCODE_BE2     = 15,
    FLASH_OPCODE_CE      = 16,
    FLASH_OPCODE_DP      = 17,
    FLASH_OPCODE_RFDP    = 18,
    FLASH_OPCODE_RDID    = 20,
    FLASH_OPCODE_HPM     = 21,
    FLASH_OPCODE_CRMR    = 22,
    FLASH_OPCODE_CRMR2   = 23,
} FLASH_OPCODE;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
 /// Flash environment structure
struct flash_env_tag
{
    uint32_t ota_app_def_addr_abs ;
    uint32_t ota_stack_app_def_addr_abs;
    uint32_t bdaddr_def_addr_abs ;
    uint32_t nvds_def_addr_abs ;
};
extern struct flash_env_tag flash_env;


void flash_advance_init(void);

/**
 ****************************************************************************************
 * @brief Initialize flash driver.
 ****************************************************************************************
 */
void flash_init(void);

/**
 ****************************************************************************************
 * @brief   Identify the flash device.
 *
 * This function is used to read the flash device ID.
 * 
 * Note: callback parameter is not used
 *
 * @param[out]   id          Pointer to id location
 * @param[in]    callback    Callback for end of identification
 * @return       status      0 if operation can start successfully
 ****************************************************************************************
 */
uint8_t flash_identify(uint8_t* id, void (*callback)(void));

/**
 ****************************************************************************************
 * @brief   Erase a flash section.
 *
 * This function is used to erase a part of the flash memory.
 * 
 * Note: callback parameter is not used
 *
 * @param[in]    flash_type  Flash type
 * @param[in]    offset      Starting offset from the beginning of the flash device
 * @param[in]    size        Size of the portion of flash to erase
 * @param[in]    callback    Callback for end of erase
 * @return       status      0 if operation can start successfully
 ****************************************************************************************
 */
uint8_t flash_erase(uint8_t flash_type, uint32_t offset, uint32_t size, void (*callback)(void));

/**
 ****************************************************************************************
 * @brief   Write a flash section.
 *
 * This function is used to write a part of the flash memory.
 * 
 * Note: callback parameter is not used
 *
 * @param[in]    flash_type  Flash type
 * @param[in]    offset      Starting offset from the beginning of the flash device
 * @param[in]    length      Size of the portion of flash to write
 * @param[in]    buffer      Pointer on data to write
 * @param[in]    callback    Callback for end of write
 * @return       status      0 if operation can start successfully
 ****************************************************************************************
 */
uint8_t flash_write(uint8_t flash_type, uint32_t offset, uint32_t length, uint8_t *buffer, void (*callback)(void));

//add 231225
uint8_t flash_write_ota(uint8_t flash_space, uint32_t address, uint32_t len, uint8_t *buffer, void (*callback)(void), uint32_t oad_uuid);

/**
 ****************************************************************************************
 * @brief   Read a flash section.
 *
 * This function is used to read a part of the flash memory.
 * 
 * Note: callback parameter is not used
 *
 * @param[in]    flash_type  Flash type
 * @param[in]    offset      Starting offset from the beginning of the flash device
 * @param[in]    length      Size of the portion of flash to read
 * @param[out]   buffer      Pointer on data to read
 * @param[in]    callback    Callback for end of read
 * @return       status      0 if operation can start successfully
 ****************************************************************************************
 */
uint8_t flash_read(uint8_t flash_type, uint32_t offset, uint32_t length, uint8_t *buffer, void (*callback)(void));


void flash_wp_256k( void);
void set_flash_clk(unsigned char clk_conf) ;

void flash_clk_conf(uint8_t clk_sel,uint8_t clk_src,uint8_t div);

void flash_set_qe(void);
void flash_write_sr(unsigned char bytes,  unsigned short val);
void flash_test(void);
void flash_set_line_mode(uint8_t mode);
void flash_read_data (uint8_t *buffer, uint32_t address, uint32_t len);
void flash_write_data (uint8_t *buffer, uint32_t address, uint32_t len);
void flash_set_dual_mode(void);
void flash_erase_sector(uint32_t address);
void flash_write_some_data(uint8_t *buffer, uint32_t address, uint32_t len);

#endif // FLASH_H_

