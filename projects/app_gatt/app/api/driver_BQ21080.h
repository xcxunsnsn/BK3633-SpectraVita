#ifndef DRIVER_BQ21080_H_
#define DRIVER_BQ21080_H_

#include <stdint.h>

#define BQ21080_EXPECTED_DEVICE_ID  0x00

typedef enum
{
    BQ21080_OK = 0,
    BQ21080_ERR_PARAM,
    BQ21080_ERR_BUS,
    BQ21080_ERR_ADDR_WRITE_NACK,
    BQ21080_ERR_REG_NACK,
    BQ21080_ERR_ADDR_READ_NACK
} bq21080_status_t;

bq21080_status_t bq21080_init(void);
bq21080_status_t bq21080_read_reg(uint8_t reg, uint8_t *value);
bq21080_status_t bq21080_read_device_id(uint8_t *device_id,
                                        uint8_t *mask_id);

#endif
