
#ifndef __I2C_H__
#define __I2C_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>        // standard integer definition
#include <string.h>        // string manipulation
#include <stddef.h>        // standard definition
#include "BK_HCI_Protocol.h"

#define REG_APB4_I2C_CN                 (*((volatile unsigned long *)   0x00806400))
#define REG_APB4_I2C_STAT               (*((volatile unsigned long *)   0x00806404))
#define REG_APB4_I2C_DAT                (*((volatile unsigned long *)   0x00806408))

#define REG_APB5_GPIOA_CFG              (*((volatile unsigned long *)   0x00806500))
#define REG_APB5_GPIOA_DATA             (*((volatile unsigned long *)   0x00806504))

#define I2C_STATUS_START            (0x1 << 10)
#define I2C_STATUS_STOP            (0x1 << 9)
#define I2C_STATUS_SI                (0x1 << 0)
#define I2C_STATUS_BUSY            (0x1 << 15)




#define BIT_I2C_ENSMB                31
#define BIT_I2C_INH                    30
#define BIT_I2C_SMBFTE                29
#define BIT_I2C_SMBTOE                28
#define BIT_I2C_SMBCS                26
#define BIT_I2C_SLV_ADDR                16
#define BIT_I2C_FREQ_DIV                6
#define BIT_I2C_SCL_CR                3
#define BIT_I2C_IDLE_CR                0


#define I2C_CONFIG_I2C_ENABLE_POSI    31
#define I2C_CONFIG_I2C_ENABLE_MASK    (0x01UL << I2C_CONFIG_I2C_ENABLE_POSI)
#define I2C_CONFIG_I2C_ENABLE_SET     (0x01UL << I2C_CONFIG_I2C_ENABLE_POSI)

#define I2C_CONFIG_SMBTOE_POSI         28
#define I2C_CONFIG_SMBTOE_MASK         (0x01UL << I2C_CONFIG_SMBTOE_POSI)
#define I2C_CONFIG_SMBTOE_SET          (0x01UL << I2C_CONFIG_SMBTOE_POSI)

#define I2C_CONFIG_SMBFTE_POSI         29
#define I2C_CONFIG_SMBFTE_MASK         (0x01UL << I2C_CONFIG_SMBFTE_POSI)
#define I2C_CONFIG_SMBFTE_SET          (0x01UL << I2C_CONFIG_SMBFTE_POSI)

#define I2C_CONFIG_INH_POSI            30
#define I2C_CONFIG_INH_MASK            (0x01UL << I2C_CONFIG_INH_POSI)
#define I2C_CONFIG_INH_CLEAR           (0x00UL << I2C_CONFIG_INH_POSI)
#define I2C_CONFIG_INH_SET             (0x01UL << I2C_CONFIG_INH_POSI)

#define I2C_CONFIG_IDLE_CR_POSI        0
#define I2C_CONFIG_IDLE_CR_MASK        (0x07UL << I2C_CONFIG_IDLE_CR_POSI)
#define I2C_CONFIG_IDLE_CR_DEFAULT     (0x03UL << I2C_CONFIG_IDLE_CR_POSI)

#define I2C_CONFIG_SCL_CR_POSI         3
#define I2C_CONFIG_SCL_CR_MASK         (0x07UL << I2C_CONFIG_SCL_CR_POSI)
#define I2C_CONFIG_SCL_CR_DEFAULT      (0x07UL << I2C_CONFIG_SCL_CR_POSI)

#define I2C_CONFIG_FREQ_DIV_POSI       6
#define I2C_CONFIG_FREQ_DIV_MASK       (0x03FFUL << I2C_CONFIG_FREQ_DIV_POSI)

#define I2C_CONFIG_SLAVE_ADDR_POSI     16
#define I2C_CONFIG_SLAVE_ADDR_MASK     (0x03FFUL << I2C_CONFIG_SLAVE_ADDR_POSI)

#define I2C_CONFIG_CLOCK_SEL_POSI      26
#define I2C_CONFIG_CLOCK_SEL_MASK      (0x03UL << I2C_CONFIG_CLOCK_SEL_POSI)
#define I2C_CONFIG_CLOCK_SEL_TIMER0    (0x00UL << I2C_CONFIG_CLOCK_SEL_POSI)
#define I2C_CONFIG_CLOCK_SEL_TIMER1    (0x01UL << I2C_CONFIG_CLOCK_SEL_POSI)
#define I2C_CONFIG_CLOCK_SEL_TIMER2    (0x02UL << I2C_CONFIG_CLOCK_SEL_POSI)
#define I2C_CONFIG_CLOCK_SEL_FREQ_DIV  (0x03UL << I2C_CONFIG_CLOCK_SEL_POSI)

#define I2C_STATUS_INT_MODE_POSI       6
#define I2C_STATUS_INT_MODE_MASK       (0x03UL << I2C_STATUS_INT_MODE_POSI)

#define I2C_STATUS_ACK_POSI            8
#define I2C_STATUS_ACK_MASK            (0x01UL << I2C_STATUS_ACK_POSI)
#define I2C_STATUS_ACK_SET             (0x01UL << I2C_STATUS_ACK_POSI)

#define I2C_STATUS_STOP_POSI           9
#define I2C_STATUS_STOP_MASK           (0x01UL << I2C_STATUS_STOP_POSI)
#define I2C_STATUS_STOP_SET            (0x01UL << I2C_STATUS_STOP_POSI)

#define I2C_STATUS_START_POSI          10
#define I2C_STATUS_START_MASK          (0x01UL << I2C_STATUS_START_POSI)
#define I2C_STATUS_START_SET           (0x01UL << I2C_STATUS_START_POSI)

//#define ICU_I2C_CLK_PWD_CLEAR()         do {REG_AHB0_ICU_I2CCLKCON = 0x00;} while (0)
//#define ICU_I2C_CLK_PWD_SET()           do {REG_AHB0_ICU_I2CCLKCON = 0x01;} while (0)


#define I2C_DEFAULT_SLAVE_ADDRESS     (0x50)// (0x72)

#define I2C_ENABLE                        0

#define I2C_MASTER              1
#define I2C_SLAVE               0
#define I2C_MODE                I2C_MASTER
typedef enum        // by gwf
{
    OK = 0,
    ERROR = -1
} STATUS;

typedef enum        // by gwf
{
    NO = 0,
    YES = 1
} ASK;
void i2c_init(uint32_t slaveAddr, uint32_t baudRate);
STATUS i2c_write(uint8_t devAddr, uint8_t addr, uint8_t*buf, uint8_t size);
STATUS i2c_read(uint8_t devAddr, uint8_t addr, uint8_t*buf, uint8_t size);
void i2c_isr(void);
void test_fram_iic(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __I2C_H__ */


