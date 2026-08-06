#ifndef _driver_AS7331_H_
#define _driver_AS7331_H_

#include <stdint.h>
#include <stdio.h>
#include "i2c.h"



#define AS7331_I2CADDR_MIN 0x74
#define AS7331_I2CADDR_MAX 0x77

#define AS7331_REG_OSR 0x00
#define AS7331_REG_AGEN 0x02
#define AS7331_REG_CREG1 0x06
#define AS7331_REG_CREG2 0x07
#define AS7331_REG_CREG3 0x08
#define AS7331_REG_BREAK 0x09
#define AS7331_REG_EDGES  0x0a
#define AS7331_REG_OPTREG 0x0b
#define AS7331_REG_STATUS 0x00
#define AS7331_REG_TEMP 0x01
#define AS7331_REG_MRES1 0x02
#define AS7331_REG_MRES2 0x03
#define AS7331_REG_MRES3 0x04
#define AS7331_REG_OUTCONVL 0x05
#define AS7331_REG_OUTCONVH 0x06

#define AS7331_PD_OFF    0x00
#define AS7331_PD_ON    0x40

#define AS7331_STATE_CONFIGURATION    0x02
#define AS7331_STATE_MEASUREMENT    0x03

#define AS7331_MEASUREMENT_STOP    0x00
#define AS7331_MEASUREMENT_START    0x80

#define AS7331_GAIN_1X        0xb0
#define AS7331_GAIN_2X        0xa0
#define AS7331_GAIN_4X        0x90
#define AS7331_GAIN_8X        0x80
#define AS7331_GAIN_16X        0x70
#define AS7331_GAIN_32X        0x60
#define AS7331_GAIN_64X        0x50
#define AS7331_GAIN_128X    0x40
#define AS7331_GAIN_256X    0x30
#define AS7331_GAIN_512X    0x20
#define AS7331_GAIN_1024X    0x10
#define AS7331_GAIN_2048X    0x00

#define AS7331_TIME_1MS    0x0
#define AS7331_TIME_2MS    0x1
#define AS7331_TIME_4MS    0x2
#define AS7331_TIME_8MS    0x3
#define AS7331_TIME_16MS 0x4
#define AS7331_TIME_32MS 0x5
#define AS7331_TIME_64MS 0x6
#define AS7331_TIME_128MS 0x7
#define AS7331_TIME_256MS 0x8
#define AS7331_TIME_512MS 0x9
#define AS7331_TIME_1024MS 0xa
#define AS7331_TIME_2048MS 0xb
#define AS7331_TIME_4096MS 0xc
#define AS7331_TIME_8192MS 0xd
#define AS7331_TIME_16384MS    0xe

#define AS7331_TEMPERATURE_ENABLE    0x40
#define AS7331_TEMPERATURE_DISBLE    0x00

#define AS7331_EN_DIV_ENABLE    0x08
#define AS7331_EN_DIV_DISBLE    0x00

#define AS7331_MMODE_CONT    0x00
#define AS7331_MMODE_CMD    0x40
#define AS7331_MMODE_SYNS    0x80
#define AS7331_MMODE_SYND    0xC0

#define AS7331_SB_OFF    0x00
#define AS7331_SB_ON    0x10

#define AS7331_RDYOD_PP    0x00
#define AS7331_RDYOD_OD    0x08

#define AS7331_CCLK_1024    0x00
#define AS7331_CCLK_2048    0x01
#define AS7331_CCLK_4096    0x02
#define AS7331_CCLK_8192    0x03

#define AS7331_CHANNEL_A    0x00
#define AS7331_CHANNEL_B    0x01
#define AS7331_CHANNEL_C    0x02

bool AS7331_Init(void);
bool AS7331_Get_ID(void);
void AS7331_Set_Measurement(uint8_t status);
void AS7331_Set_Power_Down(uint8_t status);
void AS7331_Set_Device_State(uint8_t status);
void AS7331_Reset(void);
void AS7331_Set_GAIN(uint8_t value);
void AS7331_Set_Integration_Time(uint8_t value);
void AS7331_Set_Temperature(uint8_t value);
void AS7331_Set_Digital_Divider(uint8_t status, uint8_t div);
void AS7331_Set_Mode(uint8_t value);
void AS7331_Set_Standby(uint8_t status);
void AS7331_Set_READY(uint8_t status);
void AS7331_Set_CCLK(uint8_t value);
void AS7331_Set_BREAK(uint8_t status);
void AS7331_Set_SYND(uint8_t value);
uint16_t AS7331_Get_Status(void);
float AS7331_Get_Temperature(void);
uint16_t AS7331_Get_UVA(void);
uint16_t AS7331_Get_UVB(void);
uint16_t AS7331_Get_UVC(void);
float AS7331_Get_LSB(uint8_t uv_channel, uint16_t gain, uint16_t time, uint16_t cclk);
uint16_t AS7331_Get_OUTCONVL(void);
uint16_t AS7331_Get_OUTCONVH(void);
void AS7331_Get_All(uint16_t* temp, uint16_t* uva, uint16_t* uvb, uint16_t* uvc);

void AS7331_Start(void);
void AS7331_StartWithGain(uint8_t gain_index);
bool AS7331_GetData(uint16_t *p_temp,uint16_t *p_uva,uint16_t *p_uvb,uint16_t *p_uvc);

#endif
