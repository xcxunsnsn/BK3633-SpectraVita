#ifndef SENSOR_FLASH_STORE_H_
#define SENSOR_FLASH_STORE_H_

#include <stdbool.h>
#include <stdint.h>

/* 五种传感器分别保存为独立日志；sensor_id 顺序与协议 TYPE-1 相同。 */
bool sensor_flash_store_init(void);
bool sensor_flash_store_ready(void);
bool sensor_flash_store_start(uint8_t sensor_id, uint32_t start_time,
								  uint16_t period_s);
bool sensor_flash_store_append(uint8_t sensor_id, uint32_t timestamp,
								   const uint8_t *data, uint16_t length);

#define SENSOR_FLASH_ACC_RECORD_LEN 7U
#define SENSOR_FLASH_ACC_BATCH_MAX  8U
#define SENSOR_FLASH_BATCH_ERROR    0U
#define SENSOR_FLASH_BATCH_OK       1U
#define SENSOR_FLASH_BATCH_LAST     2U
#define SENSOR_FLASH_BATCH_FULL     3U
#define SENSOR_FLASH_BATCH_BUSY     4U

/* Acceleration-only bulk path; returns status and writes the accepted count. */
uint8_t sensor_flash_store_append_acc_batch(
		const uint32_t timestamps[],
		uint8_t records[][SENSOR_FLASH_ACC_RECORD_LEN],
		uint8_t requested, uint8_t *accepted);
/* Cooperative acceleration writer. begin() copies the batch into private RAM;
 * process() advances at most one erase/program/status step per main loop. */
uint8_t sensor_flash_store_acc_async_begin(
		const uint32_t timestamps[],
		uint8_t records[][SENSOR_FLASH_ACC_RECORD_LEN], uint8_t requested);
uint8_t sensor_flash_store_acc_async_process(uint8_t *accepted);
bool sensor_flash_store_acc_async_busy(void);
bool sensor_flash_store_read(uint8_t sensor_id, uint32_t index,
								 uint32_t *timestamp, uint8_t *data,
								 uint16_t length);
bool sensor_flash_store_clear(uint8_t sensor_id);
uint32_t sensor_flash_store_count(uint8_t sensor_id);
uint16_t sensor_flash_store_record_len(uint8_t sensor_id);
bool sensor_flash_store_full(uint8_t sensor_id);

#endif /* SENSOR_FLASH_STORE_H_ */
