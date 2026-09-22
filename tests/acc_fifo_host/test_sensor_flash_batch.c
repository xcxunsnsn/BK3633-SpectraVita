#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SENSOR_FLASH_HOST_TEST 1
#include "../../projects/app_gatt/app/src/sensor_flash_store.c"

static uint8_t flash_image[EXT_FLASH_TOTAL_SIZE];
static uint32_t write_calls, erase_calls;
static int fail_write_call;
static uint8_t fake_async_active;
static uint16_t last_async_steps;
static uint32_t read_calls;
static int fail_read_call;
static uint8_t corrupt_reads;

#define CHECK(value) do { if (!(value)) { \
    printf("FAIL line %d: %s\n", __LINE__, #value); exit(1); \
} } while (0)

static void prepare(uint32_t count, uint32_t maximum)
{
    memset(flash_image, 0xFF, sizeof(flash_image));
    memset(contexts, 0, sizeof(contexts));
    store_ready = 1;
    contexts[PROTOCOL_SENSOR_ACC].valid = 1;
    contexts[PROTOCOL_SENSOR_ACC].session_id = 0x1234;
    contexts[PROTOCOL_SENSOR_ACC].period_s = 1;
    contexts[PROTOCOL_SENSOR_ACC].count = count;
    contexts[PROTOCOL_SENSOR_ACC].max_records = maximum;
    write_calls = erase_calls = 0;
    fail_write_call = -1;
    fake_async_active = 0;
    read_calls = 0;
    fail_read_call = -1;
    corrupt_reads = 0;
}

static void make_records(uint32_t timestamps[8], uint8_t records[8][7])
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        timestamps[i] = 1000 + i;
        records[i][0] = i & 1;
        records[i][1] = i; records[i][2] = 0;
        records[i][3] = i + 20; records[i][4] = 0;
        records[i][5] = i + 40; records[i][6] = 0;
    }
}

static void test_batch_and_recovery(void)
{
    uint32_t timestamps[8];
    uint8_t records[8][7], accepted, i;
    make_records(timestamps, records);
    prepare(0, 365364);
    CHECK(sensor_flash_store_append_acc_batch(timestamps, records, 8, &accepted)
          == SENSOR_FLASH_BATCH_OK);
    CHECK(accepted == 8 && write_calls == 2 && erase_calls == 1);
    for (i = 0; i < 8; i++) {
        uint8_t *slot = &flash_image[record_address(PROTOCOL_SENSOR_ACC, i)];
        CHECK(record_valid(PROTOCOL_SENSOR_ACC, slot));
        CHECK(get_le32(&slot[4]) == timestamps[i]);
        CHECK(memcmp(&slot[8], records[i], 7) == 0);
    }
    contexts[PROTOCOL_SENSOR_ACC].count = 0;
    CHECK(scan_records(PROTOCOL_SENSOR_ACC));
    CHECK(contexts[PROTOCOL_SENSOR_ACC].count == 8);
}

static void test_boundaries_and_full(void)
{
    uint32_t timestamps[8];
    uint8_t records[8][7], accepted;
    make_records(timestamps, records);

    prepare(12, 365364); /* Slot 12 starts at page offset 240. */
    CHECK(sensor_flash_store_append_acc_batch(timestamps, records, 8, &accepted)
          == SENSOR_FLASH_BATCH_OK);
    CHECK(accepted == 8 && contexts[PROTOCOL_SENSOR_ACC].count == 20);

    prepare(200, 365364); /* Four slots remain in the current 4 KiB sector. */
    CHECK(sensor_flash_store_append_acc_batch(timestamps, records, 8, &accepted)
          == SENSOR_FLASH_BATCH_OK);
    CHECK(accepted == 4 && erase_calls == 0);
    CHECK(sensor_flash_store_append_acc_batch(timestamps + 4, records + 4, 4,
                                               &accepted)
          == SENSOR_FLASH_BATCH_OK);
    CHECK(accepted == 4 && erase_calls == 1);

    prepare(0, 3);
    CHECK(sensor_flash_store_append_acc_batch(timestamps, records, 8, &accepted)
          == SENSOR_FLASH_BATCH_LAST);
    CHECK(accepted == 3 && contexts[PROTOCOL_SENSOR_ACC].count == 3);
}

static void test_magic_pass_failure(void)
{
    uint32_t timestamps[8];
    uint8_t records[8][7], accepted;
    uint8_t *first;
    make_records(timestamps, records);
    prepare(0, 365364);
    fail_write_call = 2;
    CHECK(sensor_flash_store_append_acc_batch(timestamps, records, 8, &accepted)
          == SENSOR_FLASH_BATCH_ERROR);
    first = &flash_image[record_address(PROTOCOL_SENSOR_ACC, 0)];
    CHECK(accepted == 0 && contexts[PROTOCOL_SENSOR_ACC].count == 0);
    CHECK(!record_valid(PROTOCOL_SENSOR_ACC, first));
}

static void test_ten_sector_sustained_log(void)
{
    uint32_t timestamps[8];
    uint8_t records[8][7], accepted, i;
    uint32_t before;
    prepare(0, 365364);
    while (contexts[PROTOCOL_SENSOR_ACC].count < 2040) {
        before = contexts[PROTOCOL_SENSOR_ACC].count;
        for (i = 0; i < 8; i++) {
            timestamps[i] = 1000 + before + i;
            memset(records[i], 0, 7);
            records[i][0] = 1;
            records[i][1] = (uint8_t)(before + i);
            records[i][2] = (uint8_t)((before + i) >> 8);
        }
        CHECK(sensor_flash_store_append_acc_batch(timestamps, records, 8,
                                                   &accepted)
              == SENSOR_FLASH_BATCH_OK);
        CHECK(accepted != 0);
    }
    CHECK(contexts[PROTOCOL_SENSOR_ACC].count == 2040);
    CHECK(erase_calls == 10 && write_calls == 520);
    contexts[PROTOCOL_SENSOR_ACC].count = 0;
    CHECK(scan_records(PROTOCOL_SENSOR_ACC));
    CHECK(contexts[PROTOCOL_SENSOR_ACC].count == 2040);
}

static uint8_t run_async(uint8_t *accepted)
{
    uint16_t guard = 1000;
    uint8_t result;
    last_async_steps = 0;
    do {
        last_async_steps++;
        result = sensor_flash_store_acc_async_process(accepted);
    } while (result == SENSOR_FLASH_BATCH_BUSY && --guard != 0);
    CHECK(guard != 0);
    return result;
}

static void test_async_batch_and_page_boundary(void)
{
    uint32_t timestamps[8];
    uint8_t records[8][7], accepted = 0;
    make_records(timestamps, records);
    prepare(0, 365364);
    CHECK(sensor_flash_store_acc_async_begin(timestamps, records, 8)
          == SENSOR_FLASH_BATCH_BUSY);
    CHECK(run_async(&accepted) == SENSOR_FLASH_BATCH_OK);
    CHECK(accepted == 8 && contexts[PROTOCOL_SENSOR_ACC].count == 8);
    CHECK(erase_calls == 1 && write_calls == 2);
    CHECK(last_async_steps == 4);

    prepare(12, 365364); /* Body and commit each cross the 256-byte page. */
    CHECK(sensor_flash_store_acc_async_begin(timestamps, records, 8)
          == SENSOR_FLASH_BATCH_BUSY);
    CHECK(run_async(&accepted) == SENSOR_FLASH_BATCH_OK);
    CHECK(accepted == 8 && write_calls == 4 && erase_calls == 0);
    CHECK(last_async_steps == 5);
}

static void test_fifo_record_read_and_crc_retry(void)
{
    uint8_t data[256];
    uint8_t output[256];
    uint32_t timestamp;
    uint16_t i;
    prepare(0, 365364);
    contexts[PROTOCOL_SENSOR_FIFO].valid = 1;
    contexts[PROTOCOL_SENSOR_FIFO].session_id = 0x2468;
    contexts[PROTOCOL_SENSOR_FIFO].period_s = 600;
    contexts[PROTOCOL_SENSOR_FIFO].max_records = 2;
    for (i = 0; i < sizeof(data); i++) data[i] = (uint8_t)(i ^ 0x5A);
    CHECK(sensor_flash_store_append(PROTOCOL_SENSOR_FIFO, 1234,
                                    data, sizeof(data)));
    CHECK(contexts[PROTOCOL_SENSOR_FIFO].count == 1);

    read_calls = 0;
    CHECK(sensor_flash_store_read(PROTOCOL_SENSOR_FIFO, 0, &timestamp,
                                  output, sizeof(output)));
    CHECK(timestamp == 1234 && memcmp(data, output, sizeof(data)) == 0);
    CHECK(read_calls == 1);

    read_calls = 0;
    corrupt_reads = 1;
    CHECK(sensor_flash_store_read(PROTOCOL_SENSOR_FIFO, 0, &timestamp,
                                  output, sizeof(output)));
    CHECK(read_calls == 2 && memcmp(data, output, sizeof(data)) == 0);

    read_calls = 0;
    corrupt_reads = 2;
    CHECK(!sensor_flash_store_read(PROTOCOL_SENSOR_FIFO, 0, &timestamp,
                                   output, sizeof(output)));
    CHECK(read_calls == 2 && contexts[PROTOCOL_SENSOR_FIFO].count == 1);

    read_calls = 0;
    corrupt_reads = 0;
    fail_read_call = 1;
    CHECK(!sensor_flash_store_read(PROTOCOL_SENSOR_FIFO, 0, &timestamp,
                                   output, sizeof(output)));
    CHECK(read_calls == 1 && contexts[PROTOCOL_SENSOR_FIFO].count == 1);
}

int main(void)
{
    test_batch_and_recovery();
    test_boundaries_and_full();
    test_magic_pass_failure();
    test_ten_sector_sustained_log();
    test_async_batch_and_page_boundary();
    test_fifo_record_read_and_crc_retry();
    puts("PASS: sensor Flash batch tests");
    return 0;
}

bool ext_flash_init(void) { return true; }
bool ext_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    read_calls++;
    if ((int)read_calls == fail_read_call) return false;
    if (address + length > sizeof(flash_image)) return false;
    memcpy(data, &flash_image[address], length);
    if (corrupt_reads != 0 && length > 10) {
        data[10] ^= 0x01;
        corrupt_reads--;
    }
    return true;
}
bool ext_flash_write(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint32_t i;
    write_calls++;
    if ((int)write_calls == fail_write_call || address + length > sizeof(flash_image))
        return false;
    for (i = 0; i < length; i++) flash_image[address + i] &= data[i];
    return true;
}
bool ext_flash_erase_sector(uint32_t address)
{
    address &= ~(EXT_FLASH_SECTOR_SIZE - 1);
    if (address + EXT_FLASH_SECTOR_SIZE > sizeof(flash_image)) return false;
    memset(&flash_image[address], 0xFF, EXT_FLASH_SECTOR_SIZE);
    erase_calls++;
    return true;
}
bool ext_flash_async_program_begin(uint32_t address,
                                   const uint8_t *data, uint16_t length)
{
    if (fake_async_active || !ext_flash_write(address, data, length)) return false;
    fake_async_active = 1;
    return true;
}
bool ext_flash_async_erase_begin(uint32_t address)
{
    if (fake_async_active || !ext_flash_erase_sector(address)) return false;
    fake_async_active = 1;
    return true;
}
uint8_t ext_flash_async_poll(void)
{
    if (!fake_async_active) return EXT_FLASH_ASYNC_DONE;
    fake_async_active = 0;
    return EXT_FLASH_ASYNC_DONE;
}
bool ext_flash_async_busy(void) { return fake_async_active != 0; }
int uart_printf(const char *format, ...) { (void)format; return 0; }
