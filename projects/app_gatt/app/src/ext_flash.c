/*
 * 板载 W25Q64 SPI Flash 驱动。
 *
 * 原理图连接：P04=SCLK、P05=DI、P06=DO、P07=/CS。
 * BK3633 的 SPI 驱动会把 P07 配成硬件片选；这里改成普通 GPIO 手动片选，
 * 这样“命令 + 地址 + 数据”可跨多次 SPI 调用而保持在同一事务中。
 */
#ifdef EXT_FLASH_HOST_TEST
#include "ext_flash_test_platform.h"
#else
#include "ext_flash.h"
#include "user_config.h"
#include "spi.h"
#include "gpio.h"
#include "rf.h"
#include "uart.h"
#include "rwip.h"
#endif
#include <string.h>

#define W25Q_CMD_WRITE_ENABLE      0x06
#define W25Q_CMD_READ_STATUS1      0x05
#define W25Q_CMD_READ_DATA         0x03
#define W25Q_CMD_PAGE_PROGRAM      0x02
#define W25Q_CMD_SECTOR_ERASE      0x20
#define W25Q_CMD_JEDEC_ID          0x9F
#define W25Q_CMD_POWER_DOWN        0xB9
#define W25Q_CMD_RELEASE_POWERDOWN 0xAB

#define W25Q_STATUS_BUSY           0x01
#define W25Q_CAPACITY_64MBIT       0x17
#define W25Q_READ_CHUNK            2048U
#define W25Q_SPI_RX_CHUNK          64U
#define W25Q_BUSY_POLL_LIMIT       100000UL

static uint8_t flash_ready;
static ext_flash_wait_hook_t wait_hook;
static uint8_t wait_hook_active;
static ext_flash_stats_t flash_stats;
static uint8_t async_operation;
static uint32_t async_started;

enum {
	FLASH_ASYNC_NONE = 0,
	FLASH_ASYNC_PROGRAM,
	FLASH_ASYNC_ERASE,
};

static uint32_t flash_clock_hs(void)
{
	return rwip_time_get().hs & RWIP_MAX_CLOCK_TIME;
}

static uint32_t flash_elapsed_hs(uint32_t now, uint32_t before)
{
	return (now - before) & RWIP_MAX_CLOCK_TIME;
}

void ext_flash_set_wait_hook(ext_flash_wait_hook_t hook)
{
	wait_hook = hook;
}

void ext_flash_reset_stats(void)
{
	memset(&flash_stats, 0, sizeof(flash_stats));
}

void ext_flash_get_stats(ext_flash_stats_t *stats)
{
	if (stats != 0) {
		*stats = flash_stats;
	}
}

static void flash_cs(uint8_t high)
{
	gpio_set(W25QXX_CS_PIN, high ? 1 : 0);
}

static bool spi_wait_done(void)
{
	uint32_t guard = 2000000UL;
	while (!spi_send_state_get() && guard != 0) {
		guard--;
	}
	return guard != 0;
}

static bool spi_tx(const uint8_t *data, uint32_t length)
{
	uint32_t chunk;
	while (length != 0) {
		chunk = length > 64U ? 64U : length;
		spi_write((uint8_t *)data, chunk);
		if (!spi_wait_done()) {
			return false;
		}
		data += chunk;
		length -= chunk;
	}
	return true;
}

static bool spi_rx(uint8_t *data, uint32_t length, uint32_t address)
{
	uint32_t offset = 0;
	uint32_t chunk;
	while (length != 0) {
		chunk = length > W25Q_SPI_RX_CHUNK ? W25Q_SPI_RX_CHUNK : length;
		if (!spi_read_checked(data, chunk) || !spi_wait_done()) {
			uart_printf("Flash SPI RX fail addr=%x chunk=%u\r\n",
				address + offset, chunk);
			return false;
		}
		data += chunk;
		length -= chunk;
		offset += chunk;
	}
	return true;
}

static void flash_wake(void)
{
	uint8_t command = W25Q_CMD_RELEASE_POWERDOWN;
	flash_cs(0);
	spi_tx(&command, 1);
	flash_cs(1);
	/* Compatible 64-Mbit parts specify up to 20 us release time. */
	Delay_us(25);
}

static void flash_sleep(void)
{
	uint8_t command = W25Q_CMD_POWER_DOWN;
	flash_cs(0);
	spi_tx(&command, 1);
	flash_cs(1);
}

static bool flash_write_enable(void)
{
	uint8_t command = W25Q_CMD_WRITE_ENABLE;
	flash_cs(0);
	if (!spi_tx(&command, 1)) {
		flash_cs(1);
		return false;
	}
	flash_cs(1);
	return true;
}

static bool flash_read_status(uint8_t *status)
{
	uint8_t command = W25Q_CMD_READ_STATUS1;
	flash_cs(0);
	if (!spi_tx(&command, 1) || !spi_rx(status, 1, 0)) {
		flash_cs(1);
		return false;
	}
	flash_cs(1);
	return true;
}

static bool flash_wait_ready(void)
{
	uint32_t guard = W25Q_BUSY_POLL_LIMIT;
	uint8_t status;
	while (guard-- != 0) {
		if (!flash_read_status(&status)) {
			return false;
		}
		if ((status & W25Q_STATUS_BUSY) == 0) {
			return true;
		}
		/* Status transaction is complete and CS is high. I2C is independent. */
		if (wait_hook != 0 && !wait_hook_active) {
			wait_hook_active = 1;
			wait_hook();
			wait_hook_active = 0;
		}
		Delay_us(10);
	}
	return false;
}

static void make_address_command(uint8_t command[4], uint8_t opcode,
								 uint32_t address)
{
	command[0] = opcode;
	command[1] = (uint8_t)(address >> 16);
	command[2] = (uint8_t)(address >> 8);
	command[3] = (uint8_t)address;
}

bool ext_flash_init(void)
{
	uint8_t command = W25Q_CMD_JEDEC_ID;
	uint8_t jedec[3] = {0};

	async_operation = FLASH_ASYNC_NONE;
	/* P04～P06 已由 main.c 中的 spi_init() 配置，只接管 P07 片选。 */
	gpio_config(W25QXX_CS_PIN, OUTPUT, PULL_NONE);
	flash_cs(1);
	flash_wake();
	flash_cs(0);
	if (!spi_tx(&command, 1) || !spi_rx(jedec, sizeof(jedec), 0)) {
		flash_cs(1);
		uart_printf("W25Q64 SPI communication failed\r\n");
		return false;
	}
	flash_cs(1);

	/* 第三个 JEDEC 字节 0x17 表示 64 Mbit；厂商字节可因兼容料不同而变化。 */
	flash_ready = (jedec[0] != 0x00 && jedec[0] != 0xFF &&
					jedec[2] == W25Q_CAPACITY_64MBIT);
	uart_printf("External flash JEDEC=%02x %02x %02x ready=%u rx_chunk=%u\r\n",
				jedec[0], jedec[1], jedec[2], flash_ready,
				W25Q_SPI_RX_CHUNK);
	flash_sleep();
	return flash_ready != 0;
}

bool ext_flash_is_ready(void)
{
	return flash_ready != 0;
}

bool ext_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
	uint8_t command[4];
	uint32_t chunk;

	if (!flash_ready || async_operation != FLASH_ASYNC_NONE || data == 0 ||
		address + length > EXT_FLASH_TOTAL_SIZE) {
		return false;
	}
	flash_wake();
	while (length != 0) {
		chunk = length > W25Q_READ_CHUNK ? W25Q_READ_CHUNK : length;
		make_address_command(command, W25Q_CMD_READ_DATA, address);
		flash_cs(0);
		if (!spi_tx(command, sizeof(command)) || !spi_rx(data, chunk, address)) {
			flash_cs(1);
			flash_sleep();
			return false;
		}
		flash_cs(1);
		address += chunk;
		data += chunk;
		length -= chunk;
	}
	flash_sleep();
	return true;
}

bool ext_flash_write(uint32_t address, const uint8_t *data, uint32_t length)
{
	uint8_t command[4];
	uint32_t chunk;
	uint32_t page_remaining;
	uint32_t started;
	uint32_t elapsed;

	if (!flash_ready || async_operation != FLASH_ASYNC_NONE || data == 0 ||
		address + length > EXT_FLASH_TOTAL_SIZE) {
		return false;
	}
	flash_wake();
	while (length != 0) {
		page_remaining = EXT_FLASH_PAGE_SIZE - (address & (EXT_FLASH_PAGE_SIZE - 1));
		chunk = length < page_remaining ? length : page_remaining;
		if (!flash_write_enable()) {
			flash_sleep();
			return false;
		}
		make_address_command(command, W25Q_CMD_PAGE_PROGRAM, address);
		started = flash_clock_hs();
		flash_cs(0);
		if (!spi_tx(command, sizeof(command)) || !spi_tx(data, chunk)) {
			flash_cs(1);
			flash_sleep();
			return false;
		}
		flash_cs(1);
		if (!flash_wait_ready()) {
			flash_sleep();
			return false;
		}
		elapsed = flash_elapsed_hs(flash_clock_hs(), started);
		flash_stats.page_programs++;
		if (elapsed > flash_stats.max_page_program_hs) {
			flash_stats.max_page_program_hs = elapsed;
		}
		address += chunk;
		data += chunk;
		length -= chunk;
	}
	flash_sleep();
	return true;
}

bool ext_flash_erase_sector(uint32_t address)
{
	uint8_t command[4];
	uint32_t started;
	uint32_t elapsed;

	if (!flash_ready || async_operation != FLASH_ASYNC_NONE ||
		address >= EXT_FLASH_TOTAL_SIZE) {
		return false;
	}
	address &= ~(EXT_FLASH_SECTOR_SIZE - 1);
	flash_wake();
	if (!flash_write_enable()) {
		flash_sleep();
		return false;
	}
	make_address_command(command, W25Q_CMD_SECTOR_ERASE, address);
	started = flash_clock_hs();
	flash_cs(0);
	if (!spi_tx(command, sizeof(command))) {
		flash_cs(1);
		flash_sleep();
		return false;
	}
	flash_cs(1);
	if (!flash_wait_ready()) {
		flash_sleep();
		return false;
	}
	elapsed = flash_elapsed_hs(flash_clock_hs(), started);
	flash_stats.sector_erases++;
	if (elapsed > flash_stats.max_sector_erase_hs) {
		flash_stats.max_sector_erase_hs = elapsed;
	}
	flash_sleep();
	return true;
}

bool ext_flash_async_program_begin(uint32_t address,
								   const uint8_t *data, uint16_t length)
{
	uint8_t command[4];
	uint16_t page_remaining;

	if (!flash_ready || async_operation != FLASH_ASYNC_NONE || data == 0 ||
		length == 0 || length > EXT_FLASH_PAGE_SIZE ||
		address + length > EXT_FLASH_TOTAL_SIZE) {
		return false;
	}
	page_remaining = (uint16_t)(EXT_FLASH_PAGE_SIZE -
		(address & (EXT_FLASH_PAGE_SIZE - 1)));
	if (length > page_remaining) {
		return false;
	}
	flash_wake();
	if (!flash_write_enable()) {
		flash_sleep();
		return false;
	}
	make_address_command(command, W25Q_CMD_PAGE_PROGRAM, address);
	async_started = flash_clock_hs();
	flash_cs(0);
	if (!spi_tx(command, sizeof(command)) || !spi_tx(data, length)) {
		flash_cs(1);
		flash_sleep();
		return false;
	}
	flash_cs(1);
	async_operation = FLASH_ASYNC_PROGRAM;
	return true;
}

bool ext_flash_async_erase_begin(uint32_t address)
{
	uint8_t command[4];

	if (!flash_ready || async_operation != FLASH_ASYNC_NONE ||
		address >= EXT_FLASH_TOTAL_SIZE) {
		return false;
	}
	address &= ~(EXT_FLASH_SECTOR_SIZE - 1);
	flash_wake();
	if (!flash_write_enable()) {
		flash_sleep();
		return false;
	}
	make_address_command(command, W25Q_CMD_SECTOR_ERASE, address);
	async_started = flash_clock_hs();
	flash_cs(0);
	if (!spi_tx(command, sizeof(command))) {
		flash_cs(1);
		flash_sleep();
		return false;
	}
	flash_cs(1);
	async_operation = FLASH_ASYNC_ERASE;
	return true;
}

uint8_t ext_flash_async_poll(void)
{
	uint8_t status;
	uint8_t completed;
	uint32_t elapsed;

	if (async_operation == FLASH_ASYNC_NONE) {
		return EXT_FLASH_ASYNC_DONE;
	}
	if (!flash_read_status(&status)) {
		async_operation = FLASH_ASYNC_NONE;
		flash_sleep();
		return EXT_FLASH_ASYNC_ERROR;
	}
	if (status & W25Q_STATUS_BUSY) {
		return EXT_FLASH_ASYNC_BUSY;
	}
	completed = async_operation;
	async_operation = FLASH_ASYNC_NONE;
	elapsed = flash_elapsed_hs(flash_clock_hs(), async_started);
	if (completed == FLASH_ASYNC_PROGRAM) {
		flash_stats.page_programs++;
		if (elapsed > flash_stats.max_page_program_hs)
			flash_stats.max_page_program_hs = elapsed;
	} else {
		flash_stats.sector_erases++;
		if (elapsed > flash_stats.max_sector_erase_hs)
			flash_stats.max_sector_erase_hs = elapsed;
	}
	flash_sleep();
	return EXT_FLASH_ASYNC_DONE;
}

bool ext_flash_async_busy(void)
{
	return async_operation != FLASH_ASYNC_NONE;
}
