#include <stdio.h>
#include <stdlib.h>

#define ACC_HOST_TEST 1
#include "../../projects/app_gatt/app/src/task_acc.c"

typedef struct {
    uint32_t timestamp;
    uint8_t state;
    int16_t xyz[3];
} saved_record_t;

volatile uint32_t test_gpio_status;
volatile uint32_t test_gpio_enable;
volatile uint32_t test_system_int_enable;
volatile uint8_t test_gpio_input;
static uint32_t fake_hs;
static uint32_t fake_utc = 1000;
static uint8_t fake_source;
static uint8_t fake_watermark;
static uint8_t fake_activity_armed;
static uint8_t fake_inact_threshold;
static uint8_t fake_inact_time;
static uint8_t fake_inact_control;
static uint8_t fake_fifo_count;
static uint8_t fake_pin_stuck;
static int16_t fake_fifo[32][3];
static int16_t fake_low[3] = {10, 20, 30};
static int16_t next_sample;
static int16_t sample_step;
static ext_flash_wait_hook_t flash_hook;
static saved_record_t saved[4096];
static uint16_t saved_count;
static int fail_at = -1, full_at = -1, inject_remaining;
static uint8_t status_fail;
static uint8_t fake_activity_status = ADXL345_ACT_X_SOURCE;
static uint8_t async_active, async_requested, async_steps;
static uint32_t async_timestamps[ACC_COMMIT_BATCH_MAX];
static uint8_t async_records[ACC_COMMIT_BATCH_MAX][ACC_HISTORY_RECORD_LEN];
static const char *reported_fault;
static uint16_t direct_responses;
static uint32_t flash_timer_sets, flash_timer_clears, queue_warning_logs;
static uint8_t flash_timer_last_ticks;
static uint8_t flash_timer_countdown;

static void drain_main(void);

#define CHECK(value) do { if (!(value)) { \
    printf("FAIL line %d: %s\n", __LINE__, #value); exit(1); \
} } while (0)

static void refresh_int1(void)
{
    test_gpio_input = fake_pin_stuck ||
        (fake_source & (ADXL345_INT_ACTIVITY | ADXL345_INT_INACTIVITY |
                        ADXL345_INT_WATERMARK)) != 0;
}

static void feed_one(void)
{
    int16_t value = next_sample;
    next_sample += sample_step;
    if (fake_fifo_count == 32) {
        memmove(fake_fifo, fake_fifo + 1, 31 * sizeof(fake_fifo[0]));
        fake_source |= ADXL345_INT_OVERRUN;
        fake_fifo_count = 31;
    }
    fake_fifo[fake_fifo_count][0] = value;
    fake_fifo[fake_fifo_count][1] = value + 1000;
    fake_fifo[fake_fifo_count][2] = value + 2000;
    fake_fifo_count++;
    if (fake_fifo_count >= fake_watermark) fake_source |= ADXL345_INT_WATERMARK;
    refresh_int1();
    fake_hs = (fake_hs + 64) & RWIP_MAX_CLOCK_TIME; /* 20 ms at 50 Hz. */
}

static void feed(uint8_t count)
{
    while (count-- != 0) feed_one();
}

static void fire_irq(void)
{
    if ((test_gpio_enable & acc_gpio_mask()) == 0) return;
    test_gpio_input = 1;
    test_gpio_status = acc_gpio_mask();
    acc_gpio_interrupt_callback();
}

static void reset_all(void)
{
    memset(pending, 0, sizeof(pending));
    memset(fifo_samples, 0, sizeof(fifo_samples));
    memset(&stats, 0, sizeof(stats));
    queue_head = queue_count = 0;
    latest_valid = previous_valid = 0;
    stream_active = stream_paused = stream_frozen = 0;
    processing = capture_busy = 0;
    storage_inflight = drain_action = tail_drained = queue_warned = 0;
    storage_pending = storage_timer_armed = storage_timer_ticks = 0;
    irq_masked = 0;
    storage_failed = freeze_failed = full_stop = 0;
    session_complete = 1;
    motion_state = ACC_MOTION_MARK_LOW;
    enter_low = exit_source = 0;
    last_exit_source = 0;
    low_guard_remaining = activity_armed = 0;
    soft_quiet_samples = 0;
    low_read_valid = single_pending = 0;
    last_low_read = last_check = last_stats = 0;
    irq_pending = poll_pending = 0;
    fault = reported_fault = 0;
    fake_hs = 0;
    fake_utc = 1000;
    fake_source = fake_watermark = fake_fifo_count = 0;
    fake_activity_armed = 0;
    fake_inact_threshold = fake_inact_time = fake_inact_control = 0;
    next_sample = 100;
    sample_step = 1;
    flash_hook = 0;
    saved_count = direct_responses = 0;
    fail_at = full_at = -1;
    inject_remaining = 0;
    status_fail = 0;
    fake_pin_stuck = test_gpio_input = 0;
    fake_activity_status = ADXL345_ACT_X_SOURCE;
    async_active = async_requested = async_steps = 0;
    flash_timer_sets = flash_timer_clears = queue_warning_logs = 0;
    flash_timer_last_ticks = 0;
    flash_timer_countdown = 0;
    test_gpio_status = test_gpio_enable = test_system_int_enable = 0;
}

static void arm_low(void)
{
    uint8_t i;
    for (i = 0; i < ACC_LOW_ACTIVITY_GUARD_SAMPLES; i++) {
        fake_hs += 3200;
        acc_stream_second_tick();
        drain_main();
        CHECK(motion_state == ACC_MOTION_MARK_LOW);
    }
    CHECK(saved_count == ACC_LOW_ACTIVITY_GUARD_SAMPLES);
    CHECK(activity_armed && fake_activity_armed);
}

static void start_high(void)
{
    CHECK(acc_stream_start());

    /* A stale Activity edge during the protection period must be ignored. */
    fake_source = ADXL345_INT_ACTIVITY;
    fire_irq();
    acc_stream_process();
    CHECK(motion_state == ACC_MOTION_MARK_LOW && saved_count == 0);
    CHECK(stats.ignored_activity == 1);

    arm_low();
    fake_hs += 3200; /* A real next 1 Hz sample asserts Activity. */
    fake_source = ADXL345_INT_ACTIVITY;
    fire_irq();
    acc_stream_process();
    CHECK(motion_state == ACC_MOTION_MARK_HIGH);
    CHECK(saved_count == ACC_LOW_ACTIVITY_GUARD_SAMPLES);
    CHECK(queue_count == 1 && pending[queue_head].state == ACC_MOTION_MARK_LOW);
    CHECK(fake_watermark == ACC_FIFO_WATERMARK_SAMPLES);
    CHECK(fake_inact_threshold == ACC_HIGH_INACT_THRESHOLD_REG);
    CHECK(fake_inact_time == ACC_HIGH_INACT_TIME_REG);
    CHECK(fake_inact_control == ACC_HIGH_INACT_CTL_REG);
    CHECK(stats.activity_x == 2 && stats.activity_y == 0 && stats.activity_z == 0);
}

static void drain_main(void)
{
    uint16_t guard = 10000;
    do {
        acc_stream_process();
        if (storage_timer_armed) acc_stream_storage_tick();
        acc_stream_storage_process();
        guard--;
    } while (guard != 0 && (storage_inflight || drain_action != ACC_DRAIN_NONE ||
        (fault != 0 && stream_active) ||
        irq_pending || (stream_active && !storage_failed && queue_count != 0 &&
        (motion_state == ACC_MOTION_MARK_LOW || queue_count >= ACC_COMMIT_BATCH_MAX))));
    CHECK(guard != 0);
}

static void test_irq_poll_tail_and_order(void)
{
    uint16_t i;
    reset_all();
    start_high();
    feed(16); fire_irq(); acc_stream_process(); drain_main();
    feed(16);
    acc_stream_poll_task();
    CHECK(saved_count == 18); /* Trigger LOW sample remains ordered in the partial batch. */
    acc_stream_process();
    drain_main(); /* No IRQ: main-loop fallback capture. */
    feed(7); /* Below watermark: stop must freeze and drain it. */
    CHECK(acc_stream_stop());
    drain_main();
    CHECK(acc_stream_is_stopped());
    CHECK(saved_count == 42 && stats.read == stats.committed);
    CHECK(stats.batches >= 6 && stats.batches < stats.committed);
    CHECK(queue_count == 0 && fake_fifo_count == 0 && stats.overrun == 0);
    for (i = ACC_LOW_ACTIVITY_GUARD_SAMPLES + 2; i < saved_count; i++)
        CHECK(saved[i].xyz[0] == saved[i - 1].xyz[0] + sample_step);
}

static void test_irq_mask_rearm_and_stuck_pin(void)
{
    uint32_t irq_before;
    uint32_t defer_before;
    reset_all();
    start_high();
    irq_before = stats.irq;
    feed(16);
    fire_irq();
    CHECK(irq_masked && (test_gpio_enable & acc_gpio_mask()) == 0);
    CHECK(stats.irq == irq_before + 1);
    /* Model one already-queued controller callback after P20 was masked. */
    test_gpio_status = acc_gpio_mask();
    acc_gpio_interrupt_callback();
    CHECK(stats.irq == irq_before + 1 && stats.irq_duplicate == 1);
    acc_stream_process();
    CHECK(!irq_masked && (test_gpio_enable & acc_gpio_mask()) != 0);
    CHECK(fake_fifo_count == 8 && irq_pending);
    acc_stream_process();
    drain_main();
    CHECK(fake_fifo_count == 0 && stats.irq == irq_before + 1);

    /* 24 entries need two masked slices before the level drops below WM. */
    feed(24);
    fire_irq();
    acc_stream_process();
    CHECK(irq_masked && fake_fifo_count == 16 && irq_pending);
    acc_stream_process();
    CHECK(!irq_masked && fake_fifo_count == 8 && irq_pending);
    acc_stream_process();
    drain_main();
    CHECK(fake_fifo_count == 0);

    /* A physically high INT1 defers rearm without creating an ISR storm. */
    feed(16);
    fake_pin_stuck = 1;
    fire_irq();
    defer_before = stats.irq_defer;
    acc_stream_process();
    acc_stream_process();
    CHECK(irq_masked && stats.irq_defer > defer_before);
    CHECK(fake_fifo_count == 0);
    fake_pin_stuck = 0;
    refresh_int1();
    acc_stream_poll_task();
    acc_stream_process();
    CHECK(!irq_masked && (test_gpio_enable & acc_gpio_mask()) != 0);
    CHECK(acc_stream_stop());
    drain_main();
}

static void test_ten_second_irq_rate(void)
{
    uint8_t i;
    uint32_t irq_before;
    reset_all();
    start_high();
    sample_step = 40;
    irq_before = stats.irq;
    for (i = 0; i < 31; i++) {
        feed(16);
        fire_irq();
        acc_stream_process();
        drain_main();
    }
    feed(4); /* 31 x 16 + 4 = 500 samples = 10 seconds at 50 Hz. */
    CHECK(stats.irq - irq_before == 31);
    CHECK(stats.irq_duplicate == 0 && stats.overrun == 0);
    CHECK(acc_stream_stop());
    drain_main();
    CHECK(stats.read == stats.committed && queue_count == 0);
}

static void test_storage_cadence_and_warning_hysteresis(void)
{
    reset_all();
    stream_active = 1;
    motion_state = ACC_MOTION_MARK_HIGH;
    queue_count = 31;
    acc_storage_schedule();
    CHECK(storage_timer_armed && storage_timer_ticks == 2 &&
          flash_timer_last_ticks == 2);
    queue_count = 32;
    acc_storage_schedule();
    CHECK(storage_timer_ticks == 1 && flash_timer_last_ticks == 1 &&
          flash_timer_clears == 1);
    acc_stream_storage_tick();
    storage_pending = 0;
    queue_count = 31;
    acc_storage_schedule();
    CHECK(storage_timer_ticks == 2 && flash_timer_last_ticks == 2);

    queue_count = 96;
    acc_queue_warning_update();
    CHECK(queue_warned && queue_warning_logs == 1);
    queue_count = 95;
    acc_queue_warning_update();
    queue_count = 96;
    acc_queue_warning_update();
    CHECK(queue_warning_logs == 1);
    queue_count = 63;
    acc_queue_warning_update();
    CHECK(!queue_warned);
    queue_count = 96;
    acc_queue_warning_update();
    CHECK(queue_warning_logs == 2);
}

static void test_ten_second_storage_throughput(void)
{
    uint16_t tick;
    uint8_t pin_was_high;
    reset_all();
    start_high();
    sample_step = 40;
    for (tick = 0; tick < 1000; tick++) {
        if ((tick & 1U) == 0) {
            pin_was_high = test_gpio_input;
            feed_one();
            if (!pin_was_high && test_gpio_input &&
                (test_gpio_enable & acc_gpio_mask()) != 0) fire_irq();
        }
        if ((tick % 10U) == 0) acc_stream_poll_task();
        acc_stream_process();
        if (storage_timer_armed && flash_timer_countdown != 0 &&
            --flash_timer_countdown == 0) acc_stream_storage_tick();
        acc_stream_storage_process();
    }
    /* Let the final complete batch commit; four sub-watermark FIFO samples
     * intentionally remain for the stop-tail test below. */
    for (tick = 0; tick < 20; tick++) {
        acc_stream_process();
        if (storage_timer_armed && flash_timer_countdown != 0 &&
            --flash_timer_countdown == 0) acc_stream_storage_tick();
        acc_stream_storage_process();
    }
    CHECK(stats.read >= 499 && stats.committed >= stats.read - 8);
    CHECK(queue_count <= 8 && stats.peak < ACC_RAM_QUEUE_YIELD_SAMPLES);
    CHECK(stats.overrun == 0 && fault == 0);
    CHECK(acc_stream_stop());
    drain_main();
    CHECK(stats.read == stats.committed && queue_count == 0);
}

static void test_flash_busy_capture(void)
{
    reset_all();
    start_high();
    feed(16); fire_irq();
    inject_remaining = 64; /* 1.28 seconds of samples while Flash is busy. */
    acc_stream_process();
    drain_main();
    CHECK(acc_stream_stop());
    drain_main();
    CHECK(saved_count == 83);
    CHECK(stats.read == stats.committed && stats.overrun == 0);
    CHECK(stats.peak > 16 && stats.peak < ACC_RAM_QUEUE_SAMPLES);
}

static void test_ring_wrap(void)
{
    uint16_t batch, i;
    reset_all();
    start_high();
    sample_step = 40; /* Keep HIGH active past the quiet-window threshold. */
    for (batch = 0; batch < 20; batch++) {
        feed(16); fire_irq(); acc_stream_process(); drain_main();
    }
    CHECK(acc_stream_stop());
    drain_main();
    CHECK(saved_count == 323 && queue_head != 0);
    for (i = ACC_LOW_ACTIVITY_GUARD_SAMPLES + 2; i < saved_count; i++)
        CHECK(saved[i].xyz[0] == saved[i - 1].xyz[0] + sample_step);
}

static void test_commit_failure_retains_queue(void)
{
    reset_all();
    start_high();
    fail_at = saved_count + 3;
    feed(16); fire_irq(); acc_stream_process();
    drain_main();
    CHECK(!stream_active && !acc_stream_data_complete());
    CHECK(queue_count == 17 && reported_fault != 0);
    CHECK(!acc_stream_stop());
}

static void test_overrun_is_terminal(void)
{
    reset_all();
    start_high();
    feed(33); fire_irq(); acc_stream_process();
    drain_main();
    CHECK(stats.overrun == 1);
    CHECK(!stream_active && !acc_stream_data_complete());
    CHECK(queue_count == 0 && fake_fifo_count == 0);
}

static void test_i2c_failure_and_queue_exhaustion(void)
{
    reset_all();
    start_high();
    feed(16); fire_irq(); status_fail = 1; acc_stream_process();
    drain_main();
    CHECK(!stream_active && !acc_stream_data_complete());
    CHECK(fake_fifo_count == 0 && reported_fault != 0);

    reset_all();
    start_high();
    feed(16); fire_irq();
    inject_remaining = 160; /* Longer than the 128-entry RAM protection window. */
    acc_stream_process();
    drain_main();
    CHECK(!stream_active && !acc_stream_data_complete());
    CHECK(stats.peak == ACC_RAM_QUEUE_SAMPLES && reported_fault != 0);
}

static void test_inactivity_and_full_stop(void)
{
    uint8_t i;

    /* Hardware inactivity is primary and drains a below-watermark tail. */
    reset_all();
    start_high();
    sample_step = 40;
    feed(7);
    acc_irq_mask();
    fake_source |= ADXL345_INT_INACTIVITY;
    refresh_int1();
    acc_stream_poll_task();
    acc_stream_process();
    drain_main();
    CHECK(motion_state == ACC_MOTION_MARK_LOW);
    CHECK(stats.hw_inactivity == 1);
    CHECK(saved_count == ACC_LOW_ACTIVITY_GUARD_SAMPLES + 1 + 7);
    CHECK(last_exit_source == ACC_EXIT_HW_INACTIVITY);
    CHECK(fake_fifo_count == 0 && queue_count == 0);

    /* Software fallback requires 100 consecutive quiet HIGH samples. */
    reset_all();
    start_high();
    /* 20 LSB on every axis has vector magnitude >30, but every individual
     * axis remains below 30 and must be considered quiet like the hardware. */
    sample_step = 20;
    for (i = 0; i < 8; i++) {
        feed(16); fire_irq(); acc_stream_process(); drain_main();
        if (motion_state == ACC_MOTION_MARK_LOW) break;
    }
    CHECK(motion_state == ACC_MOTION_MARK_LOW);
    CHECK(saved_count >= ACC_LOW_ACTIVITY_GUARD_SAMPLES + 1 + ACC_SOFT_QUIET_SAMPLES);
    CHECK(last_exit_source == ACC_EXIT_SOFT_QUIET);

    /* Any real motion resets the software quiet counter. */
    reset_all();
    start_high();
    sample_step = 0;
    for (i = 0; i < 4; i++) {
        feed(16); fire_irq(); acc_stream_process(); drain_main();
    }
    CHECK(soft_quiet_samples > 0 && motion_state == ACC_MOTION_MARK_HIGH);
    sample_step = 40;
    feed(16); fire_irq(); acc_stream_process(); drain_main();
    CHECK(soft_quiet_samples == 0 && stats.motion_resets > 0);
    sample_step = 0;
    for (i = 0; i < 5; i++) {
        feed(16); fire_irq(); acc_stream_process(); drain_main();
    }
    CHECK(motion_state == ACC_MOTION_MARK_HIGH);
    CHECK(acc_stream_stop());
    drain_main();

    reset_all();
    start_high();
    sample_step = 40;
    full_at = saved_count + 10;
    feed(16); fire_irq(); acc_stream_process(); drain_main();
    CHECK(!stream_active && full_stop && acc_stream_data_complete());
    CHECK(saved_count == ACC_LOW_ACTIVITY_GUARD_SAMPLES + 10 && queue_count == 0);
}

static void test_low_direct_read_is_not_history(void)
{
    reset_all();
    CHECK(acc_stream_start());
    acc_collect_once_report(0);
    CHECK(direct_responses == 1 && saved_count == 0);
    CHECK(acc_stream_stop());
    drain_main();
}

static void test_cancel_pending_upload_pause(void)
{
    reset_all();
    start_high();
    feed(7);
    CHECK(acc_stream_pause());
    acc_stream_cancel_pause();
    drain_main();
    CHECK(acc_stream_is_active() && !acc_stream_is_paused());
    CHECK(motion_state == ACC_MOTION_MARK_LOW);
    CHECK(queue_count == 0 && fake_fifo_count == 0);
    CHECK(acc_stream_stop());
    drain_main();
}

int main(void)
{
    puts("test_irq_poll_tail_and_order");
    test_irq_poll_tail_and_order();
    puts("test_flash_busy_capture");
    test_flash_busy_capture();
    puts("test_irq_mask_rearm_and_stuck_pin");
    test_irq_mask_rearm_and_stuck_pin();
    puts("test_ten_second_irq_rate");
    test_ten_second_irq_rate();
    puts("test_storage_cadence_and_warning_hysteresis");
    test_storage_cadence_and_warning_hysteresis();
    puts("test_ten_second_storage_throughput");
    test_ten_second_storage_throughput();
    puts("test_ring_wrap");
    test_ring_wrap();
    puts("test_commit_failure_retains_queue");
    test_commit_failure_retains_queue();
    puts("test_overrun_is_terminal");
    test_overrun_is_terminal();
    puts("test_i2c_failure_and_queue_exhaustion");
    test_i2c_failure_and_queue_exhaustion();
    puts("test_inactivity_and_full_stop");
    test_inactivity_and_full_stop();
    puts("test_low_direct_read_is_not_history");
    test_low_direct_read_is_not_history();
    puts("test_cancel_pending_upload_pause");
    test_cancel_pending_upload_pause();
    puts("PASS: acc FIFO host tests");
    return 0;
}

rwip_time_t rwip_time_get(void) { rwip_time_t t = {fake_hs, 0}; return t; }
void gpio_config(uint8_t p, int d, int u) { (void)p; (void)d; (void)u; }
void gpio_cb_register(void (*c)(void)) { (void)c; }
void gpio_set_int_mode(uint8_t p, uint8_t m) { (void)p; (void)m; test_gpio_enable |= acc_gpio_mask(); }
uint8_t gpio_get_input(uint8_t p) { (void)p; return test_gpio_input; }
void ke_timer_set(uint16_t i, uint16_t t, uint32_t d) {
    (void)t;
    if (i == APP_ACC_FLASH_STEP) {
        flash_timer_sets++;
        flash_timer_last_ticks = (uint8_t)d;
        flash_timer_countdown = (uint8_t)d;
    }
}
void ke_timer_clear(uint16_t i, uint16_t t) {
    (void)t;
    if (i == APP_ACC_FLASH_STEP) flash_timer_clears++;
    if (i == APP_ACC_FLASH_STEP) flash_timer_countdown = 0;
}
void app_task_quit_lowpower(uint8_t i) { (void)i; }
void app_task_enter_lowpower(uint8_t i) { (void)i; }
int uart_printf(const char *f, ...) {
    if (strstr(f, "ACC RAM queue high") != 0) queue_warning_logs++;
    return 0;
}
void utc_update(void) { fake_utc = 1000 + fake_hs / 3200; }
uint32_t utc_get_clock(void) { return fake_utc; }
void ext_flash_set_wait_hook(ext_flash_wait_hook_t hook) { flash_hook = hook; }
void ADXL345_Init(void) {}
void ADXL345_Enable(int flag) { (void)flag; }
bool ADXL345_LowRate_Start(uint8_t t, uint8_t c) {
    (void)t; (void)c; fake_fifo_count = 0; fake_activity_armed = 0;
    fake_source = 0; refresh_int1(); return true;
}
bool ADXL345_LowRate_ArmActivity(void) {
    fake_source &= (uint8_t)~ADXL345_INT_ACTIVITY;
    refresh_int1();
    fake_activity_armed = 1;
    return true;
}
bool ADXL345_Stream_Start(uint8_t watermark, uint8_t inactivity_threshold,
                          uint8_t inactivity_time,
                          uint8_t inactivity_control) {
    fake_watermark = watermark;
    fake_inact_threshold = inactivity_threshold;
    fake_inact_time = inactivity_time;
    fake_inact_control = inactivity_control;
    fake_fifo_count = 0;
    fake_source = 0;
    refresh_int1();
    return true;
}
bool ADXL345_Stream_Freeze(void) { return true; }
void ADXL345_Stream_Stop(void) { fake_fifo_count = 0; fake_source = 0; refresh_int1(); }
bool ADXL345_read_data(short *x, short *y, short *z) { *x=fake_low[0]; *y=fake_low[1]; *z=fake_low[2]; return true; }
bool ADXL345_Int_Source_Checked(uint8_t *source) {
    *source = fake_source;
    fake_source &= ADXL345_INT_WATERMARK;
    refresh_int1();
    return true;
}
bool ADXL345_Activity_Status_Checked(uint8_t *activity_status, uint8_t *source) {
    *activity_status = fake_activity_status;
    return ADXL345_Int_Source_Checked(source);
}
bool ADXL345_FIFO_Status(uint8_t *entries, uint8_t *source) {
    if (status_fail) return false;
    *entries = fake_fifo_count; *source = fake_source;
    fake_source &= ADXL345_INT_WATERMARK;
    refresh_int1();
    return true;
}
bool ADXL345_FIFO_Entries_Checked(uint8_t *entries) {
    if (status_fail) return false;
    *entries = fake_fifo_count;
    refresh_int1();
    return true;
}
bool ADXL345_FIFO_Read_Checked(int16_t xyz[][3], uint8_t limit, uint8_t *read_count) {
    uint8_t count = fake_fifo_count < limit ? fake_fifo_count : limit;
    memcpy(xyz, fake_fifo, count * sizeof(fake_fifo[0]));
    memmove(fake_fifo, fake_fifo + count, (fake_fifo_count - count) * sizeof(fake_fifo[0]));
    fake_fifo_count -= count; *read_count = count;
    if (fake_fifo_count < fake_watermark) fake_source &= ~ADXL345_INT_WATERMARK;
    refresh_int1();
    return true;
}
uint8_t app_protocol_acc_commit(uint32_t timestamp, uint8_t state,
                                int16_t x, int16_t y, int16_t z) {
    uint8_t record[1][ACC_HISTORY_RECORD_LEN];
    uint8_t accepted;
    record[0][0] = state;
    record[0][1] = (uint8_t)x; record[0][2] = (uint8_t)((uint16_t)x >> 8);
    record[0][3] = (uint8_t)y; record[0][4] = (uint8_t)((uint16_t)y >> 8);
    record[0][5] = (uint8_t)z; record[0][6] = (uint8_t)((uint16_t)z >> 8);
    return app_protocol_acc_commit_batch(&timestamp, record, 1, &accepted);
}
uint8_t app_protocol_acc_commit_batch(
        const uint32_t timestamps[],
        uint8_t records[][ACC_HISTORY_RECORD_LEN],
        uint8_t requested, uint8_t *accepted) {
    uint8_t count = requested, i;
    *accepted = 0;
    if (fail_at >= saved_count && fail_at < saved_count + requested)
        return ACC_COMMIT_ERROR;
    if (full_at >= 0) {
        if (saved_count >= full_at) return ACC_COMMIT_FULL;
        if (count > full_at - saved_count) count = (uint8_t)(full_at - saved_count);
    }
    for (i = 0; i < count; i++) {
        saved[saved_count].timestamp = timestamps[i];
        saved[saved_count].state = records[i][0];
        saved[saved_count].xyz[0] = (int16_t)((uint16_t)records[i][1] |
            ((uint16_t)records[i][2] << 8));
        saved[saved_count].xyz[1] = (int16_t)((uint16_t)records[i][3] |
            ((uint16_t)records[i][4] << 8));
        saved[saved_count].xyz[2] = (int16_t)((uint16_t)records[i][5] |
            ((uint16_t)records[i][6] << 8));
        saved_count++;
    }
    *accepted = count;
    return (full_at >= 0 && saved_count == full_at) ? ACC_COMMIT_LAST : ACC_COMMIT_OK;
}
uint8_t app_protocol_acc_commit_async_begin(
        const uint32_t timestamps[],
        uint8_t records[][ACC_HISTORY_RECORD_LEN], uint8_t requested) {
    if (async_active) return ACC_COMMIT_BUSY;
    memcpy(async_timestamps, timestamps, requested * sizeof(timestamps[0]));
    memcpy(async_records, records, requested * sizeof(records[0]));
    async_requested = requested;
    async_steps = 3;
    async_active = 1;
    return ACC_COMMIT_BUSY;
}
uint8_t app_protocol_acc_commit_async_process(uint8_t *accepted) {
    if (!async_active) { *accepted = 0; return ACC_COMMIT_OK; }
    if (inject_remaining > 0) {
        inject_remaining--;
        feed_one();
        if (fake_fifo_count >= fake_watermark) irq_pending = 1;
        *accepted = 0;
        return ACC_COMMIT_BUSY;
    }
    if (async_steps != 0) {
        async_steps--;
        *accepted = 0;
        return ACC_COMMIT_BUSY;
    }
    async_active = 0;
    return app_protocol_acc_commit_batch(async_timestamps, async_records,
                                          async_requested, accepted);
}
bool app_protocol_acc_flash_allowed(void) { return true; }
void app_protocol_acc_stream_fault(const char *reason) { reported_fault = reason; }
void app_ble_timing_report(const char *event, uint8_t reason) { (void)event; (void)reason; }
void app_protocol_sensor_data(uint8_t s, const uint8_t *d, uint16_t n) {
    (void)s; (void)d; (void)n; direct_responses++;
}
