#include "driver_ADXL345.h"
#include "acc_motion_config.h"
#include "uart.h"
#include "rf.h"

/**
 * @brief  ADXL345写寄存器
 * @param  resis 寄存器地址
 * @param  val 要写的值
 * @retval None
 */
static bool ADXL345_write_reg(uint8_t resis, uint8_t val)
{
    return i2c_write(ADXL345_ADDRESS, resis, &val, 1) == OK;
}

/**
 * @brief  ADXL345读寄存器
 * @param  resis 寄存器地址
 * @retval 读取到的寄存器值
 */
static bool ADXL345_read_reg(uint8_t resis, uint8_t *data)
{
    return i2c_read(ADXL345_ADDRESS, resis, data, 1) == OK;
}

bool ADXL345_Get_ID(void)
{
	uint8_t id = 0;
	ADXL345_read_reg(ADXL345_REG_DEVID, &id);
	uart_printf("ADXL345 ID = 0x%x\r\n",id);
	if (id != 0xE5)
		return false;
	return true;
}

void ADXL345_Init(void)
{
	// 低电平中断输出,13位全分辨率,输出数据右对齐,16g量程
	ADXL345_write_reg(ADXL345_REG_DATA_FORMAT, 0x08 | ADXL345_RANGE_16_G);
	// 数据输出速度为100Hz
	ADXL345_write_reg(ADXL345_REG_BW_RATE, ADXL345_DATARATE_100_HZ);
	// 链接使能,测量模式,省电特性
	//ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x08);
	// 不使用中断
	ADXL345_write_reg(ADXL345_REG_INT_ENABLE, 0x00);
	ADXL345_write_reg(ADXL345_REG_OFSX, 0x00);
	ADXL345_write_reg(ADXL345_REG_OFSY, 0x00);
	ADXL345_write_reg(ADXL345_REG_OFSZ, 0x05);
}

void ADXL345_Enable(int flag)
{
	if(flag == true)
	{
		// 链接使能,测量模式,省电特性
        ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x08);
	}else{
		ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x00);
	}
}

/**
 * @brief  ADXL345读取数据
 * @param  None
 * @retval None
 */
bool ADXL345_read_data(short *x, short *y, short *z)
{
    uint8_t buf[6] = {0};

	if (x == 0 || y == 0 || z == 0 ||
		i2c_read(ADXL345_ADDRESS, ADXL345_REG_DATAX0, buf, 6) != OK) {
		return false;
	}
 
    *x = (short)(((uint16_t)buf[1] << 8) + buf[0]); // 合成数据
    *y = (short)(((uint16_t)buf[3] << 8) + buf[2]);
    *z = (short)(((uint16_t)buf[5] << 8) + buf[4]);
	return true;
} 

bool ADXL345_LowRate_Start(uint8_t activity_threshold,
							   uint8_t activity_control)
{
	uint8_t id = 0;
	uint8_t source;

	if (activity_threshold == 0 ||
		!ADXL345_read_reg(ADXL345_REG_DEVID, &id) || id != 0xE5) {
		return false;
	}
	/*
	 * Sleep模式停止FIFO和DATA_READY，但DATAX/Y/Z仍按WAKEUP=1Hz更新；
	 * Activity是Sleep模式下唯一有效的运动检测功能。
	 */
	if (!ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x00) ||
		!ADXL345_write_reg(ADXL345_REG_INT_ENABLE, 0x00) ||
		!ADXL345_write_reg(ADXL345_REG_FIFO_CTL, ADXL345_FIFO_BYPASS) ||
		!ADXL345_write_reg(ADXL345_REG_DATA_FORMAT,
			0x08 | ADXL345_RANGE_16_G) ||
		!ADXL345_write_reg(ADXL345_REG_BW_RATE, ACC_HIGH_BW_RATE_REG) ||
		!ADXL345_write_reg(ADXL345_REG_THRESH_ACT, activity_threshold) ||
		!ADXL345_write_reg(ADXL345_REG_ACT_INACT_CTL, activity_control) ||
		/* INT_MAP对应位为0时映射到INT1。 */
		!ADXL345_write_reg(ADXL345_REG_INT_MAP, 0x00)) {
		return false;
	}
	ADXL345_read_reg(ADXL345_REG_INT_SOURCE, &source);
	/* 先进入1Hz但保持中断关闭，任务层保存保护期样本后再开放Activity。 */
	if (!ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x0F)) {
		return false;
	}
	return true;
}

bool ADXL345_LowRate_ArmActivity(void)
{
	uint8_t source;
	/* 读取INT_SOURCE清掉模式切换期间可能形成的Activity锁存。 */
	if (!ADXL345_read_reg(ADXL345_REG_INT_SOURCE, &source) ||
		!ADXL345_write_reg(ADXL345_REG_INT_ENABLE, ADXL345_INT_ACTIVITY)) {
		return false;
	}
	return true;
}

bool ADXL345_Stream_Start(uint8_t watermark, uint8_t inactivity_threshold,
							  uint8_t inactivity_time,
							  uint8_t inactivity_control)
{
	uint8_t id = 0;
	uint8_t verify = 0;

	if (watermark == 0 || watermark > 31 || inactivity_threshold == 0 ||
		inactivity_time == 0 || (inactivity_control & 0x07) == 0 ||
		!ADXL345_read_reg(ADXL345_REG_DEVID, &id) || id != 0xE5) {
		return false;
	}
	/* 配置中断输出前保持Standby，并先清空旧FIFO。 */
	if (!ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x00) ||
		!ADXL345_write_reg(ADXL345_REG_INT_ENABLE, 0x00) ||
		!ADXL345_write_reg(ADXL345_REG_FIFO_CTL, ADXL345_FIFO_BYPASS) ||
		!ADXL345_write_reg(ADXL345_REG_DATA_FORMAT, 0x08 | ADXL345_RANGE_16_G) ||
		/* 0x19：LOW_POWER=1，RATE=0x09，即50Hz输出。 */
		!ADXL345_write_reg(ADXL345_REG_BW_RATE, ACC_HIGH_BW_RATE_REG) ||
		!ADXL345_write_reg(ADXL345_REG_THRESH_INACT, inactivity_threshold) ||
		!ADXL345_write_reg(ADXL345_REG_TIME_INACT, inactivity_time) ||
		!ADXL345_write_reg(ADXL345_REG_ACT_INACT_CTL, inactivity_control) ||
		/* INT_MAP位为0时，Watermark和Inactivity都映射至INT1。 */
		!ADXL345_write_reg(ADXL345_REG_INT_MAP, 0x00) ||
		!ADXL345_write_reg(ADXL345_REG_FIFO_CTL,
			(uint8_t)(ADXL345_FIFO_STREAM | watermark))) {
		return false;
	}
	ADXL345_read_reg(ADXL345_REG_INT_SOURCE, &verify);
	if (!ADXL345_write_reg(ADXL345_REG_INT_ENABLE,
			(uint8_t)(ADXL345_INT_WATERMARK | ADXL345_INT_INACTIVITY)) ||
		!ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x08) ||
		!ADXL345_read_reg(ADXL345_REG_FIFO_CTL, &verify)) {
		return false;
	}
	return verify == (uint8_t)(ADXL345_FIFO_STREAM | watermark);
}

bool ADXL345_Stream_Freeze(void)
{
	/* Standby停止产生新样本，但不切换FIFO模式，已有条目仍可读出。 */
	bool disabled = ADXL345_write_reg(ADXL345_REG_INT_ENABLE, 0x00);
	bool stopped = ADXL345_write_reg(ADXL345_REG_POWER_CTL, 0x00);
	return disabled && stopped;
}

bool ADXL345_Int_Source_Checked(uint8_t *source)
{
	return source != 0 && ADXL345_read_reg(ADXL345_REG_INT_SOURCE, source);
}

bool ADXL345_Activity_Status_Checked(uint8_t *activity_status, uint8_t *source)
{
	if (activity_status == 0 || source == 0 ||
		!ADXL345_read_reg(ADXL345_REG_ACT_TAP_STATUS, activity_status)) {
		return false;
	}
	return ADXL345_Int_Source_Checked(source);
}

bool ADXL345_FIFO_Status(uint8_t *entries, uint8_t *source)
{
	uint8_t status;
	if (entries == 0 || source == 0 ||
		!ADXL345_Int_Source_Checked(source) ||
		!ADXL345_read_reg(ADXL345_REG_FIFO_STATUS, &status)) return false;
	*entries = status & ADXL345_FIFO_ENTRIES_MASK;
	return *entries <= 32;
}

bool ADXL345_FIFO_Entries_Checked(uint8_t *entries)
{
	uint8_t status;
	if (entries == 0 ||
		!ADXL345_read_reg(ADXL345_REG_FIFO_STATUS, &status)) return false;
	*entries = status & ADXL345_FIFO_ENTRIES_MASK;
	return *entries <= 32;
}

bool ADXL345_FIFO_Read_Checked(int16_t xyz[][3], uint8_t limit, uint8_t *read_count)
{
	uint8_t status, count;
	if (read_count == 0) return false;
	*read_count = 0;
	if (xyz == 0 || !ADXL345_read_reg(ADXL345_REG_FIFO_STATUS, &status)) return false;
	count = status & ADXL345_FIFO_ENTRIES_MASK;
	if (count > 32) return false;
	if (count > limit) count = limit;
	while (*read_count < count) {
		if (!ADXL345_read_data(&xyz[*read_count][0], &xyz[*read_count][1],
			&xyz[*read_count][2])) return false;
		(*read_count)++;
	}
	return true;
}

void ADXL345_Stream_Stop(void)
{
	ADXL345_Stream_Freeze();
	ADXL345_write_reg(ADXL345_REG_FIFO_CTL, ADXL345_FIFO_BYPASS);
}

uint8_t ADXL345_FIFO_Entries(void)
{
	uint8_t status = 0;
	if (!ADXL345_read_reg(ADXL345_REG_FIFO_STATUS, &status)) {
		return 0;
	}
	return status & ADXL345_FIFO_ENTRIES_MASK;
}

uint8_t ADXL345_FIFO_Read(int16_t xyz[][3], uint8_t max_samples)
{
	uint8_t count;
	uint8_t i;
	if (xyz == 0 || max_samples == 0) {
		return 0;
	}
	count = ADXL345_FIFO_Entries();
	if (count > 32) {
		count = 32;
	}
	if (count > max_samples) {
		count = max_samples;
	}
	/* I2C 100kHz本身已满足官方要求的相邻FIFO读取间隔。 */
	for (i = 0; i < count; i++) {
		if (!ADXL345_read_data(&xyz[i][0], &xyz[i][1], &xyz[i][2])) {
			break;
		}
	}
	return i;
}

uint8_t ADXL345_Read_Int_Source(void)
{
	uint8_t source = 0;
	ADXL345_read_reg(ADXL345_REG_INT_SOURCE, &source);
	return source;
}
