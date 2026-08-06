#ifndef _TYPES_H_
#define _TYPES_H_

#if 1
typedef void (*TCallback)();
typedef int (*CCallback)();

//----------------------------------------------
typedef unsigned char                BYTE;
typedef signed   long                int32;      //  有符号32位整型变量
typedef signed   short                int16;      //  有符号16位整型变量
typedef signed   char                int8;       //  有符号8位整型变量
typedef unsigned long                uint32;     //  无符号32位整型变量
typedef unsigned short                uint16;     //  无符号16位整型变量
typedef unsigned char                uint8;      //  无符号8位整型变量
typedef float                        fp32;       //  单精度浮点数（32位长度）
typedef double                        fp64;       //  双精度浮点数（64位长度）
//typedef __int64                        int64,INT64,SI64;
//typedef unsigned __int64            uint64,UINT64,UI64;
typedef long long                    LONGLONG;
typedef unsigned long long            ULONGLONG;

typedef signed long                    s32,SI32;
typedef signed short                s16,SI16;
typedef signed char                    s8,SI8;
typedef unsigned long                u32,UI32,DWORD,UNIT;
typedef unsigned short                u16,UI16,*PUI16,WORD;
typedef unsigned char                u8,UI8,*PUI8;
typedef void*                        PVOID;

typedef signed   char       int8_t;     // 有符号8位整型变量
typedef signed   short      int16_t;    // 有符号16位整型变量
//typedef signed   int       int32_t;    // 有符号32位整型变量
typedef unsigned char       uint8_t;    // 无符号8位整型变量
typedef unsigned short      uint16_t;   // 无符号16位整型变量
typedef unsigned int       uint32_t;   // 无符号32位整型变量

//typedef enum { false=0, true=!false}  bool;
// typedef enum { FALSE=0, TRUE=!FALSE }  bool;
//----------------------------------------------


#define MAX(a,b) ((a > b) ? a : b)
#define MIN(a,b) ((a < b) ? a : b)
#endif

#define BIT(n) \
    (1<<(n))
#define _BIT(n) \
    (~ BIT(n))

#define BFD(val,bs,bl) \
    (((val)&(BIT(bl)-1))<<(bs))

#define GET_BFD(val,bs,bl) \
    (((val)&((BIT(bl)-1)<<(bs)))>>(bs))

#define _BFD(val,bs,bl) \
    (~ BFD(val,bs,bl))

#define GET_ELEMENT_TBL(tbl)    (sizeof(tbl)/(sizeof(tbl[0])))

//BIG ENDIAN to LITTLE ENDIAN
#define _BYTE(n,v) \
    (((v)>>(8*n))&0xff)
//用于2种格式的变换
#define B2L_32(bv) \
    ((_BYTE(0, bv)<<24)|\
    (_BYTE(1, bv)<<16)|\
    (_BYTE(2, bv)<<8)|\
    (_BYTE(3, bv)))

#define B2L_16(bv) \
    ((_BYTE(0, bv)<<8)|\
    (_BYTE(1, bv)))

#define RW
    //可读写
#define RO
    //只读
#define WO
    //只写
#define W1C
    //写1清0
#define RC
    //读出清0

// #define PRT(var)    bim1_uart_printf(#var##"=%.8x\r\n",var)
#define REG32(x)            (*((volatile uint32 *)(x)))
#define GET_REG_ADDR(x)        (((uint32)(&(x))))
#define REG8(x)            (*((volatile uint8 *)(x)))

#endif
