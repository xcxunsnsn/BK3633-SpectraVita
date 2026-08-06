/**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief UART Driver for HCI over UART operation.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef _UART_H_
#define _UART_H_

/**
 ****************************************************************************************
 * @defgroup UART UART
 * @ingroup DRIVERS
 * @brief UART driver
 *
 * @{
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions
#include "user_config.h"      ////


#define BASEADDR_UART                                      0x00806300

#define UART_REG0X0                                        *((volatile unsigned long *) (BASEADDR_UART+0x0*4))
#define POS_UART_REG0X0_TX_ENABLE                          0
#define POS_UART_REG0X0_RX_ENABLE                          1
#define POS_UART_REG0X0_IRDA                               2
#define POS_UART_REG0X0_LEN                                3
#define POS_UART_REG0X0_PAR_EN                             5
#define POS_UART_REG0X0_PAR_MODE                           6
#define POS_UART_REG0X0_STOP_LEN                           7
#define POS_UART_REG0X0_CLK_DIVID                          8


#define UART_REG0X1                                         *((volatile unsigned long *) (BASEADDR_UART+0x1*4))
#define POS_UART_REG0X1_TX_FIFO_THRESHOLD                   0
#define POS_UART_REG0X1_RX_FIFO_THRESHOLD                   8
#define POS_UART_REG0X1_RX_STOP_DETECT_TIME                 16


#define UART_REG0X2                                         *((volatile unsigned long *) (BASEADDR_UART+0x2*4))
#define POS_UART_REG0X2_TX_FIFO_COUNT                       0
#define POS_UART_REG0X2_RX_FIFO_COUNT                       8
#define POS_UART_REG0X2_TX_FIFO_FULL                        16
#define POS_UART_REG0X2_TX_FIFO_EMPTY                       17
#define POS_UART_REG0X2_RX_FIFO_FULL                        18
#define POS_UART_REG0X2_RX_FIFO_EMPTY                       19
#define POS_UART_REG0X2_FIFO_WR_READY                       20
#define POS_UART_REG0X2_FIFO_RD_READY                       21

#define UART_REG0X3                                         *((volatile unsigned long *) (BASEADDR_UART+0x3*4))
#define POS_UART_REG0X3_UART_TX_FIFO_DIN                    0
#define POS_UART_REG0X3_UART_RX_FIFO_DOUT                   8


#define UART_REG0X4                                         *((volatile unsigned long *) (BASEADDR_UART+0x4*4))
#define POS_UART_REG0X4_TX_FIFO_NEED_WRITE_MASK             0
#define POS_UART_REG0X4_RX_FIFO_NEED_READ_MASK              1
#define POS_UART_REG0X4_RX_FIFO_OVER_FLOW_MASK              2
#define POS_UART_REG0X4_UART_RX_PARITY_ERR_MASK             3
#define POS_UART_REG0X4_UART_RX_STOP_ERR_MASK               4
#define POS_UART_REG0X4_UART_TX_STOP_END_MASK               5
#define POS_UART_REG0X4_UART_RX_STOP_END_MASK               6
#define POS_UART_REG0X4_UART_RXD_WAKEUP_MASK                7

#define UART_REG0X5                                         *((volatile unsigned long *) (BASEADDR_UART+0x5*4))
#define POS_UART_REG0X5_TX_FIFO_NEED_WRITE                  0
#define POS_UART_REG0X5_RX_FIFO_NEED_READ                   1
#define POS_UART_REG0X5_RX_FIFO_OVER_FLOW                   2
#define POS_UART_REG0X5_UART_RX_PARITY_ERROR                3
#define POS_UART_REG0X5_UART_RX_STOP_ERROR                  4
#define POS_UART_REG0X5_UART_TX_STOP_END                    5
#define POS_UART_REG0X5_UART_RX_STOP_END                    6
#define POS_UART_REG0X5_UART_RXD_WAKEUP                     7

#define UART_REG0X6                                         *((volatile unsigned long *) (BASEADDR_UART+0x6*4))
#define POS_UART_REG0X6_FLOW_CTL_LOW_CNT                    0
#define POS_UART_REG0X6_FLOW_CTL_HIGH_CNT                   8
#define POS_UART_REG0X6_FLOW_CONTROL_ENA                    16
#define POS_UART_REG0X6_RTS_POLARITY_SEL                    17
#define POS_UART_REG0X6_CTS_POLARITY_SEL                    18

#define UART_REG0X7                                         *((volatile unsigned long *) (BASEADDR_UART+0x7*4))
#define POS_UART_REG0X7_UART_WAKE_COUNT                     0
#define POS_UART_REG0X7_UART_TXD_WAIT_CNT                   10
#define POS_UART_REG0X7_UART_RXD_WAKE_EN                    20
#define POS_UART_REG0X7_UART_TXD_WAKE_EN                    21
#define POS_UART_REG0X7_RXD_NEGEDGE_WAKE_EN                 22




#define UART_FIFO_MAX_COUNT  128

#define UART_FIFO_WR_ADDR   0x0080630C

typedef enum _UART_CMD_STATE
{
    
    UART_CMD_STATE_HEAD,
    UART_CMD_STATE_OPCODE_ONE,
    UART_CMD_STATE_OPCODE_TWO,
    UART_CMD_STATE_LENGTH,
    UART_CMD_STATE_CMD,
    UART_CMD_STATE_CMD_FLASH,
    UART_CMD_STATE_LENGTH_FLASH_LEN0,
    UART_CMD_STATE_LENGTH_FLASH_LEN1,
    UART_CMD_STATE_LENGTH_FLASH_SCMD,
    UART_CMD_STATE_PAYLOAD,
    UART_CMD_STATE_ERROR_ONE,
    UART_CMD_STATE_ERROR_TWO,
    UART_CMD_STATE_ERROR_THREE,
    UART_CMD_STATE_ERROR_FOUR,
    UART_CMD_STATE_PACKET
    
} UART_CMD_STATE;


enum
{
    UART_LINK_CHECK       = 0x00,
    REG_WRITE_CMD         = 0x01,
    REG_READ_CMD          = 0x03,
    CMD_SYS_RESET         = 0xFE,
    CMD_DEBUG_DAT         = 0XAA,
};




#if (VIRTUAL_UART_H4TL == 1)
////#define UART_TXRX_BUF_SIZE        128
#define HCI_DATA_BUF_SIZE        64            ////256        ////128
#define HCI_DATA_TYPE_CMD        0x01
#define HCI_DATA_TYPE_EVENT        0x02
#endif
/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */



/*
 * STRUCT DEFINITIONS
 *****************************************************************************************
 */
/* TX and RX channel class holding data used for asynchronous read and write data
 * transactions
 */
/// UART TX RX Channel
struct uart_txrxchannel
{
    /// call back function pointer
    void (*callback) (void*, uint8_t);
    /// Dummy data pointer returned to callback when operation is over.
    void* dummy;
};

/// UART environment structure
struct uart_env_tag
{
    /// tx channel
    struct uart_txrxchannel tx;
    /// rx channel
    struct uart_txrxchannel rx;
    /// error detect
    uint8_t errordetect;
    /// external wakeup
    bool ext_wakeup;
    //// Modified
    uint8_t *uart_tx_buf;
    uint8_t *uart_rx_buf;
    uint32_t uart_tx_length;
    uint32_t uart_rx_length;
    uint8_t uart_tx_enable;        ////Maybe no need
    uint8_t uart_rx_enable;        ////Maybe no need
};
#if (VIRTUAL_UART_H4TL == 1)

//// HCI CMD, Event
struct hci_cmd_event_data
{
    /// call back function pointer
    void (*callback) (void*, uint16_t);
    /// Dummy data pointer returned to callback when operation is over.
    uint8_t data_buf[HCI_DATA_BUF_SIZE];
    uint32_t data_len;
};
#endif















#ifndef CFG_ROM
/**
 ****************************************************************************************
 * @brief Enable UART flow.
 *****************************************************************************************
 */
void uart_flow_on(void);

/**
 ****************************************************************************************
 * @brief Disable UART flow.
 *****************************************************************************************
 */
bool uart_flow_off(void);
#endif //CFG_ROM

/**
 ****************************************************************************************
 * @brief Finish current UART transfers
 *****************************************************************************************
 */
void uart_finish_transfers(void);

/**
 ****************************************************************************************
 * @brief Starts a data reception.
 *
 * @param[out] bufptr   Pointer to the RX buffer
 * @param[in]  size     Size of the expected reception
 * @param[in]  callback Pointer to the function called back when transfer finished
 * @param[in]  dummy    Dummy data pointer returned to callback when reception is finished
 *****************************************************************************************
 */
void uart_read(uint8_t *bufptr, uint32_t size, void (*callback) (void*, uint8_t), void* dummy);

/**
 ****************************************************************************************
 * @brief Starts a data transmission.
 *
 * @param[in] bufptr   Pointer to the TX buffer
 * @param[in] size     Size of the transmission
 * @param[in] callback Pointer to the function called back when transfer finished
 * @param[in] dummy    Dummy data pointer returned to callback when transmission is finished
 *****************************************************************************************
 */
void uart_write(uint8_t *bufptr, uint32_t size, void (*callback) (void*, uint8_t), void* dummy);

#if defined(CFG_ROM)
/**
 ****************************************************************************************
 * @brief Poll UART on reception and transmission.
 *
 * This function is used to poll UART for reception and transmission.
 * It is used when IRQ are not used to detect incoming bytes.
 *****************************************************************************************
 */
void uart_poll(void);
#endif //CFG_ROM

/**
 ****************************************************************************************
 * @brief Serves the data transfer interrupt requests.
 *
 * It clears the requests and executes the appropriate callback function.
 *****************************************************************************************
 */

void hci_data_init(uint8_t type);
void poki_hci_test_cmd(void);
void set_app_role(uint8_t rol);
void host_get_event_cbReg(void (*callback) (void*, uint16_t));
void uart_init(uint32_t baudrate);
void host_send_cmd(uint8_t *bufptr, uint16_t length);
void uart_h4tl_data_switch(void);
void uart_send(void *buff, uint16_t len);
void uart_isr(void);
void uart_rx_handler(uint8_t value);
uint8_t uart_download_check(void);
int uart0_printf(const char *fmt,...);
void uart1_dma_write(uint8_t *buffer,uint16_t buffer_len);

#endif /* _UART_H_ */
