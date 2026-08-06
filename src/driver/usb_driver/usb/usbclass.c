/*************************************************************
 * @file        usbclass.c
 * @brief        HW driver of Beken Usb(Mentor Graphics)
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */


/*internal symbols import section*/
#include "usbclass.h"
#include "usbdef.h"
#include "usb.h"
/*external symbols import section*/

/*variable implement section*/

UI8 bUsbState ;

extern UI8 ep0PktMaxSz;
extern UI8 GetRqtRecipient(void * setupPkt);
extern void HwUsb_SendStall(int endpn);
extern void HwUsb_SendZlp(int endpn);
//extern void SetRxParameter(CEndpoint * endp, void * setupPkt, void * dat, UI32 sz)
/*
 *函数名:
 *    SetRxParameter
 *功能:
 *    设定端点接收参数
 *参数:
 *    1.endp:端点管理器
 *    2.setupPkt:setup包
 *    3.dat:接收缓冲区指针
 *    4.sz:接收缓冲区长度
 *返回:
 *    无
 *特殊:
 *    1.目前只支持小于8bytes的数据接收，大于8的需重新定义
*/
//static
void SetRxParameter(CEndpoint*endp,void*setupPkt,void*dat,UI32 sz) {
    UI16*ptr=(UI16*)setupPkt;
    UI16 rlen=ptr[3];
    endp->lmtLen=ep0PktMaxSz;
    endp->rxPtr=dat;
    endp->rxLen=rlen;//sz;
    endp->status|=USB_EndpStt_Rx;
}

/*
 *函数名:
 *    HalUsbClassRequest
 *功能:
 *    USB标准请求处理函数
 *参数:
 *    无
 *返回:
 *    无
 *特殊:
 *    1.改变:
 *    2.stack:
*/
void HalUsbClassRequest(void*setupPkt,CEndpoint*endp) {
    UI8 *ptr=(UI8*)setupPkt;
    UI8 rcp=GetRqtRecipient(setupPkt);
    UNIT tmp;
    if((rcp>=USB_RT_RECIPIENT_OTHER)||
        (rcp==USB_RT_RECIPIENT_DEVICE)){
        HwUsb_SendStall(0);
        return;
    }
    if(rcp==USB_RT_RECIPIENT_INTF){
        tmp=ptr[4];
        if((tmp==USB_INTFID_Hid_Mse)||(tmp==USB_INTFID_Hid_Dbg)||(tmp==USB_INTFID_Hid_MKey)){//HID class cmd
            tmp=ptr[1];
            if(tmp==HID_RQT_Set_IDLE){
                UNS_SET(endp,UNS_Complete);
                HwUsb_SendZlp(0);
                return;
            }
            if(tmp==HID_RQT_Set_Report){
                //SetRxParameter(endp,setupPkt,bIntOutBuf,dbgPktMaxSz);
                //HwUdpSetRxStat(udp,0,EPSTAT_VALID);
                //UNS_SET(endp,UNS_DataOut);
                //return;
            }
        }

//        HwUsb_SendZlp(0);
//        UNS_SET(endp,UNS_Complete);
        UNS_SET(endp,UNS_Complete);
        HwUsb_SendStall(0);
        return;
    }
    if(rcp==USB_RT_RECIPIENT_ENDP){
//        HwUsb_SendZlp(0);
        UNS_SET(endp,UNS_Complete);
        HwUsb_SendStall(0);
        return;
    }

}
/*end file*/

