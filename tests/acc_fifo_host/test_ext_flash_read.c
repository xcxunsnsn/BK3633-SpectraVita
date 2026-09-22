#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXT_FLASH_HOST_TEST 1
#include "../../projects/app_gatt/app/src/ext_flash.c"

static uint8_t image_data[EXT_FLASH_TOTAL_SIZE];
static uint8_t output_data[4096];
static uint32_t read_address;
static uint32_t read_calls;
static uint32_t max_read_length;
static uint32_t read_command_count;
static int fail_read_call;
static uint8_t cs_low;

#define CHECK(value) do { if (!(value)) { \
    printf("FAIL line %d: %s\n", __LINE__, #value); exit(1); \
} } while (0)

static void prepare(void)
{
    uint32_t i;
    for (i = 0; i < sizeof(image_data); i++)
        image_data[i] = (uint8_t)(i ^ (i >> 8));
    memset(output_data, 0, sizeof(output_data));
    flash_ready = 1;
    async_operation = FLASH_ASYNC_NONE;
    read_address = read_calls = max_read_length = read_command_count = 0;
    fail_read_call = -1;
    cs_low = 0;
}

static void check_read(uint32_t address, uint32_t length,
                       uint32_t expected_calls, uint32_t expected_commands)
{
    memset(output_data, 0, sizeof(output_data));
    read_calls = max_read_length = read_command_count = 0;
    CHECK(ext_flash_read(address, output_data, length));
    CHECK(memcmp(output_data, &image_data[address], length) == 0);
    CHECK(read_calls == expected_calls);
    CHECK(read_command_count == expected_commands);
    CHECK(max_read_length <= 64);
    CHECK(!cs_low);
}

int main(void)
{
    prepare();
    check_read(0x1234, 20, 1, 1);
    check_read(0x2345, 32, 1, 1);
    check_read(0x3456, 268, 5, 1);
    check_read(0x4567, 4096, 64, 2);

    read_calls = read_command_count = 0;
    fail_read_call = 3;
    CHECK(!ext_flash_read(0x5678, output_data, 268));
    CHECK(read_calls == 3 && !cs_low);
    puts("PASS: external Flash chunked read tests");
    return 0;
}

void gpio_config(uint8_t p, int d, int u) { (void)p; (void)d; (void)u; }
void gpio_set(uint8_t p, uint8_t value) {
    (void)p;
    cs_low = value == 0;
}
void Delay_us(int count) { (void)count; }
void spi_write(uint8_t *data, uint32_t length) {
    CHECK(cs_low);
    if (length == 4 && data[0] == W25Q_CMD_READ_DATA) {
        read_address = ((uint32_t)data[1] << 16) |
            ((uint32_t)data[2] << 8) | data[3];
        read_command_count++;
    }
}
bool spi_read_checked(uint8_t *data, uint32_t length) {
    CHECK(cs_low && length <= 64);
    read_calls++;
    if (length > max_read_length) max_read_length = length;
    if ((int)read_calls == fail_read_call) return false;
    memcpy(data, &image_data[read_address], length);
    read_address += length;
    return true;
}
uint8_t spi_send_state_get(void) { return 1; }
rwip_time_t rwip_time_get(void) { rwip_time_t t = {0, 0}; return t; }
int uart_printf(const char *format, ...) { (void)format; return 0; }
