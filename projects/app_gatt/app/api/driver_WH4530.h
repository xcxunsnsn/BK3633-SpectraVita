#ifndef _driver_WH4530_H_
#define _driver_WH4530_H_

#include <stdint.h>
#include <stdio.h>
#include "i2c.h"


#define WH4530_ADDRESS       (0X38)


void WH4530_Init(void);
void WH4530_Enable(int flag);
void WH4530_Set_Gain(uint8_t gain);
uint16_t WH4530A_IIC_Read_Distance(void);

#endif
