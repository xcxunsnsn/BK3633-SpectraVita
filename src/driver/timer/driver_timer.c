//======================================================
// File Name:     BK_TIMER_API.c
// Author:        yubei
// Created Date:  @2015-02-12
// Description:   Project BK3231S - Test Firmware for Massive Production
//                GPIO protocol
//                Source from BK3231  firmware: bk3000_timer.c
//======================================================
#include <string.h>
#include "BK3633_RegList.h"
#include "driver_timer.h"


volatile uint8_t bk_timer_hit=0x00;
volatile uint32_t bk_timer0_T0_cnt,bk_timer0_T1_cnt;
volatile uint32_t bk_timer1_T0_cnt,bk_timer1_T1_cnt;
volatile uint32_t     bk_timer0_T2_cnt,bk_timer1_T2_cnt;


//#define TIMER_CLK_IS_1M

static TIMER_INT_CALLBACK_T timer0_0_int_cb = NULL;
static TIMER_INT_CALLBACK_T timer0_1_int_cb = NULL;

static TIMER_INT_CALLBACK_T timer0_2_int_cb = NULL;

static TIMER_INT_CALLBACK_T timer1_0_int_cb = NULL;

static TIMER_INT_CALLBACK_T timer1_1_int_cb = NULL;

static TIMER_INT_CALLBACK_T timer1_2_int_cb = NULL;

void timer_cb_register(uint8_t Timer, uint8_t index,TIMER_INT_CALLBACK_T cb)
{
    if(Timer == 0)
    {
        switch(index)
        {
            case 0:
            {
               if(cb)
                {
                    timer0_0_int_cb = cb;
                }
               break;
            }
            case 1:
            {
               if(cb)
                {
                    timer0_1_int_cb = cb;
                }
               break;
            }
            case 2:
            {
               if(cb)
                {
                    timer0_2_int_cb = cb;
                }
               break;
            }
        }
    }
    if(Timer == 1)
    {
        switch(index)
        {
            case 0:
            {
               if(cb)
                {
                    timer1_0_int_cb = cb;
                }
               break;
            }
            case 1:
            {
               if(cb)
                {
                    timer1_1_int_cb = cb;
                }
               break;
            }
            case 2:
            {
               if(cb)
                {
                    timer1_2_int_cb = cb;
                }
               break;
            }
        }
    }
}

void Timer_Initial(uint8_t timer,uint8_t src,uint8_t div)//timer:0:T0,1:T1  ; src:0:32k,1:16M
{
    if(timer == 0)
    {
        if(src == 0 )
        {
            clrf_SYS_Reg0x4_tim0_sel;
        }
        else
        {
            setf_SYS_Reg0x4_tim0_sel;
        }
        addTIMER0_Reg0x0 = 0;
        addTIMER0_Reg0x1 = 0;
        addTIMER0_Reg0x2 = 0;
        addTIMER0_Reg0x3 = 0;
        set_TIMER0_Reg0x3_clk_div(div);

        clrf_SYS_Reg0x3_tim0_pwd;

    }
    else if(timer == 1)
    {
        if(src == 0 )
        {
            clrf_SYS_Reg0x4_tim1_sel;
        }
        else
        {
            setf_SYS_Reg0x4_tim1_sel;
        }
        addTIMER1_Reg0x0 = 0;
        addTIMER1_Reg0x1 = 0;
        addTIMER1_Reg0x2 = 0;
        addTIMER1_Reg0x3 = 0;
        set_TIMER1_Reg0x3_clk_div(div);
        clrf_SYS_Reg0x3_tim1_pwd;
    }
    bk_timer_hit = 0;
}
/*
min time = 0xffffffff/16M = 268.43s
max time = 0xffffffff/(32k/16) = 35791min
//now the T0 is div 16,src=1M
*/
void Timer0_Start(uint8_t index,uint32_t cnt)
{
    uint32_t count;
    count = (cnt)/1000;
    //g_bk_timer_func[index]=func;

       switch(index)
    {
        case 0:
            bk_timer_hit &= ~0x01;
            addTIMER0_Reg0x0 = count;
            bk_timer0_T0_cnt = count;
            setf_TIMER0_Reg0x3_timer0_en;
            break;
        case 1:
            bk_timer_hit &= ~0x02;
            addTIMER0_Reg0x1 = count;
            bk_timer0_T1_cnt = count;
            setf_TIMER0_Reg0x3_timer1_en;
            break;
        case 2:
            bk_timer_hit &= ~0x04;
            addTIMER0_Reg0x2 = count;
            bk_timer0_T2_cnt = count;
            setf_TIMER0_Reg0x3_timer2_en;
            break;
        default:
            break;
    }

    setf_SYS_Reg0x10_int_timer0_en;
}

void Timer0_1m5_Start(uint8_t index,uint32_t cnt)
{
    uint32_t count;
    count = (cnt)/1000;
    //g_bk_timer_func[index]=func;
       switch(index)
    {
        case 0:
            bk_timer_hit &= ~0x01;
            addTIMER0_Reg0x0 = count;
            setf_TIMER0_Reg0x3_timer0_en;
            break;
        case 1:
            bk_timer_hit &= ~0x02;
            addTIMER0_Reg0x1 = count;
            setf_TIMER0_Reg0x3_timer1_en;
            break;
        case 2:
            bk_timer_hit &= ~0x04;
            addTIMER0_Reg0x2 = count;
            setf_TIMER0_Reg0x3_timer2_en;
            break;
        default:
            break;
    }
}

void Timer1_Start(uint8_t index,uint32_t cnt)
{
    uint32_t count;
    count = (cnt*32 )/1000;
       switch(index)
    {
        case 0:
            bk_timer_hit &= ~0x10;
            addTIMER1_Reg0x0 = count;
            bk_timer1_T0_cnt = count;
            setf_TIMER1_Reg0x3_timer0_en;
            break;
        case 1:
            bk_timer_hit &= ~0x20;
            addTIMER1_Reg0x1 = count;
            bk_timer1_T1_cnt = count;
            setf_TIMER1_Reg0x3_timer1_en;
            break;
        case 2:
            bk_timer_hit &= ~0x40;
            addTIMER1_Reg0x2 = count;
            bk_timer0_T2_cnt = count;
            setf_TIMER1_Reg0x3_timer2_en;
            break;
        default:
            break;
    }

    setf_SYS_Reg0x10_int_timer1_en;
}

void Timer1_ReStart(uint8_t index,uint32_t cnt)
{
  uint32_t count;
  count = (cnt*32 )/1000;
  //g_bk_timer_func[index]=func;
  switch(index)
  {
      case 0:
          bk_timer_hit &= ~0x10;
          addTIMER1_Reg0x0 = count;
          setf_TIMER1_Reg0x3_timer0_en;
          break;
      case 1:
          bk_timer_hit &= ~0x20;
          addTIMER1_Reg0x1 = count;
          setf_TIMER1_Reg0x3_timer1_en;
          break;
      case 2:
          bk_timer_hit &= ~0x40;
          addTIMER1_Reg0x2 = count;
          setf_TIMER1_Reg0x3_timer2_en;
          break;
      default:
          break;
  }
}

void Timer0_Stop(uint8_t index)
{
    if(index==0)
    {
        clrf_TIMER0_Reg0x3_timer0_en;
        bk_timer_hit &= ~0x01;

    }
    else if (index == 1)
    {
        clrf_TIMER0_Reg0x3_timer1_en;
        bk_timer_hit &= ~0x02;
    }
    else if (index == 2)
    {
        clrf_TIMER0_Reg0x3_timer2_en;
        bk_timer_hit &= ~0x04;
    }
  //  clrf_SYS_Reg0x10_int_timer0_en;
}



void Timer1_Stop(uint8_t index)
{
    if(index==0)
    {
        clrf_TIMER1_Reg0x3_timer0_en;
        bk_timer_hit &= ~0x10;

    }
    else if (index == 1)
    {
        clrf_TIMER1_Reg0x3_timer1_en;
        bk_timer_hit &= ~0x20;
    }
    else if (index == 2)
    {
        clrf_TIMER1_Reg0x3_timer2_en;
        bk_timer_hit &= ~0x40;
    }
    clrf_SYS_Reg0x10_int_timer1_en;
}

// FIQ Timer0_PT0
void Timer_ISR0(void)
{
    uint32_t timer_status;
    timer_status=addTIMER0_Reg0x3;
    if(timer_status & (1<<posTIMER0_Reg0x3_timer0_int))
    {
           bk_timer_hit |= 0x01;
        addTIMER0_Reg0x0 = bk_timer0_T0_cnt;
        if(timer0_0_int_cb)
        {
            (*timer0_0_int_cb)();
        }
    }
    if(timer_status & (1<<posTIMER0_Reg0x3_timer1_int))
    {    
           bk_timer_hit |= 0x02;
        addTIMER0_Reg0x1 = bk_timer0_T1_cnt;
        if(timer0_1_int_cb)
        {
            (*timer0_1_int_cb)();
        }


    }
    if(timer_status & (1<<posTIMER0_Reg0x3_timer2_int))
    {
           bk_timer_hit |= 0x04;
        addTIMER0_Reg0x2 = bk_timer0_T2_cnt;
        if(timer0_2_int_cb)
        {
            (*timer0_2_int_cb)();
        }
    }

    addTIMER0_Reg0x3= timer_status;
}
//IRQ Timer0 PT1,PT2
void Timer_ISR1(void)
{
    uint32_t timer_status;
    timer_status=addTIMER1_Reg0x3;
    if(timer_status & (1<<posTIMER1_Reg0x3_timer0_int))
    {
        bk_timer_hit |= 0x10;
        addTIMER1_Reg0x0 = bk_timer1_T0_cnt;
        if(timer1_0_int_cb)
        {
            (*timer1_0_int_cb)();
        }
    }
    if(timer_status & (1<<posTIMER1_Reg0x3_timer1_int))
    {
        bk_timer_hit |= 0x20;
        addTIMER1_Reg0x1 = bk_timer1_T1_cnt;
        if(timer1_1_int_cb)
        {
            (*timer1_1_int_cb)();
        }
    }
    if(timer_status & (1<<posTIMER1_Reg0x3_timer2_int))
    {
        bk_timer_hit |= 0x40;
        addTIMER1_Reg0x2 = bk_timer1_T2_cnt;

        if(timer1_2_int_cb)
        {
            (*timer1_2_int_cb)();
        }
    }

    addTIMER1_Reg0x3 = timer_status;
}


