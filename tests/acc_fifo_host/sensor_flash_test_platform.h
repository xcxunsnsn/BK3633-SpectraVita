#ifndef SENSOR_FLASH_TEST_PLATFORM_H
#define SENSOR_FLASH_TEST_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#define EXT_FLASH_TOTAL_SIZE  0x00800000UL
#define EXT_FLASH_SECTOR_SIZE 0x00001000UL
#define EXT_FLASH_PAGE_SIZE   256U

enum {
    PROTOCOL_SENSOR_MSS = 0,
    PROTOCOL_SENSOR_FIFO,
    PROTOCOL_SENSOR_UV,
    PROTOCOL_SENSOR_DIST,
    PROTOCOL_SENSOR_ACC,
    PROTOCOL_SENSOR_COUNT,
};

#define SENSOR_FLASH_ACC_RECORD_LEN 7U
#define SENSOR_FLASH_ACC_BATCH_MAX  8U
#define SENSOR_FLASH_BATCH_ERROR    0U
#define SENSOR_FLASH_BATCH_OK       1U
#define SENSOR_FLASH_BATCH_LAST     2U
#define SENSOR_FLASH_BATCH_FULL     3U
#define SENSOR_FLASH_BATCH_BUSY     4U

#define EXT_FLASH_ASYNC_ERROR 0U
#define EXT_FLASH_ASYNC_BUSY  1U
#define EXT_FLASH_ASYNC_DONE  2U

bool ext_flash_init(void);
bool ext_flash_read(uint32_t address, uint8_t *data, uint32_t length);
bool ext_flash_write(uint32_t address, const uint8_t *data, uint32_t length);
bool ext_flash_erase_sector(uint32_t address);
bool ext_flash_async_program_begin(uint32_t address,
                                   const uint8_t *data, uint16_t length);
bool ext_flash_async_erase_begin(uint32_t address);
uint8_t ext_flash_async_poll(void);
bool ext_flash_async_busy(void);
int uart_printf(const char *format, ...);

#endif
