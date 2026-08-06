#include <stdint.h>        // standard integer definition
#include <string.h>        // string manipulation
#include <stddef.h>        // standard definition
#include "user_config.h"
#include "wdt.h"

void wdt_disable(void)
{
    WDT_REG0X2=0x00;//close WDT
    WDT_REG0X1=WDKEY_ENABLE1;
    WDT_REG0X1=WDKEY_ENABLE2;
}

//每个单位31.25us，最大0xffffff，约524s
void wdt_enable(uint32_t wdt_cnt)
{
    WDT_REG0X1=0x00;
    WDT_REG0X2  = wdt_cnt;     
    WDT_REG0X1=WDKEY_ENABLE1;
    WDT_REG0X1=WDKEY_ENABLE2;    
}


void wdt_feed(void)
{
    WDT_REG0X1=WDKEY_ENABLE1;
    WDT_REG0X1=WDKEY_ENABLE2;    
}


