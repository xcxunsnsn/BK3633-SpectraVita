/*************************************************************
 * @file        usb_desc.c
 * @brief        USB descriptors ,discribed USB spec1.1 chapter 9
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */

/*file type declaration section*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*internal symbols import section*/
#include "usbclass.h"
#include "usb.h"
#include "usbdef.h"
#include "driver_usb.h"

#pragma pack(1)
typedef struct {
    int ind;
    CBufferBaseDesc desc;
} _t_index_descriptor;
#pragma pack()

#if 1
const UI8 tHidMSEReportDesc[]={
/////////////////mouse
    HidUsagePage(HID_USAGEPAGE_Generic),    //0x05,01       USAGE_PAGE (Generic Desktop)
    HidUsage(HID_USAGE_Mouse),              //0x09,0x02     USAGE (Mouse)
    HidCollectionApp(),                     //0xa1,0x01     COLLECTION (Application)
    HidReportID(0x01),                      //0x85, 0x01,   Report ID (1)
    HidUsage(HID_USAGE_Pointer),            //0x09,0x01     USAGE (Pointer)
    HidCollectionPhy(),                     //0xa1,0x01     COLLECTION (Physical)
    HidUsagePage(HID_USAGEPAGE_Buttons),    //0x05,0x09     USAGE_PAGE (Button)
    ///////////// 1 Byte
    HidUsageMin8(1),                        //0x19,0x01     USAGE_MINIMUM (Button 1)
    HidUsageMax8(5),                        //0x29,0x05     USAGE_MaxIMUM (Button 5)
    HidLogiMin8(0),                         //0x15,0x00     LOGICAL_MINIMUM (0)
    HidLogiMax8(1),                         //0x25,0x01     LOGICAL_MAXIMUM (1)
    HidReportBits(1),                       //0x75,0x01     REPORT_SIZE (1)
    HidReportCount(5),                      //0x95,0x05     Report Count (5)
    HidInput(0, 1, 0),                      //0x81,0x02     INPUT (Data,Var,Rel)
    HidReportBits(3),                       //0x75,0x03     REPORT_SIZE (3)
    HidReportCount(1),                      //0x95,1        REPORT_COUNT (1)
    HidInput(1, 0, 0),                      //0x81,0x01     INPUT (Cnst)

    HidUsagePage(HID_USAGEPAGE_Generic),    //0x05,01       USAGE_PAGE (Generic Desktop)
    HidUsage(HID_USAGE_AxisX),              //0x09, 0x30,   USAGE (X)
    HidUsage(HID_USAGE_AxisY),              //0x09, 0x31,   USAGE (Y)
    ///////////// 3 Bytes
    HidLogiMin16(-2047),                    //0x16,0x01,0xf8    Logical Minimum  (-2047)
    HidLogiMax16(2047),                     //0x26,0xff,0x07    Logical Maximum (2047)
    HidReportBits(12),                      //0x75,0x0c     REPORT_SIZE (12)
    HidReportCount(2),                      //0x95,0x02     REPORT_COUNT (2)
    HidInput(0, 1, 1),                      //0x81,0x06     INPUT (Data,Var,Rel)


    HidUsage(HID_USAGE_Wheel),              //0x09, 0x38,   USAGE (Wheel)
    ///////////// 1 Byte
    HidLogiMin8(-127),                      //0x15, 0x81,   LOGICAL MINIMUM (-127)
    HidLogiMax8(127),                       //0x25, 0x7f,   LOGICAL MAXIMUM (127)
    HidReportBits(8),                       //0x75, 0x08,   REPORT SIZE (8)
    HidReportCount(1),                      //0x95, 0x01,   REPORT COUNT (1)
    HidInput(0, 1, 1),                      //0x81, 0x06,   INPUT(Data,Var,Rel)
#if MOUSE_SWING
    HidUsagePage(HID_USAGEPAGE_Customer),   //0x05, 0x0c,   SAGE_PAGE (Generic HID_USAGEPAGE_Customer)
    HidUsage1(0x0238),                      //0x0a, 0x38, 0x02   AC PAN.  USAGE_PAGE(0x0238)
    HidReportCount(1),                      //0x95, 0x01,   REPORT_COUNT (1)
    HidInput(0, 1, 1),                      //0x81, 0x06,   INPUT(Data,Var,Rel)
#endif

    HidEndCollection(),                     //0xc0,            // end Application Collection};
    HidEndCollection(),                     //0xc0,            // end Application Collection};

/////////////////keyboard
    HidUsagePage(HID_USAGEPAGE_Generic),    //0x05,01       USAGE_PAGE (Generic Desktop)
    HidUsage(HID_USAGE_Keyboard),           //0x09,0x06     USAGE (Keyboard)
    HidCollectionApp(),                     //0xa1,0x01     COLLECTION (Application)
    HidReportID(0x02),                      //0x85, 0x02,   Report ID (2)

    HidUsagePage(HID_USAGEPAGE_Keyboard),   //0x05,0x07     USAGE_PAGE (Keyboard)
    ///////////// 2 Bytes input
    HidUsageMin8(0xe0),                     //0x19,0xe0     USAGE_MINIMUM (0xe0)
    HidUsageMax8(0xe7),                     //0x29,0xe7     USAGE_MaxIMUM (0xe7)
    HidLogiMin8(0),                         //0x15, 0,   LOGICAL MINIMUM (0)
    HidLogiMax8(1),                         //0x25, 1,   LOGICAL MAXIMUM (1)
    HidReportBits(1),                       //0x75,1      REPORT_SIZE (1)
    HidReportCount(8),                      //0x95,0x08     REPORT_COUNT (8)
    HidInput(0, 1, 0),                      //0x81,0x02     INPUT (Data,Var,Rel)
    HidReportBits(8),                       //0x75,8      REPORT_SIZE (8)
    HidReportCount(1),                      //0x95,0x01     REPORT_COUNT (1)
    HidInput(1, 0, 0),                      //0x81,0x01     INPUT (Cnst)

    HidUsagePage(HID_USAGEPAGE_LED),        //0x05,08       USAGE_PAGE (HID_USAGEPAGE_LED)
    ///////////// 1 Byte output
    HidReportBits(1),                       //0x75,1      REPORT_SIZE (1)
    HidReportCount(5),                      //0x95,0x05     REPORT_COUNT (5)
    HidUsageMin8(1),                        //0x19,1     USAGE_MINIMUM (1)
    HidUsageMax8(5),                        //0x29,5     USAGE_MaxIMUM (5)
    HidOutput(0,1,0),                       //0x91, 0x02, OUTPUT (Data,Var,Rel)
    HidReportBits(3),                       //0x75,3      REPORT_SIZE (3)
    HidReportCount(1),                      //0x95,0x05     REPORT_COUNT (1)
    HidOutput(1,0,0),                       //0x91, 0x01, OUTPUT (Cnst)


    HidUsagePage(HID_USAGEPAGE_Keyboard),   //0x05,07       USAGE_PAGE (HID_USAGEPAGE_Keyboard)
    ///////////// 6 Bytes input
    HidReportBits(8),                       //0x75,8      REPORT_SIZE (8)
    HidReportCount(6),                      //0x95,0x06     REPORT_COUNT (6)
    HidLogiMin8(0),                         //0x15, 0,   LOGICAL MINIMUM (0)
    HidLogiMax8(0xff),                      //0x25, 0xff,   LOGICAL MAXIMUM (0xff)
    HidUsageMin8(0),                        //0x19,0     USAGE_MINIMUM (0)
    HidUsageMax8(0xff),                     //0x29,0xff     USAGE_MaxIMUM (0xff)
    HidInput(0, 0, 0),                      //0x81,0x00     INPUT (Data,Ary,Abs)

    HidEndCollection(),                     //0xc0,          end Application Collection

};
const UI8 tHidMediaReportDesc[]={

/////////////////media
    HidUsagePage(HID_USAGEPAGE_Customer),   //0x05,0c       USAGE_PAGE (HID_USAGEPAGE_Customer)
    HidUsage(HID_USAGE_Pointer),            //0x09,0x01     USAGE (HID_USAGE_Pointer)
    HidCollectionApp(),                     //0xa1,0x01     COLLECTION (Application)
//  HidReportID(0x03),                      //0x85, 0x03,     Report ID (3)
    ///////////// 2 Bytes
    HidUsageMin8(0),                        //0x19,0x00     USAGE_MINIMUM (0)
    HidUsageMax16(0x029c),                  //0x2A, 0x9c, 0x02,   USAGE_MAXIMUM (29c)
    HidLogiMin8(0),                         //0x15,0x00     LOGICAL_MINIMUM (0)
    HidLogiMax16(0x029c),                   //0x26, 0x9c, 0x02,   LOGICAL_MAXIMUM (29c)
    HidReportBits(0x10),                    //0x75,0x10     REPORT_SIZE (0x10)
    HidReportCount(1),                      //0x95,0x05     Report Count (1)
    HidInput(0, 0, 0),                      //0x81,0x02     INPUT (Data,Ary,Abs)
    HidEndCollection(),                     //0xc0,          end Application Collection

};

/*
demo
{
    HidUsagePage(HID_USAGEPAGE_Generic),    //0x05,01       USAGE_PAGE (Generic Desktop)
    HidUsage(HID_USAGE_Mouse),              //0x09,0x02     USAGE (Mouse)
    HidCollectionApp(),                     //0xa1,0x01     COLLECTION (Application)
    HidReportID(0x01),                      //0x85, 0x01,      // Report ID (1)
    HidUsage(HID_USAGE_Pointer),            //0x09,0x01     USAGE (Pointer)
    HidCollectionPhy(),                     //0xa1,0x01     COLLECTION (Physical)
    HidUsagePage(HID_USAGEPAGE_Buttons),    //0x05,0x09     USAGE_PAGE (Button)
    HidUsageMin8(1),                        //0x19,0x01     USAGE_MINIMUM (Button 1)
    HidUsageMax8(5),                        //0x29,0x05    USAGE_MaxIMUM (Button 5)
    HidLogiMin8(0),                         //0x15,0x00     LOGICAL_MINIMUM (0)
    HidLogiMax8(1),                         //0x25,0x01     LOGICAL_MAXIMUM (1)
    HidReportBits(1),                       //0x75,0x01     REPORT_SIZE (1)
    HidReportCount(5),                      //0x95,0x05     Report Count (5)
    HidInput(0, 1, 0),                      //0x81,0x02     INPUT (Data,Var)
    HidReportBits(3),                       //0x75,0x03     REPORT_SIZE (3)
    HidReportCount(1),                      //0x95,1        REPORT_COUNT (1)
    HidInput(1, 0, 0),                      //0x81,0x01     INPUT (Cnst,Con)

    HidUsagePage(HID_USAGEPAGE_Generic),    //0x05,01       USAGE_PAGE (Generic Desktop)
    HidUsage(HID_USAGE_AxisX),              //0x09, 0x30,  /// USAGE (X)
    HidUsage(HID_USAGE_AxisY),              //0x09, 0x31,  /// USAGE (Y)
    HidUsage(HID_USAGE_AxisZ),              //0x09, 0x32,  /// USAGE (Z)
    HidLogiMin16(-32768),                   //0x16,0x00,0x80    Logical Minimum (-32768)
    HidLogiMax16(32767),                    //0x26,0xff,0x7f    Logical Maximum (32768)
    HidReportBits(16),                      //0x75,16      REPORT_SIZE (16)
    HidReportCount(3),                      //0x95,0x03    REPORT_COUNT (3)
    HidInput(0, 1, 1),                      //0x81,0x06    INPUT (Data,Var,Rel)

    HidEndCollection(),                     //0xc0,            // end Application Collection};
    HidEndCollection(),                     //0xc0,            // end Application Collection};
}

*/
#endif
#if 0
const UI8 tHidMSEReportDesc[]={
    HidUsagePage(HID_USAGEPAGE_Generic),
    HidUsage(HID_USAGE_Mouse),
    HidCollectionApp(),
    HidUsage(HID_USAGE_Pointer),
    HidCollectionPhy(),
    HidReportID(REPORTID_REL_MOUSE),
    HidUsagePage(HID_USAGEPAGE_Buttons),
    HidUsageMin8(1),
    HidUsageMax8(8),
    HidLogiMin8(0),
    HidLogiMax8(1),
    HidReportBits(1),
    HidReportCount(8),
    HidInput(0, 1, 0),

    HidUsagePage(HID_USAGEPAGE_Generic),
    HidUsage(HID_USAGE_AxisX),
    HidUsage(HID_USAGE_AxisY),
    HidUsage(HID_USAGE_AxisZ),
    HidLogiMin16(-32768),
    HidLogiMax16(32767),
    HidReportBits(16),
    HidReportCount(3),
    HidInput(0, 1, 1),
    HidEndCollection(),
    HidEndCollection(),


    HidUsage(HID_USAGE_Mouse),
    HidCollectionApp(),
    HidUsage(HID_USAGE_Pointer),
    HidCollectionPhy(),
    HidReportID(REPORTID_ABS_MOUSE),
    HidUsagePage(HID_USAGEPAGE_Buttons),
    HidUsageMin8(1),
    HidUsageMax8(8),
    HidLogiMin8(0),
    HidLogiMax8(1),
    HidReportBits(1),
    HidReportCount(8),
    HidInput(0, 1, 0),

    HidUsagePage(HID_USAGEPAGE_Generic),
    HidUsage(HID_USAGE_AxisX),
    HidUsage(HID_USAGE_AxisY),
    HidUsage(HID_USAGE_AxisZ),
    HidLogiMin16(0),
    HidLogiMax16(32767),
    HidReportBits(16),
    HidReportCount(3),
    HidInput(0, 1, 0),
    HidEndCollection(),
    HidEndCollection(),
};
#endif

const UI8 tHidDbgReportDesc[]={
    HidUsagePage(HID_USAGEPAGE_Customer),
    HidUsage(0x01),
    HidCollectionApp(),
    HidUsage(0x00),
    HidReportBits(8),
    HidReportCount(64),
    HidLogiMin8(0),
    HidLogiMax16(0xff),
    HidInput(0, 1, 0),
    HidUsage(0x00),
    HidReportBits(8),
    HidReportCount(64),
    HidLogiMin8(0),
    HidLogiMax16(0xff),
    HidOutput(0, 1, 0),
    HidEndCollection(),
};
const UI16 tUsbLanguageID[]={
    0x04+(USB_DESCTYPE_STRING<<8),
    _LANGUAGE_ID_
};

const UI16 tUsbProductStr[]={
    (((9+1)*2))+(USB_DESCTYPE_STRING<<8),'H','I','D',' ','M','o','u','s','e',
};

const UI16 tUsbVendorStr[]={
    (((5+1)*2))+(USB_DESCTYPE_STRING<<8),'B','e','k','e','n'
};

const CUsbDevDesc tUsbDeviceDescriptor={
    sizeof(CUsbDevDesc),
    USB_DESCTYPE_DEVICE,
    USB_VERSION,
    USB_DEVICE_CLASS(0),
    USB_DEVICE_SUBCLASS(0),
    USB_DEVICE_PROTOCOL(0),
    USB_MAX_PACKET_LEN(64),
    _VID_,
    _PID_,
    0x0101,
    _Vendor_String_ID_,
    _Product_String_ID_,
    0,
    USB_DEVICE_CONFIG_NUM(USB_MAX_CONFIG_NUM)
};

#if 1
/*
*MIC+SPK+Mouse+HID vendor IO
*/
#if(USB_MIC_ONLY)
#define SZ_Audio_ACI(nIntfAs)                9/*Interface AC*/+\
    8+((nIntfAs))+/*Header*/+\
    12    /*IT(1)*/+\
    9    /*OT(2)*/+\
    0    /*FU(1)*/+\
    0    /*FU(2)*/+\
    0
#else
#define SZ_Audio_ACI(nIntfAs)                9/*Interface AC*/+\
    8+((nIntfAs))+/*Header*/+\
    12    /*IT(1)*/+\
    12    /*IT(2)*/+\
    9    /*OT(1)*/+\
    9    /*OT(2)*/+\
    0    /*FU(1)*/+\
    0    /*FU(2)*/+\
    0
#endif
#define SZ_Audio_ASI(nSR)                9+/*Interface AS Bw0*/+\
    9    /*Interface AS Normal*/+\
    7    /*AS General*/+\
    8+((nSR)*3)    /*AS FormatI*/+\
    9    /*Endpoint for Audio class*/+\
    7    /*Endpoint AS General*/+\
    0

#define SZ_HID_Intf(nEndp)                9+/*HID Interface descriptor*/+\
    9    /*HID class descriptor*/+\
    7*(nEndp)    /*Endpoint descriptor*/+\
    0
#if(USB_MIC_ONLY)
#define SZ_CONFIG(nMicSR,nSpkSR,nIntfAs)        9/*config*/+\
    SZ_Audio_ACI(nIntfAs)+\
    SZ_Audio_ASI(nMicSR)/*MIC descs size*/+\
    SZ_HID_Intf(2)/*HID Mouse descs size*/+\
    SZ_HID_Intf(2)/*HID DBG descs size*/+\
    SZ_HID_Intf(1)/*HID Mkey descs size*/+\
    0
#else
#define SZ_CONFIG(nMicSR,nSpkSR,nIntfAs)        9/*config*/+\
    SZ_Audio_ACI(nIntfAs)+\
    SZ_Audio_ASI(nMicSR)/*MIC descs size*/+\
    SZ_Audio_ASI(nSpkSR)/*SPK descs size*/+\
    SZ_HID_Intf(2)/*HID Mouse descs size*/+\
    SZ_HID_Intf(2)/*HID DBG descs size*/+\
    SZ_HID_Intf(1)/*HID Mkey descs size*/+\
    0
#endif
const UI8 tUsbConfig[]={
#if(USB_MIC_ONLY)
    USB_Gen_Config(SZ_CONFIG(1,0,1), USB_MAX_INTERFACE_NUM, 100),
    USB_AUDIO_InterfaceAC(USB_INTFID_AudioAC),
    USB_ACS_ACI_Header(SZ_Audio_ACI(1),1),USB_INTFID_AudioAS_MIC,
    //USB_ACS_ACI_IT(USBAUDIO_IT_SPK,0,AUDIO_TERMTYPE_USB_STREAM,2,AUDIO_CHANNEL_CONFIG(1,1,0,0,0,0,0,0,0,0,0,0)),
    USB_ACS_ACI_IT(USBAUDIO_IT_MIC,0,AUDIO_TERMTYPE_IT_MIC,2,AUDIO_CHANNEL_CONFIG(1,1,0,0,0,0,0,0,0,0,0,0)),
    USB_ACS_ACI_OT(USBAUDIO_OT_MIC,0,AUDIO_TERMTYPE_USB_STREAM,USBAUDIO_IT_MIC),
    //USB_ACS_ACI_OT(USBAUDIO_OT_SPK,0,AUDIO_TERMTYPE_OT_SPK,USBAUDIO_IT_SPK),
    //MIC desc
    USB_AUDIO_IntfAltAS_BW0(USB_INTFID_AudioAS_MIC,0),
    USB_AUDIO_InterfaceAS(USB_INTFID_AudioAS_MIC,1),
    USB_ACS_ASI_General(USBAUDIO_OT_MIC,1,0x0001),
    USB_ACS_ASI_FmtTypeI(AUDIO_MIC_CHNs,AUDIO_MIC_BR,1),Byte3Data(AUDIO_MIC_SAMPRATE),
    USB_AUDIO_EndpAS_IN(USB_ENDPID_Audio_MIC,0/*AUDIO_SYNCMODE_SYNC*/,AUDIO_MAXP(AUDIO_MIC_CHNs,AUDIO_MIC_BR,AUDIO_MIC_SAMPRATE)),
    USB_ACS_EP_General(0),
#else
    USB_Gen_Config(SZ_CONFIG(1,1,2), USB_MAX_INTERFACE_NUM, 100),
    USB_AUDIO_InterfaceAC(USB_INTFID_AudioAC),
    USB_ACS_ACI_Header(SZ_Audio_ACI(2),2),USB_INTFID_AudioAS_MIC,USB_INTFID_AudioAS_SPK,
    USB_ACS_ACI_IT(USBAUDIO_IT_SPK,0,AUDIO_TERMTYPE_USB_STREAM,2,AUDIO_CHANNEL_CONFIG(1,1,0,0,0,0,0,0,0,0,0,0)),
    USB_ACS_ACI_IT(USBAUDIO_IT_MIC,0,AUDIO_TERMTYPE_IT_MIC,2,AUDIO_CHANNEL_CONFIG(1,1,0,0,0,0,0,0,0,0,0,0)),
    USB_ACS_ACI_OT(USBAUDIO_OT_MIC,0,AUDIO_TERMTYPE_USB_STREAM,USBAUDIO_IT_MIC),
    USB_ACS_ACI_OT(USBAUDIO_OT_SPK,0,AUDIO_TERMTYPE_OT_SPK,USBAUDIO_IT_SPK),
    //MIC desc
    USB_AUDIO_IntfAltAS_BW0(USB_INTFID_AudioAS_MIC,0),
    USB_AUDIO_InterfaceAS(USB_INTFID_AudioAS_MIC,1),
    USB_ACS_ASI_General(USBAUDIO_OT_MIC,1,0x0001),
    USB_ACS_ASI_FmtTypeI(AUDIO_MIC_CHNs,AUDIO_MIC_BR,1),Byte3Data(AUDIO_MIC_SAMPRATE),
    USB_AUDIO_EndpAS_IN(USB_ENDPID_Audio_MIC,0/*AUDIO_SYNCMODE_SYNC*/,AUDIO_MAXP(AUDIO_MIC_CHNs,AUDIO_MIC_BR,AUDIO_MIC_SAMPRATE)),
    USB_ACS_EP_General(0),
    //SPK desc
    USB_AUDIO_IntfAltAS_BW0(USB_INTFID_AudioAS_SPK,0),
    USB_AUDIO_InterfaceAS(USB_INTFID_AudioAS_SPK,1),
    USB_ACS_ASI_General(USBAUDIO_IT_SPK,1,0x0001),
    USB_ACS_ASI_FmtTypeI(AUDIO_SPK_CHNs,AUDIO_SPK_BR,1),Byte3Data(AUDIO_SPK_SAMPRATE),
    USB_AUDIO_EndpAS_OUT(USB_ENDPID_Audio_SPK,0/*AUDIO_SYNCMODE_SYNC*/,AUDIO_MAXP(AUDIO_SPK_CHNs,AUDIO_SPK_BR,AUDIO_SPK_SAMPRATE)),
    USB_ACS_EP_General(0),
#endif

    USB_INTERFACE_DESC(USB_INTFID_Hid_Mse, 0, 2, USB_DEVICE_CLASS_HID, 0,0),
    USB_HID_CLS_DESC(tHidMSEReportDesc),
    USB_HID_EndpIN(USB_ENDPID_Hid_MSE, 16, 1),
    USB_HID_EndpOUT(USB_ENDPID_Hid_MSE_OUT, 8, 1),
    
    USB_INTERFACE_DESC(USB_INTFID_Hid_Dbg, 0, 2, USB_DEVICE_CLASS_HID, 0,0),
    USB_HID_CLS_DESC(tHidDbgReportDesc),
    USB_HID_EndpIN(USB_ENDPID_Hid_DBG_IN, 64, 1),
    USB_HID_EndpOUT(USB_ENDPID_Hid_DBG_OUT, 64, 1),
    
    USB_INTERFACE_DESC(USB_INTFID_Hid_MKey, 0, 1, USB_DEVICE_CLASS_HID, 0,0),
    USB_HID_CLS_DESC(tHidMediaReportDesc),
    USB_HID_EndpIN(USB_ENDPID_Hid_MKEY, 16, 1)

};
#endif

const CBufferBaseDesc _c_device_desc={(void*)&tUsbDeviceDescriptor,sizeof(tUsbDeviceDescriptor)};
const CBufferBaseDesc _c_config_desc={(void*)tUsbConfig,sizeof(tUsbConfig)};
const _t_index_descriptor tblUsbHidRptDescs[]={
    {USB_INTFID_Hid_Mse,{(void*)tHidMSEReportDesc,sizeof(tHidMSEReportDesc)}},
    {USB_INTFID_Hid_Dbg,{(void*)tHidDbgReportDesc,sizeof(tHidDbgReportDesc)}},
    {USB_INTFID_Hid_MKey,{(void*)tHidMediaReportDesc,sizeof(tHidMediaReportDesc)}},
};
const _t_index_descriptor tblUsbStringDescs[]={
    {0,{(void*)tUsbLanguageID,sizeof(tUsbLanguageID)}},
    {_Vendor_String_ID_,{(void*)tUsbVendorStr,sizeof(tUsbVendorStr)}},
    {_Product_String_ID_,{(void*)tUsbProductStr,sizeof(tUsbProductStr)}},
};

void*USBDesc_GetDeviceDesc(void){
    return (void*)&_c_device_desc;
}

void*USBDesc_GetConfigDesc(void){
    return (void*)&_c_config_desc;
}
void*UsbDesc_Find(int idx,void*tb,int cnt){
    _t_index_descriptor*ptbl=(_t_index_descriptor*)tb;
    int i;
    for(i=0;i<cnt;i++){
        if(ptbl[i].ind==idx){
            return (void*)&ptbl[i].desc;
        }
    }
    return(NULL);
}

void*USBDesc_GetHidRptDesc(int idx){
    return UsbDesc_Find(idx,(void*)tblUsbHidRptDescs,GET_ELEMENT_TBL(tblUsbHidRptDescs));
}

void*USBDesc_GetStringDesc(int idx){
    return UsbDesc_Find(idx,(void*)tblUsbStringDescs,GET_ELEMENT_TBL(tblUsbStringDescs));
}

