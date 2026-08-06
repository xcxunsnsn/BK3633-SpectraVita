#include "lcd360g.h"
#include "gpio.h"
#include "spi.h"
#include "uart2.h"
#include "user_config.h"
#include "BK3633_RegList.h"
#include "dma.h"


static void Delay(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 50; x++);
    }
}



unsigned char key_press_detect(void)
{
    static uint8_t keypress=0;
    static uint32_t keypresstime=0;
    uint8_t ret=0;
    //extern uint8_t  rmac_find;
    //extern uint8_t  lmac_find;

    if(keypress == 0)
    {
        if(gpio_get_input(GPIO_KEY)==0)
        {
            uart_printf("key press1\r\n");
            keypress=1;
            keypresstime=0;
        }
    }
    else
    {
        if(gpio_get_input(GPIO_KEY)==0)
        {
            keypresstime++;
            //uart_printf("key press2\r\n");
        }
        else
        {
            if((keypresstime > 2))
            {
                keypresstime = 0;
                keypress=0;
                ret = 1; //short press
                uart_printf("key press3\r\n");
            }
        }
    }
    return ret;
}

void lcd_gpio_init(void)
{
    gpio_config(GPIO_LCD_RS, OUTPUT, PULL_HIGH);
    gpio_config(GPIO_LCD_RESET, OUTPUT, PULL_HIGH);
    gpio_config(GPIO_KEY, INPUT, PULL_HIGH);
    gpio_config(GPIO_test, OUTPUT, PULL_HIGH);
    
}

void WriteComm(unsigned char dat)
{            
    gpio_set(GPIO_LCD_RS,0);
    spi_write(&dat,1);
}

void WriteData(unsigned char dat)
{
            
    gpio_set(GPIO_LCD_RS,1);
    spi_write(&dat,1);
}

void WriteDispData(unsigned char DataH,unsigned char DataL)
{     
     WriteData(DataH);
     WriteData(DataL);
}

void LCD_Init(void)
{

    uart_printf("LCD_Init \r\n");
    gpio_set(GPIO_LCD_RESET,1);
    Delay(5000);    
    gpio_set(GPIO_LCD_RESET,0);
    Delay(20000);
    gpio_set(GPIO_LCD_RESET,1);
    Delay(20000);
     
    //PASSWORD
    WriteComm(0xDF); //Password
    WriteData(0x58);
    WriteData(0x58);
    WriteData(0xB0);

    //---------------- PAGE0 --------------
    WriteComm(0xDE);      
    WriteData(0x00);  

    //VCOM_SET
    WriteComm(0xB2);
    WriteData(0x01);
    WriteData(0x10); //VCOM  

    //Gamma_Set
    WriteComm(0xB7); 
    WriteData(0x10); //VGMP = +5.3V 
    WriteData(0x4A); 
    WriteData(0x00); //VGSP = +0.0V
    WriteData(0x10); //VGMN = -5.3V 
    WriteData(0x4A); 
    WriteData(0x00); //VGSN = -0.0V
     
    //DCDC_SEL
    WriteComm(0xBB); 
    WriteData(0x01); //
    WriteData(0x1D); //
    WriteData(0x43);
    WriteData(0x43);
    WriteData(0x21);
    WriteData(0x21);

    //GATE_POWER
    WriteComm(0xCF); 
    WriteData(0x20); //VGHO = +8V
    WriteData(0x50); //VGLO = -8V


    //SET_R_GAMMA 
    WriteComm(0xC8);
    WriteData(0x7F);
    WriteData(0x52);
    WriteData(0x3B);
    WriteData(0x2A);
    WriteData(0x22);
    WriteData(0x12);
    WriteData(0x17);
    WriteData(0x04);
    WriteData(0x21);
    WriteData(0x26);
    WriteData(0x29);
    WriteData(0x4B);
    WriteData(0x3A);
    WriteData(0x45);
    WriteData(0x3A);
    WriteData(0x35);
    WriteData(0x2C);
    WriteData(0x1E);
    WriteData(0x01);
    WriteData(0x7F);
    WriteData(0x52);
    WriteData(0x3B);
    WriteData(0x2A);
    WriteData(0x22);
    WriteData(0x12);
    WriteData(0x17);
    WriteData(0x04);
    WriteData(0x21);
    WriteData(0x26);
    WriteData(0x29);
    WriteData(0x4B);
    WriteData(0x3A);
    WriteData(0x45);
    WriteData(0x3A);
    WriteData(0x35);
    WriteData(0x2C);
    WriteData(0x1E);
    WriteData(0x01);

    //-----------------------------
    // SET page4 TCON & GIP 
    //------------------------------ 
    WriteComm(0xDE);      
    WriteData(0x04);  // page4

    //SETSTBA
    WriteComm(0xB2);      
    WriteData(0x14); //
    WriteData(0x14);

    //SETRGBCYC1
    WriteComm(0xB8);      
    WriteData(0x74); //-NEQ    PEQ[1:0] -    RGB_INV_NP[2:0]    
    WriteData(0x44); //-RGB_INV_PI[2:0] -    RGB_INV_I[2:0]
    WriteData(0x00); //RGB_N_T2[11:8],RGB_N_T1[11:8] 
    WriteData(0x01); //RGB_N_T1[7:0], 
    WriteData(0x01); //RGB_N_T2[7:0], 
    WriteData(0x00); //RGB_N_T4[11:8],RGB_N_T3[11:8] 
    WriteData(0x01); //RGB_N_T3[7:0], 
    WriteData(0x01); //RGB_N_T4[7:0], 
    WriteData(0x00); //RGB_N_T6[11:8],RGB_N_T5[11:8] 
    WriteData(0x09); //RGB_N_T5[7:0], 
    WriteData(0x82); //RGB_N_T6[7:0], 
    WriteData(0x10); //RGB_N_T8[11:8],RGB_N_T7[11:8] 
    WriteData(0x8A); //RGB_N_T7[7:0], 
    WriteData(0x03); //RGB_N_T8[7:0], 
    WriteData(0x11); //RGB_N_T10[11:8],RGB_N_T9[11:8] 
    WriteData(0x0B); //RGB_N_T9[7:0], 
    WriteData(0x84); //RGB_N_T10[7:0], 
    WriteData(0x21); //RGB_N_T12[11:8],RGB_N_T11[11:8] 
    WriteData(0x8C); //RGB_N_T11[7:0], 
    WriteData(0x05); //RGB_N_T12[7:0], 
    WriteData(0x22); //RGB_N_T14[11:8],RGB_N_T13[11:8] 
    WriteData(0x0D); //RGB_N_T13[7:0], 
    WriteData(0x86); //RGB_N_T14[7:0], 
    WriteData(0x32); //RGB_N_T16[11:8],RGB_N_T15[11:8] 
    WriteData(0x8E); //RGB_N_T15[7:0], 
    WriteData(0x07); //RGB_N_T16[7:0], 
    WriteData(0x00); //RGB_N_T18[11:8],RGB_N_T17[11:8] 
    WriteData(0x00); //RGB_N_T17[7:0], 
    WriteData(0x00); //RGB_N_T18[7:0], 


    //SETRGBCYC2
    WriteComm(0xB9);      
    WriteData(0x40); //-,ENJDT,RGB_JDT2[2:0],ENP_LINE_INV,ENP_FRM_SEL[1:0],
    WriteData(0x22); //RGB_N_T20[11:8],RGB_N_T19[11:8],
    WriteData(0x08); //RGB_N_T19[7:0],
    WriteData(0x3A); //RGB_N_T20[7:0],
    WriteData(0x22); //RGB_N_T22[11:8],RGB_N_T21[11:8],
    WriteData(0x4B); //RGB_N_T21[7:0],
    WriteData(0x7D); //RGB_N_T22[7:0],
    WriteData(0x22); //RGB_N_T24[11:8],RGB_N_T23[11:8],
    WriteData(0x8D); //RGB_N_T23[7:0],
    WriteData(0xBF); //RGB_N_T24[7:0],
    WriteData(0x32); //RGB_N_T26[11:8],RGB_N_T25[11:8],
    WriteData(0xD0); //RGB_N_T25[7:0],
    WriteData(0x02); //RGB_N_T26[7:0],
    WriteData(0x33); //RGB_N_T28[11:8],RGB_N_T27[11:8],
    WriteData(0x12); //RGB_N_T27[7:0],
    WriteData(0x44); //RGB_N_T28[7:0],
    WriteData(0x00); //-,-,-,-,RGB_N_TA1[11:8],
    WriteData(0x0A); //RGB_N_TA1[7:0],
    WriteData(0x00); //RGB_N_TA3[11:8],RGB_N_TA2[11:8],
    WriteData(0x0A); //RGB_N_TA2[7:0],
    WriteData(0x0A); //RGB_N_TA3[7:0],
    WriteData(0x00); //RGB_N_TA5[11:8],RGB_N_TA4[11:8],
    WriteData(0x0A); //RGB_N_TA4[7:0],
    WriteData(0x0A); //RGB_N_TA5[7:0],
    WriteData(0x00); //RGB_N_TA7[11:8],RGB_N_TA6[11:8],
    WriteData(0x0A); //RGB_N_TA6[7:0],
    WriteData(0x0A); //RGB_N_TA7[7:0],


    //SETRGBCYC3
    WriteComm(0xBA);      
    WriteData(0x00);//-    -    -    -    -    -    -    -
    WriteData(0x00);//RGB_N_TA9[11:8],RGB_N_TA8[11:8] 
    WriteData(0x07);//RGB_N_TA8[7:0], 
    WriteData(0x07);//RGB_N_TA9[7:0], 
    WriteData(0x00);//RGB_N_TA11[11:8],RGB_N_TA10[11:8] 
    WriteData(0x07);//RGB_N_TA10[7:0], 
    WriteData(0x07);//RGB_N_TA11[7:0], 
    WriteData(0x00);//RGB_N_TA13[11:8],RGB_N_TA12[11:8] 
    WriteData(0x07);//RGB_N_TA12[7:0], 
    WriteData(0x07);//RGB_N_TA13[7:0], 
    WriteData(0x00);//RGB_N_TC[11:8],RGB_N_TB[11:8] 
    WriteData(0x01);//RGB_N_TB[7:0],  
    WriteData(0x01);//RGB_N_TC[7:0], 
    WriteData(0x00);//RGB_N_TE[11:8],RGB_N_TD[11:8] 
    WriteData(0x0A);//RGB_N_TD[7:0], 
    WriteData(0x01);//RGB_N_TE[7:0], 
    WriteData(0x00);//-    -    -    -    RGB_N_TF[11:8] 
    WriteData(0x01);//RGB_N_TF[7:0], 
    WriteData(0x30);//RGB_CHGEN_OFF[11:8],RGB_CHGEN_ON[11:8] 
    WriteData(0x0A);//RGB_CHGEN_ON[7:0], 
    WriteData(0x40);//RGB_CHGEN_OFF[7:0], 
    WriteData(0x30);//RES_MUX_OFF[11:8],RES_MUX_ON[11:8] 
    WriteData(0x01);//RES_MUX_ON[7:0], 
    WriteData(0x3E);//RES_MUX_OFF[7:0], 
    WriteData(0x00);//-    -    -    L2_COND1_INV[12:8],
    WriteData(0x00);//-    -    -    L2_COND0_INV[12:8],
    WriteData(0x00);//L2_COND0_INV[7:0], 
    WriteData(0x00);//L2_COND1_INV[7:0], 


    //SET_TCON
    WriteComm(0xBC);      
    WriteData(0x1A);//1  MUX_SEL =1:6 ,RSO = 360H   
    WriteData(0x00);//2  LN_NO_MUL2 = 0:Gate line number=LN[10:0]*2 ,LN[10:8] = 0
    WriteData(0xB4);//3  LN[7:0] =180*2 = 360             
    WriteData(0x03);//4  PANEL[2:0] = dancing type 2
    WriteData(0x00);//5  VFP[11:8],SLT[11:8]
    WriteData(0xD0);//6  SLT[7:0] = 1/(60*(360+10+6))/4OSC(19MHZ)
    WriteData(0x08);//7  VFP[7:0] = 8
    WriteData(0x00);//8  HBP[11:8], VBP[11:8]
    WriteData(0x07);//9  VBP[7:0]
    WriteData(0x2C);//10 HBP[7:0]  
    WriteData(0x00);//11 VFP_I[11:8],SLT_I[11:8]
    WriteData(0xD0);//12 SLT_I[7:0]
    WriteData(0x08);//13 VFP_I[7:0]
    WriteData(0x00);//14 HBP_I[11:8],VBP_I[11:8]
    WriteData(0x07);//15 VBP_I[7:0]
    WriteData(0x2C);//16 HBP_I[7:0]
    WriteData(0x82);//17 HBP_NCK[3:0],HFP_NCK[3:0]
    WriteData(0x00);//18 TCON_OPT1[15:8]
    WriteData(0x03);//19 TCON_OPT1[7:0]
    WriteData(0x00);//20 VFP_PI[11:8],SLT_PI[11:8]
    WriteData(0xD0);//21 SLT_PI[7:0]
    WriteData(0x08);//22 VFP_PI[7:0]
    WriteData(0x00);//23 HBP_PI[11:8],VBP_PI[11:8]
    WriteData(0x07);//24 VBP_PI[7:0]
    WriteData(0x2C);//25 HBP_PI[7:0]


    //-------------------GIP----------------------
    //SET_GIP_EQ
    WriteComm(0xC4); 
    WriteData(0x00);//
    WriteData(0x00);//GEQ_GND1[11:8],GEO_GVDD1[11:8]
    WriteData(0x00);//GEO_GVDD1[7:0]
    WriteData(0x02);//GEQ_GND1[7:0]
    WriteData(0x00);//GEQ_GVDD2[11:8],GEQ_GVEE1[11:8]
    WriteData(0x00);//GEQ_GVEE1[7:0]
    WriteData(0x00);//GEQ_GVDD2[7:0]
    WriteData(0x00);//GEQ_GVEE2[11:8],GEQ_GND2[11:8]
    WriteData(0x02);//GEQ_GND2[7:0]
    WriteData(0x00);//GEQ_GVEE2[7:0]
    WriteData(0x00);//SD_GEQ_GND1[11:8],SD_GEQ_GVDD1[11:8]
    WriteData(0x00);//SD_GEQ_GVDD1[7:0]
    WriteData(0x02);//SD_GEQ_GND1[7:0]
    WriteData(0x00);//SD_GEQ_GVDD2[11:8],SD_GEQ_GVEE1[11:8]
    WriteData(0x00);//SD_GEQ_GVEE1[7:0]
    WriteData(0x00);//SD_GEQ_GVDD2[7:0]
    WriteData(0x00);//SD_GEQ_GVEE2[11:8],SD_GEQ_GND2[11:8]
    WriteData(0x00);//SD_GEQ_GND2[7:0]
    WriteData(0x00);//SD_GEQ_GVEE2[7:0]

    //SET_GIP_L
    WriteComm(0xC5);      
    WriteData(0x00);//DUMMY 
    WriteData(0x1F);//0
    WriteData(0x1F);//1 
    WriteData(0x1F);//2   
    WriteData(0x1E);//3 GAS  
    WriteData(0xDF);//4 BGAS
    WriteData(0x1F);//5 RSTV  
    WriteData(0xC7);//6 CKV4
    WriteData(0xC5);//7 CKV2  
    WriteData(0x1F);//8 SB
    WriteData(0x1F);//9  
    WriteData(0x1F);//10  
    WriteData(0x1F);//11 
    WriteData(0x1F);//12 
    WriteData(0x1F);//13 
    WriteData(0x1F);//14 
    WriteData(0x1F);//15 
    WriteData(0x1F);//16 
    WriteData(0x1F);//17 
    WriteData(0x1F);//18 
    WriteData(0x1F);//19 
    WriteData(0x1F);//20 
    WriteData(0x1F);//21 
    WriteData(0x1F);//22 
    WriteData(0x1F);//23 
    WriteData(0x1F);//24  
    WriteData(0x1F);//25  
     


    //SET_GIP_R
    WriteComm(0xC6);      
    WriteData(0x00);//DUMMY
    WriteData(0x1F);//0  
    WriteData(0x1F);//1 
    WriteData(0x1F);//2 
    WriteData(0x1F);//3 
    WriteData(0x1F);//4 
    WriteData(0x1F);//5 
    WriteData(0x1F);//6 
    WriteData(0x1F);//7 
    WriteData(0x1F);//8 
    WriteData(0x1F);//9   ASB
    WriteData(0x00);//10  LSTV
    WriteData(0xC4);//11  CKV1
    WriteData(0xC6);//12  CKV3
    WriteData(0xE0);//13  CKH1
    WriteData(0xE1);//14  CKH2
    WriteData(0xE2);//15  CKH3
    WriteData(0xE3);//16  CKH4
    WriteData(0xE4);//17  CKH5
    WriteData(0xE5);//18  CKH6
    WriteData(0x1F);//19
    WriteData(0x1F);//20
    WriteData(0x1F);//21
    WriteData(0x1F);//22
    WriteData(0x1F);//23
    WriteData(0x1F);//24  
    WriteData(0x1F);//25  



    //SET_GIP_L_GS
    WriteComm(0xC7);      
    WriteData(0x00);//DUMMY   
    WriteData(0x1F);//0
    WriteData(0x1F);//1 
    WriteData(0x1F);//2 
    WriteData(0xDE);//3 GAS 
    WriteData(0x1F);//4 BGAS
    WriteData(0x00);//5 RSTV
    WriteData(0xC4);//6 CKV4
    WriteData(0xC6);//7 CKV2
    WriteData(0x1F);//8 SB
    WriteData(0x1F);//9 
    WriteData(0x1F);//10
    WriteData(0x1F);//11
    WriteData(0x1F);//12
    WriteData(0x1F);//13
    WriteData(0x1F);//14
    WriteData(0x1F);//15
    WriteData(0x1F);//16
    WriteData(0x1F);//17
    WriteData(0x1F);//18
    WriteData(0x1F);//19
    WriteData(0x1F);//20
    WriteData(0x1F);//21
    WriteData(0x1F);//22
    WriteData(0x1F);//23
    WriteData(0x1F);//24
    WriteData(0x1F);//25
     
     

    //SET_GIP_R_GS
    WriteComm(0xC8);      
    WriteData(0x00);//DUMMY   
    WriteData(0x1F);//0
    WriteData(0x1F);//1   
    WriteData(0x1F);//2 
    WriteData(0x1F);//3 
    WriteData(0x1F);//4 
    WriteData(0x1F);//5 
    WriteData(0x1F);//6 
    WriteData(0x1F);//7 
    WriteData(0x1F);//8 
    WriteData(0x1F);//9   ASB
    WriteData(0x1F);//10  LSTV
    WriteData(0xC7);//11  CKV1
    WriteData(0xC5);//12  CKV3
    WriteData(0x20);//13  CKH1
    WriteData(0x21);//14  CKH2
    WriteData(0x22);//15  CKH3
    WriteData(0x23);//16  CKH4
    WriteData(0x24);//17  CKH5
    WriteData(0x25);//18  CKH6
    WriteData(0x1F);//19
    WriteData(0x1F);//20
    WriteData(0x1F);//21
    WriteData(0x1F);//22
    WriteData(0x1F);//23
    WriteData(0x1F);//24
    WriteData(0x1F);//25



    //SETGIP1
    WriteComm(0xC9);      
    WriteData(0x00);//0
    WriteData(0x00);//1   
    WriteData(0x00);//2 
    WriteData(0x00);//3   L:GAS 
    WriteData(0x10);//4   L:BGAS :VGH
    WriteData(0x00);//5   L:RSTV
    WriteData(0x10);//6   L:CKV4 :VGH
    WriteData(0x10);//7   L:CKV2 :VGH
    WriteData(0x00);//8   L:SB
    WriteData(0x00);//9   R:ASB
    WriteData(0x00);//10  R:LSTV
    WriteData(0x20);//11  R:CKV1 :VGH
    WriteData(0x20);//12  R:CKV3 :VGH
    WriteData(0x20);//13  R:CKH1 :VGH
    WriteData(0x20);//14  R:CKH2 :VGH
    WriteData(0x20);//15  R:CKH3 :VGH
    WriteData(0x20);//16  R:CKH4 :VGH
    WriteData(0x20);//17  R:CKH5 :VGH
    WriteData(0x20);//18  R:CKH6 :VGH
    WriteData(0x00);//19
    WriteData(0x00);//20
    WriteData(0x00);//21
    WriteData(0x00);//22
    WriteData(0x00);//23       
    WriteData(0x00);//24  
    WriteData(0x00);//25  

    //SETGIP2
    WriteComm(0xCB);      
    WriteData(0x01);//1  INIT_PORCH  
    WriteData(0x10);//2  INIT_W
    WriteData(0x00);//3 
    WriteData(0x00);//4 
    WriteData(0x07);//5  STV_S0
    WriteData(0x01);//6 
    WriteData(0x00);//7 
    WriteData(0x0A);//8 
    WriteData(0x00);//9  STV_NUM = 1 , STV_S1
    WriteData(0x02);//10
    WriteData(0x00);//11 STV1/0_W
    WriteData(0x00);//12 STV3/2_W
    WriteData(0x00);//13
    WriteData(0x03);//14
    WriteData(0x00);//15
    WriteData(0x00);//16
    WriteData(0x00);//17
    WriteData(0x21);//18
    WriteData(0x23);//19
    WriteData(0x30);//20 CKV_W
    WriteData(0x00);//21 
    WriteData(0x08);//22 CKV_S0
    WriteData(0x04);//23 CKV0_DUM[7:0]
    WriteData(0x00);//24
    WriteData(0x00);//25
    WriteData(0x05);//26
    WriteData(0x10);//27
    WriteData(0x01);//28 //END_W
    WriteData(0x04);//29
    WriteData(0x06);//30
    WriteData(0x10);//31
    WriteData(0x10);//32


    //SET_GIP_ONOFF
    WriteComm(0xD1);      
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x03);
    WriteData(0x60);
    WriteData(0x30);
    WriteData(0x03);
    WriteData(0x18);
    WriteData(0x30);//CKV0_OFF[11:8]
    WriteData(0x07);//CKV0_ON[7:0]
    WriteData(0x3A);//CKV0_OFF[7:0] 
    WriteData(0x30);
    WriteData(0x03);
    WriteData(0x18);
    WriteData(0x30);
    WriteData(0x03);
    WriteData(0x18);
    WriteData(0x30);
    WriteData(0x03);
    WriteData(0x18);

    //SET_GIP_ONOFF_WB
    WriteComm(0xD2);      
    WriteData(0x00);
    WriteData(0x30);//STV_OFF[11:8]
    WriteData(0x07);//STV_ON[7:0]
    WriteData(0x3A);//STV_OFF[7:0]
    WriteData(0x32);
    WriteData(0xBC);
    WriteData(0x20);
    WriteData(0x32);
    WriteData(0xBC);
    WriteData(0x20);
    WriteData(0x32);
    WriteData(0xBC);
    WriteData(0x20);
    WriteData(0x32);
    WriteData(0xBC);
    WriteData(0x20);
    WriteData(0x30);
    WriteData(0x10);
    WriteData(0x20);
    WriteData(0x30);
    WriteData(0x10);
    WriteData(0x20);
    WriteData(0x30);
    WriteData(0x10);
    WriteData(0x20);
    WriteData(0x30);
    WriteData(0x10);
    WriteData(0x20);



     
    //SETGIP8_CKH1 CKH_ON/OFF_CKH0-CKH7_odd
    WriteComm(0xD4);      
    WriteData(0x00); 
    WriteData(0x00); //CKH_T2_ODD[11:8],CKH_T1_ODD[11:8] 
    WriteData(0x03); //CKH_T1_ODD[7:0], 
    WriteData(0x14); //CKH_T2_ODD[7:0], 
    WriteData(0x00); //CKH_T4_ODD[11:8],CKH_T3_ODD[11:8] 
    WriteData(0x03); //CKH_T3_ODD[7:0], 
    WriteData(0x20); //CKH_T4_ODD[7:0], 
    WriteData(0x00); //CKH_T6_ODD[11:8],CKH_T5_ODD[11:8] 
    WriteData(0x09); //CKH_T5_ODD[7:0], 
    WriteData(0x82); //CKH_T6_ODD[7:0], 
    WriteData(0x10); //CKH_T8_ODD[11:8],CKH_T7_ODD[11:8] 
    WriteData(0x8A); //CKH_T7_ODD[7:0], 
    WriteData(0x03); //CKH_T8_ODD[7:0], 
    WriteData(0x11); //CKH_T10_ODD[11:8],CKH_T9_ODD[11:8] 
    WriteData(0x0B); //CKH_T9_ODD[7:0], 
    WriteData(0x84); //CKH_T10_ODD[7:0], 
    WriteData(0x21); //CKH_T12_ODD[11:8],CKH_T11_ODD[11:8] 
    WriteData(0x8C); //CKH_T11_ODD[7:0], 
    WriteData(0x05); //CKH_T12_ODD[7:0], 
    WriteData(0x22); //CKH_T14_ODD[11:8],CKH_T13_ODD[11:8] 
    WriteData(0x0D); //CKH_T13_ODD[7:0], 
    WriteData(0x86); //CKH_T14_ODD[7:0], 
    WriteData(0x32); //CKH_T16_ODD[11:8],CKH_T15_ODD[11:8] 
    WriteData(0x8E); //CKH_T15_ODD[7:0], 
    WriteData(0x07); //CKH_T16_ODD[7:0], 
    WriteData(0x00); //CKH_T18_ODD[11:8],CKH_T17_ODD[11:8] 
    WriteData(0x00); //CKH_T17_ODD[7:0], 
    WriteData(0x00); //CKH_T18_ODD[7:0], 
    WriteData(0x00); //CKH_T20_ODD[11:8],CKH_T19_ODD[11:8] 
    WriteData(0x00); //CKH_T19_ODD[7:0], 
    WriteData(0x00); //CKH_T20_ODD[7:0], 

     
    ///-----------------------------------------------------------------------------------------
    //---------------- PAGE0 --------------
    WriteComm(0xDE);      
    WriteData(0x00); 
    // RAM_CTRL
    WriteComm(0xD7);      
    WriteData(0x20);  //GM=1;RP=0;RM=0;DM=00
    WriteData(0x00);  
    WriteData(0x00); 

    //---------------- PAGE1 --------------
    WriteComm(0xDE);      
    WriteData(0x01); 

    ////MCMD_CTRL
    WriteComm(0xCA);      
    WriteData(0x00);

    //---------------- PAGE2 --------------
    WriteComm(0xDE);      
    WriteData(0x02); 

    //OSC DIV
    WriteComm(0xC5);      
    WriteData(0x03); //FPS 60HZ (0x03) to 30HZ (0x0B) ,47HZ(0x0F),42HZ(0x0E)
     
    //---------------- PAGE0 --------------
    WriteComm(0xDE);      
    WriteData(0x00);  

    //Color Pixel Format
    WriteComm(0x3A);      
    WriteData(0x05); //666

    //TE ON
    WriteComm(0x35);      
    WriteData(0x00);  

    //SLP OUT
    WriteComm(0x11);      // SLPOUT
        Delay(11200);

    //The host could send 1st image at this time
    //DISP ON
    WriteComm(0x29);      // DSPON

}

void BlockWrite(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend) 
{
    
    WriteComm(0x2A); 
    WriteData(Xstart>>8);             
    WriteData(Xstart);             
    WriteData(Xend>>8);             
    WriteData(Xend);             
    
    WriteComm(0x2B);  
    WriteData(Ystart>>8);             
    WriteData(Ystart);             
    WriteData(Yend>>8);             
    WriteData(Yend);   
    
    WriteComm(0x2c);

}

void display_image(unsigned int xstart,unsigned int xend,unsigned int ystart,unsigned int yend,uint8_t *image)
{
    unsigned int i;

    for(int j=0;j<10;j++)
    {
        for(int i=0;i<10;i++)
        {
            uart_printf("%x ",image[i]);
        }
        uart_printf("\r\n");
    }
    BlockWrite(xstart,xend,ystart,yend);
    
    gpio_set(GPIO_LCD_RS,1);
    for(i=0;i<20;i++)
    spi_dma_write(image+i*2560,2560,NULL);
    

}


void display_color(uint8_t* buff)
{
    unsigned int i;

    BlockWrite(0,COL-1,0,ROW-1);

    gpio_set(GPIO_LCD_RS,1);
    for(i=0;i<75;i++)//360*360*2
    {
        spi_dma_write(buff,3456,spi_dma_write_result_callback);
        
    }
}



void DispFrame(void)
{
    unsigned int i,j;
    
    BlockWrite(0,COL-1,0,ROW-1);
      

    WriteData(COLOR_RED>>8);
    WriteData(COLOR_RED&0xff);
    for(i=0;i<COL-2;i++)
    {WriteData(0XFF);WriteData(0XFF);}
    WriteData(COLOR_RED>>8);WriteData(COLOR_RED&0xff);

    for(j=0;j<ROW-2;j++)
    {
        WriteData(COLOR_RED>>8);WriteData(COLOR_RED&0xff);
        for(i=0;i<COL-2;i++){WriteData(COLOR_BLACK>>8);WriteData(COLOR_BLACK&0xff);}
        WriteData(COLOR_RED>>8);WriteData(COLOR_RED&0xff);
    }

    WriteData(COLOR_RED>>8);WriteData(COLOR_RED&0xff);
    for(i=0;i<COL-2;i++){WriteData(0XFF);WriteData(0XFF);}
    WriteData(COLOR_RED>>8);WriteData(COLOR_RED&0xff);
}










#ifdef SLEEP_TEST
void EnterSLP(void)
{
    //FOR OTM3225A

    WriteComm(0x0007);WriteData(0x0131); // Set D1=0, D0=1
    Delay(10);
    WriteComm(0x0007);WriteData(0x0130); // Set D1=0, D0=0
    Delay(10);
    WriteComm(0x0007);WriteData(0x0000); // display OFF
    //************* Power OFF sequence **************//
    WriteComm(0x0010);WriteData(0x0080); // SAP, BT[3:0], APE, AP, DSTB, SLP
    WriteComm(0x0011);WriteData(0x0000); // DC1[2:0], DC0[2:0], VC[2:0]
    WriteComm(0x0012);WriteData(0x0000); // VREG1OUT voltage
    WriteComm(0x0013);WriteData(0x0000); // VDV[4:0] for VCOM amplitude
    Delay(200);                          // Dis-charge capacitor power voltage
    WriteComm(0x0010);WriteData(0x0082); // SAP, BT[3:0], APE, AP, DSTB, SLP
}

void ExitSLP(void)
{
    //FOR OTM3225A

    //Power On sequence
    WriteComm(0x0010);WriteData(0x0080); // SAP, BT[3:0], AP, DSTB, SLP
    WriteComm(0x0011);WriteData(0x0000); // DC1[2:0], DC0[2:0], VC[2:0]
    WriteComm(0x0012);WriteData(0x0000); // VREG1OUT voltage
    WriteComm(0x0013);WriteData(0x0000); // VDV[4:0] for VCOM amplitude
    Delay(200);                          // Dis-charge capacitor power voltage
    WriteComm(0x0010);WriteData(0x1490); // SAP, BT[3:0], AP, DSTB, SLP, STB
    WriteComm(0x0011);WriteData(0x0227); // R11h=0x0221 at VCI=3.3V DC1[2:0], DC0[2:0], VC[2:0]
    Delay(50);                           // Delay 50ms
    WriteComm(0x0012);WriteData(0x009B); // External reference voltage =Vci;
    Delay(50);                           // Delay 50ms
    WriteComm(0x0013);WriteData(0x1800); // R13h=0x1200 when R12=009D VDV[4:0] for VCOM amplitude
    WriteComm(0x0029);WriteData(0x0027); // R29h=0x000C when R12=009D VCM[5:0] for VCOMH
    Delay(50);                           // Delay 50ms
    WriteComm(0x0007);WriteData(0x0133); // 262K color and display ON
}

#endif


