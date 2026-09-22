#ifndef EXT_FLASH_TEST_PLATFORM_H
#define EXT_FLASH_TEST_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#define EXT_FLASH_TOTAL_SIZE  0x00800000UL
#define EXT_FLASH_SECTOR_SIZE 0x00001000UL
#define EXT_FLASH_PAGE_SIZE   256U
#define W25QXX_CS_PIN 0x07
#define OUTPUT 0
#define PULL_NONE 0
#define RWIP_MAX_CLOCK_TIME ((1UL << 28) - 1)
#define EXT_FLASH_ASYNC_ERROR 0U
#define EXT_FLASH_ASYNC_BUSY  1U
#define EXT_FLASH_ASYNC_DONE  2U

typedef struct { uint32_t hs, hus; } rwip_time_t;
typedef void (*ext_flash_wait_hook_t)(void);
typedef struct {
    uint32_t page_programs;
    uint32_t sector_erases;
    uint32_t max_page_program_hs;
    uint32_t max_sector_erase_hs;
} ext_flash_stats_t;

void gpio_config(uint8_t pin, int direction, int pull);
void gpio_set(uint8_t pin, uint8_t value);
void Delay_us(int count);
void spi_write(uint8_t *data, uint32_t length);
bool spi_read_checked(uint8_t *data, uint32_t length);
uint8_t spi_send_state_get(void);
rwip_time_t rwip_time_get(void);
int uart_printf(const char *format, ...);

#endif
