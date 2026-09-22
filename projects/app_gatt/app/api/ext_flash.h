#ifndef EXT_FLASH_H_
#define EXT_FLASH_H_

#include <stdbool.h>
#include <stdint.h>

/* 板载 W25Q64：64 Mbit = 8 MiB，4 KiB 扇区，256 B 页。 */
#define EXT_FLASH_TOTAL_SIZE       0x00800000UL
#define EXT_FLASH_SECTOR_SIZE      0x00001000UL
#define EXT_FLASH_PAGE_SIZE        256U

bool ext_flash_init(void);
/* Called only with CS released; hook must never call a Flash operation. */
typedef void (*ext_flash_wait_hook_t)(void);
void ext_flash_set_wait_hook(ext_flash_wait_hook_t hook);

/* Nonblocking W25Q64 operation status.  begin() transmits one command and
 * returns immediately; poll() performs one status-register transaction. */
#define EXT_FLASH_ASYNC_ERROR 0U
#define EXT_FLASH_ASYNC_BUSY  1U
#define EXT_FLASH_ASYNC_DONE  2U

bool ext_flash_async_program_begin(uint32_t address,
                                   const uint8_t *data, uint16_t length);
bool ext_flash_async_erase_begin(uint32_t address);
uint8_t ext_flash_async_poll(void);
bool ext_flash_async_busy(void);
typedef struct {
	uint32_t page_programs;
	uint32_t sector_erases;
	uint32_t max_page_program_hs;
	uint32_t max_sector_erase_hs;
} ext_flash_stats_t;
void ext_flash_reset_stats(void);
void ext_flash_get_stats(ext_flash_stats_t *stats);
bool ext_flash_is_ready(void);
bool ext_flash_read(uint32_t address, uint8_t *data, uint32_t length);
bool ext_flash_write(uint32_t address, const uint8_t *data, uint32_t length);
bool ext_flash_erase_sector(uint32_t address);

#endif /* EXT_FLASH_H_ */
