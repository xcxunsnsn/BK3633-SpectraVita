#ifndef ACC_TEST_PLATFORM_H
#define ACC_TEST_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "acc_motion_config.h"

#define ADXL_IN1_PIN 0x20
#define INPUT 0
#define PULL_NONE 0
#define RISE_EDGE 1
#define POS_SYS_REG0X10_INT_EN_GPIO 0
#define APP_ACC_FIFO_POLL 1
#define APP_READ_ACC 2
#define APP_ACC_FLASH_STEP 3
#define TASK_APP 0
#define TASK_ID_ACC 4
#define PROTOCOL_SENSOR_ACC 4
#define RWIP_MAX_CLOCK_TIME ((1UL << 28) - 1)
#define ADXL345_INT_OVERRUN 0x01
#define ADXL345_INT_WATERMARK 0x02
#define ADXL345_INT_INACTIVITY 0x08
#define ADXL345_INT_ACTIVITY 0x10
#define ADXL345_ACT_X_SOURCE 0x40
#define ADXL345_ACT_Y_SOURCE 0x20
#define ADXL345_ACT_Z_SOURCE 0x10
#define ACC_COMMIT_ERROR 0U
#define ACC_COMMIT_OK 1U
#define ACC_COMMIT_LAST 2U
#define ACC_COMMIT_FULL 3U
#define ACC_COMMIT_BUSY 4U

typedef struct { int16_t acc_x1, acc_y1, acc_z1; } SensorAccStruct;
typedef struct { uint32_t hs, hus; } rwip_time_t;
typedef void (*ext_flash_wait_hook_t)(void);

extern volatile uint32_t test_gpio_status;
extern volatile uint32_t test_gpio_enable;
extern volatile uint32_t test_system_int_enable;
extern volatile uint8_t test_gpio_input;
#define REG_GPIO_WUATOD_STATUS test_gpio_status
#define REG_GPIO_WUATOD_ENABLE test_gpio_enable
#define SYS_REG0X10_INT_EN test_system_int_enable

rwip_time_t rwip_time_get(void);
void gpio_config(uint8_t pin, int direction, int pull);
void gpio_cb_register(void (*callback)(void));
void gpio_set_int_mode(uint8_t pin, uint8_t mode);
uint8_t gpio_get_input(uint8_t pin);
void ke_timer_set(uint16_t id, uint16_t task, uint32_t delay);
void ke_timer_clear(uint16_t id, uint16_t task);
void app_task_quit_lowpower(uint8_t id);
void app_task_enter_lowpower(uint8_t id);
int uart_printf(const char *format, ...);
void utc_update(void);
uint32_t utc_get_clock(void);
void ext_flash_set_wait_hook(ext_flash_wait_hook_t hook);

void ADXL345_Init(void);
void ADXL345_Enable(int flag);
bool ADXL345_LowRate_Start(uint8_t threshold, uint8_t control);
bool ADXL345_LowRate_ArmActivity(void);
bool ADXL345_Stream_Start(uint8_t watermark, uint8_t inactivity_threshold,
                          uint8_t inactivity_time,
                          uint8_t inactivity_control);
bool ADXL345_Stream_Freeze(void);
void ADXL345_Stream_Stop(void);
bool ADXL345_read_data(short *x, short *y, short *z);
bool ADXL345_FIFO_Status(uint8_t *entries, uint8_t *source);
bool ADXL345_FIFO_Entries_Checked(uint8_t *entries);
bool ADXL345_FIFO_Read_Checked(int16_t xyz[][3], uint8_t limit,
                              uint8_t *read_count);
bool ADXL345_Int_Source_Checked(uint8_t *source);
bool ADXL345_Activity_Status_Checked(uint8_t *activity_status,
                                     uint8_t *source);

uint8_t app_protocol_acc_commit(uint32_t timestamp, uint8_t state,
                                int16_t x, int16_t y, int16_t z);
#define ACC_COMMIT_BATCH_MAX 8U
#define ACC_HISTORY_RECORD_LEN 7U
uint8_t app_protocol_acc_commit_batch(
        const uint32_t timestamps[],
        uint8_t records[][ACC_HISTORY_RECORD_LEN],
        uint8_t requested, uint8_t *accepted);
uint8_t app_protocol_acc_commit_async_begin(
        const uint32_t timestamps[],
        uint8_t records[][ACC_HISTORY_RECORD_LEN], uint8_t requested);
uint8_t app_protocol_acc_commit_async_process(uint8_t *accepted);
bool app_protocol_acc_flash_allowed(void);
void app_protocol_acc_stream_fault(const char *reason);
void app_ble_timing_report(const char *event, uint8_t reason);
void app_protocol_sensor_data(uint8_t sensor, const uint8_t *data,
                              uint16_t length);

void acc_gpio_interrupt_callback(void);
bool acc_stream_start(void);
bool acc_stream_stop(void);
bool acc_stream_pause(void);
void acc_stream_cancel_pause(void);
bool acc_stream_resume(void);
void acc_stream_process(void);
void acc_stream_storage_process(void);
void acc_stream_storage_tick(void);
void acc_stream_poll_task(void);
void acc_stream_second_tick(void);
bool acc_stream_is_active(void);
bool acc_stream_is_stopped(void);
bool acc_stream_is_paused(void);
bool acc_stream_storage_busy(void);
bool acc_stream_can_yield_flash(void);
bool acc_stream_data_complete(void);
void acc_stream_history_cleared(void);
uint8_t acc_stream_motion_state(void);
void app_read_acc_task(void);
void acc_collect_once_report(uint8_t again);

#endif
