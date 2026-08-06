#include <stdbool.h>
#include "user_config.h"
#include "gpio.h"
#include "driver_BQ21080.h"

#if DEBUG_HW
#error "BQ21080 uses P26/P27, but DEBUG_HW also uses these pins"
#endif

#if I2S_DRIVER
#error "BQ21080 uses P26/P27, but I2S_DRIVER also uses these pins"
#endif

/* Schematic connection: P27 -> SCL, P26 -> SDA. */
#define BQ_SCL_PIN                  0x27
#define BQ_SDA_PIN                  0x26

/* Values fixed by the BQ21080 data sheet. */
#define BQ21080_I2C_ADDR            0x6A
#define BQ21080_REG_MASK_ID         0x0C
#define BQ21080_DEVICE_ID_MASK      0x0F

/* Conservative delay for GPIO bit-banged I2C at the normal 16-MHz clock. */
#define BQ_I2C_DELAY_COUNT          40
#define BQ_SCL_HIGH_TIMEOUT         1000

static void bq_i2c_delay(void)
{
    volatile uint16_t count = BQ_I2C_DELAY_COUNT;

    while (count--)
    {
    }
}

/* Open-drain emulation: drive low for 0, release the pin for 1. */
static void bq_scl_low(void)
{
    gpio_config(BQ_SCL_PIN, OUTPUT, PULL_NONE);
}

static void bq_scl_release(void)
{
    gpio_config(BQ_SCL_PIN, INPUT, PULL_HIGH);
}

static void bq_sda_low(void)
{
    gpio_config(BQ_SDA_PIN, OUTPUT, PULL_NONE);
}

static void bq_sda_release(void)
{
    gpio_config(BQ_SDA_PIN, INPUT, PULL_HIGH);
}

static uint8_t bq_scl_read(void)
{
    return gpio_get_input(BQ_SCL_PIN) ? 1 : 0;
}

static uint8_t bq_sda_read(void)
{
    return gpio_get_input(BQ_SDA_PIN) ? 1 : 0;
}

static bool bq_scl_high(void)
{
    uint16_t timeout = BQ_SCL_HIGH_TIMEOUT;

    bq_scl_release();

    while (timeout--)
    {
        if (bq_scl_read())
        {
            bq_i2c_delay();
            return true;
        }
    }

    return false;
}

static bool bq_i2c_start(void)
{
    bq_sda_release();

    if (!bq_scl_high() || !bq_sda_read())
    {
        return false;
    }

    /* SDA high-to-low while SCL is high generates START. */
    bq_sda_low();
    bq_i2c_delay();
    bq_scl_low();
    bq_i2c_delay();

    return true;
}

static bool bq_i2c_stop(void)
{
    bq_sda_low();
    bq_i2c_delay();

    if (!bq_scl_high())
    {
        bq_sda_release();
        return false;
    }

    /* SDA low-to-high while SCL is high generates STOP. */
    bq_sda_release();
    bq_i2c_delay();

    return bq_scl_read() && bq_sda_read();
}

static bool bq_i2c_write_byte(uint8_t data, bool *ack)
{
    uint8_t bit;

    if (ack == 0)
    {
        return false;
    }

    for (bit = 0; bit < 8; bit++)
    {
        if (data & 0x80)
        {
            bq_sda_release();
        }
        else
        {
            bq_sda_low();
        }

        bq_i2c_delay();

        if (!bq_scl_high())
        {
            return false;
        }

        bq_scl_low();
        bq_i2c_delay();
        data <<= 1;
    }

    /* Ninth clock: the BQ21080 drives ACK low. */
    bq_sda_release();

    if (!bq_scl_high())
    {
        return false;
    }

    *ack = (bq_sda_read() == 0);
    bq_scl_low();
    bq_i2c_delay();

    return true;
}

static bool bq_i2c_read_byte(uint8_t *value, bool ack)
{
    uint8_t bit;
    uint8_t data = 0;

    if (value == 0)
    {
        return false;
    }

    bq_sda_release();

    for (bit = 0; bit < 8; bit++)
    {
        data <<= 1;

        if (!bq_scl_high())
        {
            return false;
        }

        if (bq_sda_read())
        {
            data |= 0x01;
        }

        bq_scl_low();
        bq_i2c_delay();
    }

    /* ACK means more bytes; NACK terminates this one-byte read. */
    if (ack)
    {
        bq_sda_low();
    }
    else
    {
        bq_sda_release();
    }

    bq_i2c_delay();

    if (!bq_scl_high())
    {
        return false;
    }

    bq_scl_low();
    bq_sda_release();
    bq_i2c_delay();

    *value = data;
    return true;
}

bq21080_status_t bq21080_init(void)
{
    uint8_t clock_count;

    bq_sda_release();

    if (!bq_scl_high())
    {
        return BQ21080_ERR_BUS;
    }

    /* Up to nine clocks recover a target left in a partial transfer. */
    for (clock_count = 0;
         (clock_count < 9) && !bq_sda_read();
         clock_count++)
    {
        bq_scl_low();
        bq_i2c_delay();

        if (!bq_scl_high())
        {
            return BQ21080_ERR_BUS;
        }
    }

    if (!bq_i2c_stop())
    {
        return BQ21080_ERR_BUS;
    }

    return BQ21080_OK;
}

bq21080_status_t bq21080_read_reg(uint8_t reg, uint8_t *value)
{
    bool ack;

    if (value == 0)
    {
        return BQ21080_ERR_PARAM;
    }

    if (!bq_i2c_start())
    {
        return BQ21080_ERR_BUS;
    }

    if (!bq_i2c_write_byte((uint8_t)(BQ21080_I2C_ADDR << 1), &ack))
    {
        bq_i2c_stop();
        return BQ21080_ERR_BUS;
    }

    if (!ack)
    {
        bq_i2c_stop();
        return BQ21080_ERR_ADDR_WRITE_NACK;
    }

    if (!bq_i2c_write_byte(reg, &ack))
    {
        bq_i2c_stop();
        return BQ21080_ERR_BUS;
    }

    if (!ack)
    {
        bq_i2c_stop();
        return BQ21080_ERR_REG_NACK;
    }

    /* Repeated START followed by the read address 0xD5. */
    if (!bq_i2c_start())
    {
        bq_i2c_stop();
        return BQ21080_ERR_BUS;
    }

    if (!bq_i2c_write_byte(
            (uint8_t)((BQ21080_I2C_ADDR << 1) | 0x01), &ack))
    {
        bq_i2c_stop();
        return BQ21080_ERR_BUS;
    }

    if (!ack)
    {
        bq_i2c_stop();
        return BQ21080_ERR_ADDR_READ_NACK;
    }

    if (!bq_i2c_read_byte(value, false))
    {
        bq_i2c_stop();
        return BQ21080_ERR_BUS;
    }

    if (!bq_i2c_stop())
    {
        return BQ21080_ERR_BUS;
    }

    return BQ21080_OK;
}

bq21080_status_t bq21080_read_device_id(uint8_t *device_id,
                                        uint8_t *mask_id)
{
    bq21080_status_t status;
    uint8_t value;

    if ((device_id == 0) || (mask_id == 0))
    {
        return BQ21080_ERR_PARAM;
    }

    status = bq21080_read_reg(BQ21080_REG_MASK_ID, &value);

    if (status != BQ21080_OK)
    {
        return status;
    }

    *mask_id = value;
    *device_id = value & BQ21080_DEVICE_ID_MASK;

    return BQ21080_OK;
}
