#ifndef PERIOD_DUE_QUEUE_H_
#define PERIOD_DUE_QUEUE_H_

#include <stdbool.h>
#include <stdint.h>

#define PERIOD_DUE_SENSOR_COUNT 4U

typedef struct {
	uint32_t due;
	uint32_t started;
	uint32_t late_total_s;
	uint16_t max_late_s;
	uint16_t missed;
} period_due_stats_t;

typedef struct {
	uint8_t mask;
	uint32_t tick_s;
	uint32_t due_tick[PERIOD_DUE_SENSOR_COUNT];
	period_due_stats_t stats[PERIOD_DUE_SENSOR_COUNT];
} period_due_queue_t;

void period_due_queue_init(period_due_queue_t *queue);
void period_due_queue_tick(period_due_queue_t *queue);
void period_due_queue_mark(period_due_queue_t *queue, uint8_t sensor,
							   bool already_running);
bool period_due_queue_take(period_due_queue_t *queue, uint8_t running_mask,
							   bool allowed, uint8_t *sensor,
							   uint16_t *late_s);
void period_due_queue_clear(period_due_queue_t *queue, uint8_t sensor);
void period_due_queue_clear_all(period_due_queue_t *queue);
uint8_t period_due_queue_mask(const period_due_queue_t *queue);
const period_due_stats_t *period_due_queue_stats(
		const period_due_queue_t *queue, uint8_t sensor);

#endif /* PERIOD_DUE_QUEUE_H_ */
