#include "driver_WH4530.h"
#include "uart.h"
#include "rf.h"

static void WH4530A_IIC_Write_Register(uint8_t resis,uint8_t data)
{
    i2c_write(WH4530_ADDRESS, resis, &data, 1);
}

void WH4530_Init(void)
{
    WH4530A_IIC_Write_Register(0x00, 0x00);  //模式控制，复位功能，0x80 = 1000 0000，启动先复位
    WH4530A_IIC_Write_Register(0x01, 0x00);   //中断控制
    WH4530A_IIC_Write_Register(0x02, 0x00);   //中断标志位
    WH4530A_IIC_Write_Register(0x03, 0x00);  //测试等待时间，每个时间单位为5mS，0x00即为一个时间单位

    WH4530A_IIC_Write_Register(0x06, 0x17);   //LED发射控制，包括电流和脉冲宽度，高2位为电流 0x00：50 mA  0x01：100 mA  0x02：150 mA 0x03：200 mA
    WH4530A_IIC_Write_Register(0x07, 0x00);    //PS增益控制， 0x00: x1	0x01: x2  0x02: x4	0x03: x8
    WH4530A_IIC_Write_Register(0x08, 0x05);	 //PS脉冲个数， 0x01即为2个脉冲
    WH4530A_IIC_Write_Register(0x09, 0x07);    //PS输出数据的AD转换时间和分辨率，以256为一个时间单位，共16位
    //WH4530A_IIC_Write_Register(PS_FLITER, 0x0F);  
    WH4530A_IIC_Write_Register(0x0B, 0x11);    //中断事件设置，高4位为的PS，低4位为ALS，各16位


    WH4530A_IIC_Write_Register(0x14, 0x0);
    WH4530A_IIC_Write_Register(0x15, 0x0);

    //WH4530A_IIC_Write_Register(0x00, 0x02);   //模式控制，0x02为开启PS功能 
}

void WH4530_Enable(int flag)
{
	if(flag == true)
	{
		WH4530A_IIC_Write_Register(0x00, 0x02);   //模式控制，0x02为开启PS功能 
	}else{
		WH4530A_IIC_Write_Register(0x00, 0x00);
	}
}

void WH4530_Set_Gain(uint8_t gain)
{
	WH4530A_IIC_Write_Register(0x07, gain);    //PS增益控制， 0x00: x1	0x01: x2  0x02: x4	0x03: x8
}

uint16_t WH4530A_IIC_Read_Distance(void)
{
    uint16_t ret = 0;
    uint8_t data[2];

    i2c_read(WH4530_ADDRESS, 0x18, data, 2);
    ret = data[0] + (data[1]<<8);
    
    return ret;
}
  

