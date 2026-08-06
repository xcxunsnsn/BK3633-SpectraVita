#include <string.h>             // for memcpy
#include <stdint.h> 
#include "rf.h"                 // RF interface        
#include "uart2.h"        
#include "bk3633_reglist.h"
#include "user_config.h"

void CLK32K_AutoCali_init(void);
volatile uint32_t XVR_ANALOG_REG_BAK[32] = {0};

void pll_clk_calibration(void)
{
    XVR_ANALOG_REG_BAK[0x1e]  |= 0x20000;
    addXVR_Reg0x1e = XVR_ANALOG_REG_BAK[0x1e];
    Delay_ms(2);
    XVR_ANALOG_REG_BAK[0x1e]  &= ~0x20000;
    addXVR_Reg0x1e = XVR_ANALOG_REG_BAK[0x1e];
}

void xvr_reg_initial_24(void)
{

    XVR_ANALOG_REG_BAK[2] &= ~(1<<27);
    XVR_ANALOG_REG_BAK[2] |= (1<<14);
    addXVR_Reg0x2 = XVR_ANALOG_REG_BAK[2];

    addXVR_Reg0x2c = 0x10004040;//0x106A4C71;
    addXVR_Reg0x2d = 0x082ac441;//0x082CC446;// REG_2D 0x082CC444

    addXVR_Reg0x3a |= (1<<14);// REG_3A
    addXVR_Reg0x3b &= ~(3<<2);// REG_3B 0x22341048
}
void xvr_reg_initial(void) 
{
    addXVR_Reg0x0 = 0xC4B0323F  ;
    XVR_ANALOG_REG_BAK[0] = 0xC4B0323F;
    addXVR_Reg0x1 = 0x8295C200  ;
    XVR_ANALOG_REG_BAK[1] = 0x8295C200;
    addXVR_Reg0x2 = 0x2742E000  ;
    XVR_ANALOG_REG_BAK[2] = 0x2742E000;
    addXVR_Reg0x3 = 0x60035C62  ;
    XVR_ANALOG_REG_BAK[3] = 0x60035C62;
    addXVR_Reg0x4 = 0xFF76AACF  ;
    XVR_ANALOG_REG_BAK[4] = 0xFF76AACF;//0xFFD6BBCC
    addXVR_Reg0x5 = 0x4620501F  ;
    XVR_ANALOG_REG_BAK[5] = 0x4620501F; //0x4620501F 03.31 // 0x4420501F 04.01
    #if(LDO_MODE)
    addXVR_Reg0x6 = 0x8487CC00;//0x80B7CE20  ;
    XVR_ANALOG_REG_BAK[6] = 0x8487CC00;//0x80B7CE20;
    #else
    if(get_sys_mode() == DUT_FCC_MODE)    // dut is in LDO mode
    {
        XVR_ANALOG_REG_BAK[6] = 0x8487CC00;//0x80B7CE20;
    }
    else
    {
        XVR_ANALOG_REG_BAK[6] = 0x84A7CC00;//0x8097CE20  ;
    }
    addXVR_Reg0x6 = XVR_ANALOG_REG_BAK[6];//0x80B7CE20  ;
    #endif
    addXVR_Reg0x7 = 0xAA023FC0;//0xAA023DC0  ;
    XVR_ANALOG_REG_BAK[7] = 0xAA023FC0;//0xAA023DC0;
    addXVR_Reg0x8 = 0x0FB0C02F  ;
    XVR_ANALOG_REG_BAK[8] = 0x0FB0C02F;

    addXVR_Reg0x9 = 0x7080224C;////0x7080220C  ;
    XVR_ANALOG_REG_BAK[9] = 0x7080224C;

    #if(LDO_MODE)
    addXVR_Reg0xa = 0x9C03F86B;//0x9C27785B  ;
    XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86B;//0x9C27785B;
    #else

    if(get_sys_mode() == DUT_FCC_MODE)    // dut is in LDO mode
    {
        XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86B;//0x9C27785B;
    }
    else
    {
        XVR_ANALOG_REG_BAK[0xa] = 0x9C03F86F;//0x9C27785B  ;
    }

    addXVR_Reg0xa = XVR_ANALOG_REG_BAK[0xa];//0x9C27785B  ;
    #endif
    addXVR_Reg0xb = 0x0FD93F23  ;
    XVR_ANALOG_REG_BAK[0xb] = 0x0FD93F23;
    addXVR_Reg0xc = 0x80001008  ;
    XVR_ANALOG_REG_BAK[0xc] = 0x80001008;
    addXVR_Reg0xd = 0xCC42BF23  ;
    XVR_ANALOG_REG_BAK[0xd] = 0xCC42BF23;
    addXVR_Reg0xe = 0x00309350  ;
    XVR_ANALOG_REG_BAK[0xe] = 0x00309350;
    addXVR_Reg0xf = 0x3126E978  ;
    XVR_ANALOG_REG_BAK[0xf] = 0x3126E978;
 

    addXVR_Reg0x1c = 0x999CDDC5  ;
    XVR_ANALOG_REG_BAK[0x1c] = 0x999CDDC5;
    addXVR_Reg0x1d = 0xEA8501C0  ;
    XVR_ANALOG_REG_BAK[0x1d] = 0xEA8501C0;
    addXVR_Reg0x1e = 0x80010180  ;
    XVR_ANALOG_REG_BAK[0x1e] = 0x80010180;
    addXVR_Reg0x1f = 0x00000000  ;
    XVR_ANALOG_REG_BAK[0x1f] = 0x00000000;

    xtal_cal_set(0x35);

    addXVR_Reg0x20 = 0x8E89BED6;// REG_20
    addXVR_Reg0x21 = 0x96000000;//0x96000000;// REG_21
    addXVR_Reg0x22 = 0x78000000;// REG_22
    addXVR_Reg0x23 = 0xA0000000;// REG_23
    addXVR_Reg0x24 = 0x000A0382;//0x000A0782;// REG_24
    addXVR_Reg0x25 = 0X00200000;// REG_25
    addXVR_Reg0x26 = 0x10200502;// REG_26 0x10200502 0x14a40505
    addXVR_Reg0x27 = 0x0008C900;// REG_27
    addXVR_Reg0x28 = 0x01011010;// REG_28
    addXVR_Reg0x29 = 0x3C104E00;// REG_29
    addXVR_Reg0x2a = 0x0e103830;//0x0e10404d;//0x0e103D68;// REG_2A
    addXVR_Reg0x2b = 0x00000408;// REG_2B
    //addXVR_Reg0x2c = 0x006A404d;// REG_2C   //0x006a404d
    addXVR_Reg0x2d = 0x082CC446;// REG_2D 0x082CC444
    addXVR_Reg0x2e = 0x00000100;//0x00000000;// REG_2E
    addXVR_Reg0x2f = 0X00000000;// REG_2F

    addXVR_Reg0x30 = 0x10010011;// REG_30//0x10010001
    addXVR_Reg0x31 = 0X00000000;// REG_31
    addXVR_Reg0x32 = 0X00000000;// REG_32
    addXVR_Reg0x33 = 0X00000000;// REG_33
    addXVR_Reg0x34 = 0X00000000;// REG_34
    addXVR_Reg0x35 = 0X00000000;// REG_35
    addXVR_Reg0x36 = 0X00000000;// REG_36
    addXVR_Reg0x37 = 0X00000000;// REG_37
    addXVR_Reg0x38 = 0X00000000;// REG_38
    addXVR_Reg0x39 = 0X00000000;// REG_39
    addXVR_Reg0x3a = 0x0012C000;// REG_3A 0x00128000
    addXVR_Reg0x3b = 0x36341040;// REG_3B 0x36341048
    addXVR_Reg0x3c = 0x01FF1c80;// REG_3C
    addXVR_Reg0x3d = 0x00000000;// REG_3D
    addXVR_Reg0x3e = 0X0000D940;// REG_3E
    addXVR_Reg0x3f = 0X00000000;// REG_3F

    addXVR_Reg0x40 = 0x01000000;// REG_40
    addXVR_Reg0x41 = 0x07050402;// REG_41
    addXVR_Reg0x42 = 0x120F0C0A;// REG_42
    addXVR_Reg0x43 = 0x221E1A16;// REG_43
    addXVR_Reg0x44 = 0x35302B26;// REG_44
    addXVR_Reg0x45 = 0x4B45403A;// REG_45
    addXVR_Reg0x46 = 0x635D5751;// REG_46
    addXVR_Reg0x47 = 0x7C767069;// REG_47
    addXVR_Reg0x48 = 0x968F8983;// REG_48
    addXVR_Reg0x49 = 0xAEA8A29C;// REG_49
    addXVR_Reg0x4a = 0xC5BFBAB4;// REG_4A
    addXVR_Reg0x4b = 0xD9D4CFCA;// REG_4B
    addXVR_Reg0x4c = 0xE9E5E1DD;// REG_4C
    addXVR_Reg0x4d = 0xF5F3F0ED;// REG_4D
    addXVR_Reg0x4e = 0xFDFBFAF8;// REG_4E
    addXVR_Reg0x4f = 0xFFFFFFFE;// REG_4F
    
    addPMU_Reg0x10 |= (0X1 << 8);
    addPMU_Reg0x12 &= ~(0X1 << 8);
    addPMU_Reg0x13 = 0XFFFFFF80;
    
    /*// open if
    addPMU_Reg0x10 |= (0X1 << 18)|(0X1 << 23);
    addPMU_Reg0x12 &= ~((0X1 << 18)|(0X1 << 23));

    XVR_ANALOG_REG_BAK[8] |= 1<<31;
    addXVR_Reg0x8 = XVR_ANALOG_REG_BAK[8]  ;

    XVR_ANALOG_REG_BAK[5] &= ~(1<<9);
    addXVR_Reg0x5 = XVR_ANALOG_REG_BAK[5]  ;
    */

    #ifdef __RF250_RF__
    RF250_calibration();
    #else
    kmod_calibration();
    #endif
    pll_clk_calibration();

    #if (INTER_RC32K)
    XVR_ANALOG_REG_BAK[9] &= ~(0x01 << 26);
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];

    XVR_ANALOG_REG_BAK[0x1e]  |= 0x80000000;
    addXVR_Reg0x1e = XVR_ANALOG_REG_BAK[0x1e];
    CLK32K_AutoCali_init();
    Delay_ms(50);
    #else
    XVR_ANALOG_REG_BAK[9] |= (0x01 << 26);
    addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
    addXVR_Reg0x1e = 0x00010180  ;
    XVR_ANALOG_REG_BAK[0x1e] = 0x00010180;
    #endif
}

void Delay_us(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 10; x++);
    }
}

void Delay(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 50; x++);
    }
}

void Delay_ms(int num) //sync from svn revision 18
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 3260; x++);
    }

}



void kmod_calibration(void) 
{
    /* 
    1���ڳ�ʼ��0X26�� [16:28] = 0x1084 
    2����0X30��BT ���ó� BT = 1ȥУ׼

    3��У׼��ɺ���0X30��BT ���ó� BT = 0.5
    */

    uint32_t value;
    uint32_t value_kcal_result;

    addXVR_Reg0x24 &= ~(0x1 << 17);
    Delay_ms(10);
    addXVR_Reg0x24 &= ~(0x7f);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<12);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<13);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<11);
    Delay_ms(10);
    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);
    XVR_ANALOG_REG_BAK[3] |= (0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<16);
    Delay_ms(50);
    value = addXVR_Reg0x12;

    value = ((value >> 16) & 0x1fff);

    value_kcal_result =  ((500*256/value)&0x1ff);// modify by shanghai   2021/03/04((256*250/value)&0x1ff) ; 
    addXVR_Reg0x30 &= (~(0x1ff<<8));
    addXVR_Reg0x30 |= (value_kcal_result<<8);
    Delay_ms(50);

    addXVR_Reg0x25 &= ~(1<<16);
    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);

    XVR_ANALOG_REG_BAK[3] |= (0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    addXVR_Reg0x25 &= ~(1<<11);

    addXVR_Reg0x25 &= ~(1<<13);
    addXVR_Reg0x25 &= ~(1<<12); 
    addXVR_Reg0x24 |= (0x1 << 17);
}

void RF250_calibration(void)
{
    uint32_t value,xvr_tmp;
    uint32_t value_kcal_result;
// XVR26
    xvr_tmp = addXVR_Reg0x26;

    addXVR_Reg0x26 &= ~(0xf<<4);
    Delay_ms(10);

    addXVR_Reg0x26 &= ~(0xf);
    Delay_ms(10);

    addXVR_Reg0x26 |= 2;
    Delay_ms(10);

    addXVR_Reg0x26 &= ~(0x1ff<<16);
    Delay_ms(10);

    addXVR_Reg0x26 |= (0x408<<16);
    Delay_ms(10);
// XVR 5
    XVR_ANALOG_REG_BAK[5] &= ~(0x0f<<24);
    XVR_ANALOG_REG_BAK[5] |= 6<<24;
    addXVR_Reg0x5 = XVR_ANALOG_REG_BAK[5];
    Delay_ms(10);
// XVR 2e
    addXVR_Reg0x2e = 0x100;
    Delay_ms(10);
// XVR 30
    addXVR_Reg0x30 &= ~(0x1ff<<8);
    Delay_ms(10);
    addXVR_Reg0x30 |= (0x100<<8);
    Delay_ms(10);

    addXVR_Reg0x30 &= ~(0x3);
    Delay_ms(10);

    addXVR_Reg0x30 |= (1<<3);
    Delay_ms(10);

// XVR 24
    addXVR_Reg0x24 |= (1<<31);
    Delay_ms(10);

    addXVR_Reg0x24 |= (1<<30);
    Delay_ms(10);

    addXVR_Reg0x24 &= ~(1<<17);
    Delay_ms(10);

    addXVR_Reg0x24 &= ~(0x7f);
    Delay_ms(10);

    addXVR_Reg0x24 |= 2;
    Delay_ms(10);

// XVR 25
    addXVR_Reg0x25 |= (1<<12);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<13);
    Delay_ms(10);
    addXVR_Reg0x25 |= (1<<11);
    Delay_ms(10);
// XVR 3
    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);

    XVR_ANALOG_REG_BAK[3] |= (0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
    Delay_ms(10);

// XVR 25
    addXVR_Reg0x25 |= (1<<16);
    Delay_ms(200);


    value = addXVR_Reg0x12;

    value = ((value >> 16) & 0x1fff);

    value_kcal_result =  ((256*125/value)&0x1ff) ;

// result
    addXVR_Reg0x30 &= (~(0x1ff<<8));
    addXVR_Reg0x30 |= (value_kcal_result<<8);

// XVR 25
    addXVR_Reg0x25 &= ~(1<<16);
// XVR 3

    XVR_ANALOG_REG_BAK[3] &= ~(0x1 << 7);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];

    XVR_ANALOG_REG_BAK[3] |= (0x1 << 6);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3];
// XVR 25
    addXVR_Reg0x25 &= ~(1<<11);
    addXVR_Reg0x25 &= ~(1<<13);
    addXVR_Reg0x25 &= ~(1<<12);
// XVR 24
    addXVR_Reg0x24 |= (0x1 << 17);
    addXVR_Reg0x24 &= ~(1<<30);
    addXVR_Reg0x24 &= ~(1<<31);
// XVR 30
    addXVR_Reg0x30 &= ~(1<<3);
    addXVR_Reg0x30 |= 1;

    addXVR_Reg0x5 = XVR_ANALOG_REG_BAK[5];
    xvr_tmp &= (0xff);
    addXVR_Reg0x26 &= ~(0xff);
    addXVR_Reg0x26 |= xvr_tmp;

}



void CLK32K_AutoCali_init(void)
{
    XVR_ANALOG_REG_BAK[0xc] &= ~(0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc]; 

    XVR_ANALOG_REG_BAK[0xc] |= (0x01 << 15);
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc];

    XVR_ANALOG_REG_BAK[0xc] |= (0x1388 << 16);
    XVR_ANALOG_REG_BAK[0xc] |= (0x1 << 14);    
    addXVR_Reg0xc = XVR_ANALOG_REG_BAK[0xc]; 
    addXVR_Reg0xc = 0x13881004; 
    Delay_ms(10);
    XVR_ANALOG_REG_BAK[0xc] = 0x1388d004;
    addXVR_Reg0xc = 0x1388d004;
}


//���õ��ز�����
//freq:Ƶ�����ã�˫Ƶ��(2-80)
//power:���ʵȼ�(0x1-0xf)
void singleWaveCfg(uint8_t freq, uint8_t power_level)
{
    uint32_t val = 0;
    uint32_t reg = XVR_ANALOG_REG_BAK[0x04];
    
    addXVR_Reg0x4 = reg | (0x1 << 29);
    
    val |= freq;
    val |= (power_level<< 7);
    addXVR_Reg0x24 = val;
    addXVR_Reg0x25 |= (0x1<<12) |(0x1<<13);

    while(1);
}


//�޸ķ��书��
//power_level:���ʵȼ�(0x0-0xf)
void set_power(uint8_t power_level)
{
    uint32_t val = 0;
    uint32_t reg = XVR_ANALOG_REG_BAK[0x04];

    addXVR_Reg0x24 &= ~(0x1 << 20);
    addXVR_Reg0x4 = reg | (0x1 << 29);
    val |= (power_level << 7);
    addXVR_Reg0x24 &= ~(0xf << 7);
    addXVR_Reg0x24 |= val;
}

///����Ƶƫ����
///cal_dataĬ��Ϊ0x35,���Ϊ0x7f
void xtal_cal_set(uint8_t cal_data)
{
    if(cal_data>0x7f)
        cal_data=0x7f;
    XVR_ANALOG_REG_BAK[3] = 0x60000C62|(cal_data<<12);
    addXVR_Reg0x3 = XVR_ANALOG_REG_BAK[3] ;
}







