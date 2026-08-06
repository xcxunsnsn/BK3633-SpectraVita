/*************************************************************
 * @file        usb.h
 * @brief        APIs for usb App
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */
#ifndef _USB_H_ //this file's unique symbol checking section
#define _USB_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef void (*USB_MOD_CTRL_CBK)(int en_dis);
/*
 *函数原型:
 void USB_MOD_CTRL_CBK_PROC(int en_dis){
    if(en_dis){
        //打开usb模块电源
        //打开USB模块时钟
        //打开usb其他相关控制
    }else{
        //关闭usb其他相关控制
        //关闭usb模块时钟
        //关闭usb模块电源
    }
 }
*/

typedef void (*USB_MOD_IE_CBK)(int en_dis);
/*
 *函数原型:
 void USB_MOD_IE_CBK_PROC(int en_dis){
    if(en_dis){
        //usb中断使能
    }else{
        //usb中断禁能
    }
 }
*/

/*define current USB App feature */
#define USB_MAX_CONFIG_NUM                1
#define USB_MAX_STRING_NUM                2
#define USB_MIC_ONLY                   1

#define _VID_                    0x25a7
#define _PID_                    0x192a

#define _Vendor_String_ID_        0x01
#define _Product_String_ID_        0x02

#if(USB_MIC_ONLY)
enum {
    USB_INTFID_AudioAC=0,
    USB_INTFID_AudioAS_MIC,
    
    USB_INTFID_Hid_Mse,
    USB_INTFID_Hid_Dbg,
    USB_INTFID_Hid_MKey,
    
    USB_MAX_INTERFACE_NUM,
    USB_INTFID_AudioAS_SPK,
};
#else
enum {
    USB_INTFID_AudioAC=0,
    USB_INTFID_AudioAS_MIC,
    USB_INTFID_AudioAS_SPK,
    USB_INTFID_Hid_Mse,
    USB_INTFID_Hid_Dbg,
    USB_INTFID_Hid_MKey,
    USB_MAX_INTERFACE_NUM,
};
#endif

enum{
    USB_ENDPID_Default=0,
    USB_ENDPID_Audio_MIC,
    USB_ENDPID_Audio_SPK,

    USB_ENDPID_Hid_MSE,
    USB_ENDPID_Hid_MSE_OUT,
    USB_ENDPID_Hid_DBG_OUT,
    USB_ENDPID_Hid_DBG_IN,
    USB_ENDPID_Hid_MKEY,
    USB_MAX_ENDPOINT_NUM,
};
//注意：
//3435\3431Q usb endpoint个数为5（0~4）， fifo大小为64Bytes，TXMAXP，RXMAXP最大值为8
#define AUDIO_MIC_SAMPRATE            16000
#define AUDIO_MIC_CHNs                1
#define AUDIO_MIC_BR                16

#define AUDIO_SPK_SAMPRATE            32000
#define AUDIO_SPK_CHNs                1
#define AUDIO_SPK_BR                16
#define AUDIO_MAXP(chns,br,sr)        (((sr)/1000*1000)==(sr))?((sr/1000)*chns*br/8):((sr/1000+1)*chns*br/8)

#define USBAUDIO_IT_MIC                1
#define USBAUDIO_IT_SPK                2
#define USBAUDIO_OT_MIC                3
#define USBAUDIO_OT_SPK                4

extern volatile unsigned char bUsbConfig; //usb configed value

/*
 *函数名:
 *    AplUsb_StartTx
 *功能:
 *    启动usb在指定endp发送数据
 *参数:
 *    1.epn:endp号
 *    2.buf,len:数据参数
 *返回:
 *    无
 *特殊:
 *
*/
void AplUsb_StartTx(int epn,void*buf,int len);

/*
 *函数名:
 *    AplUsb_SetTxCbk
 *功能:
 *    指定endp发送数据cbk
 *参数:
 *    1.epn:endp号
 *    2.cbk:CALLBACK函数
 *返回:
 *    无
 *特殊:
 *
*/
void AplUsb_SetTxCbk(int endp_no,void*cbk);

/*
 *函数名:
 *    AplUsb_SetRxCbk
 *功能:
 *    指定endp收到数据cbk
 *参数:
 *    1.epn:endp号
 *    2.cbk:CALLBACK函数
 *返回:
 *    无
 *特殊:
 *
*/
void AplUsb_SetRxCbk(int endp_no,void*cbk);

/*
 *函数名:
 *    AplUsb_GetRxBuf
 *功能:
 *    获取endp接收数据指针
 *参数:
 *    1.epn:endp号
 *返回:
 *    接收数据缓冲区指针
 *特殊:
 *
*/
void *AplUsb_GetRxBuf(int endp_no);

/*
 *函数名:
 *    AplUsb_GetTxBuf
 *功能:
 *    获取endp发送数据指针
 *参数:
 *    1.epn:endp号
 *返回:
 *    发送数据缓冲区指针
 *特殊:
 *
*/
void *AplUsb_GetTxBuf(int endp_no);

/*
 *函数名:
 *    AplUsb_GetTxCbk
 *功能:
 *    获取指定endp发送数据cbk
 *参数:
 *    1.epn:endp号
 *返回:
 *    cbk函数指针
 *特殊:
 *
*/
void *AplUsb_GetTxCbk(int endp_no);

/*
 *函数名:
 *    AplUsb_GetRxCbk
 *功能:
 *    获取指定endp收到数据cbk
 *参数:
 *    1.epn:endp号
 *返回:
 *    cbk函数指针
 *特殊:
 *
*/
void *AplUsb_GetRxCbk(int endp_no);

/*
 *函数名:
 *    AplUsb_GetSOF
 *功能:
 *    获取USB SOF计数器
 *参数:
 *    1.无
 *返回:
 *    SOF计数值
 *特殊:
 *
*/
int AplUsb_GetSOF(void);

/*
 *函数名:
 *    AplUsb_IsConfigured
 *功能:
 *    usb设备是否已配置
 *参数:
 *    1.无
 *返回:
 *    0=未配置,1=已配置
 *特殊:
 *
*/
int AplUsb_IsConfigured(void);

/*
 *函数名:
 *    usb_init
 *功能:
 *    usb device 初始化
 *参数:
 *    1.usb_mod_ctrl_cbk:usb modal控制回调，usb模块依赖于芯片其他部分的初始化：时钟、电源开关等
 *    2.usb_mod_int_ie_cbk:usb中断使能/禁能回调
 *返回:
 *    无
 *特殊:
 *
*/
void usb_init(void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk);

/*
 *函数名:
 *    usb_deinit
 *功能:
 *    usb模块关闭
 *参数:
 *    1.usb_mod_ctrl_cbk:usb modal控制回调，usb模块依赖于芯片其他部分的初始化：时钟、电源开关等
 *    2.usb_mod_int_ie_cbk:usb中断使能/禁能回调
 *返回:
 *    无
 *特殊:
 *
*/
void usb_deinit(void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk);
void check_usb_suspendmode(void);
void usb_suspend_wakeup(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif
/*end file*/
