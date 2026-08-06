#include "w25x40c.h"
#include "spi.h"
#include "user_config.h"
#include "uart2.h"
#include "lcd360g.h"


#define   FLASH_DEVICE_APP_CFG 1
#define   FLASH_DEVICE_PWR_CFG 1

#define SPI_SOFTWARE        0


static void Delay_ms(int num) //sync from svn revision 18
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 3260; x++);
    }

}

#if  SPI_SOFTWARE



uint8_t  spi_device_wr_byte( uint8_t spi_data )
{
    for( int i = 0; i < 8; i ++ )
    {
        if( spi_data & 0x80 ) spi_device_mosi_set( 1 );
        else                  spi_device_mosi_set( 0 );
        spi_data  <<=  1;
        spi_device_sclk_set( 0 );
        if( spi_device_miso_read() ) spi_data ++;
        spi_device_sclk_set( 1 );
    }
    return  spi_data;
}
#else

uint8_t  spi_device_wr_byte( uint8_t spi_data )
{
    return 0;
}
void  spi_device_w_byte( uint8_t spi_data )
{
    uint8_t p_data[1]  ;
    p_data[0] = spi_data;
//    gpio_config(0x07,OUTPUT,PULL_HIGH);  //lcd 片选disable
//    gpio_set(0x07,1);
    spi_write( p_data, 1 );
//    gpio_config(0x07,SC_FUN,PULL_NONE);
}
uint8_t  spi_device_r_byte( void )
{
    uint8_t p_data_r[1] = {0} ;
//    gpio_config(0x07,OUTPUT,PULL_HIGH);
//    gpio_set(0x07,1);
    spi_read( p_data_r, 1 );
//    gpio_config(0x07,SC_FUN,PULL_NONE);
    return p_data_r[0] ;

}
void  spi_device_w_bytes( uint8_t *spi_data, uint16_t length )
{

    uint16_t i;
//    gpio_config(0x07,OUTPUT,PULL_HIGH);
//    gpio_set(0x07,1);
      i = length/64;
      if(i>0)
      {
          for(uint16_t m=0;m<i;m++)
          {    
              spi_write( spi_data , 64 );
            
            spi_data = spi_data+m*64;
          }
      }      

      i = length%64;
      if(i>0)    
      spi_write( spi_data , i );
      
//      gpio_config(0x07,SC_FUN,PULL_NONE);
}
void  spi_device_r_bytes( uint8_t *spi_data, uint16_t length )
{

  uint16_t i;

  i = length/4095;
  if(i>0)
  {
      for(uint16_t m=0;m<i;m++)
      {    
          spi_read( spi_data , 4095 );
        
        spi_data = spi_data+m*4095;
      }
  }      

  i = length%4095;
  if(i>0)
  spi_read( spi_data , i );


}
#endif


#if  FLASH_DEVICE_APP_CFG

void flash_device_spi_init( void )
{
    uint32_t ChipID = 0 ;
    gpio_config( SPI_CS_PIN, OUTPUT, PULL_NONE );
    gpio_config( SPI_PRO_PIN, OUTPUT, PULL_NONE);

    spi_device_cs_set( 1 );
    gpio_set(SPI_PRO_PIN,1);
    //   spi_device_wr_byte( 0xff );
    flash_device_power_down_set( false );
    ChipID  = flash_device_chipID_get() ;
    uart_printf("ChipID = %x\r\n",ChipID);
    if( W25X40 == ChipID )
    {
        //     LOG_PRINTF(0,"ID获取成功 \r\n");
        //ID获取成功
    }
    else
    {
        // LOG_PRINTF(0,"ID获得失败\r\n");
        //ID获得失败
        // while(1);
    }
    flash_device_power_down_set( true ); //进入睡眠

}
#else
uint8_t  flash_buffer8[1000];
uint32_t flash_buffer32[36];
uint8_t  flash_counter = 0;
uint16_t FLAHS_ID = 0;
void flash_device_spi_init( void )
{
    spi_device_sclk_set( 1 );
    spi_device_cs_set( 1 );
    spi_device_wr_byte( 0xff );
    flash_device_power_down_set( false );
    FLAHS_ID = flash_device_chipID_get();

    while( 1 )
    {
        // FLAHS_ID = flash_device_chipID_get();
        // flash_buffer32[1] = flash_device_deviceID_get(&flash_buffer8[0]);
        memset( flash_buffer8, 0, 1000 );
        flash_device_read_data( flash_buffer8, 0, 1000 );
        for( int i = 0; i < 1000; i ++ )
        {
            flash_buffer8[i] = 0Xf2;
        }
        flash_device_erase_sector( 0 );
        flash_device_write_data( flash_buffer8, 0, 1000 );
        memset( flash_buffer8, 0, 1000 );
        flash_device_read_data( flash_buffer8, 0, 1000 );
    }
}
#endif

uint8_t flash_device_spi_status_get( void )
{
    uint8_t  byte;
    //LCM_CS_OUT_SET(1);
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_ReadStatusReg );
    byte = spi_device_r_byte();
    spi_device_cs_set( 1 );
    return byte;
}

bool flash_device_busy_wait( void )
{
    uint32_t flash_device_timeout_counter = 0;
    while( ( ( flash_device_spi_status_get() & 0x01 ) == 0x01 ) && ( ++flash_device_timeout_counter <  FLASH_TIMEOUT_VALUE ) );
    if( !( flash_device_spi_status_get() & 0x01 ) )
        return  true;
    else
        return  false;
}

void flash_device_spi_status_set( uint8_t spi_status_state )
{
    //LCM_CS_OUT_SET(1);
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_WriteStatusReg );
    spi_device_w_byte( spi_status_state );
    spi_device_cs_set( 1 );
}

void flash_device_write_enable( void )
{
    //LCM_CS_OUT_SET(1);
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_WriteEnable );
    spi_device_cs_set( 1 );
}

void flash_device_write_disable( void )
{
    //LCM_CS_OUT_SET(1);
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_WriteDisable );
    spi_device_cs_set( 1 );
}

uint16_t  flash_device_chipID_get( void )
{
    uint16_t chipID_value = 0;
    uint8_t rdbuffer[2]={0};
    //LCM_CS_OUT_SET(1);
    spi_device_cs_set( 0 );
    spi_device_w_byte( 0x90 );
    spi_device_w_byte( 0x00 );
    spi_device_w_byte( 0x00 );
    spi_device_w_byte( 0x00 );
    Delay_ms(2);
    spi_device_r_bytes(rdbuffer,2);
    chipID_value = (rdbuffer[0] << 8) | rdbuffer[1];
    spi_device_cs_set( 1 );
    return chipID_value;
}
//u16 SPI_Flash_ReadID(void)
//{
//    u16 Temp = 0;
//    SPI_FLASH_CS=0;
//    SPI2_ReadWriteByte(0x90);//发送读取ID命令
//    SPI2_ReadWriteByte(0x00);
//    SPI2_ReadWriteByte(0x00);
//    SPI2_ReadWriteByte(0x00);
//    Temp|=SPI2_ReadWriteByte(0xFF)<<8;
//    Temp|=SPI2_ReadWriteByte(0xFF);
//    SPI_FLASH_CS=1;
//    return Temp;
//}
uint32_t flash_device_deviceID_get( uint8_t *p_flashID )
{
    uint32_t Device_ID = 0;
    //LCM_CS_OUT_SET(1);
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_JedecDeviceID );
    *p_flashID = spi_device_r_byte(  );
    Device_ID |= spi_device_r_byte(  ) << 8;
    Device_ID |= spi_device_r_byte(  );
    spi_device_cs_set( 1 );
    return Device_ID;
}

void flash_device_read_data( uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead )
{

    spi_device_cs_set( 0 );

    spi_device_w_byte( W25X_ReadData );
    spi_device_w_byte( ( uint8_t )( ( ReadAddr ) >> 16 ) );
    spi_device_w_byte( ( uint8_t )( ( ReadAddr ) >> 8 ) );
    spi_device_w_byte( ( uint8_t )ReadAddr );
    spi_dma_read(pBuffer,NumByteToRead,spi_dma_read_result_callback);
    Delay_ms(20);//wait read data finish
    spi_device_cs_set( 1 );

}

void flash_device_erase_sector( uint32_t flas_sector )
{

    if( flas_sector <= 450 && flas_sector > 0 )
    {
        return ;
    }

    flas_sector *= FLASH_DEVICE_SECTOR_SIZE;
    flash_device_write_enable();
    flash_device_busy_wait();
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_SectorErase );
    spi_device_w_byte( ( uint8_t )( ( flas_sector ) >> 16 ) );
    spi_device_w_byte( ( uint8_t )( ( flas_sector ) >> 8 ) );
    spi_device_w_byte( ( uint8_t )flas_sector );
    spi_device_cs_set( 1 );
    flash_device_busy_wait();
}

bool flash_device_write_page( uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite )
{
    
    uart_printf("WriteAddr = %x,NumByteToWrite=%x\r\n",WriteAddr,NumByteToWrite);
    flash_device_write_enable();
    
  //  LCM_CS_OUT_SET( 1 );
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_PageProgram );
    spi_device_w_byte( ( uint8_t )( ( WriteAddr ) >> 16 ) );
    spi_device_w_byte( ( uint8_t )( ( WriteAddr ) >> 8 ) );
    spi_device_w_byte( ( uint8_t )WriteAddr );
    
    spi_device_w_bytes( pBuffer,NumByteToWrite );
    Delay_ms(50);
    spi_device_cs_set( 1 );
    return flash_device_busy_wait();
}

int32_t flash_device_write_data( uint8_t *wr_data_ptr, uint32_t address, uint32_t size )
{

    if( address <= 0x1C2000 && address >= 0x1000 ) //*4096
    {
       // return false;
    }

    uint32_t bytes_written;
    uint32_t feasible_size = size;
    uint32_t currentAddress = address;
    uint32_t currentEndOfPage = ( currentAddress / FLASH_DEVICE_PAGE_SIZE + 1 ) * FLASH_DEVICE_PAGE_SIZE - 1;
    uint32_t bytes_left_to_send;
    if( size > ( FLASH_DEVICE_SIZE - address ) )feasible_size = FLASH_DEVICE_SIZE - address;
    bytes_left_to_send = feasible_size;
    bytes_written = 0;
    while ( bytes_written < feasible_size )
    {
        if ( currentAddress + bytes_left_to_send > currentEndOfPage ) bytes_left_to_send = currentEndOfPage - currentAddress + 1;
        if ( flash_device_write_page( wr_data_ptr + bytes_written, currentAddress, bytes_left_to_send ) != true )
            return false;
        bytes_written     += bytes_left_to_send;
        currentAddress     = currentEndOfPage + 1;
        currentEndOfPage  += FLASH_DEVICE_PAGE_SIZE;
        bytes_left_to_send = feasible_size - bytes_written;
    }
    return bytes_written;
}

bool flash_device_erase_chip( void )
{
    flash_device_power_down_set( false );
    flash_device_write_enable();
    flash_device_spi_status_set(0x02);
    flash_device_busy_wait();
    //LCM_CS_OUT_SET(1);
    flash_device_write_enable();
    spi_device_cs_set( 0 );
    spi_device_w_byte( W25X_ChipErase );
    spi_device_cs_set( 1 );
    return flash_device_busy_wait();
}

void flash_device_power_down_set( bool flag )
{
    //LCM_CS_OUT_SET(1);
    spi_device_cs_set( 0 );
    if( flag )
    {
        spi_device_w_byte( W25X_PowerDown );
    }
    else
    {
        spi_device_w_byte( W25X_ReleasePowerDown );
    }
    spi_device_cs_set( 1 );
    //  nrf_delay_us(10);
}
//

void app_flash_read( uint8_t *pdata, uint32_t address, uint16_t length )
{
   // LCM_CS_OUT_SET( 1 );
    gpio_config(0x07,OUTPUT,PULL_HIGH);
    gpio_set(0x07,1);
    flash_device_power_down_set( false );
    flash_device_read_data( pdata, address, length );
    flash_device_power_down_set( true );
    gpio_config(0x07,SC_FUN,PULL_NONE);
}
//

void app_flash_write( uint8_t *pdata, uint32_t address, uint16_t length )
{
    flash_device_power_down_set( false );
    
    flash_device_write_data( pdata, address, length );
    flash_device_power_down_set( true );
}
//#define CHINESE_FONT_BASE_ADDR   0X0000
//static void    chinese_font_from_flash_get(uint8_t *pstring,uint8_t *pcode)
//{ //16*16
//        uint8_t  code_high, code_low;
//        uint32_t offset;
//        code_high  =  *pstring ++;
//        code_low   =  *pstring;
//        if( ( code_high >= 0XB0 && code_high <= 0XF7 ) &&
//                    ( code_low  >= 0XA1 && code_low  <= 0XFE ) )
//                {
//                    code_high   -= 0XB0,   code_low  -= 0XA1;
//                        offset  =  (uint32_t)((code_high * 94 + code_low) * 32);
//                        flash_device_power_down_set(true);
//                        flash_device_read_data(pcode,CHINESE_FONT_BASE_ADDR+offset,32);
//                        flash_device_power_down_set(false);
//                }
//                else
//                {
//                    memset(pcode,0x00,32);
//                }
//}

