#ifndef _driver_WH4530_H_
#define _driver_WH4530_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "i2c.h"


#define WH4530_ADDRESS       (0X38)

#define WH4530_STATUS_DATA_INVALID  0x40
#define WH4530_STATUS_PS_READY      0x02

bool WH4530_Reset(void);
bool WH4530_Init(void);
bool WH4530_Enable(bool enable);
bool WH4530_Set_Gain(uint8_t gain);
bool WH4530_Clear_Status(void);
bool WH4530_Read_Status(uint8_t *status);
bool WH4530_Read_Distance(uint16_t *distance, uint8_t raw_data[2]);

#endif
