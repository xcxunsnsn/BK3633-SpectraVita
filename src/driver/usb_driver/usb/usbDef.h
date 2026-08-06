/*************************************************************
 * @file        usbDef.h
 * @brief        Usb protocol defintions by USB spec1.1
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */

#ifndef _USBDEF_H_ //this file's unique symbol checking section
#define _USBDEF_H_
#include "types.h"
#include "usb.h"
/*constant define section*/
/*data format define*/
#define BYTE2(a)                ((a)&0xff),((a>>8)&0xff)
#define WordData(dat)            ((dat)&0xff),(((dat)>>8)&0xff)
#define Byte3Data(dat)            ((dat)&0xff),(((dat)>>8)&0xff),(((dat)>>16)&0xff)
#define Byte4Data(dat)            ((dat)&0xff),(((dat)>>8)&0xff),(((dat)>>16)&0xff),(((dat)>>24)&0xff)
#define Comb2Byte(b0,b1)        ((b0)|((b1)<<8))
#define Comb3Byte(b0,b1,b2)        ((b0)|((b1)<<8)|((b2)<<16))
#define Comb4Byte(b0,b1,b2,b3)    \
    ((b0)|((b1)<<8)|((b2)<<16)|((b3)<<24))


/*manufacturer info*/
#define _SELF_POWER_            0
#define _REMOTE_WAKEUP_            1

    //UNICODE language ID
#define _LANGUAGE_ID_            0x0409
/*usb version*/
#define USB_VERSION                0x0110

/*standard usb desriptor type define*/
#define USB_DESCTYPE_DEVICE        0x01
#define USB_DESCTYPE_CONFIG        0x02
#define USB_DESCTYPE_STRING        0x03
#define USB_DESCTYPE_INTERFACE    0x04
#define USB_DESCTYPE_ENDPOINT    0x05

/*usb-if device class define*/
#define USB_DEVICE_CLASS(cls)        (cls)
#define USB_DEVICE_CLASS_AUDIO        0x01
#define USB_DEVICE_CLASS_MASS
#define USB_DEVICE_CLASS_HUB
#define USB_DEVICE_CLASS_HID        0x03

#define USB_DEVICE_SUBCLASS(subcls)        (subcls)

#define USB_DEVICE_PROTOCOL(prt)        (prt)

#define USB_PID_VID(vid,pid)            (((pid)<<16)+(vid))
#define USB_DEVICE_VERSION(ver)            ((ver)&0xff),(((ver)>>8)&0xff)
#define USB_DEVICE_CONFIG_NUM(num)        (num)

/**/
//定义配置的信息长度
#define USB_CONFIG_TOTAL_LEN(len)        (len)
#define USB_CONFIG_INTF_NUM(num)        (num)
//定义usb端点支持的最大封包的大小
#define USB_MAX_PACKET_LEN(len)            (len)

//定义usb对象的序号
#define USB_OBJECT_NO(no)                (no)
#define USB_STRING_INDEX(idx)            (idx)

#define USB_CONFIG_ATTRIBUTE(selfpwr,remote_wakeup)        \
    (0x80|((selfpwr)<<6)|((remote_wakeup)<<5))
#define USB_CONFIG_MAX_POWER(pwr)        ((pwr)>>1)

#define USB_INTF_ALTERSETTING(no)        (no)
#define USB_INTF_ENDP_NUM(num)            (num)
#define USB_INTF_CLASS(cls)                (cls)
#define USB_INTF_SUBCLASS(scls)            (scls)
#define USB_INTF_PROTOCOL(prt)            (prt)

#define USB_ENDP_ADDRESS(inp,addr)        \
    (((inp)<<7)|(addr))
#define USB_ENDP_ATTRIBUTE(attr)        (attr)
#define USB_ENDP_CTRL                    0x00
#define USB_ENDP_ISO                    0x01
#define USB_ENDP_BULK                    0x02
#define USB_ENDP_INT                    0x03
#define USB_ENDP_INTERVAL(n_ms)            (n_ms)

//USB Request define
#define USB_RT_RECIPT_MASK            0x1f
#define USB_RT_RECIPIENT_DEVICE        0x00
#define USB_RT_RECIPIENT_INTF        0x01
#define USB_RT_RECIPIENT_ENDP        0x02
#define USB_RT_RECIPIENT_OTHER        0x03
#define FIELD_USB_RT_RECIPIENT        0,5

//USB std request cmd
#define USB_GET_STATUS                    0x00
#define USB_GET_DESCRIPTOR                0x06
#define USB_CLEAR_FEATURE                0x01
#define USB_SET_FEATURE                    0x03
#define USB_SET_ADDRESS                    0x05
#define USB_GET_CONFIGURATION            0x08
#define USB_SET_CONFIGURATION            0x09
#define USB_GET_INTERFACE                0x0a
#define USB_SET_INTERFACE                0x0b

//define a device descriptor
#define USB_DEVICE_DESC(ver,clss,vid,pid,maxp,iMan,iPro,iSer)    \
    0x12,\
    USB_DESCTYPE_DEVICE,\
    WordData(USB_VERSION),\
    0,0,clss,\
    USB_MAX_PACKET_LEN(maxp),\
    WordData(vid),\
    WordData(pid),\
    WordData(ver),\
    iMan,iPro,iSer,\
    USB_DEVICE_CONFIG_NUM(USB_MAX_CONFIG_NUM)

#define USB_DEVICE_MSGS(iMan,iPro,iSer)        \
    iMan,iPro,iSer
#define USB_DEVICE_CLASSDESC(cls,scls,prt)    \
    cls,scls,prt

//define a config descriptor
#define USB_CONFIG_DESC(szCfg,n_intf,pwr)    \
    0x09,\
    USB_DESCTYPE_CONFIG,\
    BYTE2(/*sizeof*/(szCfg)),\
    USB_CONFIG_INTF_NUM(n_intf),\
    USB_OBJECT_NO(1),\
    USB_STRING_INDEX(0),\
    USB_CONFIG_ATTRIBUTE(0,0),\
    USB_CONFIG_MAX_POWER(pwr)

//define a interface descriptor
#define USB_INTERFACE_DESC(oid,aid,n_endp,cls,sub_cls,prt)    \
    9,\
    USB_DESCTYPE_INTERFACE,\
    USB_OBJECT_NO(oid),\
    USB_INTF_ALTERSETTING(aid),\
    USB_INTF_ENDP_NUM(n_endp),\
    USB_INTF_CLASS(cls),\
    USB_INTF_SUBCLASS(sub_cls),\
    USB_INTF_PROTOCOL(prt),/*1=keyboard,2=mouse*/\
    USB_STRING_INDEX(0)

//define a endpoint descriptor
#define USB_ENDPOINT_DESC(i_o,epn,tra_typ,maxp,inv)    \
    7,\
    USB_DESCTYPE_ENDPOINT,\
    USB_ENDP_ADDRESS(i_o,epn),\
    USB_ENDP_ATTRIBUTE(tra_typ),\
    BYTE2(maxp),\
    USB_ENDP_INTERVAL(inv)

/*hardware define section*/

/*function macro define section*/

/*struct and union define(export) section*/
//定义usb 的状态
#define USB_STATE_INIT            0x00    /*初始化*/
#define USB_STATE_ATTACHED        0x01    /*连接到host*/
#define USB_STATE_POWERED        0x02    /*上电*/
#define USB_STATE_DEFAULT        0x03    /*使用缺省地址*/
#define USB_STATE_ADDRESS        0x04    /*可寻址*/
#define USB_STATE_CONFIG        0x05    /*已配置*/
#define USB_STATE_USING            0x06    /*使用中,config 100ms之后进入该状态*/
#define USB_STATE_SLEEP            0x07    /*睡眠*/

#define SET_USBSTATE(ust)    \
    (bUsbState=(ust))

//UNS:USB Next Stage
#define UNS_Undef        0
#define UNS_DataIn        1
#define UNS_DataOut        2
#define UNS_StatusOut    3
#define UNS_StatusIn    4
#define UNS_Complete    5

#define UNS_SET(endp,stg)        \
    (((CEndpoint*)endp)->stage)=stg

#define UNS_GET(endp)        \
    (((CEndpoint*)endp)->stage)

#define USB_EndpStt_Tx        BIT(0)
#define USB_EndpStt_Rx        BIT(1)


/*struct define section*/
#pragma pack(1)
typedef struct CBufferBaseDesc{
    void*ptr;
    int sz;
}CBufferBaseDesc;

typedef struct CEndpoint{
    PUI8 txPtr;
    PUI8 rxPtr;
    UI16 txLen;
    UI16 rxLen;
    UI16 lmtLen;
    UI8 stage;
    UI8 status;
}CEndpoint;

typedef union CUsbSetupPkt{
    struct{
        UI16 cmd;
        UI16 tar;
        UI16 idx;
        UI16 len;
    }pkt;
    UI8 dat[8];
}CUsbSetupPkt;
typedef struct CUsbDevDesc{
    UI8 len;
    UI8 devType;
    UI16 usbVer;
    UI8 clsCode;
    UI8 subClsCode;
    UI8 prtCode;
    UI8 maxPkt;
    UI16 vid;
    UI16 pid;
    UI16 devVer;
    UI8 iVendor;
    UI8 iProd;
    UI8 iSerial;
    UI8 nConfig;
}CUsbDevDesc;

typedef struct CUsbConfigDesc{
    UI8 len;
    UI8 devType;
    UI16 wTotalLength;
    UI8 bNumberOfIntf;
    UI8 bConfigValue;
    UI8 iConfig;
    UI8 bmAttribute;
    UI8 MaxPower;
}CUsbConfigDesc;

typedef struct CUsbIntfDesc{
    UI8 len;
    UI8 devType;
    UI8 bIntfNo;
    UI8 bAlertIntf;
    UI8 bNumberOfEndp;
    UI8 bIntfClass;
    UI8 bIntfSubClass;
    UI8 bIntfProtool;
    UI8 iIntf;
}CUsbIntfDesc;

typedef struct CHidClassDesc{
    UI8 len;
    UI8 devType;
    UI16 bcdHID;
    UI8 bCountryCode;
    UI8 bNumDesc;
    UI8 bDescType;
    UI16 wDescLength;
}CHidClassDesc;

typedef struct CUsbEndpDesc{
    UI8 len;
    UI8 devType;
    UI8 bEndpAddr;
    UI8 bmAttribute;
    UI16 wMaxPktSz;
    UI8 bInterval;
}CUsbEndpDesc;

typedef struct CUsbConfig{
    CUsbConfigDesc cfg;
    CUsbIntfDesc dbgIntf;
    CHidClassDesc dbgHidCls;
    CUsbEndpDesc dbgEndp;
    CUsbIntfDesc mseIntf;
    CHidClassDesc mseHidCls;
    CUsbEndpDesc mseEndp;
}CUsbConfig;

typedef struct CUsbCommonDesc{
    UI8 len;
    UI8 devType;
    UI8 dat[1];
}CUsbCommonDesc;


typedef struct CSuspendMode{
    UI8 usb_frame;
    UI8 usb_frame_old;
    UI32 usb_frame_stop_cnt;
    UI8 usb_suspend_flag;

}CSuspendMode;
#pragma pack()

typedef void (*CBK_USBAPP)(void*buf,int  sz);
typedef void (*CBK_USB_SUSPEND)(void);

#endif
/*end file*/
