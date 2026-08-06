/**
****************************************************************************************
* @addtogroup RTC
* @ingroup beken corp
* @brief RTC
* @author Alen
*
* This is the driver block for RTC
* @{
****************************************************************************************
*/


#ifndef __RTC_H__
#define __RTC_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//=====================================================================================
//RTC
//=====================================================================================
#define REG_APB6_RTC_CFG                (*((volatile unsigned long *)   0x00806600))
#define BIT_RTC_ENABLE                  0
#define BIT_RTC_CLEAR                   1
#define BIT_RTC_ALARM_EN                2
#define BIT_RTC_ALARM_MODE              3
#define SET_RTC_ENABLE                  (0x1 << BIT_RTC_ENABLE     )
#define SET_RTC_CLEAR                   (0x1 << BIT_RTC_CLEAR      )
#define SET_RTC_ALARM_EN                (0x1 << BIT_RTC_ALARM_EN   )
#define SET_RTC_ALARM_MODE              (0x1 << BIT_RTC_ALARM_MODE )

#define REG_APB6_RTC_UNIT               (*((volatile unsigned long *)   0x00806604))
#define BIT_RTC_DIV                     0
#define BIT_RTC_MSUNIT                  2
#define BIT_RTC_SUNIT                   8
#define SET_RTC_DIV                     (0x3    << BIT_RTC_DIV   )
#define SET_RTC_MSUNIT                  (0x3F   << BIT_RTC_MSUNIT)
#define SET_RTC_SUNIT                   (0xFFFF << BIT_RTC_SUNIT )

#define REG_APB6_RTC_SET                (*((volatile unsigned long *)   0x00806608))
#define BIT_RTC_SECOND                  0
#define BIT_RTC_MINUTE                  6
#define BIT_RTC_HOUR                    12
#define BIT_RTC_WEEK                    17
#define SET_RTC_SECOND                  (0x3F << BIT_RTC_SECOND )
#define SET_RTC_MINUTE                  (0x3F << BIT_RTC_MINUTE )
#define SET_RTC_HOUR                    (0x1F << BIT_RTC_HOUR   )
#define SET_RTC_WEEK                    (0x7  << BIT_RTC_WEEK   )

#define REG_APB6_RTC_ALM_TIME           (*((volatile unsigned long *)   0x0080660C))
#define BIT_RTC_ALARM_SECOND            0
#define BIT_RTC_ALARM_MINUTE            6
#define BIT_RTC_ALARM_HOUR              12
#define BIT_RTC_ALARM_MILLISEC          17
#define SET_RTC_ALARM_SECOND            (0x3F  << BIT_RTC_ALARM_SECOND   )
#define SET_RTC_ALARM_MINUTE            (0x3F  << BIT_RTC_ALARM_MINUTE   )
#define SET_RTC_ALARM_HOUR              (0x1F  << BIT_RTC_ALARM_HOUR     )
#define SET_RTC_ALARM_MILLISEC          (0x3FF << BIT_RTC_ALARM_MILLISEC )

#define REG_APB6_RTC_ALM_FLAG           (*((volatile unsigned long *)   0x00806610))

// RTC Test Firmware Definition
#define BK_RTC_CLOCK_MODE         0
#define BK_RTC_MILLISEC_MODE      1
//=====================================================================================

//----------------------------------------------
// RTC driver description
//----------------------------------------------
typedef struct
{
    unsigned char       second;     // second, 0~59
    unsigned char       minute;     // minute, 0~59
    unsigned char       hour;       // hour,   0~23
    unsigned char       week_day;   // week_day, 0~6
} RTC_DATE_DESC;


#define ENABLE_RTC                        0

#define ICU_RTC_CLK_PWD_CLEAR()        do {REG_AHB0_ICU_RTCCLKCON = 0x00;} while (0)
#define ICU_RTC_CLK_PWD_SET()          do {REG_AHB0_ICU_RTCCLKCON = 0x01;} while (0)


void rtc_init(RTC_DATE_DESC *p_RTC_date_desc);
void rtc_enable(void);
void rtc_disable(void);
void rtc_alarm_init(unsigned char ucMode, RTC_DATE_DESC *p_RTC_alarm_time, 
                           unsigned long ulMiiliSecond, void (*p_Int_Handler)(void));
void rtc_alarm_enable(void);
void rtc_alarm_disable(void);
void rtc_set_time(RTC_DATE_DESC *p_RTC_date_desc);
void rtc_get_time(RTC_DATE_DESC *p_RTC_date_desc);
void rtc_int_handler_clear(void);

void rtc_isr(void);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __RTC_H__ */

