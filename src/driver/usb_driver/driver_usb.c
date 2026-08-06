/*************************************************************
 * @file        driver_usb.c
 * @brief        HW driver of Beken Usb(Mentor Graphics)
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver_usb.h"
#include "usbdef.h"
#include "app.h"

/*
extern int bim1_uart_printf(const char * fmt,...);
#define DBG_PRINT(fmt...)    \
    {char __buf[128];sprintf(__buf,##fmt);bim1_uart_printf(__buf,strlen(__buf));}
#define PRT(var)    bim1_uart_printf(#var##"=%.8x\r\n",var)
*/

extern void HalUsbSetupHandle(void*setupPkt,CEndpoint *endp);
extern uint8 bUsbState;
extern CSuspendMode suspend_status;

CEndpoint ep0;
//CUsbSetupPkt setupPkt;

void print(void * msg,void*dat, int len);
void DelayNops_usb(volatile unsigned long nops)
{
    while (nops --)
    {
    }
}

void HwUsb_SetRxIE(uint16 ie){
    REG_USB_INTRRX1E=(ie&0xff);
    REG_USB_INTRRX2E=((ie>>8)&0xff);

}

uint32 HwUsb_GetRxIE(void){
    uint32 r;
    r=(REG_USB_INTRRX1E);
    r|=((REG_USB_INTRRX2E)<<8);
    return(r);
}
uint32 HwUsb_GetRxIF(void){
    uint32 r;
    r=(REG_USB_INTRRX1);
    r|=((REG_USB_INTRRX2)<<8);
    return(r);
}

void HwUsb_SetTxIE(uint16 ie){
    (REG_USB_INTRTX1E)=(ie&0xff);
    (REG_USB_INTRTX2E)=((ie>>8)&0xff);

}
uint32 HwUsb_GetTxIF(void){
    uint32 r;
    r=(REG_USB_INTRTX1);
    r|=((REG_USB_INTRTX2)<<8);
    return(r);
}

uint32 HwUsb_GetTxIE(void){
    uint32 r;
    r=(REG_USB_INTRTX1E);
    r|=((REG_USB_INTRTX2E)<<8);
    return(r);
}

void HwUsb_Switch2Endp(int endpn){
    REG_USB_INDEX=endpn;
}

uint32 HwUsb_GetCSR(void){
    uint32 r;
    r=REG_USB_CSR0;
    r|=(REG_USB_CSR02<<8);
    return(r);
}
uint32 HwUsb_GetRxCount(void){
    uint32 r;
    r=REG_USB_COUNT0;
    r|=(REG_USB_RXCOUNT2<<8);
    return(r);
}

#define _Read8(_offset) *((volatile uint8 *)(REG_USB_BASE_ADDR + _offset))
#define _Read32(_offset) *((volatile uint32 *)(REG_USB_BASE_ADDR + _offset))
#define _Write8(_offset, _data) (*((volatile uint8 *)(REG_USB_BASE_ADDR + _offset)) = _data)
#define _Write32(_offset, _data) (*((volatile uint32 *)(REG_USB_BASE_ADDR + _offset)) = _data)


uint32 HwUsb_EndpRead(int endpn,void*buf){
    uint32 cnt;
    uint32 val;
    uint8*p8=(uint8*)buf;
    HwUsb_Switch2Endp(endpn);
    cnt=HwUsb_GetRxCount();
    //os_printf("ep%d recved %d bytes\r\n",endpn,cnt);
    uint32 dwIndex, dwIndex32;
    uint32 dwCount32 = ((uint32)buf & 3) ? 0 : (cnt >> 2);
    uint8 bFifoOffset = 0x20+endpn*4;
//     if(cnt>64)return(0);
    for (dwIndex = dwIndex32 = 0; dwIndex32 < dwCount32; dwIndex32++, dwIndex += 4){
        val=_Read32(bFifoOffset);
        memcpy(&p8[dwIndex],&val,4);
//        *((uint32*)&(((char*)buf)[dwIndex])) = _Read32(bFifoOffset);
    }
    while (dwIndex < cnt){
        p8[dwIndex++] = _Read8(bFifoOffset);
//        ((char*)buf)[dwIndex++] = _Read8(bFifoOffset);
    }
    //print("rxpack",buf,cnt);
    if(endpn){
         REG_USB_RXCSR1=0;
//        REG_USB_RXCSR1|=BIT(4);//3288的usb fifo应该是有2级，如果进行flush fifo，那么所有fifo已接收到的信息都被清除
    }else{
        REG_USB_CSR0=0;
    }
    return(cnt);
}
//uint8 samp[0x40];
void HwUsb_EndpWrite(int endpn,void*buf,int len){
    uint32 dwIndex, dwIndex32;
    uint32 dwCount32 = ((uint32)buf& 3) ? 0 : (len >> 2);
    uint8 bFifoOffset = 0x20+endpn*4;//MGC_FIFO_OFFSET(bEnd);
    uint8 *p8=(uint8*)buf;
    uint32 val;
//    int i;
//    for(i=0;i<sizeof(samp);i++){
//        samp[i]=i+1;
//    }
//    memcpy(samp,buf,len);
    p8=(uint8*)buf;//samp;
    //print("tx packet",p8,len);
    /* doublewords when possible */
    for (dwIndex = dwIndex32 = 0; dwIndex32 < dwCount32; dwIndex32++, dwIndex += 4){
        memcpy(&val,&p8[dwIndex],4);
        _Write32(bFifoOffset, val);
//        _Write32(bFifoOffset, *((uint32*)&(((uint8*)buf)[dwIndex])));
    }
    while (dwIndex < len){
        _Write8(bFifoOffset, p8[dwIndex++]);
//        _Write8(bFifoOffset, ((uint8*)buf)[dwIndex++]);
    }
}



void HwUsb_SendStall(int endpn){
    HwUsb_Switch2Endp(endpn);
    REG_USB_CSR0=BIT(5);
}

void HwUsb_SendZlp(int endpn){
    HwUsb_Switch2Endp(endpn);
    REG_USB_CSR02=BIT(0);
    REG_USB_CSR0=BIT(1)|BIT(3);
}

/*
* 函数名:
*    HwUdpSendPkt
* 功能:
*    udp端点发送包
* 参数:
*    1.udp:usb设备指针
*    2.epn:端点号
*    3.endp:端点管理器
* 输出:
*    无
* 返回:
*    无
*/
void HwUsb_SendPkt(UI8 epn,CEndpoint*endp)  {
    PUI8 ptr;
    UI16 len;
    if(endp->status&USB_EndpStt_Tx){
        if(endp->txLen==0){
            endp->status^=USB_EndpStt_Tx;
            HwUsb_SendZlp(epn);
            UNS_SET(endp,UNS_StatusOut);
        }else{
            ptr=(PUI8)endp->txPtr;
            if(endp->txLen<endp->lmtLen){
                len=endp->txLen;
                endp->txLen=0;
                UNS_SET(endp,UNS_StatusOut);
            }else{
                len=endp->lmtLen;
                endp->txLen-= endp->lmtLen;
                UNS_SET(endp,UNS_DataIn);
            }
            //UNS_SET(endp,UNS_DataIn);
            endp->txPtr+=len;
            HwUsb_EndpWrite(epn, ptr, len);
            if(endp->txLen)REG_USB_CSR0=BIT(1);
            else
                REG_USB_CSR0=BIT(1)|BIT(3);
        }
    }
}

void AplUsb_StartTx(int epn,void*buf,int len){
    HwUsb_Switch2Endp(epn);
    REG_USB_CSR0=BIT(3);
    HwUsb_EndpWrite(epn, buf, len);
    REG_USB_CSR0=BIT(0);//csr1;
     REG_USB_CSR02|=BIT(5);
}

void HwUsb_SetAddress(int addr){
    REG_USB_FADDR=addr&0x7f;
}

extern void HwUsbOpen(void);
void usb_init_sw(){
    memset(&ep0,0,sizeof(ep0));
//    memset(&setupPkt,0,sizeof(setupPkt));
    SET_USBSTATE(USB_STATE_INIT);
    HwUsbOpen();
}

void usb_init(void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk)
{
    unsigned char ucUSBIntStatus;
    USB_MOD_CTRL_CBK ctrl_cbk=(USB_MOD_CTRL_CBK)usb_mod_ctrl_cbk;
    USB_MOD_IE_CBK ie_cbk=(USB_MOD_IE_CBK)usb_mod_int_ie_cbk;

    ctrl_cbk(1);//usb modal enable，使能USB模块

    REG_USB_INTRUSBE = 0x0;
    HwUsb_SetRxIE(0x0000);//default 0x001e
    HwUsb_SetTxIE(0x0000);//default 0x001f
    REG_AHB2_USB_VTH &= _BIT(7);//disable interrupt of VBUS changed

    HwUsb_SetRxIE(0x07);
    HwUsb_SetTxIE(0x07);

    REG_USB_INTRUSBE = 0x3F;//set USB bus interrupt
    REG_AHB2_USB_DMA_ENDP=0;
    REG_AHB2_USB_OTG_CFG = 0x08;        // dp pull up
    REG_AHB2_USB_DEV_CFG = 0xF4;//select controller
    REG_AHB2_USB_OTG_CFG |= 0x01;       // device

    DelayNops_usb(500);

    //clear VBUS-interrupt
    ucUSBIntStatus = REG_AHB2_USB_INT;
    DelayNops_usb(500);
    REG_AHB2_USB_INT = ucUSBIntStatus;
    DelayNops_usb(500);

    REG_AHB2_USB_GEN=BFD(7,4,4)|BFD(7,0,4);    //set D+\D-drive current
    //usb protocol init
    usb_init_sw();
//    config_device();

    REG_USB_FADDR=0;
    REG_USB_DEVCTL=BIT(0);//start device 'B'
    ie_cbk(1);//使能usb中断
    suspend_status.usb_frame_old = 0;
}

void usb_deinit(void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk){
    USB_MOD_CTRL_CBK ctrl_cbk=(USB_MOD_CTRL_CBK)usb_mod_ctrl_cbk;
    USB_MOD_IE_CBK ie_cbk=(USB_MOD_IE_CBK)usb_mod_int_ie_cbk;
    REG_USB_INTRRX1E = 0x0;
    REG_USB_INTRTX1E = 0x0;
    REG_USB_INTRUSBE = 0x0;
    ie_cbk(0);
//     ICU_INT_ENABLE_CLEAR(ICU_INT_ENABLE_IRQ_USB_MASK);
    REG_AHB2_USB_GEN = 0;
    REG_AHB2_USB_OTG_CFG = 0;
//     ICU_USB_PWD_SET();
    ctrl_cbk(0);
}

// extern void *AplUsb_GetRxBuf(int endp_no);
// extern void *AplUsb_GetTxBuf(int endp_no);
// extern void *AplUsb_GetTxCbk(int endp_no);
// extern void *AplUsb_GetRxCbk(int endp_no);

int AplUsb_GetSOF(void){
    return(REG_USB_FRAME1+(REG_USB_FRAME2<<8));
}

//void print(void*msg,void*dat,int len){
//    char*ptr=(char*)dat;
//    int i;
//    DBG_PRINT(msg);
//    DBG_PRINT("[%d]={\r\n\t",len);
//    for(i=0;i<len;i++){
//        DBG_PRINT("%.2x ",ptr[i]);
//        if((i&0x07)==0x07)DBG_PRINT("\r\n\t");
//    }
//    DBG_PRINT("\r\n}\r\n");
//}

void _ep0_isr(){
    int csr,cnt;
    char buf[0x40];

    UI16 setup_tmp[4];
    HwUsb_Switch2Endp(0);
    csr=HwUsb_GetCSR();
    if(csr&BIT(0)){//RxPacket Ready
        cnt=HwUsb_EndpRead(0, buf);
        if(cnt>0){
            memcpy(setup_tmp,buf,cnt);
            HalUsbSetupHandle(setup_tmp,&ep0);
        }else{
            HwUsb_SendPkt(0, &ep0);
        }
    }else if(csr&BIT(4)){
        REG_USB_CSR0=BIT(7);
        UNS_SET(&ep0, UNS_Complete);
    }else{
        if(UNS_GET(&ep0)==UNS_DataIn){
            HwUsb_SendPkt(0, &ep0);
        }else{
            UNS_SET(&ep0, UNS_Complete);
        }
    }
}

void _endp_tx(int endpn){
    char*txbuf;
    //int sz;
    HwUsb_Switch2Endp(endpn);
    //UI16 csr1=HwUsb_GetCSR();
    txbuf=(char*)AplUsb_GetTxBuf(endpn);
    CBK_USBAPP cbk=(CBK_USBAPP)AplUsb_GetTxCbk(endpn);
    if(cbk)cbk(txbuf,0);
}
extern int uart2_printf(const char * fmt,...);
extern void gpio_set_neg(uint8_t gpio);

#define EPx_DATAERROR   BIT(3)
#define TEST_PIN2               0x03

void _endp_rx(int endpn){
    char*rxBuf;
    int sz;
    UI16 csr1;
    HwUsb_Switch2Endp(endpn);
    csr1=(REG_USB_RXCSR2<<8)|REG_USB_RXCSR1;
    if(csr1&EPx_DATAERROR)
         gpio_set_neg(TEST_PIN2);
    rxBuf=(char*)AplUsb_GetRxBuf(endpn);
    sz=HwUsb_EndpRead(endpn, rxBuf);
    CBK_USBAPP cbk=(CBK_USBAPP)AplUsb_GetRxCbk(endpn);
    if(cbk)cbk(rxBuf,sz);
    //uart2_printf("%d %.4x %d\r\n",endpn,csr1,sz);
}

void USB_IsrDispatch(int rxIF,int txIF,int usbIF){
    int i;
    if(txIF&BIT(0)){
        _ep0_isr();
        return;
    }
    for(i=1;i</*16*/16;i++){
        if(txIF&BIT(i)){
            _endp_tx(i);
#if 0
            char*txbuf;
            int sz;
            HwUsb_Switch2Endp(i);
            UI16 csr1=HwUsb_GetCSR();
//             REG_USB_CSR02&=_BIT(5);
            txbuf=(char*)AplUsb_GetTxBuf(i);
            CBK_USBAPP cbk=(CBK_USBAPP)AplUsb_GetTxCbk(i);
            if(cbk)cbk(txbuf,0);
            //bim1_uart_printf("ep%d csr1=%.4x\r\n",i,csr1);
#endif
            //tx packet end
        }
        if(rxIF&BIT(i)){
            //rx packet ready
            _endp_rx(i);
            #if 0
            UI8 bcsr1=REG_USB_RXCSR1;
            if(bcsr1&BIT(0)){
                char*rxBuf;
                int sz;
                rxBuf=(char*)AplUsb_GetRxBuf(i);
                sz=HwUsb_EndpRead(i, rxBuf);
//                 static int cnt=0;
//                 uart_send(&cnt,1);
//                 uart_send(&sz,1);
//                 cnt++;
                CBK_USBAPP cbk=(CBK_USBAPP)AplUsb_GetRxCbk(i);
                if(cbk)cbk(rxBuf,sz);
            }
            #endif
        }
    }
    if(usbIF){
        if(usbIF&BIT(0)){//suspend
            REG_USB_POWER=1;//enable suspend
            REG_USB_DEVCTL=0;//stop PHY CLKOUT,save power,soft disconnect from host
            extern CBK_USB_SUSPEND cbk_suspend;
            cbk_suspend();
        }
        if(usbIF&BIT(1)){//resume
            REG_USB_POWER=0;//end suspend
            REG_USB_DEVCTL=1;//start device
        }

        if(usbIF&BIT(2)){//bus reset
            REG_USB_FADDR=0;
            //usb_init_sw();
            extern volatile UI8 bUsbConfig;
            bUsbConfig=0;
        }
        if(usbIF&BIT(3)){//sof
        }
    }
}

void USB_InterruptHandler(void)
{
    uint8 bIntrUsbValue = REG_USB_INTRUSB;
    uint16 wIntrTxValue ;
    uint16 wIntrRxValue ;
    uint8 bIntrUsbIndex = REG_USB_INDEX;

    wIntrTxValue=HwUsb_GetTxIF();
    wIntrRxValue=HwUsb_GetRxIF();

    USB_IsrDispatch(wIntrRxValue, wIntrTxValue, bIntrUsbValue);
    REG_USB_INDEX = bIntrUsbIndex;
}
