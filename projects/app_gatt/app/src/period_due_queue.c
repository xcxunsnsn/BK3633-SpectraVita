#include "period_due_queue.h"
#include <string.h>

void period_due_queue_init(period_due_queue_t *queue)
{
	if (queue != 0) memset(queue, 0, sizeof(*queue));
}

void period_due_queue_tick(period_due_queue_t *queue)
{
	if (queue != 0) queue->tick_s++;
}

void period_due_queue_mark(period_due_queue_t *queue, uint8_t sensor,
							   bool already_running)
{
	uint8_t bit;
	if (queue == 0 || sensor >= PERIOD_DUE_SENSOR_COUNT) return;
	bit = (uint8_t)(1U << sensor);
	queue->stats[sensor].due++;
	if ((queue->mask & bit) != 0 || already_running) {
		if (queue->stats[sensor].missed != 0xFFFF)
			queue->stats[sensor].missed++;
		return;
	}
	queue->mask |= bit;
	queue->due_tick[sensor] = queue->tick_s;
}

bool period_due_queue_take(period_due_queue_t *queue, uint8_t running_mask,
							   bool allowed, uint8_t *sensor,
							   uint16_t *late_s)
{
	uint8_t i;
	uint32_t late;
	if (queue == 0 || sensor == 0 || late_s == 0 || !allowed ||
		(running_mask & 0x0FU) != 0) return false;
	for (i = 0; i < PERIOD_DUE_SENSOR_COUNT; i++) {
		if ((queue->mask & (1U << i)) == 0) continue;
		queue->mask &= (uint8_t)~(1U << i);
		late = queue->tick_s - queue->due_tick[i];
		if (late > 0xFFFFUL) late = 0xFFFFUL;
		queue->stats[i].started++;
		queue->stats[i].late_total_s += late;
		if (late > queue->stats[i].max_late_s)
			queue->stats[i].max_late_s = (uint16_t)late;
		*sensor = i;
		*late_s = (uint16_t)late;
		return true;
	}
	return false;
}

void period_due_queue_clear(period_due_queue_t *queue, uint8_t sensor)
{
	if (queue != 0 && sensor < PERIOD_DUE_SENSOR_COUNT)
		queue->mask &= (uint8_t)~(1U << sensor);
}

void period_due_queue_clear_all(period_due_queue_t *queue)
{
	if (queue != 0) queue->mask = 0;
}

uint8_t period_due_queue_mask(const period_due_queue_t *queue)
{
	return queue != 0 ? queue->mask : 0;
}

const period_due_stats_t *period_due_queue_stats(
		const period_due_queue_t *queue, uint8_t sensor)
{
	return queue != 0 && sensor < PERIOD_DUE_SENSOR_COUNT ?
		&queue->stats[sensor] : 0;
}
