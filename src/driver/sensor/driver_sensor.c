#include "driver_sensor.h"
#include "gpio.h"
#define UART_PRINTF    uart_printf


extern SYSTEM_STRUCT_DATA system_data;
extern void Delay_ms(int num);
extern void Delay_us(int num);


uint8_t bSensorCheck;

#define SENTABLE_LEN 32 //31
const uint8_t register_3205[SENTABLE_LEN][2] =
{
    0x09, 0x5a, //0x5A = Disable, register 0x0A ~ 0x7F can be read/written
    0x0D, 0x0F,
    0x1D, 0xE3,
    0x28, 0xB4,
    0x29, 0x46,
    0x2A, 0x96,
    0x2B, 0x8C,
    0x2C, 0x6E,
    0x2D, 0x64,
    0x38, 0x5F,
    0x39, 0x0F,
    0x3A, 0x32,
    0x3B, 0x47,
    0x42, 0x10,
    0x54, 0x2E,
    0x55, 0xF2,
    0x61, 0xF4,
    0x63, 0x70,
    0x75, 0x52,
    0x76, 0x41,
    0x77, 0xED,
    0x78, 0x23,
    0x79, 0x46,
    0x7A, 0xE5,
    0x7C, 0x48,
    0x7D, 0xD2,
    0x7E, 0x77,
    0x1B, 0x35, //For better tracking on dark surface
    //0x7F, 0x01, //(Switch to Bank1). DO NOT read back!
    //0x0B, 0x00,
    0x7F, 0x00, //(Switch to Bank0)
    0x09, 0x00  // 0x0a-0x7f write protect
};

void driver_sensor_paw3204_spi_init(void)
{
#if(SENSOR_TEST)
    gpio_set(PAW3205_SCLK_PIN,0);
    gpio_config(PAW3205_SCLK_PIN,OUTPUT,PULL_NONE);
    gpio_set(PAW3205_SDIO_PIN,0);
    gpio_set(PAW3205_MOTSWK_PIN,0);
    gpio_config(PAW3205_SDIO_PIN,OUTPUT,PULL_NONE);
    gpio_config(PAW3205_MOTSWK_PIN,INPUT,PULL_HIGH);
#else

    gpio_set(PAW3205_SCLK_PIN,1);
    gpio_config(PAW3205_SCLK_PIN,OUTPUT,PULL_NONE);
    gpio_config(PAW3205_SDIO_PIN,INPUT,PULL_HIGH);
    gpio_config(PAW3205_MOTSWK_PIN,INPUT,PULL_HIGH);

#endif
}

void driver_sensor_paw3204_send_byte(uint8_t value)
{
    uint8_t i;
#if 0
    /*OUTPUT  GPIO_LOW*/
    gpio_set(PAW3205_SDIO_PIN,0);
    gpio_config(PAW3205_SDIO_PIN,OUTPUT,PULL_NONE);

    for(i=0; i<8; i++)
    {
        gpio_set(PAW3205_SCLK_PIN,0);
        if(value & 0x80)
        gpio_set(PAW3205_SDIO_PIN,1);
        else
        gpio_set(PAW3205_SDIO_PIN,0);
        gpio_set(PAW3205_SCLK_PIN,1);
        value <<= 1;
    }
#else
 // SDIO is P03,set SDIO is output
     *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SDIO_PIN>>4)&0x0f)*8+(PAW3205_SDIO_PIN&0x0f))))&= ~(0x2c) ;

    for(i=0; i<8; i++)
    {
        //gpio_set(PAW3205_SCLK_PIN,0);
         *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SCLK_PIN>>4)&0x0f)*8+(PAW3205_SCLK_PIN&0x0f)))) &= ~0X02;
        if(value & 0x80)
             *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SDIO_PIN>>4)&0x0f)*8+(PAW3205_SDIO_PIN&0x0f)))) |= 0X02;//gpio_set(PAW3205_SDIO_PIN,1);
        else
             *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SDIO_PIN>>4)&0x0f)*8+(PAW3205_SDIO_PIN&0x0f)))) &= ~0X02;//gpio_set(PAW3205_SDIO_PIN,0);

         *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SCLK_PIN>>4)&0x0f)*8+(PAW3205_SCLK_PIN&0x0f)))) |= 0X02;//gpio_set(PAW3205_SCLK_PIN,1);
        value <<= 1;
    }
#endif

}

uint8_t driver_sensor_paw3204_read_byte(void)
{
    uint8_t i, ret=0;
#if 0
    gpio_config(PAW3205_SDIO_PIN,INPUT,PULL_HIGH);
    for(i=0; i<8; i++)
    {
        gpio_set(PAW3205_SCLK_PIN,0);
        ret <<= 1;
        gpio_set(PAW3205_SCLK_PIN,1);

        if(gpio_get_input(PAW3205_SDIO_PIN))
        ret |= 0x01;
    }
    gpio_set(PAW3205_SDIO_PIN,0);
    gpio_config(PAW3205_SDIO_PIN,OUTPUT,PULL_NONE);
#else

    // SDIO is P03,set SDIO is input
     *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SDIO_PIN>>4)&0x0f)*8+(PAW3205_SDIO_PIN&0x0f)))) |= 0X3C ;
    for(i=0; i<8; i++)
    {

         *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SCLK_PIN>>4)&0x0f)*8+(PAW3205_SCLK_PIN&0x0f)))) &= ~0X02;//gpio_set(PAW3205_SCLK_PIN,0);
        ret <<= 1;
         *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SCLK_PIN>>4)&0x0f)*8+(PAW3205_SCLK_PIN&0x0f)))) |= 0X02;//gpio_set(PAW3205_SCLK_PIN,1);

        if( *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SDIO_PIN>>4)&0x0f)*8+(PAW3205_SDIO_PIN&0x0f))))& 0x01)
            ret |= 0x01;
    }

    // SDIO is P03,set SDIO is output
     *((volatile unsigned long *)(BASEADDR_GPIO+4*(((PAW3205_SDIO_PIN>>4)&0x0f)*8+(PAW3205_SDIO_PIN&0x0f)))) &= ~(0x2c) ;
#endif

    return ret;
}


/*! \fn void driver_sensor_paw3204_read_data(UINT8 addr, UINT8 *value, UINT8 len)
    \brief Read \a len bytes data to the pointer \a value from the address \addr in paw3204.
    \param addr - the address of written
    \param value - the pointer stored the read data
    \param len - the length of data read
    \return void
*/
void driver_sensor_paw3204_read_data(uint8_t addr, uint8_t *value, uint8_t len)
{
    uint8_t i;
    for(i=0; i<len; i++)
    {
        driver_sensor_paw3204_send_byte(addr++);
        *value++ = driver_sensor_paw3204_read_byte();
    }
}

/*! \fn void driver_sensor_paw3204_send_data(UINT8 addr , UINT8 value)
    \brief Send a byte data \a value to the address \addr in paw3204.

    \param addr - the address of written
    \param value - the data
    \return void
*/
void driver_sensor_paw3204_send_data(uint8_t addr , uint8_t value)
{
    driver_sensor_paw3204_send_byte(addr |0x80);
    driver_sensor_paw3204_send_byte(value);
}

void resync(void)
{
    gpio_set(PAW3205_SCLK_PIN,0);
    Delay_ms(10);//10ms
    gpio_set(PAW3205_SCLK_PIN,1);
    Delay_ms(5);// 1.8ms
}

void resync1(void)
{
    gpio_set(PAW3205_SCLK_PIN,0);
    Delay_us(10);//10ms
    gpio_set(PAW3205_SCLK_PIN,1);
    Delay_us(5);// 1.8ms
}

///***************************************************
// u_data:  0: 1000 , else: 1300,  2 :1600,
///***************************************************
void driver_sensor_paw3204_cpi_set_value(uint8_t u_data)
{
  //  return;

    uint8_t buf,tmp_data;
    uint8_t count;
    count = 5;
 //   driver_sensor_paw3204_read_data(0x06, &tmp_data, 1);
 //   tmp_data &= 0xf8;
    switch(system_data.sensor_val.type)
    {
        case SENSOR_3065:
            switch(u_data)
            {
                case 0:
                    tmp_data = 1;       //1000
                    break;
                case 2:
                    tmp_data = 3;       // 1600
                    break;
                default:
                    tmp_data = 2;       // 1300
                    break;
            }
            break;
        case SENSOR_3205:
            switch(u_data)
            {
                case 0:
                    tmp_data = 2;       //1000
                    break;
                case 2:
                    tmp_data = 4;       // 1600
                    break;
                default:
                    tmp_data = 3;       // 1300
                    break;
            }
            break;
        default:
            break;
    }

    while(count--)
    {
        driver_sensor_paw3204_send_data(0x06, tmp_data);
        Delay_us(10);
        driver_sensor_paw3204_read_data(0x02, &buf, 1);
        if( (buf&0x07) == tmp_data)
            break;
        Delay_us(20);
    }
    //driver_sensor_paw3204_send_data(0x09 , 0x00); //reset SDIO

}

boolean app_sensor_paw3065_inital(void)
{
    
#if(SENSOR_TEST)
    return 1;
#else
    uint8_t buf[3];
    uint8_t count = 0;
    //resync();
    do
    {
        driver_sensor_paw3204_read_data(0x00, buf, 2);
        uart_printf("s3065= %x,%x\r\n",buf[0],buf[1]);
        count++;
        if ((buf[0] != 0x30)&&(buf[0] != 0x31))
            resync();

        if(count >= 17)
            return 0;
    } while( !( buf[0] == 0x31 && (buf[1] == 0x70)) );

    driver_sensor_paw3204_send_data(0x09 , 0x5a); // 0x0a-0x7f write protect
    //driver_sensor_paw3204_send_data(0x0C , 0x70);
    driver_sensor_paw3204_send_data(0x0D , 0x12);
    driver_sensor_paw3204_cpi_set_value(1); //dpi is 1000 default
    driver_sensor_paw3204_send_data(0x09 , 0x00); // 0x0a-0x7f write protect
    return 1;
#endif    
}

boolean app_sensor_paw3205_inital(void)
{
    
#if(SENSOR_TEST)
    return 1;
#else
    uint8_t buf[3];
    uint8_t count = 0;
    resync();
    do
    {
        driver_sensor_paw3204_read_data(0x00, buf, 1);
        uart_printf("s3205= %x\r\n",buf[0]);
        count++;
        if ((buf[0] != 0x30)&&(buf[0] != 0x31))
        {
            resync();
        }
        else
        {
            driver_sensor_paw3204_send_data(0x06, 0x83);                 //just reset,
            break;
        }
        if(count >= 17)
        {
            return 0;
        }
    } while(1);
#if    0
    for(i=0; i<31; i++)  ///12
    {
        driver_sensor_paw3204_send_data(register_3205[i][0], register_3205[i][1]);
    }
#else
    driver_sensor_paw3204_send_data(0x09 , 0x5a); // 0x0a-0x7f write protect
    driver_sensor_paw3204_send_data(0x0D , 0x10);
    driver_sensor_paw3204_send_data(0x7c , 0x0a);
    driver_sensor_paw3204_cpi_set_value(1); //dpi is 1000 default
    driver_sensor_paw3204_send_data(0x09 , 0x00); // 0x0a-0x7f write protect
#endif

    return 1;
#endif
}

void app_sensor_get_id(void)
{
    uint8_t buf[2];
    uint8_t count = 0;
    system_data.sensor_val.type  = SENSOR_NONE;

    do
   {
       driver_sensor_paw3204_read_data(0x00, buf, 2);
 //      bprintf("sensor= %x,%x\r\n",buf[0],buf[1]);
       count++;
       if ((buf[0] != 0x30)&&(buf[0] != 0x31))
           resync();
       else
       {
           break;
       }

       if(count >= 17)
           return ;
   } while(1);

   if( (buf[0] == 0x31) && (buf[1] == 0x70))
       system_data.sensor_val.type = SENSOR_3065;
   else if( buf[0] == 0x31)
       system_data.sensor_val.type = SENSOR_3205;

}
boolean app_sensor_init(void)
{

#if(SENSOR_TEST)
    driver_sensor_paw3204_spi_init();
    return 1;
#else
    bSensorCheck = 0x0;
    // continue 16 bytes,then maybe sync
    driver_sensor_paw3204_spi_init();
    resync();
    app_sensor_get_id();

    switch(system_data.sensor_val.type)
    {
        case SENSOR_3065:
            app_sensor_paw3065_inital();
            break;
        case SENSOR_3205:
            app_sensor_paw3205_inital();
            break;
        default:
            return 0;

    }
    bSensorCheck = 0x01;
    return 1;
#endif    
}



boolean app_sensor_3205_check(void)
{
#if(SENSOR_TEST)
    static uint8_t dcount = 100;
    Delay_us(350);
    dcount++;
    if(dcount>=100)
    {
        dcount = 0;

        if ((system_data.sensor_val.x == 0) && (system_data.sensor_val.y == 4))
        {
            system_data.sensor_val.x=4;
            system_data.sensor_val.y = 0;
        }
        else if ((system_data.sensor_val.x == 4) && (system_data.sensor_val.y == 0))
        {
            system_data.sensor_val.x = 0;
            system_data.sensor_val.y = -4;
        }
        else if ((system_data.sensor_val.x == 0) && (system_data.sensor_val.y== -4))
        {
            system_data.sensor_val.x = -4;
            system_data.sensor_val.y = 0;
        }
        else if((system_data.sensor_val.x == -4) && (system_data.sensor_val.y == 0))
        {
            system_data.sensor_val.x = 0;
            system_data.sensor_val.y = 4;
        }
        else
        {
            system_data.sensor_val.x = 4;
            system_data.sensor_val.y = 0;
        }
    }
    return 1;
#else
    uint8_t buf[3];
    uint16_t deltaX_l, deltaY_l, deltaX_h, deltaY_h;
    uint8_t count = 0;
    deltaX_h = 0;
    deltaY_h = 0;
    system_data.sensor_val.x = 0;
    system_data.sensor_val.y = 0;

    do
    {
        driver_sensor_paw3204_read_data(0x00, buf, 1);

//        bprintf("sensor= %x\r\n",buf[0]);
        count++;
        if ((buf[0] != 0x30)&&(buf[0] != 0x31))
        {
            resync1();
        }
        else
        {
            break;
        }
        if(count >= 17)
        {
            switch(system_data.sensor_val.type)
            {
                case SENSOR_3065:
                    app_sensor_paw3065_inital();
                    break;
                case SENSOR_3205:
                    app_sensor_paw3205_inital();
                    break;
                default:
                    break;
            }
            return 0;
        }
    } while(1);

    if(driver_sensor_check_up())
        system_data.sensor_val.sensor_up =1;
    else
        system_data.sensor_val.sensor_up =0;

    if(gpio_get_input(PAW3205_MOTSWK_PIN))
    {
        driver_sensor_paw3204_send_data(0x09 , 0x00);

        return 1;
    }

    driver_sensor_paw3204_read_data(0x02, buf , 3);
    deltaX_l = buf[1];
    deltaY_l =buf[2];

    if(deltaX_l & 0x0080)
    deltaX_h = 0xff00;
    if(deltaY_l & 0x0080)
    deltaY_h = 0xff00;
#if(USER1_DEFINE)


    system_data.sensor_val.x = deltaX_h | deltaX_l;
    system_data.sensor_val.y = deltaY_h | deltaY_l;
    system_data.sensor_val.x =  (~system_data.sensor_val.x)+1;
#else
    system_data.sensor_val.x = deltaY_h | deltaY_l;
    system_data.sensor_val.y = deltaX_h | deltaX_l;
#endif

    driver_sensor_paw3204_send_data(0x09 , 0x00);
    return 1;
#endif    
}

void app_sensor_check(void)
{

    app_sensor_3205_check();

}

void sensor_powerdown(void)
{   
#if(!SENSOR_TEST)
    uint8_t tmp_cnt=0;
    while(0==gpio_get_input(PAW3205_MOTSWK_PIN))
    {
        if(tmp_cnt++>100)
            break;
        app_sensor_check();
        Delay_us(4000);
    }
#endif
}


uint8_t driver_sensor_check_up(void)
//sensor ?¦Ì???????¨®?¨¹?¨®¡ä£¤??¡ê??¨¨¨®?<2¨¤¡ä¡À¨ª¨º?¨¬¡ì¨¤?¦Ì???
//return:1:¨¬¡ì?e¡ê? 0 : no
{
    uint8_t tmp_data=0;

    driver_sensor_paw3204_read_data(0x07,&tmp_data,1);
     switch(system_data.sensor_val.type)
    {
    case SENSOR_3065:
        if(tmp_data<2)
            return 1;
        else
            return 0;
        //break;
    case SENSOR_3205:
    default:
        if(tmp_data<0x15)
            return 1;
        else
            return 0;
        //break;
    }
}

void app_sensor_powerdown(void)
{
#if(!SENSOR_TEST)
    switch(system_data.sensor_val.type)
    {
    case SENSOR_3065:
        driver_sensor_paw3204_send_data(0x06 ,0x08);
        break;
    case SENSOR_3205:
        driver_sensor_paw3204_send_data(0x06 ,0x08);
        break;
    default:
        break;
    }
    driver_sensor_paw3204_send_data(0x09 , 0x00);
#endif
}

void app_sensor_wakeup(void)
{
#if(!SENSOR_TEST)
    driver_sensor_paw3204_cpi_set_value(1);
#endif
}

