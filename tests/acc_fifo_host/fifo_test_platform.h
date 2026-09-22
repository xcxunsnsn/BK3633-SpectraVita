#ifndef FIFO_TEST_PLATFORM_H
#define FIFO_TEST_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#define SENSOR_DEFAULT_FIFO_TIME 720U
#define SENSOR_DEFAULT_FIFO_GAIN 5U

#define TASK_ID_FIFO 1U
#define PROTOCOL_SENSOR_FIFO 1U
#define APP_READ_FIFO 100U
#define TASK_APP 1U

bool AS7341_StartFlickerFIFO(uint16_t fd_time, uint8_t fd_gain);
bool AS7341_GetFlickerFIFOStatus(uint8_t *level, uint8_t *status6);
bool AS7341_ReadFlickerFIFO(uint16_t *buf, uint8_t max_samples,
								 uint8_t *len);
bool AS7341_StopFlickerFIFO(void);
bool app_task_get_mutex(uint8_t task_id);
void app_task_quit_lowpower(uint8_t task_id);
void app_task_enter_lowpower(uint8_t task_id);
void app_protocol_sensor_data(uint8_t sensor_id, const uint8_t *data,
								  uint16_t len);
void ke_timer_set(uint16_t timer_id, uint16_t task, uint32_t delay);
int uart_printf(const char *format, ...);

#endif
