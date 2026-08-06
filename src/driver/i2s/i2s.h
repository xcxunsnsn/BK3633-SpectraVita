#ifndef        __I2S_H__
#define        __I2S_H__
#define TEST_DATA_LEN          48


#define        I2S_CLK                    16000000

#define    I2s_48000_CLK            393216000/8/4
#define    I2s_44100_CLK            361267200/8/4
#define    I2s_16000_CLK            393216000/8/4
#define    I2s_8000_CLK             393216000/8/4

#define        PHILIPS          0


#define        OPEN_INT        1
#define   CLOSE_INT   0
#define        MASTER            1
#define        SLAVE              0

#define        INVT_POL    1

#define        I2S_TXBUF_LESS_1   0
#define        I2S_TXBUF_LESS_8   1
#define        I2S_TXBUF_LESS_16  2
#define        I2S_TXBUF_LESS_24  3

#define        I2S_RXBUF_MORE_1   0
#define        I2S_RXBUF_MORE_8   1
#define        I2S_RXBUF_MORE_16  2
#define        I2S_RXBUF_MORE_24  3

#define        I2S_DATA_LEN32         31
#define        I2S_DATA_LEN16         15

typedef struct i2s_message
{
    unsigned char    rxfifo_level;
    unsigned char    txfifo0_level;
    //unsigned char    txfifo1_level;
    //unsigned char    txfifo2_level;
    unsigned char    trans_done;
    unsigned long    remain_data_cnt;
} i2s_message;


typedef    struct{
    uint8_t     i2s_enable        ;
    uint8_t        master_nslave    ;
    uint8_t        i2s_mode            ;
    uint8_t        lrck_pol            ;
    uint8_t        sck_pol                ;
    uint8_t     lsb_first     ;
    uint8_t     sync_len          ;
    uint8_t     data_len      ;
    uint8_t     pcm_d_len     ;
    uint16_t    samp_radio        ;
    uint16_t    bit_radio     ;


    uint8_t        txint_levl        ;
    uint8_t        rxint_levl        ;
    uint8_t        txint_dnov_en ;
    uint8_t     rxint_upov_en ;
    uint8_t        txint_en      ;
    uint8_t     rxint_en      ;


} I2S_PARAM ;



typedef        struct{
    uint16_t    total_tlen    ;
    uint16_t    remain_tlen;
    uint16_t    total_rlen ;
    uint16_t    remain_rlen;
    uint32_t    txstart_p;
    uint32_t    rxstart_p;
    uint8_t     tx_done;
    uint8_t     rx_done;


} I2S_TRX_FLAG ;

extern void i2s_isr(void) ;
extern void test_i2s(uint8_t mode) ;
extern void I2S_Test(unsigned char role);    //0: Slave; 1: Master
extern void i2s_init(unsigned long smp_ratio, unsigned char role, unsigned char data_width,
                  unsigned char mode_sel, unsigned long data_cnt);

#endif
