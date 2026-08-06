#include "driver_AS7331.h"
#include "uart.h"
#include "rf.h"

static float uva_lsb,uvb_lsb,uvc_lsb;
static uint16_t user_temp,user_uva,user_uvb,user_uvc;
static uint8_t as7331_i2c_address = AS7331_I2CADDR_MIN;
static bool as7331_detected = false;

static void AS7331_Delay(void)
{
	volatile uint32_t count;

	for (count = 0; count < 50000; count++)
	{
		__nop();
	}
}

/***************************************************************************************************************
AS7331 Read Command
****************************************************************************************************************/
void AS7331_ReadCommand(uint8_t reg_addr, uint8_t* rev_data, uint8_t length)
{
	i2c_read(as7331_i2c_address, reg_addr, rev_data, length);
}

/***************************************************************************************************************
AS7331 Write Command
****************************************************************************************************************/
void AS7331_WriteCommand(uint8_t reg_addr, uint8_t* send_data, uint16_t length)
{
	i2c_write(as7331_i2c_address, reg_addr, send_data, length);
}

/***************************************************************************************************************
AS7331 Get ID
****************************************************************************************************************/
bool AS7331_Get_ID(void)
{
	uint8_t address;
	uint8_t id;
	uint8_t reset_command = 0x0A;

	/*
	 * Register 0x02 is AGEN only on the configuration register page.  A
	 * warm MCU reset can leave AS7331 on the measurement page, where 0x02
	 * is MRES1 and often reads 0x00.  Reset every possible address before
	 * checking AGEN so identification is independent of the previous app.
	 */
	for (address = AS7331_I2CADDR_MIN; address <= AS7331_I2CADDR_MAX; address++)
	{
		as7331_i2c_address = address;
		i2c_write(address, AS7331_REG_OSR, &reset_command, 1);
		AS7331_Delay();
		id = 0xFF;
		i2c_read(address, AS7331_REG_AGEN, &id, 1);
		uart_printf("AS7331 scan addr=0x%02x AGEN=0x%02x\r\n", address, id);

		if (id == 0x21)
		{
			as7331_i2c_address = address;
			as7331_detected = true;
			uart_printf("AS7331 check success, addr=0x%02x\r\n", address);
			return true;
		}
	}

	as7331_detected = false;
	as7331_i2c_address = AS7331_I2CADDR_MIN;
	uart_printf("AS7331 check failed\r\n");
	return false;
}

/***************************************************************************************************************
AS7331 Set measurement state.
 value:
 AS7331_MEASUREMENT_START
 AS7331_MEASUREMENT_STOP
****************************************************************************************************************/
void AS7331_Set_Measurement(uint8_t status)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_OSR, &config, 1);
	if ((config & 0x07) != AS7331_STATE_MEASUREMENT)
	{
		config &= 0x78;
		config |= AS7331_STATE_MEASUREMENT;
	}
	config |= status;
	AS7331_WriteCommand(AS7331_REG_OSR, &config, 1);
}

/***************************************************************************************************************
AS7331 Power Down state switched
 value:
 AS7331_PD_OFF disable power down mode
 AS7331_PD_ON  enable power down mode
****************************************************************************************************************/
void AS7331_Set_Power_Down(uint8_t status)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_OSR, &config, 1);
	config &= 0xbf;
	config |= status;
	AS7331_WriteCommand(AS7331_REG_OSR, &config, 1);
}

/***************************************************************************************************************
AS7331 Software reset
****************************************************************************************************************/
void AS7331_Reset(void)
{
	/* Datasheet reset command: SW_RES=1 and DOS=CONFIGURATION (0x0A). */
	uint8_t config = 0x0A;
	AS7331_WriteCommand(AS7331_REG_OSR, &config, 1);
	AS7331_Delay();
}

/***************************************************************************************************************
AS7331 Set Device operational state.
 value:
 AS7331_STATE_CONFIGURATION CONFIGURATION
 AS7331_STATE_MEASUREMENT MEASUREMENT
****************************************************************************************************************/
void AS7331_Set_Device_State(uint8_t status)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_OSR, &config, 1);
	config &= 0xf8;
	config |= status;
	AS7331_WriteCommand(AS7331_REG_OSR, &config, 1);
}

/***************************************************************************************************************
AS7331 Set GAIN.
 value:
 AS7331_GAIN_xX
****************************************************************************************************************/
void AS7331_Set_GAIN(uint8_t value)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG1, &config, 1);
	config &= 0x0f;
	config |= value;
	AS7331_WriteCommand(AS7331_REG_CREG1, &config, 1);
}

/***************************************************************************************************************
AS7331 Set defines the integration time of the measurement.
 value:
 AS7331_TIME_xMS
****************************************************************************************************************/
void AS7331_Set_Integration_Time(uint8_t value)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG1, &config, 1);
	config &= 0xf0;
	config |= value;
	AS7331_WriteCommand(AS7331_REG_CREG1, &config, 1);
}

/***************************************************************************************************************
AS7331 Set temperature status.
 value:
 AS7331_TEMPERATURE_ENABLE
 AS7331_TEMPERATURE_DISABLE
****************************************************************************************************************/
void AS7331_Set_Temperature(uint8_t value)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG2, &config, 1);
	config &= 0xbf;
	config |= (value);
	AS7331_WriteCommand(AS7331_REG_CREG2, &config, 1);
}

/***************************************************************************************************************
AS7331 Set Digital divider of the measurement result registers state
 status:
 AS7331_EN_DIV_ENABLE
 AS7331_EN_DIV_DISABLE
****************************************************************************************************************/
void AS7331_Set_Digital_Divider(uint8_t status, uint8_t div)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG2, &config, 1);
	config &= 0xf0;
	config |= (status | div);
	AS7331_WriteCommand(AS7331_REG_CREG2, &config, 1);
}

/***************************************************************************************************************
AS7331 Set mode
 status:
 AS7331_MMODE_CONT(continuous measurement).
 AS7331_MMODE_CMD(measurement per command).
 AS7331_MMODE_SYNS(externally synchronized start of measurement).
 AS7331_MMODE_SYND(start and end of measurement are externally synchronized).
****************************************************************************************************************/
void AS7331_Set_Mode(uint8_t value)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG3, &config, 1);
	config &= 0x3f;
	config |= value;
	AS7331_WriteCommand(AS7331_REG_CREG3, &config, 1);
}

/***************************************************************************************************************
AS7331 Set standby switched
 value:
 AS7331_SB_OFF disable standby mode
 AS7331_SB_ON  enable standby mode
****************************************************************************************************************/
void AS7331_Set_Standby(uint8_t status)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG3, &config, 1);
	config &= 0xef;
	config |= status;
	AS7331_WriteCommand(AS7331_REG_CREG3, &config, 1);
}

/***************************************************************************************************************
AS7331 Set READY
 value:
 AS7331_RDYOD_PP
 AS7331_RDYOD_OD
****************************************************************************************************************/
void AS7331_Set_READY(uint8_t status)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG3, &config, 1);
	config &= 0xf7;
	config |= status;
	AS7331_WriteCommand(AS7331_REG_CREG3, &config, 1);
}

/***************************************************************************************************************
AS7331 Set CCLK
 value:
 AS7331_CCLK_1024(KHz)
 AS7331_CCLK_2048(KHz)
 AS7331_CCLK_4096(KHz)
 AS7331_CCLK_8192(KHz)
****************************************************************************************************************/
void AS7331_Set_CCLK(uint8_t value)
{
	uint8_t config = 0;
	AS7331_ReadCommand(AS7331_REG_CREG3, &config, 1);
	config &= 0xfc;
	config |= value;
	AS7331_WriteCommand(AS7331_REG_CREG3, &config, 1);
}

/***************************************************************************************************************
AS7331 Set Device operational state.
 Break time TBREAK between two measurements (except CMD mode): from 0 to 2040 祍, step size 8 祍.
 The value 0h results in a minimum time of 3 clocks of fCLK.
 value: 0 ~ 255
****************************************************************************************************************/
void AS7331_Set_BREAK(uint8_t value)
{
	AS7331_WriteCommand(AS7331_REG_BREAK, &value, 1);
}

/***************************************************************************************************************
AS7331 Set register EDGES becomes operative in SYND mode.
The value EDGES = ??is not allowed and results in the initial value ??
 value:
 disable:0
 enable:1-255
****************************************************************************************************************/
void AS7331_Set_SYND(uint8_t value)
{
	AS7331_WriteCommand(AS7331_REG_EDGES, &value, 1);
}

/***************************************************************************************************************
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 Note that this algorithm is inverted by RTrobot according to the manual, if you need to be more accurate,
 please refer to the manual for other methods, if you have a better algorithm, please provide it to us. --RTrobot
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
****************************************************************************************************************/
float AS7331_Get_LSB(uint8_t uv_channel, uint16_t gain, uint16_t time, uint16_t cclk)
{
	if (uv_channel == AS7331_CHANNEL_A)
	{
		return (1 << (gain >> 4)) * 170.0 / ((1 << time) * (cclk + 1) * 1024.0);
	}
	else if (uv_channel == AS7331_CHANNEL_B)
	{
		return (1 << (gain >> 4)) * 189.0 / ((1 << time) * (cclk + 1) * 1024.0);
	}
	else if (uv_channel == AS7331_CHANNEL_C)
	{
		return (1 << (gain >> 4)) * 83.0 / ((1 << time) * (cclk + 1) * 1024.0);
	}
	else
		return 0;
}

/***************************************************************************************************************
AS7331 get status
****************************************************************************************************************/
uint16_t AS7331_Get_Status(void)
{
	uint8_t rev_data[2] = { 0 };
	AS7331_ReadCommand(AS7331_REG_STATUS, rev_data, 2);
	/* In measurement state a two-byte read at 0x00 returns OSR then STATUS. */
	return rev_data[1];
}

/***************************************************************************************************************
AS7331 get temperature
****************************************************************************************************************/
float AS7331_Get_Temperature(void)
{
	uint8_t rev_data[2] = { 0 };
	AS7331_ReadCommand(AS7331_REG_TEMP, rev_data, 2);
	return ((uint16_t)(((uint16_t)rev_data[1]) << 8 | rev_data[0])) * 0.05f - 66.9f;
}

/***************************************************************************************************************
AS7331 Measurement Result A-Channel
****************************************************************************************************************/
uint16_t AS7331_Get_UVA(void)
{
	uint8_t rev_data[2] = { 0 };
	AS7331_ReadCommand(AS7331_REG_MRES1, rev_data, 2);
	return (uint16_t)(((uint16_t)rev_data[1]) << 8 | rev_data[0]);
}

/***************************************************************************************************************
AS7331 Measurement Result B-Channel
****************************************************************************************************************/
uint16_t AS7331_Get_UVB(void)
{
	uint8_t rev_data[2] = { 0 };
	AS7331_ReadCommand(AS7331_REG_MRES2, rev_data, 2);
	return (uint16_t)(((uint16_t)rev_data[1]) << 8 | rev_data[0]);
}

/***************************************************************************************************************
AS7331 Measurement Result C-Channel
****************************************************************************************************************/
uint16_t AS7331_Get_UVC(void)
{
	uint8_t rev_data[2] = { 0 };
	AS7331_ReadCommand(AS7331_REG_MRES3, rev_data, 2);
	return (uint16_t)(((uint16_t)rev_data[1]) << 8 | rev_data[0]);
}

/***************************************************************************************************************
AS7331 Time reference, result of conversion time measurement (least significant byte and middle byte).
****************************************************************************************************************/
uint16_t AS7331_Get_OUTCONVL(void)
{
	uint8_t rev_data[2] = { 0 };
	AS7331_ReadCommand(AS7331_REG_OUTCONVL, rev_data, 2);
	return (uint16_t)(((uint16_t)rev_data[1]) << 8 | rev_data[0]);
}

/***************************************************************************************************************
AS7331 Time reference, result of conversion time measurement (most significant byte and one empty byte with 00h).
****************************************************************************************************************/
uint16_t AS7331_Get_OUTCONVH(void)
{
	uint8_t rev_data[2] = { 0 };
	AS7331_ReadCommand(AS7331_REG_OUTCONVH, rev_data, 2);
	return (uint16_t)(((uint16_t)rev_data[1]) << 8 | rev_data[0]);
}

/***************************************************************************************************************
AS7331 get all result and temperature
****************************************************************************************************************/
void AS7331_Get_All(uint16_t* temp, uint16_t* uva, uint16_t* uvb, uint16_t* uvc)
{
	uint8_t rev_data[8] = { 0 };
	AS7331_ReadCommand(AS7331_REG_TEMP, rev_data, 8);
	*temp = ((uint16_t)(((uint16_t)rev_data[1]) << 8 | rev_data[0])) * 0.5f - 669;
	*uva = (uint16_t)(((uint16_t)rev_data[3]) << 8 | rev_data[2]);
	*uvb = (uint16_t)(((uint16_t)rev_data[5]) << 8 | rev_data[4]);
	*uvc = (uint16_t)(((uint16_t)rev_data[7]) << 8 | rev_data[6]);
}


bool AS7331_Init(void)
{
	if ((!as7331_detected) && (!AS7331_Get_ID()))
	{
		return false;
	}

	AS7331_Reset();
	
	return true;
}

void AS7331_StartWithGain(uint8_t gain_index)
{
	static const uint8_t gain_register[12] = {
		AS7331_GAIN_1X, AS7331_GAIN_2X, AS7331_GAIN_4X,
		AS7331_GAIN_8X, AS7331_GAIN_16X, AS7331_GAIN_32X,
		AS7331_GAIN_64X, AS7331_GAIN_128X, AS7331_GAIN_256X,
		AS7331_GAIN_512X, AS7331_GAIN_1024X, AS7331_GAIN_2048X
	};
	uint8_t gain;

	if (gain_index > 11) {
		gain_index = 9;
	}
	gain = gain_register[gain_index];
	/***
	 * Configure modules
	 ***/
	//start configure mode
	AS7331_Set_Device_State(AS7331_STATE_CONFIGURATION);
	//continuous measurement
	AS7331_Set_Mode(AS7331_MMODE_CONT);
	//set frequency to 1.024MHz
	AS7331_Set_CCLK(AS7331_CCLK_1024);
	//if you want to use RDY port ,set this one.
	//AS7331_Set_READY(AS7331_RDYOD_PP);
	//set gain
	AS7331_Set_GAIN(gain);
	//set integration time
	AS7331_Set_Integration_Time(AS7331_TIME_64MS);
	//set break time to 200 us
	AS7331_Set_BREAK(25);
	uva_lsb = AS7331_Get_LSB(AS7331_CHANNEL_A, gain, AS7331_TIME_64MS, AS7331_CCLK_1024);
	uvb_lsb = AS7331_Get_LSB(AS7331_CHANNEL_B, gain, AS7331_TIME_64MS, AS7331_CCLK_1024);
	uvc_lsb = AS7331_Get_LSB(AS7331_CHANNEL_C, gain, AS7331_TIME_64MS, AS7331_CCLK_1024);
	//enable Measurement state
	AS7331_Set_Device_State(AS7331_STATE_MEASUREMENT);
	//start Measurement
	AS7331_Set_Measurement(AS7331_MEASUREMENT_START);   
}

bool AS7331_GetData(uint16_t *p_temp,uint16_t *p_uva,uint16_t *p_uvb,uint16_t *p_uvc)
{
    uint16_t status = AS7331_Get_Status();
	if ((status & 0x0008) == 0)
	{
		return false;
	}
	else
	{
		uint16_t uva = 0, uvb = 0, uvc = 0;
		uint16_t temp = 0.0;
		AS7331_Get_All(&temp, &uva, &uvb, &uvc);
		user_temp = temp;
		user_uva = (int)(uva * uva_lsb);
		user_uvb = (int)(uvb * uvb_lsb);
		user_uvc = (int)(uvc * uvc_lsb);
	}
	*p_temp = user_temp;
	*p_uva = user_uva;
	*p_uvb = user_uvb;
	*p_uvc = user_uvc;
	return true;
}

void AS7331_Start(void)
{
	AS7331_StartWithGain(9);
}
