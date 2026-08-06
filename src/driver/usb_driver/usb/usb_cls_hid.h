/*************************************************************
 * @file        usb_cls_hid.h
 * @brief        HID definitions and HUT
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */
#ifndef _USB_CLS_HID_H_
#define _USB_CLS_HID_H_

//HID special Descriptor
#define USB_DESCTYPE_HID        0x21

#define USB_DESCTYPE_HID_Report        0x22
#define USB_DESCTYPE_HID_Physical    0x23

//HID class request
#define HID_RQT_Set_IDLE    0x0a
#define HID_RQT_Get_IDLE    0x02
#define HID_RQT_Set_Report    0x09
#define HID_RQT_Get_Report    0x0a
#define HID_RQT_Set_Protocol    0x0b
#define HID_RQT_Get_Protocol    0x03
//HID class descriptor
#define USB_HID_CLS_DESC(reportDescName)        \
    9/*SZB(descName)*/,/*desc length*/        \
    USB_DESCTYPE_HID,/**/            \
    BYTE2(0x0100)/*USB_DEVICE_VERSION(0x0100)*/,/*hid版本*/\
    0x00,            /*区域代码*/    \
    0x01,            /*HID特有描述符集合中描述符的个数*/    \
    USB_DESCTYPE_HID_Report,            /*type of descriptor*/    \
    BYTE2(sizeof(reportDescName))/*WordData(SZB(reportDescName))*/    /*length of report descriptor,<255 */

#define USB_Mouse_Config(cfgTotalSz)    \
    0x09,\
    USB_DESCTYPE_CONFIG,\
    WordData(cfgTotalSz),\
    USB_CONFIG_INTF_NUM(1),\
    USB_OBJECT_NO(1),\
    USB_STRING_INDEX(0),\
    USB_CONFIG_ATTRIBUTE(0,0),\
    USB_CONFIG_MAX_POWER(50)

#define USB_KBD_Interface(oid)    \
    9,\
    USB_DESCTYPE_INTERFACE,\
    USB_OBJECT_NO(oid),\
    USB_INTF_ALTERSETTING(0),\
    USB_INTF_ENDP_NUM(1),\
    USB_INTF_CLASS(3),\
    USB_INTF_SUBCLASS(0),\
    USB_INTF_PROTOCOL(0),/*1=keyboard,2=mouse*/\
    USB_STRING_INDEX(0)

#define USB_Mouse_Interface(oid)    \
    9,\
    USB_DESCTYPE_INTERFACE,\
    USB_OBJECT_NO(oid),\
    USB_INTF_ALTERSETTING(0),\
    USB_INTF_ENDP_NUM(1),\
    USB_INTF_CLASS(3),\
    USB_INTF_SUBCLASS(1),\
    USB_INTF_PROTOCOL(2),/*1=keyboard,2=mouse*/\
    USB_STRING_INDEX(0)


#define USB_HID_Endp(epn,maxp,i_o,inv)    \
    7/*SZB(endpDn)*/,\
    USB_DESCTYPE_ENDPOINT,\
    USB_ENDP_ADDRESS(i_o,epn),\
    USB_ENDP_ATTRIBUTE(USB_ENDP_INT),\
    BYTE2(maxp),\
    USB_ENDP_INTERVAL(inv)

#define USB_HID_EndpIN(epn,maxp,inv)    \
    USB_HID_Endp(epn,maxp,1,inv)

#define USB_HID_EndpOUT(epn,maxp,inv)    \
    USB_HID_Endp(epn,maxp,0,inv)

#define USB_KBD_Endpoint(epn,inv)        \
    USB_HID_Endp(epn, 8, 1, inv)

#define USB_Mouse_Endpoint(endpDesc,epn,inv)        \
    USB_HID_Endp(epn, 8, 1, inv)

#define USB_Gen_Config(cfgDesc,nintf,pwr)    \
    0x09,\
    USB_DESCTYPE_CONFIG,\
    BYTE2(/*sizeof*/(cfgDesc)),\
    USB_CONFIG_INTF_NUM(nintf),\
    USB_OBJECT_NO(1),\
    USB_STRING_INDEX(0),\
    USB_CONFIG_ATTRIBUTE(0,0),\
    USB_CONFIG_MAX_POWER(pwr)


#define USB_HID_Interface(oid,aid,sc,prt,istr)    \
    9,\
    USB_DESCTYPE_INTERFACE,\
    USB_OBJECT_NO(oid),\
    USB_INTF_ALTERSETTING(aid),\
    USB_INTF_ENDP_NUM(1),\
    USB_INTF_CLASS(3),\
    USB_INTF_SUBCLASS(sc),\
    USB_INTF_PROTOCOL(prt),/*1=keyboard,2=mouse*/\
    USB_STRING_INDEX(istr)


///////////////////////////////////////////////////////////////////////////////////////////////////
//HUT
/*HID report Main items define*/
#define HidReportFormat(_type,_tag,_size)    \
    ((_size)+((_tag)<<4)+((_type)<<2))

    /*item type defines*/
    #define HID_ITEM_TYP_MAIN        0
    #define HID_ITEM_TYP_GLOBAL        1
    #define HID_ITEM_TYP_LOCAL        2
    #define HidMainItem(_tag,_size)    \
        HidReportFormat(HID_ITEM_TYP_MAIN, _tag, _size)
    /*main item tag defines*/
    #define HID_ITEM_MAIN_INPUT            0x08
    #define HID_ITEM_MAIN_OUTPUT        0x09
    #define HID_ITEM_MAIN_FEATURE        0x0b
    #define HID_ITEM_MAIN_COLLECTION    0x0a
    #define HID_ITEM_MAIN_ENDCOLLECT    0x0c

/**/

/*HID report Main items define*/
#define HidCollection(_v)        \
    HidMainItem(HID_ITEM_MAIN_COLLECTION,1), (_v)
    /*collection type*/
    #define HID_COLLECTION_PHYSICAL        0x00
    #define HID_COLLECTION_APPLiCATION    0x01
    #define HID_COLLECTION_LOGICAL        0x02
    #define HidCollectionApp()        HidCollection(HID_COLLECTION_APPLiCATION)
    #define HidCollectionPhy()        HidCollection(HID_COLLECTION_PHYSICAL)
    #define HidCollectionLog()        HidCollection(HID_COLLECTION_LOGICAL)
#define HidEndCollection()        \
    HidMainItem(HID_ITEM_MAIN_ENDCOLLECT,0)

#define HidInput(con_dat,var_arr,rel_abs)            \
    HidMainItem(HID_ITEM_MAIN_INPUT,1), \
    HID_INPUT_ATTRIBUTE(con_dat, var_arr, rel_abs)
    /**/
    #define HID_INPUT_ATTRIBUTE(con_dat,var_arr,rel_abs)    \
        ((con_dat)|((var_arr)<<1)|((rel_abs)<<2))
#define    HidOutput(con_dat,var_arr,rel_abs)            \
    HidMainItem(HID_ITEM_MAIN_OUTPUT,1),\
    HID_INPUT_ATTRIBUTE(con_dat, var_arr, rel_abs)

#define HidFeature(con_dat,var_arr,rel_abs)            \
    HidMainItem(HID_ITEM_MAIN_FEATURE,1),    \
    HID_INPUT_ATTRIBUTE(con_dat, var_arr, rel_abs)

/*HID report Global items define*/
#define HidGlobalItem(_tag,_size)            \
    HidReportFormat(HID_ITEM_TYP_GLOBAL, _tag, _size)
#define    HidUsagePage(_v)            \
    HidGlobalItem(HID_ITEM_GLB_Page,1), (_v)
    /**/
    #define HID_ITEM_GLB_Page            0x00
        /**/
        #define HID_USAGEPAGE_Generic        0x01
        #define HID_USAGEPAGE_SimulationControl        0x02
        #define HID_USAGEPAGE_VRControl        0x03
        #define HID_USAGEPAGE_GameControl    0x05
        #define HID_USAGEPAGE_SportControl    0x04
        #define HID_USAGEPAGE_Keyboard        0x07
        #define HID_USAGEPAGE_LED            0x08
        #define HID_USAGEPAGE_Buttons        0x09
        #define HID_USAGEPAGE_Ordinal        0x0a
        #define HID_USAGEPAGE_TelePhony        0x0b
        #define HID_USAGEPAGE_Customer        0x0c    /*消费类*/
            #define HUC_Volumn                0xe0    /*音量*/
            #define HUC_ChannelInc            0x9c    /*频道加*/
            #define HUC_ChannelDec            0x9d    /*频道减*/
            #define HUC_VolumnInc            0xe9    /*音量加*/
            #define HUC_VolumnDec            0xea    /*音量减*/

            #define HUC_VolumnMute            0xe2    /*静音*/
            #define HUC_MediaPlay            0xcd    /*播放/暂停,0xb0*/
            #define HUC_MediaStop            0xb7    /*停止*/
            #define HUC_MediaPause            0xb1    /*暂停*/
            #define HUC_MediaRecord         0xb2    /*录音*/
            #define HUC_MediaNext            0xb5    /*下一曲*/
            #define HUC_MediaPrev            0xb6    /*上一曲*/
            #define HUC_MediaFastFwd        0xb3    /*快进*/
            #define HUC_MediaRewind            0xb4    /*快退*/
            #define HUC_AL_CustomerCtrl        0x183    /*打开默认的媒体播放器*/
            #define HUC_AL_WordProc            0x184    /*打开字处理软件*/
            #define HUC_AL_TextProc            0x185    /*打开编辑软件*/
            #define HUC_AL_EmailProc        0x18a    /*打开邮件处理软件*/
            #define HUC_AL_CalendProc        0x18e    /*打开日历*/
            #define HUC_AL_CalcProc            0x192    /*打开计算器*/
            #define HUC_AL_TaskMana            0x18f    /*打开任务管理器*/
            #define HUC_AL_Desktop            0x1aa    /*打开桌面*/
            #define HUC_AL_IE                0x196    /*打开ie*/
            #define HUC_AL_CtrlPanel        0x19f    /*打开控制面板*/
            /*还有其他启动应用程序的宣告,此处不再定义,用时参考hut.pdf*/
        #define HID_USAGEPAGE_Digitizer        0x0d    //
        #define HID_USAGEPAGE_PID            0x0f    //for ffb
        #define HID_USAGEPAGE_Unicode        0x10
        #define HID_USAGEPAGE_AlphaDisp        0x14
        #define HID_USAGEPAGE_Medical        0x40
    #define HID_ITEM_GLB_LogicMin        0x01
    #define HID_ITEM_GLB_LogicMax        0x02
    #define HID_ITEM_GLB_PhysicalMin    0x03
    #define HID_ITEM_GLB_PhysicalMax    0x04
    #define HID_ITEM_GLB_UnitExponent    0x05
    #define HID_ITEM_GLB_Unit            0x06
        /*unit code*/
        #define HID_UNIT_SYSTEM            0x00    //单位:系统
        #define HID_UNIT_Length            0x01    //单位:长度
        #define HID_UNIT_Mass            0x02    //单位:质量
        #define HID_UNIT_Time            0x03    //单位:时间
        #define HID_UNIT_Temperature    0x04    //单位:温度
        #define HID_UNIT_Current        0x05    //单位:电流
        #define HID_UNIT_Luminous        0x06    //单位:光度
        /*unit system*/
        #define HID_UNIT_SILiear        0x01
        #define HID_UNIT_SIRotation        0x02
        #define HID_UNIT_EnglishLinear    0x03
        #define HID_UNIT_EnglishRotation    0x04

    #define HID_ITEM_GLB_ReportSize        0x07
    #define HID_ITEM_GLB_ReportID        0x08
    #define HID_ITEM_GLB_ReportCount    0x09
    #define HID_ITEM_GLB_Push            0x0a
    #define HID_ITEM_GLB_Pop            0x0b
#define HidReportID(_v)        \
    HidGlobalItem(HID_ITEM_GLB_ReportID,1),(_v)
#define HidReportBits(_v)    \
    HidGlobalItem(HID_ITEM_GLB_ReportSize,1), (_v)
#define HidReportCount(_v)    \
    HidGlobalItem(HID_ITEM_GLB_ReportCount,1), (_v)
#define HidLogiMin8(_v)     \
    HidGlobalItem(HID_ITEM_GLB_LogicMin,1), (_v)
#define HidLogiMin16(_v)    \
    HidGlobalItem(HID_ITEM_GLB_LogicMin,2), WordData(_v)
#define HidLogiMax8(_v)     \
    HidGlobalItem(HID_ITEM_GLB_LogicMax,1), (_v)
#define HidLogiMax16(_v)    \
    HidGlobalItem(HID_ITEM_GLB_LogicMax,2), WordData(_v)
#define HidPhysicalMin8(_v)     \
    HidGlobalItem(HID_ITEM_GLB_PhysicalMin,1), (_v)
#define HidPhysicalMin16(_v)    \
    HidGlobalItem(HID_ITEM_GLB_PhysicalMin,2), WordData(_v)
#define HidPhysicalMax8(_v)     \
    HidGlobalItem(HID_ITEM_GLB_PhysicalMax,1), (_v)
#define HidPhysicalMax16(_v)    \
    HidGlobalItem(HID_ITEM_GLB_PhysicalMax,2), WordData(_v)
#define HidUnit(_us,_ut)                \
    HidGlobalItem(HID_ITEM_GLB_Unit, 1),((_ut<<4)+(_us))
#define HidUnitExp(_v)            \
    HidGlobalItem(HID_ITEM_GLB_UnitExponent,1),(_v)

/*HID report Local items define*/
#define HidLocalItem(_tag,_size)        \
    HidReportFormat(HID_ITEM_TYP_LOCAL, _tag, _size)

    /**/
    #define HID_ITEM_LOC_USAGE                0x00
        /**/
        /*belong to generical desktop usage page*/
        #define HID_USAGE_Pointer            0x01
        #define HID_USAGE_Mouse                0x02
        #define HID_USAGE_Joystick            0x04
        #define HID_USAGE_Gamepad            0x05
        #define HID_USAGE_Keyboard            0x06
        #define HID_USAGE_Keypad            0x07

        #define HID_USAGE_AxisX                0x30
        #define HID_USAGE_AxisY                0x31
        #define HID_USAGE_AxisZ                0x32
        #define HID_USAGE_AxisRx            0x33
        #define HID_USAGE_AxisRy            0x34
        #define HID_USAGE_AxisRz            0x35
        #define HID_USAGE_Slider            0x36
        #define HID_USAGE_Dial                0x37
        #define HID_USAGE_Wheel                0x38
        #define HID_USAGE_Hatswitch            0x39

    #define HID_ITEM_LOC_Min                0x01
    #define HID_ITEM_LOC_Max                0x02
    #define HID_ITEM_LOC_DesignatorIdx        0x03
    #define HID_ITEM_LOC_DesignatorMin        0x04
    #define HID_ITEM_LOC_DesignatorMax        0x05
    #define HID_ITEM_LOC_StringIdx            0x06
    #define HID_ITEM_LOC_StringMin            0x07
    #define HID_ITEM_LOC_StringMax            0x08
    #define HID_ITEM_LOC_SetDelimiter        0x09
#define HidUsage(_v)        \
    HidLocalItem(HID_ITEM_LOC_USAGE,1), (_v)
#define HidUsage1(_v)        \
    HidLocalItem(HID_ITEM_LOC_USAGE,2), WordData(_v)

#define HidUsageMin8(_v)    \
    HidLocalItem(HID_ITEM_LOC_Min,1),(_v)
#define HidUsageMin16(_v)    \
    HidLocalItem(HID_ITEM_LOC_Min,2),WordData(_v)
#define HidUsageMax8(_v)    \
    HidLocalItem(HID_ITEM_LOC_Max,1),(_v)
#define HidUsageMax16(_v)    \
    HidLocalItem(HID_ITEM_LOC_Max,2),WordData(_v)
/////////////////////////////////////////////////////////////////////////////////////////////////
//some HID report descriptor examples
#define Hid_Mouse_Report(rptDescName)    \
    HidUsagePage(HID_USAGEPAGE_Generic),\
    HidUsage(HID_USAGE_Mouse),\
    HidCollectionApp(),\
        HidUsage(HID_USAGE_Pointer),\
        HidCollectionPhy(),\
            HidUsagePage(HID_USAGEPAGE_Buttons),\
            HidUsageMin8(1),\
            HidUsageMax8(3),\
            HidLogiMin8(0),\
            HidLogiMax8(1),\
            HidReportCount(3),\
            HidReportBits(1),\
            HidInput(0,1,0),\
            \
            HidReportCount(1),\
            HidReportBits(5),\
            HidInput(1, 0, 0),\
            \
            HidUsagePage(HID_USAGEPAGE_Generic),\
            HidUsage(HID_USAGE_AxisX),\
            HidUsage(HID_USAGE_AxisY),\
            HidLogiMin8(-127),\
            HidLogiMax8(127),\
            HidReportBits(8),\
            HidReportCount(2),\
            HidInput(0, 1, 1),\
        HidEndCollection(),\
    HidEndCollection()

#define USB_HIDT_Report()    \
    HidUsagePage(HID_USAGEPAGE_Generic),\
    HidUsage(HID_USAGE_Mouse),\
    HidCollectionApp(),\
        HidUsage(HID_USAGE_Pointer),\
        HidCollectionPhy(),\
            HidUsagePage(HID_USAGEPAGE_Buttons),\
            HidUsageMin8(1),\
            HidUsageMax8(3),\
            HidLogiMin8(0),\
            HidLogiMax8(1),\
            HidReportCount(3),\
            HidReportBits(1),\
            HidInput(0,1,0),\
            \
            HidReportCount(1),\
            HidReportBits(5),\
            HidInput(1, 0, 0),\
            \
            HidUsagePage(HID_USAGEPAGE_Generic),\
            HidUsage(HID_USAGE_AxisX),\
            HidUsage(HID_USAGE_AxisY),\
            HidLogiMin8(-127),\
            HidLogiMax8(127),\
            HidReportBits(8),\
            HidReportCount(2),\
            HidInput(0, 1, 1),\
        HidEndCollection(),\
    HidEndCollection()

#define HID_KBD_Report() \
    HidUsagePage(HID_USAGEPAGE_Generic),\
    HidUsage(HID_USAGE_Keyboard),\
    HidCollectionApp(),\
        HidUsage(HID_USAGE_Keypad),\
        HidUsageMin8(224),\
        HidUsageMax8(231),\
        HidLogiMin8(0),\
        HidLogiMax8(1),\
        HidReportBits(1),\
        HidReportCount(8),\
        HidInput(0,1, 0),\
        \
        HidReportBits(1),\
        HidReportCount(8),\
        HidUsagePage(HID_USAGEPAGE_LED),\
        HidUsageMin8(1),\
        HidUsageMax8(5),\
        HidOutput(0,1,0),\
        \
        HidReportBits(3),\
        HidReportCount(1),\
        HidOutput(1, 0, 0),\
        \
        HidReportBits(8),\
        HidReportCount(6),\
        HidLogiMin8(0),\
        HidLogiMax8(101),\
        HidUsagePage(HID_USAGEPAGE_Keyboard),\
        HidUsageMin8(0),\
        HidUsageMax8(101),\
        HidInput(0, 0, 0),\
    HidEndCollection()

#define USB_Audio_Report()    \
    HidUsagePage(HID_USAGEPAGE_Customer),\
    HidUsage(0x01), \
    HidCollectionApp(),\
    \
        /*volume control*/\
        HidUsage(HUC_VolumnMute),\
        HidUsage(HUC_VolumnInc),\
        HidUsage(HUC_VolumnDec),\
        HidLogiMin8(0),\
        HidLogiMax8(1),\
        HidReportBits(1),\
        HidReportCount(3),\
        HidInput(0, 1, 0),\
        \
        HidReportBits(1),\
        HidReportCount(5),\
        HidInput(1, 1, 0),\
        \
        /*对音量的控制宣告*/\
        HidUsage(HUC_MediaPlay),\
        HidUsage(HUC_MediaStop),\
        HidUsage(HUC_MediaPause),\
        HidUsage(HUC_MediaNext),\
        HidUsage(HUC_MediaPrev),\
        HidUsage(HUC_MediaRecord),\
        HidUsage(HUC_MediaFastFwd),\
        HidUsage(HUC_MediaRewind),\
        /*对媒体控制的宣告*/\
        HidReportBits(1),\
        HidReportCount(8),\
        HidInput(0, 1, 0),\
        \
        HidUsage(0x00),\
        HidReportBits(8),\
        HidReportCount(6),\
        HidInput(0, 1, 0),\
        \
        HidUsage(0x00),\
        HidReportBits(8),\
        HidReportCount(8),\
        HidOutput(0, 0, 0),\
        \
    HidEndCollection()


#endif

