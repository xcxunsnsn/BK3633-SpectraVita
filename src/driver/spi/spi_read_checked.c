#include <stddef.h>
#include "spi.h"
#include "icu.h"
#include "BK3633_RegList.h"

extern spi_param_t spi_param;

/*
 * The BK3633 polling RX path can return before a long transfer has filled the
 * caller's buffer.  Flash deliberately calls this interface with <=64 bytes.
 * Any timeout aborts the hardware transaction and is reported to the caller.
 */
bool spi_read_checked(uint8_t *rbuf, uint32_t r_size)
{
    uint16_t idle_loops = 0;

    if (rbuf == NULL || r_size == 0 || r_size > 64U) return false;
    spi_waitbusying();
    SPI_REG0X1_CN = 0;
    SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_RX_FINISH_INT);
    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_SPI);

    while (SPI_REG0X2_STAT & (1<<POS_SPI_REG0X2_RXFIFO_RD_READY))
        (void)SPI_REG0X3_DAT;

    SPI_REG0X1_CN = (r_size<<POS_SPI_REG0X1_RX_TRANS_LEN) |
        (1<<POS_SPI_REG0X1_RX_EN) |
        (1<<POS_SPI_REG0X1_RX_FINISH_INT_EN);

    while (r_size != 0) {
        if (SPI_REG0X2_STAT & (1<<POS_SPI_REG0X2_RXFIFO_RD_READY)) {
            *rbuf++ = (uint8_t)(SPI_REG0X3_DAT & 0xFF);
            r_size--;
            idle_loops = 0;
        } else if (++idle_loops > 30000U) {
            SPI_REG0X1_CN = 0;
            SPI_REG0X2_STAT |= (1<<POS_SPI_REG0X2_RX_FINISH_INT);
            SYS_REG0X10_INT_EN &= ~(0x01 << POS_SYS_REG0X10_INT_EN_SPI);
            spi_param.spi_state = 1;
            return false;
        }
    }
    return true;
}
