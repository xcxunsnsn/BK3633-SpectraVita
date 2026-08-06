#include "driver_ADXL345.h"
#include "uart.h"
#include "rf.h"

/**
 * @brief  ADXL345写寄存器
 * @param  resis 寄存器地址
 * @param  val 要写的值
 * @retval None
 */
static void ADXL345_write_reg(uint8_t resis, uint8_t val)
{
    i2c_write(ADXL345_ADDRESS, resis, &val, 1);
}

/**
 * @brief  ADXL345读寄存器
 * @param  resis 寄存器地址
 * @retval 读取到的寄存器值
 */
static uint8_t ADXL345_read_reg(uint8_t resis)
{
    uint8_t data = 0;
    
    i2c_read(ADXL345_ADDRESS, resis, &data, 1);
    
    return data;
}

bool ADXL345_Get_ID(void)
{
	uint8_t id = 0;
	id = ADXL345_read_reg(ADXL345_REG_DEVID);
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
void ADXL345_read_data(short *x, short *y, short *z)
{
    uint8_t buf[6] = {0};
    
    i2c_read(ADXL345_ADDRESS, ADXL345_REG_DATAX0, buf, 6);
 
    *x = (short)(((uint16_t)buf[1] << 8) + buf[0]); // 合成数据
    *y = (short)(((uint16_t)buf[3] << 8) + buf[2]);
    *z = (short)(((uint16_t)buf[5] << 8) + buf[4]);
} 

