
#ifndef __LCD_h__
#define __LCD_h__

#ifdef DEBUG
    #define DEBUG_X  50
    #define DEBUG_Y  15
    //extern unsigned int code debug_code[X][Y];
    extern unsigned int code debug_code[50][15];
    
    #define END 0xffff
#endif


#define GPIO_LCD_RESET 0x35
#define GPIO_LCD_RS 0x36
#define GPIO_KEY 0x32
#define GPIO_test 0x31
#define ROW 360            //显示的行、列数
#define COL 360  


#define COLOR_RED    0xF800         //定义颜色常量 
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F
#define COLOR_WHITE  0xFFFF
#define COLOR_BLACK  0x0000
#define COLOR_GRAY   0xEF5D         //0x2410
#define COLOR_GRAY75 0x39E7 
#define COLOR_GRAY50 0x7BEF    
#define COLOR_GRAY25 0xADB5    
extern unsigned short int pic_eval[];
//extern unsigned char code pic22[];

//---------------------------------------------------------------------
void WriteComm(unsigned char dat);
void WriteData(unsigned char dat);
void WriteDispData(unsigned char DataH,unsigned char DataL);
void LCD_Init(void);
void BlockWrite(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend) ;
void DispBand(void);
void display_color(unsigned char* buff);
unsigned char key_press_detect(void);
void lcd_gpio_init(void);

void DispFrame(void);
//void DispPic(unsigned int code *picture);
//void DispPicFromSD(unsigned char PicNum);
 //void picture();

void DispScaleHor1(void);
void DispScaleVer(void);
void DispScaleVer_Red(void);
void DispScaleVer_Green(void);
void DispScaleVer_Blue(void);
void DispScaleVer_Gray(void);
void DispGrayHor16(void);
void DispGrayHor32(void);
void DispScaleHor2(void);
void DispSnow(void);
void DispBlock(void);

void WriteOneDot(unsigned int color);
unsigned char ToOrd(unsigned char ch); 
void  DispOneChar(unsigned char ord,unsigned int Xstart,unsigned int Ystart,unsigned int TextColor,unsigned int BackColor);     // ord:0~95
void DispStr(unsigned char *str,unsigned int Xstart,unsigned int Ystart,unsigned int TextColor,unsigned int BackColor);
void DispInt(unsigned int i,unsigned int Xstart,unsigned int Ystart,unsigned int TextColor,unsigned int BackColor);

unsigned int ReadData(void);
void DispRegValue(unsigned int RegIndex,unsigned char ParNum);

void Debug(void);

void PutPixel(unsigned int x,unsigned int y,unsigned int color);
void DrawLine(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend,unsigned int color);
void DrawGird(unsigned int color);

#endif
