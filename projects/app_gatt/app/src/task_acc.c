/* ADXL345: IRQ/main-loop FIFO capture, bounded RAM queue, ordered Flash commit. */
#ifdef ACC_HOST_TEST
#include "acc_test_platform.h"
#else
#include "task_acc.h"
#include "driver_ADXL345.h"
#include "acc_motion_config.h"
#include "user_config.h"
#include "app_task.h"
#include "ke_timer.h"
#include "gpio.h"
#include "icu.h"
#include "uart.h"
#include "app_user.h"
#include "ext_flash.h"
#include "utc_clock.h"
#include "rwip.h"
#endif
#include <string.h>

typedef struct {
    uint32_t timestamp;
    int16_t xyz[3];
    uint8_t state;
} acc_pending_t;

static acc_pending_t pending[ACC_RAM_QUEUE_SAMPLES];
static uint16_t queue_head, queue_count;
static int16_t fifo_samples[32][3];
static int16_t latest_xyz[3], previous_xyz[3];
static uint8_t latest_valid, previous_valid;
static uint8_t stream_active, stream_paused, stream_frozen;
static uint8_t processing, capture_busy;
static uint8_t storage_inflight, drain_action, tail_drained, queue_warned;
static uint8_t storage_failed, freeze_failed, full_stop;
static uint8_t session_complete = 1;
static uint8_t motion_state = ACC_MOTION_MARK_LOW;
static uint8_t enter_low, exit_source;
static uint8_t last_exit_source;
static uint8_t low_guard_remaining, activity_armed;
static uint16_t soft_quiet_samples;
static uint8_t low_read_valid, single_pending;
static uint32_t last_low_read, last_check, last_stats;
static volatile uint8_t irq_pending, irq_masked;
static volatile uint8_t storage_pending;
static uint8_t storage_timer_armed, storage_timer_ticks;
static uint8_t poll_pending;
static const char *fault;
static struct {
    volatile uint32_t irq;
    uint32_t polls, read, committed, batches, peak, overrun, max_gap, full_discard;
    uint32_t hw_inactivity, motion_resets, ignored_activity;
    uint32_t activity_x, activity_y, activity_z, storage_steps;
    uint32_t irq_rearm, irq_defer, irq_duplicate;
    uint32_t storage_wakes, storage_urgent_wakes;
} stats;

enum {
    ACC_EXIT_NONE = 0,
    ACC_EXIT_HW_INACTIVITY,
    ACC_EXIT_SOFT_QUIET,
};

enum {
    ACC_DRAIN_NONE = 0,
    ACC_DRAIN_TO_LOW,
    ACC_DRAIN_RESTART_LOW,
    ACC_DRAIN_PAUSE,
    ACC_DRAIN_STOP,
};

static bool acc_enter_low(void);
static void acc_storage_schedule(void);

static uint32_t acc_clock_hs(void)
{
    return rwip_time_get().hs & RWIP_MAX_CLOCK_TIME;
}

static uint32_t acc_elapsed_hs(uint32_t now, uint32_t before)
{
    return (now - before) & RWIP_MAX_CLOCK_TIME;
}

static uint32_t acc_gpio_mask(void)
{
    return 1UL << (((ADXL_IN1_PIN >> 4) * 8) + (ADXL_IN1_PIN & 15));
}

static void acc_irq_mask(void)
{
    REG_GPIO_WUATOD_ENABLE &= ~acc_gpio_mask();
    irq_masked = 1;
}

static void acc_gpio_interrupt_enable(void)
{
    gpio_config(ADXL_IN1_PIN, INPUT, PULL_NONE);
    gpio_cb_register(acc_gpio_interrupt_callback);
    acc_irq_mask();
    REG_GPIO_WUATOD_STATUS = acc_gpio_mask();
    gpio_set_int_mode(ADXL_IN1_PIN, RISE_EDGE);
    REG_GPIO_WUATOD_STATUS = acc_gpio_mask();
    irq_masked = 0;
    SYS_REG0X10_INT_EN |= 1UL << POS_SYS_REG0X10_INT_EN_GPIO;
}

static bool acc_irq_try_rearm(void)
{
    uint32_t mask = acc_gpio_mask();
    if (!irq_masked) return true;
    if (!stream_active || stream_paused || stream_frozen || fault != 0 ||
        enter_low || drain_action != ACC_DRAIN_NONE) return false;
    REG_GPIO_WUATOD_STATUS = mask;
    if (gpio_get_input(ADXL_IN1_PIN) != 0) {
        stats.irq_defer++;
        return false;
    }
    REG_GPIO_WUATOD_ENABLE |= mask;
    irq_masked = 0;
    stats.irq_rearm++;
    /* Close the low-check/enable race. Polling remains the final fallback. */
    if (gpio_get_input(ADXL_IN1_PIN) != 0) {
        acc_irq_mask();
        REG_GPIO_WUATOD_STATUS = mask;
        stats.irq_defer++;
        return false;
    }
    return true;
}

static void acc_cancel_service(void)
{
    ke_timer_clear(APP_ACC_FIFO_POLL, TASK_APP);
    acc_irq_mask();
    REG_GPIO_WUATOD_STATUS = acc_gpio_mask();
    irq_pending = poll_pending = 0;
}

void acc_gpio_interrupt_callback(void)
{
    if ((REG_GPIO_WUATOD_STATUS & acc_gpio_mask()) == 0) return;
    acc_irq_mask();
    REG_GPIO_WUATOD_STATUS = acc_gpio_mask();
    if (irq_pending) {
        stats.irq_duplicate++;
        return;
    }
    if (stream_active && !stream_paused && !stream_frozen) {
        stats.irq++;
        irq_pending = 1; /* No kernel allocation, I2C, logging or Flash in ISR. */
    }
}

static void acc_fault(const char *reason)
{
    if (fault == 0) fault = reason;
    session_complete = 0;
}

static void acc_stats(const char *reason)
{
#if ACC_MOTION_DEBUG
    uart_printf("ACC5 %s io=%u/%u/%u/%u\r\n", reason, stats.irq,
        stats.polls, stats.read, stats.committed);
    uart_printf("ACC5 store=%u/%u wake=%u/%u q=%u/%u\r\n",
        stats.batches, stats.storage_steps, stats.storage_wakes,
        stats.storage_urgent_wakes, queue_count, stats.peak);
    uart_printf("ACC5 fifo ov=%u gap=%u\r\n",
        stats.overrun, (stats.max_gap * 5) / 16);
    uart_printf("ACC5 irqctl=%u/%u/%u masked=%u\r\n", stats.irq_rearm,
        stats.irq_defer, stats.irq_duplicate, irq_masked);
    uart_printf("ACC5 state=%u/%u inact=%u quiet=%u reset=%u ignored=%u\r\n",
        activity_armed, low_guard_remaining, stats.hw_inactivity,
        soft_quiet_samples, stats.motion_resets, stats.ignored_activity);
    uart_printf("ACC5 act=%u/%u/%u exit=%s fault=%s\r\n",
        stats.activity_x, stats.activity_y, stats.activity_z,
        last_exit_source == ACC_EXIT_HW_INACTIVITY ? "HW_INACT" :
        (last_exit_source == ACC_EXIT_SOFT_QUIET ? "SW_QUIET" : "none"),
        fault != 0 ? fault : "none");
    app_ble_timing_report(reason, 0xFF);
#endif
    last_stats = acc_clock_hs();
}

void acc_stream_link_report(uint8_t reason)
{
    uart_printf("ACC5 link r=%02x mode=%u q=%u/%u flash=%u/%u timer=%u\r\n",
        reason, motion_state, queue_count, stats.peak, storage_inflight,
        storage_pending, storage_timer_armed);
    uart_printf("ACC5 link irq=%u poll=%u dup=%u gap=%u\r\n", stats.irq,
        stats.polls, stats.irq_duplicate, (stats.max_gap * 5) / 16);
}

static bool acc_storage_has_work(void)
{
    if (storage_failed) return false;
    if (storage_inflight) return true;
    if (queue_count == 0) return false;
    return queue_count >= ACC_COMMIT_BATCH_MAX ||
        drain_action != ACC_DRAIN_NONE ||
        motion_state == ACC_MOTION_MARK_LOW;
}

static void acc_storage_schedule(void)
{
    uint8_t ticks;
    if (!acc_storage_has_work()) {
        if (storage_timer_armed)
            ke_timer_clear(APP_ACC_FLASH_STEP, TASK_APP);
        storage_timer_armed = storage_timer_ticks = 0;
        return;
    }
    ticks = (queue_count >= ACC_RAM_QUEUE_YIELD_SAMPLES ||
        drain_action != ACC_DRAIN_NONE) ?
        ACC_FLASH_STEP_URGENT_TICKS : ACC_FLASH_STEP_NORMAL_TICKS;
    if (!storage_timer_armed || ticks < storage_timer_ticks) {
        if (storage_timer_armed)
            ke_timer_clear(APP_ACC_FLASH_STEP, TASK_APP);
        storage_timer_armed = 1;
        storage_timer_ticks = ticks;
        ke_timer_set(APP_ACC_FLASH_STEP, TASK_APP, ticks);
    }
}

void acc_stream_storage_tick(void)
{
    if (!storage_timer_armed) return;
    storage_timer_armed = storage_timer_ticks = 0;
    storage_pending = 1;
    stats.storage_wakes++;
    if (queue_count >= ACC_RAM_QUEUE_YIELD_SAMPLES ||
        drain_action != ACC_DRAIN_NONE) stats.storage_urgent_wakes++;
}

static void acc_latest(const int16_t xyz[3])
{
    memcpy(latest_xyz, xyz, sizeof(latest_xyz));
    latest_valid = 1;
}

static void acc_queue_warning_update(void)
{
    if (queue_count >= ACC_RAM_QUEUE_WARN_SAMPLES && !queue_warned) {
        queue_warned = 1;
        uart_printf("ACC RAM queue high: q=%u\r\n", queue_count);
    } else if (queue_count < ACC_RAM_QUEUE_WARN_RESET_SAMPLES) {
        queue_warned = 0;
    }
}

static void acc_motion_sample(const int16_t xyz[3])
{
    uint8_t i;
    uint32_t maximum = 0;
    int32_t delta;
    if (previous_valid) {
        for (i = 0; i < 3; i++) {
            delta = (int32_t)xyz[i] - previous_xyz[i];
            if (delta < 0) delta = -delta;
            if ((uint32_t)delta > maximum) maximum = (uint32_t)delta;
        }
    }
    memcpy(previous_xyz, xyz, sizeof(previous_xyz));
    previous_valid = 1;
    if (maximum >= ACC_MOTION_EXIT_DELTA_LSB) {
        if (soft_quiet_samples != 0 || enter_low) stats.motion_resets++;
        soft_quiet_samples = 0;
        /* A latched hardware inactivity event is authoritative.  FIFO tail
         * samples may contain the small edge motion that asserted INT1 and
         * must not cancel the already-requested freeze/drain transition. */
        if (exit_source != ACC_EXIT_HW_INACTIVITY) {
            enter_low = 0;
            exit_source = ACC_EXIT_NONE;
        }
        return;
    }
    if (soft_quiet_samples < ACC_SOFT_QUIET_SAMPLES) soft_quiet_samples++;
    if (soft_quiet_samples >= ACC_SOFT_QUIET_SAMPLES && !enter_low) {
        enter_low = 1;
        exit_source = ACC_EXIT_SOFT_QUIET;
    }
}

static void acc_enqueue(uint8_t state, const int16_t xyz[3], uint32_t timestamp)
{
    uint16_t tail = (queue_head + queue_count) % ACC_RAM_QUEUE_SAMPLES;
    pending[tail].timestamp = timestamp;
    pending[tail].state = state;
    memcpy(pending[tail].xyz, xyz, sizeof(pending[tail].xyz));
    queue_count++;
    stats.read++;
    if (queue_count > stats.peak) stats.peak = queue_count;
    acc_queue_warning_update();
    acc_storage_schedule();
    acc_latest(xyz);
    if (state == ACC_MOTION_MARK_HIGH) acc_motion_sample(xyz);
}

/* force drains even a short tail. Never commits Flash or changes sensor mode. */
static void acc_capture(uint8_t force)
{
    uint8_t count, source, received = 0, remaining = 0, i, limit;
    uint32_t now = acc_clock_hs(), gap = acc_elapsed_hs(now, last_check), timestamp;
    bool ok;
    if (capture_busy || motion_state != ACC_MOTION_MARK_HIGH || storage_failed) return;
    capture_busy = 1;
    if (!stream_frozen && gap > stats.max_gap) stats.max_gap = gap;
    last_check = now;
    if (!ADXL345_FIFO_Status(&count, &source)) {
        acc_fault("I2C FIFO status");
        capture_busy = 0;
        return;
    }
    if (source & ADXL345_INT_OVERRUN) {
        stats.overrun++;
        acc_fault("FIFO overrun");
        force = 1;
    }
    if (source & ADXL345_INT_INACTIVITY) {
        stats.hw_inactivity++;
        enter_low = 1;
        exit_source = ACC_EXIT_HW_INACTIVITY;
        force = 1;
    }
    if (count == 0 || (!force && count < ACC_FIFO_WATERMARK_SAMPLES)) {
        if (force && count == 0) tail_drained = 1;
        if (!stream_frozen && fault == 0 && !enter_low)
            acc_irq_try_rearm();
        capture_busy = 0;
        return;
    }
    limit = (uint8_t)(ACC_RAM_QUEUE_SAMPLES - queue_count);
    if (limit > ACC_FIFO_READ_SLICE_SAMPLES)
        limit = ACC_FIFO_READ_SLICE_SAMPLES;
    if (limit == 0) {
        acc_fault("RAM queue full");
        capture_busy = 0;
        return;
    }
    utc_update();
    timestamp = utc_get_clock();
    ok = ADXL345_FIFO_Read_Checked(fifo_samples, limit, &received);
    for (i = 0; i < received; i++) acc_enqueue(ACC_MOTION_MARK_HIGH, fifo_samples[i], timestamp);
    if (!ok) acc_fault("I2C FIFO read");
    if (ok && !ADXL345_FIFO_Entries_Checked(&remaining))
        acc_fault("I2C FIFO recheck");
    if (received == limit && queue_count == ACC_RAM_QUEUE_SAMPLES && !stream_frozen)
        acc_fault("RAM queue full");
    if (fault == 0 && !stream_frozen && !enter_low) {
        if (remaining >= ACC_FIFO_WATERMARK_SAMPLES) {
            irq_pending = 1;
        } else {
            acc_irq_try_rearm();
            /* Drain the below-watermark tail in the next BLE-separated slice. */
            if (remaining != 0) irq_pending = 1;
        }
    }
    capture_busy = 0;
}

static void acc_flash_wait_service(void)
{
    /* Kept for synchronous writes from the four low-rate sensors only.
     * Acceleration Flash writes use the cooperative state machine. */
    if (!stream_active || stream_paused || stream_frozen || fault != 0 ||
        motion_state != ACC_MOTION_MARK_HIGH) return;
    if (acc_elapsed_hs(acc_clock_hs(), last_check) >= ACC_FLASH_SERVICE_HS)
        acc_capture(0);
}

static bool acc_freeze(void)
{
    if (stream_frozen) return !freeze_failed;
    acc_cancel_service();
    freeze_failed = !ADXL345_Stream_Freeze();
    stream_frozen = 1;
    if (freeze_failed) acc_fault("I2C freeze");
    return !freeze_failed;
}

static void acc_storage_fail(const char *reason)
{
    storage_failed = 1;
    acc_fault(reason);
}

static void acc_storage_step(void)
{
    uint32_t timestamps[ACC_COMMIT_BATCH_MAX];
    uint8_t records[ACC_COMMIT_BATCH_MAX][ACC_HISTORY_RECORD_LEN];
    uint16_t index;
    uint16_t value;
    uint8_t count, accepted = 0, i;
    uint8_t result;

    if (storage_failed) return;
    if (storage_inflight) {
        stats.storage_steps++;
        result = app_protocol_acc_commit_async_process(&accepted);
        if (result == ACC_COMMIT_BUSY) return;
        storage_inflight = 0;
        if (result == ACC_COMMIT_ERROR || accepted > queue_count) {
            acc_storage_fail("Flash async commit");
            return;
        }
        if (accepted != 0) {
            queue_head = (queue_head + accepted) % ACC_RAM_QUEUE_SAMPLES;
            queue_count -= accepted;
            stats.committed += accepted;
            stats.batches++;
            acc_queue_warning_update();
        }
        if ((result == ACC_COMMIT_OK || result == ACC_COMMIT_LAST) &&
            accepted == 0) {
            acc_storage_fail("Flash async made no progress");
            return;
        }
        if (result == ACC_COMMIT_FULL || result == ACC_COMMIT_LAST) {
            full_stop = 1;
            acc_freeze();
            stats.full_discard += queue_count;
            queue_count = 0;
            drain_action = ACC_DRAIN_STOP;
            tail_drained = 1;
        }
        return;
    }
    if (queue_count == 0 || !app_protocol_acc_flash_allowed()) return;
    if (queue_count < ACC_COMMIT_BATCH_MAX && drain_action == ACC_DRAIN_NONE &&
        motion_state == ACC_MOTION_MARK_HIGH) return;

    count = queue_count > ACC_COMMIT_BATCH_MAX ?
        ACC_COMMIT_BATCH_MAX : (uint8_t)queue_count;

    for (i = 0; i < count; i++) {
        index = (queue_head + i) % ACC_RAM_QUEUE_SAMPLES;
        timestamps[i] = pending[index].timestamp;
        records[i][0] = pending[index].state;
        value = (uint16_t)pending[index].xyz[0];
        records[i][1] = (uint8_t)value;
        records[i][2] = (uint8_t)(value >> 8);
        value = (uint16_t)pending[index].xyz[1];
        records[i][3] = (uint8_t)value;
        records[i][4] = (uint8_t)(value >> 8);
        value = (uint16_t)pending[index].xyz[2];
        records[i][5] = (uint8_t)value;
        records[i][6] = (uint8_t)(value >> 8);
    }
    stats.storage_steps++;
    result = app_protocol_acc_commit_async_begin(timestamps, records, count);
    if (result == ACC_COMMIT_BUSY) {
        storage_inflight = 1;
    } else if (result == ACC_COMMIT_FULL) {
        full_stop = 1;
        acc_freeze();
        stats.full_discard += queue_count;
        queue_count = 0;
        drain_action = ACC_DRAIN_STOP;
        tail_drained = 1;
    } else {
        acc_storage_fail("Flash async begin");
    }
}

void acc_stream_storage_process(void)
{
    if (!storage_pending) return;
    storage_pending = 0;
    acc_storage_step();
    acc_storage_schedule();
}

bool acc_stream_storage_busy(void)
{
    return storage_inflight != 0;
}

bool acc_stream_can_yield_flash(void)
{
    return !storage_inflight && drain_action == ACC_DRAIN_NONE &&
        (storage_failed || queue_count < ACC_RAM_QUEUE_YIELD_SAMPLES);
}

static bool acc_enter_low(void)
{
    ke_timer_clear(APP_ACC_FIFO_POLL, TASK_APP);
    if (!ADXL345_LowRate_Start(ACC_LOW_ACTIVITY_THRESHOLD_REG, ACC_LOW_ACTIVITY_CTL_REG)) {
        acc_fault("LOW configuration");
        return false;
    }
    motion_state = ACC_MOTION_MARK_LOW;
    stream_frozen = freeze_failed = 0;
    enter_low = previous_valid = 0;
    exit_source = ACC_EXIT_NONE;
    soft_quiet_samples = 0;
    low_guard_remaining = ACC_LOW_ACTIVITY_GUARD_SAMPLES;
    activity_armed = 0;
    low_read_valid = 0;
    last_low_read = acc_clock_hs();
    irq_pending = poll_pending = 0;
    acc_gpio_interrupt_enable();
    uart_printf("ACC mode -> LOW, ODR=1Hz, activity_reg=%u, guard=%u\r\n",
        ACC_LOW_ACTIVITY_THRESHOLD_REG, low_guard_remaining);
    return true;
}

static bool acc_enter_high(void)
{
    if (!ADXL345_Stream_Start(ACC_FIFO_WATERMARK_SAMPLES,
            ACC_HIGH_INACT_THRESHOLD_REG, ACC_HIGH_INACT_TIME_REG,
            ACC_HIGH_INACT_CTL_REG)) {
        acc_fault("HIGH configuration");
        return false;
    }
    motion_state = ACC_MOTION_MARK_HIGH;
    stream_frozen = freeze_failed = 0;
    memcpy(previous_xyz, latest_xyz, sizeof(previous_xyz));
    previous_valid = latest_valid;
    enter_low = 0;
    exit_source = ACC_EXIT_NONE;
    soft_quiet_samples = 0;
    activity_armed = 0;
    last_check = acc_clock_hs();
    irq_pending = poll_pending = 0;
    acc_gpio_interrupt_enable();
    ke_timer_set(APP_ACC_FIFO_POLL, TASK_APP, ACC_FIFO_POLL_TICKS);
    uart_printf("ACC mode -> HIGH, ODR=50Hz, watermark=%u, inact=%u/%us\r\n",
        ACC_FIFO_WATERMARK_SAMPLES, ACC_HIGH_INACT_THRESHOLD_REG,
        ACC_HIGH_INACT_TIME_REG);
    return true;
}

static void acc_low_sample(uint8_t periodic)
{
    uint8_t source;
    uint8_t activity_status;
    uint8_t sample_saved = 0;
    int16_t xyz[3];
    uint32_t now = acc_clock_hs();
    if (!ADXL345_Activity_Status_Checked(&activity_status, &source)) {
        acc_fault("I2C LOW status");
        return;
    }
    if (source & ADXL345_INT_ACTIVITY) {
        if (activity_status & ADXL345_ACT_X_SOURCE) stats.activity_x++;
        if (activity_status & ADXL345_ACT_Y_SOURCE) stats.activity_y++;
        if (activity_status & ADXL345_ACT_Z_SOURCE) stats.activity_z++;
        uart_printf("ACC activity: armed=%u axis=%c%c%c\r\n", activity_armed,
            activity_status & ADXL345_ACT_X_SOURCE ? 'X' : '-',
            activity_status & ADXL345_ACT_Y_SOURCE ? 'Y' : '-',
            activity_status & ADXL345_ACT_Z_SOURCE ? 'Z' : '-');
    }
    if (!activity_armed && (source & ADXL345_INT_ACTIVITY))
        stats.ignored_activity++;
    if (!periodic && (!activity_armed || !(source & ADXL345_INT_ACTIVITY))) {
        return;
    }
    /* Periodic tick and activity may refer to the same 1 Hz sample. */
    if (acc_elapsed_hs(now, last_low_read) >= ACC_LOW_DUPLICATE_HS ||
        (!low_read_valid && (source & ADXL345_INT_ACTIVITY))) {
        if (!ADXL345_read_data(&xyz[0], &xyz[1], &xyz[2])) { acc_fault("I2C LOW read"); return; }
        utc_update();
        acc_enqueue(ACC_MOTION_MARK_LOW, xyz, utc_get_clock());
        last_low_read = now;
        low_read_valid = 1;
        sample_saved = 1;
    }
    if (!activity_armed) {
        if (sample_saved && low_guard_remaining != 0) low_guard_remaining--;
        if (low_guard_remaining == 0) {
            if (!ADXL345_LowRate_ArmActivity()) {
                acc_fault("Activity arm");
                return;
            }
            activity_armed = 1;
            uart_printf("ACC LOW activity armed after %u samples\r\n",
                ACC_LOW_ACTIVITY_GUARD_SAMPLES);
        }
        return;
    }
    if ((source & ADXL345_INT_ACTIVITY) && latest_valid && fault == 0 && !full_stop)
        acc_enter_high();
}

static void acc_request_drain(uint8_t action)
{
    if (action > drain_action) drain_action = action;
    if (!stream_frozen) acc_freeze();
    if (motion_state != ACC_MOTION_MARK_HIGH || fault != 0 || full_stop)
        tail_drained = 1;
    acc_storage_schedule();
}

static void acc_finish_drain(void)
{
    uint8_t action;
    if (drain_action == ACC_DRAIN_NONE || !tail_drained ||
        storage_inflight || (queue_count != 0 && !storage_failed)) return;
    action = drain_action;
    drain_action = ACC_DRAIN_NONE;
    ADXL345_Stream_Stop();
    acc_cancel_service();
    if ((action == ACC_DRAIN_TO_LOW || action == ACC_DRAIN_RESTART_LOW) &&
        fault == 0 && !full_stop) {
        if (action == ACC_DRAIN_TO_LOW && !enter_low) {
            uart_printf("ACC HIGH exit cancelled by FIFO tail motion\r\n");
            acc_enter_high();
            return;
        }
        last_exit_source = exit_source;
        if (action == ACC_DRAIN_RESTART_LOW) {
            uart_printf("ACC pause cancelled -> LOW restart\r\n");
        } else {
            uart_printf("ACC HIGH exit -> LOW, source=%s quiet=%u\r\n",
                exit_source == ACC_EXIT_HW_INACTIVITY ?
                "HW_INACT" : "SW_QUIET", soft_quiet_samples);
        }
        if (!acc_enter_low()) acc_request_drain(ACC_DRAIN_STOP);
        return;
    }
    if (action == ACC_DRAIN_PAUSE && fault == 0 && !full_stop) {
        stream_active = stream_paused = 1;
        stream_frozen = 1;
        app_task_enter_lowpower(TASK_ID_ACC);
        acc_stats("paused");
        return;
    }
    stream_active = stream_paused = 0;
    stream_frozen = 1;
    app_task_enter_lowpower(TASK_ID_ACC);
    acc_stats(fault == 0 ? "stopped" : "incomplete");
    if (fault != 0 || full_stop)
        app_protocol_acc_stream_fault(full_stop ? "Flash full" : fault);
}

void acc_stream_process(void)
{
    uint8_t irq, poll;
    if (processing || !stream_active || stream_paused) return;
    processing = 1;
    irq = irq_pending;
    irq_pending = 0;
    poll = poll_pending;
    poll_pending = 0;
    if (drain_action != ACC_DRAIN_NONE) {
        if (!stream_frozen) acc_freeze();
        if (!tail_drained && motion_state == ACC_MOTION_MARK_HIGH &&
            fault == 0 && !full_stop) {
            acc_capture(1);
        } else if (motion_state != ACC_MOTION_MARK_HIGH || fault != 0 || full_stop) {
            tail_drained = 1;
        }
    } else if (fault == 0 && !full_stop && !stream_frozen) {
        if (motion_state == ACC_MOTION_MARK_LOW) {
            if (irq) {
                acc_low_sample(0);
                if (motion_state == ACC_MOTION_MARK_LOW && fault == 0)
                    acc_irq_try_rearm();
            }
        } else {
            if (irq || poll) acc_capture(irq);
            if (fault == 0 && !full_stop && enter_low)
                acc_request_drain(ACC_DRAIN_TO_LOW);
        }
    }
    if ((fault != 0 || full_stop) && drain_action == ACC_DRAIN_NONE)
        acc_request_drain(ACC_DRAIN_STOP);
    acc_finish_drain();
    if (stream_active &&
        acc_elapsed_hs(acc_clock_hs(), last_stats) >= ACC_STATS_HS)
        acc_stats("running");
    processing = 0;
}

void acc_stream_poll_task(void)
{
    if (!stream_active || stream_paused || stream_frozen || motion_state != ACC_MOTION_MARK_HIGH) return;
    stats.polls++;
    poll_pending = 1;
    ke_timer_set(APP_ACC_FIFO_POLL, TASK_APP, ACC_FIFO_POLL_TICKS);
}

bool acc_stream_stop(void)
{
    if (!stream_active && !stream_paused && drain_action == ACC_DRAIN_NONE)
        return !storage_failed && !freeze_failed;
    ke_timer_clear(APP_READ_ACC, TASK_APP);
    single_pending = 0;
    if (stream_paused) {
        stream_paused = 0;
        stream_active = 0;
        drain_action = ACC_DRAIN_NONE;
        return !storage_failed && !freeze_failed;
    }
    acc_request_drain(ACC_DRAIN_STOP);
    return true;
}

bool acc_stream_pause(void)
{
    if (stream_paused) return true;
    if (!stream_active && queue_count == 0 && !storage_inflight) return true;
    acc_request_drain(ACC_DRAIN_PAUSE);
    return true;
}

void acc_stream_cancel_pause(void)
{
    if (drain_action == ACC_DRAIN_PAUSE) {
        drain_action = ACC_DRAIN_RESTART_LOW;
        enter_low = 1;
        exit_source = ACC_EXIT_NONE;
    } else if (stream_paused) {
        acc_stream_resume();
    }
}

bool acc_stream_start(void)
{
    if (stream_active || stream_paused || drain_action != ACC_DRAIN_NONE ||
        storage_inflight || queue_count != 0) return false;
    ke_timer_clear(APP_ACC_FLASH_STEP, TASK_APP);
    storage_pending = storage_timer_armed = storage_timer_ticks = 0;
    memset(&stats, 0, sizeof(stats));
    fault = 0;
    session_complete = 1;
    full_stop = storage_failed = freeze_failed = 0;
    last_exit_source = ACC_EXIT_NONE;
    queue_head = queue_count = 0;
    storage_inflight = drain_action = tail_drained = queue_warned = 0;
    latest_valid = 0;
    last_stats = acc_clock_hs();
    stream_active = 1;
    stream_paused = 0;
    app_task_quit_lowpower(TASK_ID_ACC);
    ext_flash_set_wait_hook(acc_flash_wait_service);
    ADXL345_Init();
    if (!acc_enter_low()) { acc_stream_stop(); return false; }
    uart_printf("ACC FIFO5 ready: INT1=P20 WM=16 slice=8 RAM=128 batch=8 async-flash irq-mask flash=20/10ms guard=2 inact=3/2s\r\n");
    return true;
}

bool acc_stream_resume(void)
{
    if (!stream_active) return false;
    if (!stream_paused) return true;
    app_task_quit_lowpower(TASK_ID_ACC);
    stream_paused = 0;
    latest_valid = 0;
    drain_action = ACC_DRAIN_NONE;
    if (!acc_enter_low()) { acc_stream_stop(); return false; }
    return true;
}

void acc_stream_second_tick(void)
{
    if (!stream_active || stream_paused || drain_action != ACC_DRAIN_NONE || processing) return;
    if (motion_state == ACC_MOTION_MARK_LOW && fault == 0 && !full_stop) {
        processing = 1;
        acc_low_sample(1);
        if (motion_state == ACC_MOTION_MARK_LOW && fault == 0)
            acc_irq_try_rearm();
        processing = 0;
    }
}

bool acc_stream_is_active(void) { return stream_active != 0; }
bool acc_stream_is_stopped(void)
{
    return !stream_active && !stream_paused && drain_action == ACC_DRAIN_NONE &&
        !storage_inflight;
}
bool acc_stream_is_paused(void)
{
    return stream_paused && drain_action == ACC_DRAIN_NONE && !storage_inflight &&
        queue_count == 0;
}
bool acc_stream_data_complete(void) { return session_complete != 0; }

void acc_stream_history_cleared(void)
{
    if (!stream_active && queue_count == 0) {
        fault = 0;
        session_complete = 1;
        storage_failed = freeze_failed = full_stop = 0;
    }
}

uint8_t acc_stream_motion_state(void) { return motion_state; }

void app_read_acc_task(void)
{
    SensorAccStruct sample;
    if (stream_active) { poll_pending = 1; return; }
    if (!single_pending) return;
    if (ADXL345_read_data(&sample.acc_x1, &sample.acc_y1, &sample.acc_z1))
        app_protocol_sensor_data(PROTOCOL_SENSOR_ACC, (uint8_t *)&sample, sizeof(sample));
    ADXL345_Enable(false);
    single_pending = 0;
    app_task_enter_lowpower(TASK_ID_ACC);
}

void acc_collect_once_report(uint8_t again)
{
    SensorAccStruct sample;
    (void)again;
    if (storage_failed || freeze_failed) return;
    if (stream_active && !stream_paused) {
        if (!latest_valid) {
            if (motion_state == ACC_MOTION_MARK_HIGH) acc_capture(1);
            else if (ADXL345_read_data(&sample.acc_x1, &sample.acc_y1, &sample.acc_z1)) {
                app_protocol_sensor_data(PROTOCOL_SENSOR_ACC,
                    (uint8_t *)&sample, sizeof(sample));
                return;
            }
        }
        if (latest_valid) {
            sample.acc_x1 = latest_xyz[0]; sample.acc_y1 = latest_xyz[1]; sample.acc_z1 = latest_xyz[2];
            app_protocol_sensor_data(PROTOCOL_SENSOR_ACC, (uint8_t *)&sample, sizeof(sample));
        }
        return;
    }
    if (!single_pending) {
        app_task_quit_lowpower(TASK_ID_ACC);
        ADXL345_Init(); ADXL345_Enable(true);
        single_pending = 1;
        ke_timer_set(APP_READ_ACC, TASK_APP, 10);
    }
}
