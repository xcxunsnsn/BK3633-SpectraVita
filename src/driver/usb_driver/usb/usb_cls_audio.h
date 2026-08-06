/*************************************************************
 * @file        usb_cls_audio.h
 * @brief        USB Audio class definition
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */
#ifndef _USB_CLS_AUDIO_H_
#define _USB_CLS_AUDIO_H_

/*Audio subclass code define*/
#define AUDIO_SUBCLASS_UNDEF        0x00
#define AUDIO_SUBCLASS_CONTROL        0x01
#define AUDIO_SUBCLASS_STREAM        0x02
#define AUDIO_SUBCLASS_MIDISTREAM    0x03

/*Audio protocol code define*/
#define AUDIO_PROTOCOL_UNDEF        0x00

/*Audio class-specific descriptor type define*/
#define AUDIO_CS_UNDEF            0x20
#define AUDIO_CS_DEVICE            0x21
#define AUDIO_CS_CONFIG            0x22
#define AUDIO_CS_STRING            0x23
#define AUDIO_CS_INTERFACE        0x24
#define AUDIO_CS_ENDPOINT        0x25

/*Audio class-specific Audio-Control interface descriptor subtype*/
#define AUDIO_ACI_UNDEF            0x00
#define AUDIO_ACI_HEADER        0x01
#define AUDIO_ACI_INPUT_TERM    0x02
#define AUDIO_ACI_OUTPUT_TERM    0x03
#define AUDIO_ACI_MIXER_UNIT    0x04
#define AUDIO_ACI_SEL_UNIT        0x05
#define AUDIO_ACI_FEATURE_UNIT    0x06
#define AUDIO_ACI_PROCESS_UNIT    0x07
#define AUDIO_ACI_EXT_UINT        0x08

/*Audio class-specific Audio-Stream interface descriptor subtype*/
#define AUDIO_ASI_UNDEF            0x00
#define AUDIO_ASI_GENERAL        0x01
#define AUDIO_ASI_FORMAT_TYPE    0x02
#define AUDIO_ASI_FORMAT_SPEC    0x03

/*Audio class-specific processing unit process type*/
#define AUDIO_ACI_PU_UNDEF                    0x00
#define AUDIO_ACI_PU_UP_DOWNMIX_PROCESS        0x01
#define AUDIO_ACI_PU_DOLBY_PROLOGIC_PROCESS    0x02
#define AUDIO_ACI_PU_3D_STEREO_PROCESS        0x03
#define AUDIO_ACI_PU_REVERBERATION_PROCESS    0x04
#define AUDIO_ACI_PU_CHORUS_PROCESS            0x05
#define AUDIO_ACI_PU_DYN_RANGE_COMP_PROCESS    0x06

/*Audio class-specific endpoint descriptor subtypes*/
#define AUDIO_ACS_EP_UNDEF            0x00
#define AUDIO_ACS_EP_GENERAL        0x01

/*Audio class-specific request codes */
#define AUDIO_RC_UNDEF            0x00
#define AUDIO_RC_SET_CUR        0x01
#define AUDIO_RC_GET_CUR        0x81
#define AUDIO_RC_SET_MIN        0x02
#define AUDIO_RC_GET_MIN        0x82
#define AUDIO_RC_SET_MAX        0x03
#define AUDIO_RC_GET_MAX        0x83
#define AUDIO_RC_SET_RES        0x04
#define AUDIO_RC_GET_RES        0x84
#define AUDIO_RC_SET_MEM        0x05
#define AUDIO_RC_GET_MEM        0x85
#define AUDIO_RC_GET_STAT        0xff

/*Audio control selector commands code */

/*Audio Terminal control selector(CS)*/
#define AUDIO_ACC_TERM_CS_UNDEF            0x00
#define AUDIO_ACC_TERM_CS_COPY_PROTECT    0x01

/*Audio selector unit control selector(CS)*/

/*Audio Feature Unit(FU) control selector(CS)*/
#define AUDIO_ACC_FU_CS_UNDEF            0x00
#define AUDIO_ACC_FU_CS_MUTE            0x01
#define AUDIO_ACC_FU_CS_VOLUMN            0x02
#define AUDIO_ACC_FU_CS_BASS            0x03
#define AUDIO_ACC_FU_CS_MID                0x04
#define AUDIO_ACC_FU_CS_TREBLE            0x05
#define AUDIO_ACC_FU_CS_GRAPH_EQU        0x06
#define AUDIO_ACC_FU_CS_AGC                0x07
#define AUDIO_ACC_FU_CS_DELAY            0x08
#define AUDIO_ACC_FU_CS_BASS_BOOST        0x09
#define AUDIO_ACC_FU_CS_LOUDNESS        0x0a

/*Audio Processing Unit control selector(CS)*/
/*Audio Processing Unit control up/down-mix selector*/
#define AUDIO_ACC_PU_CS_UD_UNDEF        0x00
#define AUDIO_ACC_PU_CS_UD_ENABLE        0x01
#define AUDIO_ACC_PU_CS_UD_SELMODE        0x02

/*Audio Processing Unit control dynamic range compressor control(CS)*/
#define AUDIO_ACC_PU_CS_DR_UNDEF        0x00
#define AUDIO_ACC_PU_CS_DR_ENABLE        0x01
#define AUDIO_ACC_PU_CS_DR_COMP_RATE    0x02
#define AUDIO_ACC_PU_CS_DR_MAXAMPL        0x03
#define AUDIO_ACC_PU_CS_DR_THRESGOLD    0x04
#define AUDIO_ACC_PU_CS_DR_ATTACK_TIME    0x05
#define AUDIO_ACC_PU_CS_DR_RELEASE_TIME    0x06

/*Audio endpoint control selector(CS)*/
#define AUDIO_EPC_CS_UNDEF                0x00
#define AUDIO_EPC_CS_SAMPLE_FREQ        0x01
#define AUDIO_EPC_CS_PITCH                0x02

/*audio terminal types define */
/*audio usb type terminal*/
#define AUDIO_TERMTYPE_USB_UNDEF            0x0100
#define AUDIO_TERMTYPE_USB_STREAM            0x0101
#define AUDIO_TERMTYPE_USB_VENDOR            0x01ff

/*audio input type terminal*/
#define AUDIO_TERMTYPE_IT_UNDEF                0x0200
#define AUDIO_TERMTYPE_IT_MIC                0x0201
#define AUDIO_TERMTYPE_IT_DesktopMic        0x0202
#define AUDIO_TERMTYPE_IT_PersonalMic        0x0203
#define AUDIO_TERMTYPE_IT_OmniDirectMic        0x0204
#define AUDIO_TERMTYPE_IT_MicArray            0x0205
#define AUDIO_TERMTYPE_IT_PersonalMicArray    0x0206

/*audio output type terminal*/
#define AUDIO_TERMTYPE_OT_UNDEF                0x0300
#define AUDIO_TERMTYPE_OT_SPK                0x0301
#define AUDIO_TERMTYPE_OT_Headphones        0x0302
#define AUDIO_TERMTYPE_OT_VR                0x0303    //head mounted display audio,for VR device
#define AUDIO_TERMTYPE_OT_DesktopSpk        0x0304
#define AUDIO_TERMTYPE_OT_RoomSpk            0x0305
#define AUDIO_TERMTYPE_OT_CommSpk            0x0306
#define AUDIO_TERMTYPE_OT_LFESpk            0x0307    //low frequency effects speaker

/*audio bi-directional type terminal*/
#define AUDIO_TERMTYPE_BIDT_UNDEF            0x0400
#define AUDIO_TERMTYPE_BIDT_Handset            0x0401
#define AUDIO_TERMTYPE_BIDT_Headset            0x0402

/*audio channel config*/
#define AUDIO_CHANNEL_CONFIG(l,r,c,lfe,ls,rs,lc,rc,s,sl,sr,t)    \
    (l)|/*left front channel*/\
    (r<<1)|/*right front channel*/\
    (c<<2)|/*center front channel*/\
    (lfe<<3)|/*low frequency enhancement*/\
    (ls<<4)|/*left surround channel*/\
    (rs<<5)|/*right surround channel*/\
    (lc<<6)|/*left surround channel(in front)*/\
    (rc<<7)|/*right surround channel(in front)*/\
    (s<<8)|/*surround channel(rear)*/\
    (sl<<9)|/*side left channel(left wall)*/\
    (sr<<10)|/*side right channel(right wall)*/\
    (t<<11)/*top channel(over head)*/


#define AUDIO_CONTROLS(m,v,b,mid,t,ge,agc,d,bb,l)    \
    WordData(\
        (m)|/*mute*/\
        (v<<1)|/*volume*/\
        (b<<2)|/*bass*/\
        (mid<<3)|/**/\
        (t<<4)|/*treble*/\
        (ge<<5)|/*graphic equalizer*/\
        (agc<<6)|/**/\
        (d<<7)|/*delay*/\
        (bb<<8)|/*bass boost*/\
        (l<<9)/*loudness*/\
        )

/*Audio class descriptor set*/
#define USB_AUDIO_Config(szdn,oid,nIntf)    \
    0x09,\
    USB_DESCTYPE_CONFIG,\
    WordData((szdn)),\
    USB_CONFIG_INTF_NUM(nIntf),\
    USB_OBJECT_NO(oid),\
    USB_STRING_INDEX(0),\
    USB_CONFIG_ATTRIBUTE(0,0),\
    USB_CONFIG_MAX_POWER(90)

#define USB_AUDIO_InterfaceAC(oid)    \
    9,\
    USB_DESCTYPE_INTERFACE,\
    USB_OBJECT_NO(oid),\
    USB_INTF_ALTERSETTING(0),\
    USB_INTF_ENDP_NUM(0),\
    USB_INTF_CLASS(USB_DEVICE_CLASS_AUDIO),\
    USB_INTF_SUBCLASS(AUDIO_SUBCLASS_CONTROL),\
    USB_INTF_PROTOCOL(0),\
    USB_STRING_INDEX(0)

//USB_ACS_ACI_Header用法，后跟AS接口列表
#define USB_ACS_ACI_Header(sz_acdn,n)    \
    9+((n)-1),\
    AUDIO_CS_INTERFACE,\
    AUDIO_ACI_HEADER,\
    USB_DEVICE_VERSION(0x0100),\
    WordData(sz_acdn),\
    n/*number of AS interfaces*/\
    /*ID list of AS interface */


#define USB_ACS_ACI_IT(tid,otid,term_typ,nchns,ccfg)    \
    12,\
    AUDIO_CS_INTERFACE,\
    AUDIO_ACI_INPUT_TERM,\
    tid,\
    WordData(term_typ),\
    otid,/*the OT associated to this IT*/\
    nchns,/*bNrChannels,*/\
    WordData(ccfg),/*wChannelConfig,need a para fmt:AUDIO_CHANNEL_CONFIG*/\
    0,/*iChannelNames,第一个非预设通道名称索引*/\
    0/*iTeminal,NOT USING*/

#define USB_ACS_ACI_OT(tid,itid,term_typ,sid)    \
    9,\
    AUDIO_CS_INTERFACE,\
    AUDIO_ACI_OUTPUT_TERM,\
    tid,\
    WordData(term_typ),\
    itid,/*the IT associated to this OT*/\
    sid,/*source ID of Unit or terminal*/\
    0x00

//USB_ACS_ACI_FU后跟(chns+1)个AUDIO_CONTROLS，然后再加一个0
#define USB_ACS_ACI_FU(uid,sid,chns)        \
    6+((chns+1)*2)+1,\
    AUDIO_CS_INTERFACE,\
    AUDIO_ACI_FEATURE_UNIT,\
    uid,/*my own ID*/\
    sid,/*source ID of Unit or terminal,TBD*/\
    2/*每个AudioControl的大小*/


/*
#define USB_ACS_ACI_PU(dn,uid,ut,spins,ccfg)        \
    SZB(dn),\
    AUDIO_CS_INTERFACE,\
    AUDIO_ACI_PROCESS_UNIT,\
    uid,\
    WordData(ut),\
    spins,\
    ccfg,\
    0
*/

#define USB_ACS_ACI_SU(dn,uid,spins)    \
    (dn),\
    AUDIO_CS_INTERFACE,\
    AUDIO_ACI_SEL_UNIT,\
    uid,\
    spins,\
    0
//TO BE COMPLETED
#define USB_ACS_ACI_MU(dn,uid,spins,ccfg,ctl)        \
    (dn),\
    AUDIO_CS_INTERFACE,\
    AUDIO_ACI_MIXER_UNIT,\
    uid,\
    spins,/*set of input pins,*/\
    ccfg,\
    ctl,\
    0
/*
#define USB_ACS_ACI_AI(dn, sid)        \
    SZB(dn),\
    AUDIO_CS_INTERFACE,\
    audi
*/

#define USB_AUDIO_EndpiontAC(inp,addr,maxp,interval)    \
    9,\
    USB_DESCTYPE_ENDPOINT,\
    USB_ENDP_ADDRESS(inp, addr),\
    USB_ENDP_ATTRIBUTE(USB_ENDP_INT),\
    USB_MAX_PACKET_LEN(maxp),\
    USB_ENDP_INTERVAL(interval),\
    0,\
    0

#define USB_AUDIO_InterfaceAS(oid,altoid)    \
    9,\
    USB_DESCTYPE_INTERFACE,\
    USB_OBJECT_NO(oid),\
    USB_INTF_ALTERSETTING(altoid),\
    USB_INTF_ENDP_NUM(1),\
    USB_INTF_CLASS(USB_DEVICE_CLASS_AUDIO),\
    USB_INTF_SUBCLASS(AUDIO_SUBCLASS_STREAM),\
    USB_INTF_PROTOCOL(0),\
    USB_STRING_INDEX(0)

#define USB_AUDIO_IntfAltAS_BW0(oid,altoid)        \
    9,\
    USB_DESCTYPE_INTERFACE,\
    USB_OBJECT_NO(oid),\
    USB_INTF_ALTERSETTING(altoid),\
    USB_INTF_ENDP_NUM(0),\
    USB_INTF_CLASS(USB_DEVICE_CLASS_AUDIO),\
    USB_INTF_SUBCLASS(AUDIO_SUBCLASS_STREAM),\
    USB_INTF_PROTOCOL(0),\
    USB_STRING_INDEX(0)

#define USB_ACS_ASI_General(tid,dly,fmt)        \
    7,\
    AUDIO_CS_INTERFACE,\
    AUDIO_ASI_GENERAL,\
    tid,/*the termainal ID linked to*/\
    dly,\
    WordData(fmt)/*PCM=0x0001*/

#define AUDIO_ASI_FT_TypeI        0x01
#define AUDIO_ASI_FT_TypeII        0x02

#define USB_ACS_ASI_FmtTypeI(chns,br,nSR)    \
    8+(nSR)*3,\
    AUDIO_CS_INTERFACE,\
    AUDIO_ASI_FORMAT_TYPE,\
    AUDIO_ASI_FT_TypeI,\
    chns,/*number of channels*/\
    2,/*sizeof subframe in bytes*/\
    br,/*bit resolution of a subframe*/\
    (nSR)


#define AUDIO_SYNCMODE_ASYNC    0x01
#define AUDIO_SYNCMODE_ADAPTIVE    0x02
#define AUDIO_SYNCMODE_SYNC        0x03

#define USB_AUDIO_EndpiontAS(in_out,epn,syncm,mpn)    \
    9,\
    USB_DESCTYPE_ENDPOINT,\
    USB_ENDP_ADDRESS(in_out, epn),\
    USB_ENDP_ATTRIBUTE(USB_ENDP_ISO|(syncm<<2)),\
    WordData(USB_MAX_PACKET_LEN(mpn)),\
    USB_ENDP_INTERVAL(1),\
    0,0

#define USB_AUDIO_EndpAS_IN(epn,syncm,mpn)    \
    USB_AUDIO_EndpiontAS(1,epn,syncm,mpn)

#define USB_AUDIO_EndpAS_OUT(epn,syncm,mpn)    \
    USB_AUDIO_EndpiontAS(0,epn,syncm,mpn)

#define USB_ACS_EP_General(ff)    \
    7,\
    AUDIO_CS_ENDPOINT,\
    AUDIO_ACS_EP_GENERAL,\
    ff,\
    0,\
    0,\
    0

#endif

