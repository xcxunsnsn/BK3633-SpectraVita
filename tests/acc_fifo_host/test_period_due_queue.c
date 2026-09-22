#include <stdio.h>
#include <stdlib.h>

#include "../../projects/app_gatt/app/src/period_due_queue.c"

#define CHECK(value) do { if (!(value)) { \
    printf("FAIL line %d: %s\n", __LINE__, #value); exit(1); \
} } while (0)

static void mark_all(period_due_queue_t *queue)
{
    uint8_t i;
    for (i = 0; i < PERIOD_DUE_SENSOR_COUNT; i++)
        period_due_queue_mark(queue, i, false);
}

static void test_sixty_second_pair(void)
{
    period_due_queue_t queue;
    uint8_t sensor;
    uint16_t late;
    period_due_queue_init(&queue);
    period_due_queue_tick(&queue);
    period_due_queue_mark(&queue, 0, false);
    period_due_queue_mark(&queue, 2, false);
    CHECK(period_due_queue_take(&queue, 0, true, &sensor, &late));
    CHECK(sensor == 0 && late == 0);
    CHECK(!period_due_queue_take(&queue, 1, true, &sensor, &late));
    period_due_queue_tick(&queue);
    CHECK(period_due_queue_take(&queue, 0, true, &sensor, &late));
    CHECK(sensor == 2 && late == 1);
}

static void test_six_hundred_second_four_way(void)
{
    period_due_queue_t queue;
    uint8_t sensor;
    uint16_t late;
    uint8_t expected;
    period_due_queue_init(&queue);
    period_due_queue_tick(&queue);
    mark_all(&queue);
    for (expected = 0; expected < PERIOD_DUE_SENSOR_COUNT; expected++) {
        CHECK(period_due_queue_take(&queue, 0, true, &sensor, &late));
        CHECK(sensor == expected && late == expected);
        if (expected + 1 < PERIOD_DUE_SENSOR_COUNT)
            period_due_queue_tick(&queue);
    }
    CHECK(period_due_queue_mask(&queue) == 0);
}

static void test_pressure_and_missed(void)
{
    period_due_queue_t queue;
    const period_due_stats_t *stats;
    uint8_t sensor;
    uint16_t late;
    period_due_queue_init(&queue);
    period_due_queue_tick(&queue);
    period_due_queue_mark(&queue, 1, false);
    CHECK(!period_due_queue_take(&queue, 0, false, &sensor, &late));
    CHECK(period_due_queue_mask(&queue) == (1U << 1));
    period_due_queue_tick(&queue);
    period_due_queue_mark(&queue, 1, false);
    stats = period_due_queue_stats(&queue, 1);
    CHECK(stats->due == 2 && stats->missed == 1);
    CHECK(period_due_queue_take(&queue, 0, true, &sensor, &late));
    CHECK(sensor == 1 && late == 1);
    stats = period_due_queue_stats(&queue, 1);
    CHECK(stats->started == 1 && stats->max_late_s == 1 &&
          stats->late_total_s == 1);
}

int main(void)
{
    test_sixty_second_pair();
    test_six_hundred_second_four_way();
    test_pressure_and_missed();
    puts("PASS: period due queue tests");
    return 0;
}
