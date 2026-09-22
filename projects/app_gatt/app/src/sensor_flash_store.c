/*
 * 五传感器外部 Flash 日志。
 *
 * 每种传感器使用独立分区，第一扇区保存本次采集会话信息，其余扇区保存
 * 带时间戳和 CRC 的定长记录。数据扇区在首次使用时按需擦除，避免启动一次
 * 采集就阻塞数秒。上传完成后擦除会话头即可立即“逻辑清空”；旧数据所在
 * 扇区会在下一次写到该位置时再物理擦除。
 */
#ifdef SENSOR_FLASH_HOST_TEST
#include "sensor_flash_test_platform.h"
#else
#include "sensor_flash_store.h"
#include "ext_flash.h"
#include "app_user.h"
#include "uart.h"
#endif
#include <string.h>

#define STORE_HEADER_MAGIC         0x34465653UL /* 内存中小端字节为 "SVF4" */
#define STORE_HEADER_VERSION       1U
#define STORE_HEADER_SIZE          20U
#define STORE_RECORD_MAGIC         0xA55AU
#define STORE_SECONDS_PER_DAY      86400UL
#define STORE_SCAN_BUFFER_SIZE     EXT_FLASH_SECTOR_SIZE
#define STORE_ACC_SLOT_SIZE        20U

typedef struct {
	uint32_t base;
	uint32_t size;
	uint16_t record_len;
	uint16_t slot_size;
} store_partition_t;

typedef struct {
	uint8_t valid;
	uint16_t session_id;
	uint16_t period_s;
	uint32_t count;
	uint32_t max_records;
} store_context_t;

/*
 * W25Q64 共 8 MiB：
 * 0x000000～0x0FFFFF：四种低速数据，各 256 KiB；
 * 0x100000～0x7FFFFF：变速加速度数据，使用剩余 7 MiB。
 */
static const store_partition_t partitions[PROTOCOL_SENSOR_COUNT] = {
	{0x000000UL, 0x040000UL, 22U,  32U},
	{0x040000UL, 0x040000UL, 256U, 268U},
	{0x080000UL, 0x040000UL, 8U,   20U},
	{0x0C0000UL, 0x040000UL, 4U,   16U},
	/* 加速度：运动标记(1 byte) + X/Y/Z(3 x int16_t)，占用剩余 7 MiB。 */
	{0x100000UL, 0x700000UL, 7U,   20U},
};

static store_context_t contexts[PROTOCOL_SENSOR_COUNT];
static uint8_t scan_buffer[STORE_SCAN_BUFFER_SIZE];
static uint8_t acc_batch_body[SENSOR_FLASH_ACC_BATCH_MAX * STORE_ACC_SLOT_SIZE];
static uint8_t acc_batch_commit[SENSOR_FLASH_ACC_BATCH_MAX * STORE_ACC_SLOT_SIZE];
static uint8_t store_ready;
static struct {
	uint8_t state;
	uint8_t count;
	uint8_t final_result;
	uint16_t total_len;
	uint16_t offset;
	uint16_t chunk;
	uint32_t address;
} acc_async;

enum {
	ACC_ASYNC_IDLE = 0,
	ACC_ASYNC_ERASE_BEGIN,
	ACC_ASYNC_ERASE_WAIT,
	ACC_ASYNC_BODY_BEGIN,
	ACC_ASYNC_BODY_WAIT,
	ACC_ASYNC_COMMIT_BEGIN,
	ACC_ASYNC_COMMIT_WAIT,
	ACC_ASYNC_COMPLETE,
};

static uint16_t get_le16(const uint8_t *data)
{
	return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t get_le32(const uint8_t *data)
{
	return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static void put_le16(uint8_t *data, uint16_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
}

static void put_le32(uint8_t *data, uint32_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
	data[2] = (uint8_t)(value >> 16);
	data[3] = (uint8_t)(value >> 24);
}

static uint16_t store_crc16(const uint8_t *data, uint16_t length)
{
	uint16_t crc = 0xFFFF;
	uint8_t bit;
	while (length-- != 0) {
		crc ^= (uint16_t)(*data++) << 8;
		for (bit = 0; bit < 8; bit++) {
			crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) :
				(uint16_t)(crc << 1);
		}
	}
	return crc;
}

static uint32_t slots_per_sector(uint8_t sensor_id)
{
	return EXT_FLASH_SECTOR_SIZE / partitions[sensor_id].slot_size;
}

static uint32_t physical_capacity(uint8_t sensor_id)
{
	uint32_t data_sectors = partitions[sensor_id].size / EXT_FLASH_SECTOR_SIZE - 1;
	return data_sectors * slots_per_sector(sensor_id);
}

static uint32_t one_day_capacity(uint8_t sensor_id, uint16_t period_s)
{
	uint32_t day_records;
	uint32_t capacity;
	if (period_s == 0) {
		return 0;
	}
	capacity = physical_capacity(sensor_id);
	/* LOW 1Hz/HIGH 50Hz 是变速数据，无法按固定 period_s 推算一天条数。 */
	if (sensor_id == PROTOCOL_SENSOR_ACC) {
		return capacity;
	}
	day_records = (STORE_SECONDS_PER_DAY + period_s - 1) / period_s;
	return day_records < capacity ? day_records : capacity;
}

static uint32_t record_address(uint8_t sensor_id, uint32_t index)
{
	uint32_t per_sector = slots_per_sector(sensor_id);
	uint32_t sector = index / per_sector;
	uint32_t slot = index % per_sector;
	return partitions[sensor_id].base + EXT_FLASH_SECTOR_SIZE +
		sector * EXT_FLASH_SECTOR_SIZE + slot * partitions[sensor_id].slot_size;
}

static bool header_decode(uint8_t sensor_id, const uint8_t header[STORE_HEADER_SIZE])
{
	uint16_t period_s;
	if (get_le32(header) != STORE_HEADER_MAGIC ||
		header[4] != STORE_HEADER_VERSION || header[5] != sensor_id ||
		get_le16(&header[8]) != partitions[sensor_id].record_len ||
		get_le16(&header[10]) != partitions[sensor_id].slot_size ||
		store_crc16(header, 18) != get_le16(&header[18])) {
		return false;
	}
	period_s = get_le16(&header[16]);
	if (period_s == 0) {
		return false;
	}
	contexts[sensor_id].valid = 1;
	contexts[sensor_id].session_id = get_le16(&header[6]);
	contexts[sensor_id].period_s = period_s;
	contexts[sensor_id].max_records = one_day_capacity(sensor_id, period_s);
	return true;
}

static bool record_valid(uint8_t sensor_id, const uint8_t *record)
{
	uint16_t crc_offset = 8 + partitions[sensor_id].record_len;
	return get_le16(record) == STORE_RECORD_MAGIC &&
		get_le16(&record[2]) == contexts[sensor_id].session_id &&
		store_crc16(&record[2], (uint16_t)(crc_offset - 2)) ==
		get_le16(&record[crc_offset]);
}

static bool scan_records(uint8_t sensor_id)
{
	uint32_t sector;
	uint32_t slot;
	uint32_t sector_count = partitions[sensor_id].size / EXT_FLASH_SECTOR_SIZE - 1;
	uint32_t per_sector = slots_per_sector(sensor_id);
	uint32_t address = partitions[sensor_id].base + EXT_FLASH_SECTOR_SIZE;

	contexts[sensor_id].count = 0;
	for (sector = 0; sector < sector_count; sector++, address += EXT_FLASH_SECTOR_SIZE) {
		if (!ext_flash_read(address, scan_buffer, sizeof(scan_buffer))) {
			return false;
		}
		for (slot = 0; slot < per_sector; slot++) {
			uint8_t *record = &scan_buffer[slot * partitions[sensor_id].slot_size];
			if (!record_valid(sensor_id, record)) {
				return true;
			}
			contexts[sensor_id].count++;
			if (contexts[sensor_id].count >= contexts[sensor_id].max_records) {
				return true;
			}
		}
	}
	return true;
}

bool sensor_flash_store_init(void)
{
	uint8_t sensor_id;
	uint8_t header[STORE_HEADER_SIZE];

	memset(contexts, 0, sizeof(contexts));
	memset(&acc_async, 0, sizeof(acc_async));
	store_ready = ext_flash_init() ? 1 : 0;
	if (!store_ready) {
		return false;
	}
	/* 上电扫描有效日志，断电重启后仍可继续上传当天已保存的数据。 */
	for (sensor_id = 0; sensor_id < PROTOCOL_SENSOR_COUNT; sensor_id++) {
		if (!ext_flash_read(partitions[sensor_id].base, header, sizeof(header))) {
			store_ready = 0;
			return false;
		}
		if (header_decode(sensor_id, header)) {
			if (!scan_records(sensor_id)) {
				store_ready = 0;
				return false;
			}
			uart_printf("Flash history sensor=%u count=%u/%u\r\n",
						sensor_id + 1, contexts[sensor_id].count,
						contexts[sensor_id].max_records);
		}
	}
	return true;
}

bool sensor_flash_store_ready(void)
{
	return store_ready != 0;
}

bool sensor_flash_store_start(uint8_t sensor_id, uint32_t start_time,
								  uint16_t period_s)
{
	uint8_t header[STORE_HEADER_SIZE];
	uint16_t session_id;

	if (!store_ready || sensor_id >= PROTOCOL_SENSOR_COUNT || period_s == 0 ||
		one_day_capacity(sensor_id, period_s) == 0) {
		return false;
	}
	memset(header, 0xFF, sizeof(header));
	put_le32(&header[0], STORE_HEADER_MAGIC);
	header[4] = STORE_HEADER_VERSION;
	header[5] = sensor_id;
	session_id = (uint16_t)(start_time ^ (start_time >> 16) ^ sensor_id ^ 0x5A5A);
	if (session_id == 0 || session_id == 0xFFFF) {
		session_id = (uint16_t)(0x100 + sensor_id);
	}
	put_le16(&header[6], session_id);
	put_le16(&header[8], partitions[sensor_id].record_len);
	put_le16(&header[10], partitions[sensor_id].slot_size);
	put_le32(&header[12], start_time);
	put_le16(&header[16], period_s);
	put_le16(&header[18], store_crc16(header, 18));

	/* 新的 0x4E 只清空所选传感器，不影响另外四个分区。 */
	if (!ext_flash_erase_sector(partitions[sensor_id].base) ||
		!ext_flash_write(partitions[sensor_id].base, header, sizeof(header))) {
		return false;
	}
	contexts[sensor_id].valid = 1;
	contexts[sensor_id].session_id = session_id;
	contexts[sensor_id].period_s = period_s;
	contexts[sensor_id].count = 0;
	contexts[sensor_id].max_records = one_day_capacity(sensor_id, period_s);
	return true;
}

bool sensor_flash_store_append(uint8_t sensor_id, uint32_t timestamp,
								   const uint8_t *data, uint16_t length)
{
	uint8_t record[268];
	uint8_t magic[2];
	uint16_t crc_offset;
	uint32_t address;
	uint32_t per_sector;

	if (!store_ready || sensor_id >= PROTOCOL_SENSOR_COUNT ||
		!contexts[sensor_id].valid || data == 0 ||
		length != partitions[sensor_id].record_len ||
		contexts[sensor_id].count >= contexts[sensor_id].max_records) {
		return false;
	}
	per_sector = slots_per_sector(sensor_id);
	address = record_address(sensor_id, contexts[sensor_id].count);
	/* 每个数据扇区第一次写入前物理擦除，随后顺序追加。 */
	if ((contexts[sensor_id].count % per_sector) == 0 &&
		!ext_flash_erase_sector(address)) {
		return false;
	}
	memset(record, 0xFF, partitions[sensor_id].slot_size);
	put_le16(&record[2], contexts[sensor_id].session_id);
	put_le32(&record[4], timestamp);
	memcpy(&record[8], data, length);
	crc_offset = 8 + length;
	put_le16(&record[crc_offset], store_crc16(&record[2], (uint16_t)(crc_offset - 2)));

	/* 记录魔数最后写入，相当于提交标志；掉电造成的半条记录不会被上传。 */
	if (!ext_flash_write(address + 2, &record[2],
						 partitions[sensor_id].slot_size - 2)) {
		return false;
	}
	put_le16(magic, STORE_RECORD_MAGIC);
	if (!ext_flash_write(address, magic, sizeof(magic))) {
		return false;
	}
	contexts[sensor_id].count++;
	return true;
}

uint8_t sensor_flash_store_append_acc_batch(
		const uint32_t timestamps[],
		uint8_t records[][SENSOR_FLASH_ACC_RECORD_LEN],
		uint8_t requested, uint8_t *accepted)
{
	const uint8_t sensor_id = PROTOCOL_SENSOR_ACC;
	uint32_t per_sector;
	uint32_t sector_remaining;
	uint32_t capacity_remaining;
	uint32_t address;
	uint16_t offset;
	uint16_t crc_offset;
	uint16_t total_len;
	uint8_t count;
	uint8_t i;

	if (accepted == 0 || acc_async.state != ACC_ASYNC_IDLE ||
		ext_flash_async_busy()) {
		return SENSOR_FLASH_BATCH_ERROR;
	}
	*accepted = 0;
	if (!store_ready || !contexts[sensor_id].valid || timestamps == 0 ||
		records == 0 || requested == 0 ||
		partitions[sensor_id].record_len != SENSOR_FLASH_ACC_RECORD_LEN ||
		partitions[sensor_id].slot_size != STORE_ACC_SLOT_SIZE) {
		return SENSOR_FLASH_BATCH_ERROR;
	}
	if (contexts[sensor_id].count >= contexts[sensor_id].max_records) {
		return SENSOR_FLASH_BATCH_FULL;
	}

	per_sector = slots_per_sector(sensor_id);
	sector_remaining = per_sector - (contexts[sensor_id].count % per_sector);
	capacity_remaining = contexts[sensor_id].max_records - contexts[sensor_id].count;
	count = requested > SENSOR_FLASH_ACC_BATCH_MAX ?
		SENSOR_FLASH_ACC_BATCH_MAX : requested;
	if (count > sector_remaining) {
		count = (uint8_t)sector_remaining;
	}
	if (count > capacity_remaining) {
		count = (uint8_t)capacity_remaining;
	}
	if (count == 0) {
		return SENSOR_FLASH_BATCH_FULL;
	}

	address = record_address(sensor_id, contexts[sensor_id].count);
	if ((contexts[sensor_id].count % per_sector) == 0 &&
		!ext_flash_erase_sector(address)) {
		return SENSOR_FLASH_BATCH_ERROR;
	}

	total_len = (uint16_t)count * STORE_ACC_SLOT_SIZE;
	memset(acc_batch_body, 0xFF, total_len);
	memset(acc_batch_commit, 0xFF, total_len);
	crc_offset = 8U + SENSOR_FLASH_ACC_RECORD_LEN;
	for (i = 0; i < count; i++) {
		offset = (uint16_t)i * STORE_ACC_SLOT_SIZE;
		put_le16(&acc_batch_body[offset + 2], contexts[sensor_id].session_id);
		put_le32(&acc_batch_body[offset + 4], timestamps[i]);
		memcpy(&acc_batch_body[offset + 8], records[i], SENSOR_FLASH_ACC_RECORD_LEN);
		put_le16(&acc_batch_body[offset + crc_offset],
			store_crc16(&acc_batch_body[offset + 2], (uint16_t)(crc_offset - 2)));
		put_le16(&acc_batch_commit[offset], STORE_RECORD_MAGIC);
	}

	/* Two page-oriented passes retain the original "magic written last" rule. */
	if (!ext_flash_write(address, acc_batch_body, total_len) ||
		!ext_flash_write(address, acc_batch_commit, total_len)) {
		return SENSOR_FLASH_BATCH_ERROR;
	}
	contexts[sensor_id].count += count;
	*accepted = count;
	return contexts[sensor_id].count >= contexts[sensor_id].max_records ?
		SENSOR_FLASH_BATCH_LAST : SENSOR_FLASH_BATCH_OK;
}

static uint16_t acc_async_page_chunk(void)
{
	uint16_t remaining = (uint16_t)(acc_async.total_len - acc_async.offset);
	uint16_t page_remaining = (uint16_t)(EXT_FLASH_PAGE_SIZE -
		((acc_async.address + acc_async.offset) & (EXT_FLASH_PAGE_SIZE - 1)));
	return remaining < page_remaining ? remaining : page_remaining;
}

uint8_t sensor_flash_store_acc_async_begin(
		const uint32_t timestamps[],
		uint8_t records[][SENSOR_FLASH_ACC_RECORD_LEN], uint8_t requested)
{
	const uint8_t sensor_id = PROTOCOL_SENSOR_ACC;
	uint32_t per_sector;
	uint32_t sector_remaining;
	uint32_t capacity_remaining;
	uint16_t offset;
	uint16_t crc_offset;
	uint8_t count;
	uint8_t i;

	if (acc_async.state != ACC_ASYNC_IDLE || ext_flash_async_busy())
		return SENSOR_FLASH_BATCH_BUSY;
	if (!store_ready || !contexts[sensor_id].valid || timestamps == 0 ||
		records == 0 || requested == 0 ||
		partitions[sensor_id].record_len != SENSOR_FLASH_ACC_RECORD_LEN ||
		partitions[sensor_id].slot_size != STORE_ACC_SLOT_SIZE)
		return SENSOR_FLASH_BATCH_ERROR;
	if (contexts[sensor_id].count >= contexts[sensor_id].max_records)
		return SENSOR_FLASH_BATCH_FULL;

	per_sector = slots_per_sector(sensor_id);
	sector_remaining = per_sector - (contexts[sensor_id].count % per_sector);
	capacity_remaining = contexts[sensor_id].max_records - contexts[sensor_id].count;
	count = requested > SENSOR_FLASH_ACC_BATCH_MAX ?
		SENSOR_FLASH_ACC_BATCH_MAX : requested;
	if (count > sector_remaining) count = (uint8_t)sector_remaining;
	if (count > capacity_remaining) count = (uint8_t)capacity_remaining;
	if (count == 0) return SENSOR_FLASH_BATCH_FULL;

	acc_async.address = record_address(sensor_id, contexts[sensor_id].count);
	acc_async.count = count;
	acc_async.total_len = (uint16_t)count * STORE_ACC_SLOT_SIZE;
	acc_async.offset = 0;
	acc_async.chunk = 0;
	acc_async.final_result =
		contexts[sensor_id].count + count >= contexts[sensor_id].max_records ?
		SENSOR_FLASH_BATCH_LAST : SENSOR_FLASH_BATCH_OK;
	memset(acc_batch_body, 0xFF, acc_async.total_len);
	memset(acc_batch_commit, 0xFF, acc_async.total_len);
	crc_offset = 8U + SENSOR_FLASH_ACC_RECORD_LEN;
	for (i = 0; i < count; i++) {
		offset = (uint16_t)i * STORE_ACC_SLOT_SIZE;
		put_le16(&acc_batch_body[offset + 2], contexts[sensor_id].session_id);
		put_le32(&acc_batch_body[offset + 4], timestamps[i]);
		memcpy(&acc_batch_body[offset + 8], records[i],
			SENSOR_FLASH_ACC_RECORD_LEN);
		put_le16(&acc_batch_body[offset + crc_offset],
			store_crc16(&acc_batch_body[offset + 2], (uint16_t)(crc_offset - 2)));
		put_le16(&acc_batch_commit[offset], STORE_RECORD_MAGIC);
	}
	acc_async.state = (contexts[sensor_id].count % per_sector) == 0 ?
		ACC_ASYNC_ERASE_BEGIN : ACC_ASYNC_BODY_BEGIN;
	return SENSOR_FLASH_BATCH_BUSY;
}

static bool acc_async_start_body(void)
{
	acc_async.chunk = acc_async_page_chunk();
	if (!ext_flash_async_program_begin(acc_async.address + acc_async.offset,
		&acc_batch_body[acc_async.offset], acc_async.chunk)) return false;
	acc_async.state = ACC_ASYNC_BODY_WAIT;
	return true;
}

static bool acc_async_start_commit(void)
{
	acc_async.chunk = acc_async_page_chunk();
	if (!ext_flash_async_program_begin(acc_async.address + acc_async.offset,
		&acc_batch_commit[acc_async.offset], acc_async.chunk)) return false;
	acc_async.state = ACC_ASYNC_COMMIT_WAIT;
	return true;
}

static uint8_t acc_async_complete(uint8_t *accepted)
{
	uint8_t result = acc_async.final_result;
	contexts[PROTOCOL_SENSOR_ACC].count += acc_async.count;
	*accepted = acc_async.count;
	memset(&acc_async, 0, sizeof(acc_async));
	return result;
}

uint8_t sensor_flash_store_acc_async_process(uint8_t *accepted)
{
	uint8_t poll;
	if (accepted == 0) return SENSOR_FLASH_BATCH_ERROR;
	*accepted = 0;
	switch (acc_async.state) {
	case ACC_ASYNC_IDLE:
		return SENSOR_FLASH_BATCH_OK;
	case ACC_ASYNC_ERASE_BEGIN:
		if (!ext_flash_async_erase_begin(acc_async.address))
			goto failed;
		acc_async.state = ACC_ASYNC_ERASE_WAIT;
		return SENSOR_FLASH_BATCH_BUSY;
	case ACC_ASYNC_ERASE_WAIT:
		poll = ext_flash_async_poll();
		if (poll == EXT_FLASH_ASYNC_ERROR) goto failed;
		if (poll == EXT_FLASH_ASYNC_DONE && !acc_async_start_body()) goto failed;
		return SENSOR_FLASH_BATCH_BUSY;
	case ACC_ASYNC_BODY_BEGIN:
		if (!acc_async_start_body()) goto failed;
		return SENSOR_FLASH_BATCH_BUSY;
	case ACC_ASYNC_BODY_WAIT:
		poll = ext_flash_async_poll();
		if (poll == EXT_FLASH_ASYNC_ERROR) goto failed;
		if (poll == EXT_FLASH_ASYNC_BUSY) return SENSOR_FLASH_BATCH_BUSY;
		acc_async.offset = (uint16_t)(acc_async.offset + acc_async.chunk);
		if (acc_async.offset < acc_async.total_len) {
			if (!acc_async_start_body()) goto failed;
		} else {
			acc_async.offset = 0;
			if (!acc_async_start_commit()) goto failed;
		}
		return SENSOR_FLASH_BATCH_BUSY;
	case ACC_ASYNC_COMMIT_BEGIN:
		if (!acc_async_start_commit()) goto failed;
		return SENSOR_FLASH_BATCH_BUSY;
	case ACC_ASYNC_COMMIT_WAIT:
		poll = ext_flash_async_poll();
		if (poll == EXT_FLASH_ASYNC_ERROR) goto failed;
		if (poll == EXT_FLASH_ASYNC_BUSY) return SENSOR_FLASH_BATCH_BUSY;
		acc_async.offset = (uint16_t)(acc_async.offset + acc_async.chunk);
		if (acc_async.offset < acc_async.total_len) {
			if (!acc_async_start_commit()) goto failed;
			return SENSOR_FLASH_BATCH_BUSY;
		}
		return acc_async_complete(accepted);
	case ACC_ASYNC_COMPLETE:
		return acc_async_complete(accepted);
	default:
		break;
	}
failed:
	memset(&acc_async, 0, sizeof(acc_async));
	return SENSOR_FLASH_BATCH_ERROR;
}

bool sensor_flash_store_acc_async_busy(void)
{
	return acc_async.state != ACC_ASYNC_IDLE || ext_flash_async_busy();
}

bool sensor_flash_store_read(uint8_t sensor_id, uint32_t index,
								 uint32_t *timestamp, uint8_t *data,
								 uint16_t length)
{
	uint8_t record[268];
	uint8_t attempt;
	uint32_t address;
	if (!store_ready || sensor_id >= PROTOCOL_SENSOR_COUNT ||
		!contexts[sensor_id].valid || index >= contexts[sensor_id].count ||
		timestamp == 0 || data == 0 || length != partitions[sensor_id].record_len) {
		return false;
	}
	address = record_address(sensor_id, index);
	for (attempt = 0; attempt < 2; attempt++) {
		if (!ext_flash_read(address, record, partitions[sensor_id].slot_size)) {
			uart_printf("Flash record read fail sensor=%u index=%u addr=%x\r\n",
				sensor_id + 1, index, address);
			return false;
		}
		if (record_valid(sensor_id, record)) {
			if (attempt != 0)
				uart_printf("Flash CRC retry ok sensor=%u index=%u\r\n",
					sensor_id + 1, index);
			break;
		}
		if (attempt == 0) {
			uart_printf("Flash CRC retry sensor=%u index=%u addr=%x\r\n",
				sensor_id + 1, index, address);
		}
	}
	if (attempt == 2) {
		uart_printf("Flash CRC fail sensor=%u index=%u addr=%x\r\n",
			sensor_id + 1, index, address);
		return false;
	}
	*timestamp = get_le32(&record[4]);
	memcpy(data, &record[8], length);
	return true;
}

bool sensor_flash_store_clear(uint8_t sensor_id)
{
	if (!store_ready || sensor_id >= PROTOCOL_SENSOR_COUNT) {
		return false;
	}
	/* 擦掉索引头后旧记录不可见；数据扇区下次使用时再逐扇区物理擦除。 */
	if (!ext_flash_erase_sector(partitions[sensor_id].base)) {
		return false;
	}
	memset(&contexts[sensor_id], 0, sizeof(contexts[sensor_id]));
	return true;
}

uint32_t sensor_flash_store_count(uint8_t sensor_id)
{
	return sensor_id < PROTOCOL_SENSOR_COUNT ? contexts[sensor_id].count : 0;
}

uint16_t sensor_flash_store_record_len(uint8_t sensor_id)
{
	return sensor_id < PROTOCOL_SENSOR_COUNT ? partitions[sensor_id].record_len : 0;
}

bool sensor_flash_store_full(uint8_t sensor_id)
{
	return sensor_id < PROTOCOL_SENSOR_COUNT && contexts[sensor_id].valid &&
		contexts[sensor_id].count >= contexts[sensor_id].max_records;
}
