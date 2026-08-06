#ifndef __W25X40C__H
#define __W25X40C__H

#include  "gpio.h"

#include <stdint.h>
#include <stdbool.h>



#define  SPI_PRO_PIN     0x12
#define  SPI_CS_PIN      0x13
#define  SPI_SCLK_PIN    0x04
#define  SPI_MOSI_PIN    0x05
#define  SPI_MISO_PIN    0x06 


//#define LCM_RES_OUT_SET(value)             (value)?( REG_APB5_GPIOB_DATA |= 0x00000008):(REG_APB5_GPIOB_DATA &= ~0x00000008) //gpio_set(LCM_RES_PORT,value)
//#define LCM_POWER_CONTROL_OUT_SET(value)    (value)?(REG_APB5_GPIOB_DATA |= 0x00000001):(REG_APB5_GPIOB_DATA &= ~0x00000001)//gpio_set(LCM_POWER_CONTROL_PORT,value)
//#define LCM_IMITATE_SA0_OUT_SET(value)      (value)?(REG_APB5_GPIOA_DATA |= 0x00000008):(REG_APB5_GPIOA_DATA &= ~0x00000008)//gpio_set(LCM_SA0_PORT,value)


//#define  spi_device_cs_set(cs)     (cs)?( REG_APB5_GPIOD_DATA |= 0x00000008):(REG_APB5_GPIOD_DATA &= ~0x00000008)
#define  spi_device_cs_set(cs)      gpio_set(SPI_CS_PIN,cs)
#define  spi_device_sclk_set(sclk)  gpio_set(SPI_SCLK_PIN,sclk)
#define  spi_device_mosi_set(mosi)  gpio_set(SPI_MOSI_PIN,mosi)
#define  spi_device_miso_read()     gpio_get_input(SPI_MISO_PIN)

#define  FLASH_25Q40B             (4000000)


#define  FLASH_SIZE_NKB(N)        (N * 1024)
#define  FLASH_SIZE_BYTES         (FLASH_25Q40B/8)
#define  FLASH_DEVICE_SIZE        (FLASH_SIZE_BYTES * FLASH_SIZE_NKB(1))
#define  FLASH_DEVICE_SECTOR_SIZE FLASH_SIZE_NKB(4)
#define  FLASH_DEVICE_PAGE_SIZE   (256)

#define  FLASH_TIMEOUT_VALUE      (50000)

#define W25X10     0XE010
#define W25X20     0XE011
#define W25X40  0XE012
#define W25X80     0XE013

//Ö¸Áî±í
#define  W25X_WriteEnable          0x06 
#define  W25X_WriteDisable          0x04 
#define  W25X_ReadStatusReg          0x05             //¶ÁÈ¡×´Ì¬¼Ä´æÆ÷
#define  W25X_WriteStatusReg      0x01 
#define  W25X_ReadData              0x03 
#define  W25X_FastReadData          0x0B 
#define  W25X_FastReadDual          0x3B 
#define  W25X_PageProgram          0x02 
#define  W25X_BlockErase          0xD8 
#define  W25X_SectorErase          0x20             //ÉÈÇø²Á³ýÖ¸Áî
#define  W25X_ChipErase              0xC7      //ÕûÆ¬²Á³ýÖ¸Áî
#define  W25X_PowerDown              0xB9 
#define  W25X_ReleasePowerDown    0xAB 
#define  W25X_DeviceID              0xAB 
#define  W25X_ManufactDeviceID      0x90 
#define  W25X_JedecDeviceID          0x9F 

void     flash_device_spi_init(void);
bool     flash_device_erase_chip(void);
void     flash_device_erase_sector(uint32_t flas_sector);
bool     flash_device_write_page(uint8_t* pBuffer,uint32_t WriteAddr,uint32_t NumByteToWrite);
int32_t  flash_device_write_data(uint8_t *wr_data_ptr, uint32_t address, uint32_t size);
void     flash_device_read_data(uint8_t* pBuffer,uint32_t ReadAddr,uint32_t NumByteToRead);
uint32_t flash_device_deviceID_get(uint8_t *p_flashID);
uint16_t flash_device_chipID_get(void);
void     flash_device_power_down_set(bool flag);
void     app_flash_read(uint8_t *pdata,uint32_t address,uint16_t length);
void     app_flash_write(uint8_t *pdata,uint32_t address,uint16_t length);
#endif
