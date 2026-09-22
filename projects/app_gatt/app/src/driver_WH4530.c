#include "driver_WH4530.h"
#include "uart.h"
#include "rf.h"

#define WH4530_REG_SYSTEM_CTRL   0x00
#define WH4530_REG_INT_CTRL      0x01
#define WH4530_REG_INT_FLAG      0x02
#define WH4530_REG_WAIT_TIME     0x03
#define WH4530_REG_LED_CTRL      0x06
#define WH4530_REG_PS_GAIN       0x07
#define WH4530_REG_PS_PULSE      0x08
#define WH4530_REG_PS_TIME       0x09
#define WH4530_REG_PERSISTENCE   0x0B
#define WH4530_REG_PS_OFFSET_L   0x14
#define WH4530_REG_PS_OFFSET_H   0x15
#define WH4530_REG_PS_DATA_L     0x18

#define WH4530_SYSTEM_RESET      0x80
#define WH4530_SYSTEM_PS_ENABLE  0x02

static bool WH4530_Write_Register(uint8_t reg, uint8_t value)
{
    return i2c_write(WH4530_ADDRESS, reg, &value, 1) == OK;
}

static bool WH4530_Read_Register(uint8_t reg, uint8_t *value)
{
    if (value == NULL) {
        return false;
    }
    *value = 0xFF;
    return i2c_read(WH4530_ADDRESS, reg, value, 1) == OK;
}

bool WH4530_Reset(void)
{
    /* SYSM_CTRL.bit7 is the software-reset bit; writing 0 does not reset. */
    return WH4530_Write_Register(WH4530_REG_SYSTEM_CTRL, WH4530_SYSTEM_RESET);
}

bool WH4530_Init(void)
{
    uint8_t ctrl = 0xFF;
    uint8_t led = 0xFF;
    uint8_t gain = 0xFF;
    uint8_t pulse = 0xFF;
    uint8_t ps_time = 0xFF;
    uint8_t persistence = 0xFF;
    bool ok;

    /*
     * PRS_PS=0 makes INT_PS assert after every completed PS conversion.
     * The interrupt pin itself remains disabled; task_dist polls INT_FLAG.
     */
    ok = WH4530_Write_Register(WH4530_REG_SYSTEM_CTRL, 0x00) &&
         WH4530_Write_Register(WH4530_REG_INT_CTRL, 0x00) &&
         WH4530_Write_Register(WH4530_REG_INT_FLAG, 0x00) &&
         WH4530_Write_Register(WH4530_REG_WAIT_TIME, 0x00) &&
         WH4530_Write_Register(WH4530_REG_LED_CTRL, 0x17) &&
         WH4530_Write_Register(WH4530_REG_PS_GAIN, 0x00) &&
         WH4530_Write_Register(WH4530_REG_PS_PULSE, 0x05) &&
         WH4530_Write_Register(WH4530_REG_PS_TIME, 0x07) &&
         WH4530_Write_Register(WH4530_REG_PERSISTENCE, 0x00) &&
         WH4530_Write_Register(WH4530_REG_PS_OFFSET_L, 0x00) &&
         WH4530_Write_Register(WH4530_REG_PS_OFFSET_H, 0x00);

    ok = ok &&
         WH4530_Read_Register(WH4530_REG_SYSTEM_CTRL, &ctrl) &&
         WH4530_Read_Register(WH4530_REG_LED_CTRL, &led) &&
         WH4530_Read_Register(WH4530_REG_PS_GAIN, &gain) &&
         WH4530_Read_Register(WH4530_REG_PS_PULSE, &pulse) &&
         WH4530_Read_Register(WH4530_REG_PS_TIME, &ps_time) &&
         WH4530_Read_Register(WH4530_REG_PERSISTENCE, &persistence);

    uart_printf("WH4530 cfg: CTRL=%02x LED=%02x GAIN=%02x PULSE=%02x TIME=%02x PRS=%02x ok=%u\r\n",
                ctrl, led, gain, pulse, ps_time, persistence, ok ? 1 : 0);

    return ok && ctrl == 0x00 && led == 0x17 && gain == 0x00 &&
           pulse == 0x05 && ps_time == 0x07 && persistence == 0x00;
}

bool WH4530_Enable(bool enable)
{
    uint8_t expected = enable ? WH4530_SYSTEM_PS_ENABLE : 0x00;
    uint8_t value = 0xFF;

    return WH4530_Write_Register(WH4530_REG_SYSTEM_CTRL, expected) &&
           WH4530_Read_Register(WH4530_REG_SYSTEM_CTRL, &value) &&
           value == expected;
}

bool WH4530_Set_Gain(uint8_t gain)
{
    uint8_t value = 0xFF;

    gain &= 0x03;
    return WH4530_Write_Register(WH4530_REG_PS_GAIN, gain) &&
           WH4530_Read_Register(WH4530_REG_PS_GAIN, &value) &&
           value == gain;
}

bool WH4530_Clear_Status(void)
{
    return WH4530_Write_Register(WH4530_REG_INT_FLAG, 0x00);
}

bool WH4530_Read_Status(uint8_t *status)
{
    return WH4530_Read_Register(WH4530_REG_INT_FLAG, status);
}

bool WH4530_Read_Distance(uint16_t *distance, uint8_t raw_data[2])
{
    uint8_t data[2] = {0xFF, 0xFF};

    if (distance == NULL || i2c_read(WH4530_ADDRESS, WH4530_REG_PS_DATA_L,
                                     data, sizeof(data)) != OK) {
        return false;
    }

    *distance = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    if (raw_data != NULL) {
        raw_data[0] = data[0];
        raw_data[1] = data[1];
    }
    return true;
}

