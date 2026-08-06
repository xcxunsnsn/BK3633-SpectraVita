
#include <stdint.h>        // standard integer definition
#include <string.h>        // string manipulation
#include "flash.h"         // flash definition
#include "co_error.h"      // error definition
#include "uart.h"
#include "rwip.h"
#include "ll.h"
#include "wdt.h"
#include "rwprf_config.h"
#include "adc.h"

#define MAX(x, y)                  (((x) > (y)) ? (x) : (y))
#define MIN(x, y)                  (((x) < (y)) ? (x) : (y))
#define max(x, y)                  (((x) > (y)) ? (x) : (y))
#define min(x, y)                  (((x) < (y)) ? (x) : (y))

extern uint8_t oad_firmware_type;
/// Flash environment structure variable
struct flash_env_tag flash_env;


#ifdef CHECK_LOW_VOLT_ENABLE
extern void check_low_volt_sleep(void);
#endif

static uint8_t flash_enable_write_flag1;
static uint8_t flash_enable_write_flag2;
static uint8_t flash_enable_write_flag3;
static uint8_t flash_enable_write_flag4;
static uint8_t flash_enable_erase_flag1;
static uint8_t flash_enable_erase_flag2;
static uint32_t flash_mid = 0;

void set_flash_info()
{
    uint32_t flash_type = flash_mid & 0xff;
    if(flash_type==0x14)
    {
        flash_env.ota_app_def_addr_abs = FLASH_ENV_OTA_APP_DEF_ADDR_8M_ABS;
        flash_env.ota_stack_app_def_addr_abs = FLASH_ENV_OTA_APP_STACK_DEF_ADDR_8M_ABS;
        flash_env.bdaddr_def_addr_abs = FLASH_ENV_BDADDR_DEF_ADDR_8M_ABS;
        flash_env.nvds_def_addr_abs = FLASH_ENV_NVDS_DEF_ADDR_8M_ABS;
    }
    else
    {
        flash_env.ota_app_def_addr_abs = FLASH_ENV_OTA_APP_DEF_ADDR_4M_ABS;
        flash_env.ota_stack_app_def_addr_abs = FLASH_ENV_OTA_APP_STACK_DEF_ADDR_4M_ABS;
        flash_env.bdaddr_def_addr_abs = FLASH_ENV_BDADDR_DEF_ADDR_4M_ABS;
        flash_env.nvds_def_addr_abs = FLASH_ENV_NVDS_DEF_ADDR_4M_ABS;
    }
}
void set_flash_clk(unsigned char clk_conf) 
{
    //note :>16M don't use la for flash debug
    unsigned int temp0;
    temp0 = REG_FLASH_CONF;
    REG_FLASH_CONF = (  (clk_conf << BIT_FLASH_CLK_CONF)
                      | (temp0    &  SET_MODE_SEL)
                      | (temp0    &  SET_FWREN_FLASH_CPU)
                      | (temp0    &  SET_WRSR_DATA)
                      | (temp0    &  SET_CRC_EN));
    while(REG_FLASH_OPERATE_SW & 0x80000000){;}
}

uint32_t get_flash_ID(void)
{
    unsigned int temp0;

    while(REG_FLASH_OPERATE_SW & 0x80000000);

    REG_FLASH_OPERATE_SW = ( FLASH_ADDR_FIX
                            | (FLASH_OPCODE_RDID << BIT_OP_TYPE_SW)
                            | (0x1 << BIT_OP_SW));
    while(REG_FLASH_OPERATE_SW & 0x80000000);

    for (temp0=0; temp0<8; temp0++)
            REG_FLASH_DATA_SW_FLASH = 0xffffffff;

    return REG_FLASH_RDID_DATA_FLASH ;
}

uint32_t flash_read_sr(void)
{
    uint32_t temp = 0;

    while(REG_FLASH_OPERATE_SW & 0x80000000);

    REG_FLASH_OPERATE_SW = ( FLASH_ADDR_FIX
                            | (FLASH_OPCODE_RDSR << BIT_OP_TYPE_SW)
                            | (0x1 << BIT_OP_SW));

    while(REG_FLASH_OPERATE_SW & 0x80000000);

    temp = (REG_FLASH_SR_DATA_CRC_CNT&0xff);

    REG_FLASH_OPERATE_SW = (FLASH_ADDR_FIX
                            | (FLASH_OPCODE_RDSR2 << BIT_OP_TYPE_SW)
                            | (0x1 << BIT_OP_SW));

    while(REG_FLASH_OPERATE_SW & 0x80000000);

    temp |= (REG_FLASH_SR_DATA_CRC_CNT&0xff) << 8;

    return temp ;
}

void flash_write_sr_temp( uint8_t bytes,  uint16_t val )
{
    if(flash_mid != get_flash_ID())
    return;
    switch(flash_mid)
    {
        case GD_FLASH_1:  //QD xx ,
        case BY25Q80:
        case PN25f04:
            REG_FLASH_CONF &= 0xfefe0fff;
        break;
        case P25Q40U:
        case TH25D40HB:
        case GT25Q40:
            REG_FLASH_CONF &= 0xfef00fff;
        break;
        case GD_25WD80E:
            REG_FLASH_CONF &= 0xfffe0fff;
        break;
        case XTX_FLASH_1:   //XTX xx
        case GD_MD25D40:
        case GD_GD25WD40:
        default:
            REG_FLASH_CONF &= 0xffff0fff;
        break;
    }

    if(bytes==0||bytes>2)
    {
        return;
    }
    REG_FLASH_CONF |= (val << BIT_WRSR_DATA)|SET_FWREN_FLASH_CPU;
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {

    }
    
    if(flash_mid != get_flash_ID())
        return;
    if( bytes == 1 ) 
    {
        REG_FLASH_OPERATE_SW = (FLASH_ADDR_FIX|(FLASH_OPCODE_WRSR << BIT_OP_TYPE_SW)
                               | (0x1<< BIT_OP_SW)
                               | (0x1<< BIT_WP_VALUE));
        
    }
    else if(bytes == 2 )
    {
        REG_FLASH_OPERATE_SW = (FLASH_ADDR_FIX|(FLASH_OPCODE_WRSR2 << BIT_OP_TYPE_SW)
                               | (0x1<< BIT_OP_SW)
                               | (0x1<< BIT_WP_VALUE));
    }
        
    while(REG_FLASH_OPERATE_SW & 0x80000000);


    REG_FLASH_OPERATE_SW = FLASH_ADDR_FIX; 

    while(REG_FLASH_OPERATE_SW & 0x80000000);
}

void flash_write_sr( uint8_t bytes,  uint16_t val )
{
    static uint8_t write_sr_cnt = 0;
    uint16_t sr_data=0;
    #ifdef CHECK_LOW_VOLT_ENABLE
    check_low_volt_sleep();
    #endif

    sr_data=flash_read_sr();
    //uart_printf("sr_data = 0x%x,VAL=%x\r\n", sr_data,val);
    if(flash_mid==GD_GD25WD40 || flash_mid==GD_25WD80E)
    {
        if((sr_data&0xbf)==val)
            return;
    }
    if(sr_data == val)
    {
        return; 
    }

    flash_write_sr_temp(bytes, val);
    sr_data=flash_read_sr();
    if(flash_mid==GD_GD25WD40 || flash_mid==GD_25WD80E)
    {
        sr_data &= 0xbf;
    }

    while (sr_data != val)
    {
        flash_write_sr_temp(bytes, val);

        write_sr_cnt++;
        if(write_sr_cnt > 10)
        {
            //uart_printf("boot write sr error! WDT_RESET!!!\r\n");
            //wdt_enable(0x10);
            uart_printf("write sr error! cpu_reset!!!\r\n");
            uart_printf("sr = 0x%x\r\n", flash_read_sr());
            extern void cpu_reset(void);
            cpu_reset();
            while(1);
        }
        sr_data=flash_read_sr();
        if(flash_mid==GD_GD25WD40 || flash_mid==GD_25WD80E)
        {
            sr_data &= 0xbf;
        }
    }
}

void flash_wp_256k( void)
{
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    check_low_volt_sleep();
    #endif
    flash_sr=flash_read_sr( );
    switch(flash_mid)
    {
        case XTX_FLASH_1:   //XTX xx
            if(flash_sr!=0xAC)    
                flash_write_sr( 1, 0xAC );
            break;   
        case GD_FLASH_1:   //QD xx ,
        case BY25Q80:
        case PN25f04:
            if(flash_sr!=0x00ac)
                flash_write_sr( 2, 0x00ac );
            break;
        case P25Q40U:
        case TH25D40HB:
        case GT25Q40:
            if(flash_sr!=0x002c)
                flash_write_sr( 2, 0x002c );  
            break;
        case GD_25WD80E:
            if(flash_sr!=0x98)
                flash_write_sr(1, 0x98);  //portect 0 ~ 768KB area
        break;
        case GD_MD25D40:
        case GD_GD25WD40:
        default:
            if(flash_sr!=0x98)
                flash_write_sr( 1, 0x98 );
            break;    
    }
}
void flash_wp_496k( void)
{
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    check_low_volt_sleep();
    #endif
    flash_sr=flash_read_sr( );
    switch(flash_mid)
    {
        case XTX_FLASH_1:   //XTX xx
            if(flash_sr!=0xAC)
                flash_write_sr( 1, 0xAC );
            break;   
        case GD_FLASH_1:   //QD xx ,
        case BY25Q80:
        case PN25f04:
            if(flash_sr!=0x40ac)
                flash_write_sr( 2, 0x40ac );
            break;
        case P25Q40U:
        case TH25D40HB:
        case GT25Q40:
            if(flash_sr!=0x404c)
                flash_write_sr( 2, 0x404c );  
            break;
        case GD_25WD80E:
            if(flash_sr!=0x98)
                flash_write_sr(1, 0x98);  //portect 0 ~ 768KB area
        break;
        case GD_MD25D40:
        case GD_GD25WD40:
        default:
            if(flash_sr!=0x88)
                flash_write_sr( 1, 0x88 );
            break;    
    }
}
void flash_wp_all( void )
{
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    check_low_volt_sleep();
    #endif
    flash_sr=flash_read_sr();
    
    switch(flash_mid)
    {
        case XTX_FLASH_1:   //XTX xx
        if(flash_sr!=0xBC)
            flash_write_sr( 1, 0xBC );
        break;  
        case GD_FLASH_1:  //QD xx ,
        case BY25Q80:
        case PN25f04:
        if(flash_sr!=0x0094)
            flash_write_sr( 2, 0x0094 );
        break;
        case P25Q40U:
        case TH25D40HB:
        case GT25Q40:
        if(flash_sr!=0x0010)
            flash_write_sr( 2, 0x0010 );
        break;    
        case GD_MD25D40:
        case GD_GD25WD40:
        case GD_25WD80E:
        default:
        if(flash_sr!=0x9c)    
            flash_write_sr( 1, 0x9c );
        break;    
    }
}
void flash_wp_none( void)
{
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    check_low_volt_sleep();
    #endif
    flash_sr=flash_read_sr( );
    switch(flash_mid)
    {
        case GD_25WD80E:
        if(flash_sr!=0x80)
            flash_write_sr(1, 0x80);  //portect none area

        break;
        default:
        break;    
    }
}


void flash_advance_init(void)
{
    flash_mid = get_flash_ID();
    set_flash_info();
    flash_wp_all();
    uart_printf("flash_mid=%x\n",flash_mid);   
}


void flash_init(void)
{   
    flash_advance_init();
    flash_set_dual_mode();       
    set_flash_clk(0x08);    
}


void flash_erase_sector(uint32_t address)
{
    GLOBAL_INT_DISABLE();
    flash_set_line_mode(1);
    if(flash_enable_erase_flag1==FLASH_ERASE_ENABLE1&&flash_enable_erase_flag2==FLASH_ERASE_ENABLE2)    
    {
        // 2021/7/16 user maybe modify this ,the SRreg(proctor) is more slower by writed, n*100 times?
        #if (BLE_OADS_SERVER)
        if(oad_firmware_type)
            flash_wp_256k();
        else
        #endif
            flash_wp_496k();
        
        while(REG_FLASH_OPERATE_SW & 0x80000000);

        REG_FLASH_OPERATE_SW = ( (address << BIT_ADDRESS_SW)
                               | (FLASH_OPCODE_SE<< BIT_OP_TYPE_SW)
                               | (0x1             << BIT_OP_SW));

        while(REG_FLASH_OPERATE_SW & 0x80000000);
        flash_set_line_mode(4);
        // 2021/7/16 user maybe modify this ,the SRreg(proctor) is more slower by writed, n*100 times?
        flash_wp_all();
    }
    GLOBAL_INT_RESTORE();
}



void flash_read_data (uint8_t *buffer, uint32_t address, uint32_t len)
{
    uint32_t i;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8];
    uint8_t *pb = (uint8_t *)&buf[0];
   
    if (len == 0)
        return;
    GLOBAL_INT_DISABLE();
    while(REG_FLASH_OPERATE_SW & 0x80000000);
 
    while (len)
    {
        REG_FLASH_OPERATE_SW = (  (addr << BIT_ADDRESS_SW)
                                | (FLASH_OPCODE_READ << BIT_OP_TYPE_SW)
                                | (0x1 << BIT_OP_SW));
        while(REG_FLASH_OPERATE_SW & 0x80000000);
        addr+=32;

        for (i = 0; i < 8; i++)
            buf[i] = REG_FLASH_DATA_FLASH_SW;

        for (i = (address & 0x1F); i < 32; i++)
        {
            *buffer++ = pb[i];
            address++;
            len--;
            if (len == 0)
                break;
        }
    }    
    REG_FLASH_OPERATE_SW=FLASH_ADDR_FIX ;
    for (i=0; i<8; i++)
        REG_FLASH_DATA_SW_FLASH = 0xffffffff;
    GLOBAL_INT_RESTORE();
}



// 2021/7/16   verify by lipenghui
void flash_write_data (uint8_t *buffer, uint32_t address, uint32_t len)
{
    uint32_t  i;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8] = {~0x00UL};
    uint8_t *pb = (uint8_t *)&buf[0];   
    uint8_t count_num;
    
    if (len == 0)
        return;
    if (address<0x40000)
        return;
    GLOBAL_INT_DISABLE();
    flash_set_line_mode(1);

    while(REG_FLASH_OPERATE_SW & 0x80000000);

    flash_enable_write_flag3=FLASH_WRITE_ENABLE3; 
    // 2021/7/16 user maybe modify this ,the SRreg(proctor) is more slower by writed, n*100 times?
    if((flash_mid==GD_25WD80E) && address<768*1024)
    {    
        flash_wp_none();
    }
    else
    {
        #if (BLE_OADS_SERVER)
        if(oad_firmware_type)
            flash_wp_256k();
        else
        #endif
            flash_wp_496k();
    }

    while(len) 
    {
      //  if((address & 0x1F) || (len < 32))
        flash_read_data(pb, addr, 32);
        count_num = 0;

        for(i =0;i< 32;  i++) 
        {
            if((i>=(address & 0x1F))&&(i<((address & 0x1F)+len)))
            {

                pb[i] =(~pb[i])|(*buffer++);  //don't set bit 0 if this bit is 0 GD25Q40
                count_num++;
                //     len--;
            }
            else
            {
                pb[i] = 0xff;
            }
        }
        address +=count_num;
        len -=count_num;
        
        flash_enable_write_flag4=FLASH_WRITE_ENABLE4; 
        for (i=0; i<8; i++)
            REG_FLASH_DATA_SW_FLASH = buf[i];


        if(flash_enable_write_flag1==FLASH_WRITE_ENABLE1 && flash_enable_write_flag2==FLASH_WRITE_ENABLE2 )
        {
            while(REG_FLASH_OPERATE_SW & 0x80000000);
            
            if(flash_enable_write_flag3==FLASH_WRITE_ENABLE3 && flash_enable_write_flag4==FLASH_WRITE_ENABLE4)
            {
                if(addr<0x40000)
                    return;
                REG_FLASH_OPERATE_SW = (  (addr << BIT_ADDRESS_SW)
                                    | (FLASH_OPCODE_PP << BIT_OP_TYPE_SW)
                                    | (0x1 << BIT_OP_SW));
            }
            while(REG_FLASH_OPERATE_SW & 0x80000000);
        }
        addr+=32;
    }
    // 2021/7/16 user maybe modify this ,the SRreg(proctor) is more slower by writed, n*100 times?
    flash_wp_all();
    REG_FLASH_OPERATE_SW=FLASH_ADDR_FIX ;
    flash_enable_write_flag3=0;
    flash_enable_write_flag4=0;
    for (i=0; i<8; i++)
        REG_FLASH_DATA_SW_FLASH = 0xffffffff;
    flash_set_line_mode(4);
    GLOBAL_INT_RESTORE();
}


void flash_set_qe(void)
{
    uint32_t temp0;
    while(REG_FLASH_OPERATE_SW & 0x80000000){;}

    temp0 = REG_FLASH_CONF; //WRSR Status data

    if(flash_mid == XTX_FLASH_1) 
        return;

    REG_FLASH_CONF = ((temp0 & SET_FLASH_CLK_CONF)
                    | (temp0 &  SET_MODE_SEL)
                    | (temp0 &  SET_FWREN_FLASH_CPU)
                    |(temp0 & SET_WRSR_DATA)
                    |(0x01 << 19)
                    | (temp0 &  SET_CRC_EN));

    //Start WRSR
    REG_FLASH_OPERATE_SW = ( FLASH_ADDR_FIX
                            | (FLASH_OPCODE_WRSR2 << BIT_OP_TYPE_SW)
                            | (0x1  << BIT_OP_SW)); 
    while(REG_FLASH_OPERATE_SW & 0x80000000);
}



void clr_flash_qwfr(void)
{
    uint32_t temp0,mod_sel;    
    
    temp0 = REG_FLASH_CONF;
    while(REG_FLASH_OPERATE_SW & 0x80000000){;}
    mod_sel = temp0 & (0xC << BIT_MODE_SEL); //??3ymode_sel?D
    mod_sel |= (0x1 << BIT_MODE_SEL);
    REG_FLASH_CONF = (  (temp0 &  SET_FLASH_CLK_CONF)
                        | mod_sel
                        | (temp0 &  SET_FWREN_FLASH_CPU)
                        | (temp0 &  SET_WRSR_DATA)
                        | (temp0 &  SET_CRC_EN));
    //reset flash
    
    if(flash_mid == XTX_FLASH_1)
    {
        REG_FLASH_OPERATE_SW = (  (FLASH_ADDR_FIX<< BIT_ADDRESS_SW)
                                | (FLASH_OPCODE_CRMR << BIT_OP_TYPE_SW)
                                | (0x1               << BIT_OP_SW));
    }
    else
    {
        REG_FLASH_OPERATE_SW = (  (FLASH_ADDR_FIX<< BIT_ADDRESS_SW)
                                | (FLASH_OPCODE_CRMR2 << BIT_OP_TYPE_SW)
                                | (0x1               << BIT_OP_SW));
    }

    while(REG_FLASH_OPERATE_SW & 0x80000000);
}


void flash_set_dual_mode(void)
{
    clr_flash_qwfr();
    REG_FLASH_CONF &= (~(7<<BIT_MODE_SEL));
    REG_FLASH_CONF |= (1<<BIT_MODE_SEL);
    while(REG_FLASH_OPERATE_SW & 0x80000000);
}

void flash_set_line_mode(uint8_t mode)
{

}

// 2021/7/16   verify by lipenghui
uint8_t flash_read(uint8_t flash_space, uint32_t address, uint32_t len, uint8_t *buffer, void (*callback)(void))
{
    uint32_t pre_address;
    uint32_t post_address;
    uint32_t pre_len;
    uint32_t post_len;
    uint32_t page0;
    uint32_t page1;
    page0 = address &(~FLASH_PAGE_MASK);
    page1 = (address + len) &(~FLASH_PAGE_MASK);
    if(page0 != page1)
    {
        pre_address = address;
        pre_len = page1 - address;
        flash_read_data(buffer, pre_address, pre_len);
        post_address = page1;
        post_len = address + len - page1;
        flash_read_data((buffer + pre_len), post_address, post_len);
    }
    else
    {
        flash_read_data(buffer, address, len);
    }
    return CO_ERROR_NO_ERROR;
}


// 2021/7/16   verify by lipenghui
uint8_t flash_write(uint8_t flash_space, uint32_t address, uint32_t len, uint8_t *buffer, void (*callback)(void))
{
    uint32_t pre_address;
    uint32_t post_address;
    uint32_t pre_len;
    uint32_t post_len;
    uint32_t page0;
    uint32_t page1;
    
    if(flash_mid != get_flash_ID())
    {
        uart_printf("flash = 0x%x\r\n", get_flash_ID());
        return CO_ERROR_UNDEFINED;
    }

    flash_enable_write_flag1=FLASH_WRITE_ENABLE1; 
    
    page0 = address &(~FLASH_PAGE_MASK);
    page1 = (address + len) &(~FLASH_PAGE_MASK);
     
    if(page0 != page1)
    {
        pre_address = address;
        pre_len = page1 - address;
        flash_enable_write_flag2=FLASH_WRITE_ENABLE2;
        flash_write_data(buffer, pre_address, pre_len);
        
        post_address = page1;
        post_len = address + len - page1;
        flash_write_data((buffer + pre_len), post_address, post_len);

    }
    else
    {
        flash_enable_write_flag2=FLASH_WRITE_ENABLE2;
        flash_write_data(buffer, address, len);

    }
    flash_enable_write_flag1=0; 
    flash_enable_write_flag2=0;    
    
    return CO_ERROR_NO_ERROR;
}

uint8_t flash_erase(uint8_t flash_type, uint32_t address, uint32_t len, void (*callback)(void))
{
    /* assume: the worst span is four sectors*/
    int erase_addr;
    int erase_len;

    flash_enable_erase_flag1=FLASH_ERASE_ENABLE1;
    
    if(flash_mid != get_flash_ID())
    {
        return CO_ERROR_UNDEFINED;
    }
    
    {
        erase_addr = address & (~FLASH_ERASE_SECTOR_SIZE_MASK);
        erase_len = len;
    }
    do
    {
        int i;
        int erase_whole_sector_cnt;
        //erase_whole_sector_cnt = erase_len >> FLASH_ERASE_SECTOR_SIZE_RSL_BIT_CNT;
        erase_whole_sector_cnt = erase_len/FLASH_ERASE_SECTOR_SIZE + (erase_len%FLASH_ERASE_SECTOR_SIZE>0? 1:0);//��Ҫ�󳤶���4k������
        flash_enable_erase_flag2=FLASH_ERASE_ENABLE2;
        for(i = 0; i < erase_whole_sector_cnt; i ++)
        {
            flash_erase_sector(erase_addr);
            erase_addr += FLASH_ERASE_SECTOR_SIZE;
            //erase_len -= FLASH_ERASE_SECTOR_SIZE;
        }
    }
    while(0);
    
    flash_enable_erase_flag1=0;
    flash_enable_erase_flag2=0;
    return CO_ERROR_NO_ERROR;
}


void flash_write_some_data(uint8_t *buffer, uint32_t address, uint32_t len)
{
    unsigned char flash_temp[1024];
    #if (BLE_OADS_SERVER)
    if(oad_firmware_type)
        flash_wp_256k();
    else
    #endif
        flash_wp_496k();
    flash_read(0,address&0xfffffc00, 1024,flash_temp, (void*)0);//the addr is the word,so the addr +20
    memcpy(&flash_temp[address&0x3ff],buffer,len);
    flash_erase(0,address,0x1000,0);
    flash_write(0,address&0xfffffc00,1024,flash_temp,(void*)0);
    flash_wp_all();

}

#if GD_25WD80E_1MB
#define TEST_FLASH_ADDRESS  0xFC000  
#else
#define TEST_FLASH_ADDRESS  0x7E000
#endif
#define TEST_LEN 0xff
void flash_test(void)
{
    unsigned char w_temp[TEST_LEN];
    unsigned char r_temp[TEST_LEN];
    int i;

    //flash_wp_256k();

    uart_printf("flash_test\n ");
    flash_read(0,TEST_FLASH_ADDRESS ,TEST_LEN,r_temp,NULL);

    uart_printf("r_temp1: ");
    for (i=0; i<TEST_LEN; i++)
    {
        uart_printf("%x,",r_temp[i]);
    }
    uart_printf("\n ");
    
    for (i=0; i<TEST_LEN; i++)
    {
        w_temp[i] = i;
    }
    
    flash_write(0,TEST_FLASH_ADDRESS ,TEST_LEN,w_temp,NULL);
    
    flash_read(0,TEST_FLASH_ADDRESS ,TEST_LEN,r_temp,NULL);

    uart_printf("r_temp1: ");
    for (i=0; i<TEST_LEN; i++)
    {
        uart_printf("%x,",r_temp[i]);
    }
    uart_printf("\n ");
    
    flash_erase(0,TEST_FLASH_ADDRESS ,0x1000,NULL);
    
    flash_read(0,TEST_FLASH_ADDRESS ,TEST_LEN,r_temp,NULL);

    uart_printf("r_temp1: ");
    for (i=0; i<TEST_LEN; i++)
    {
        uart_printf("%x,",r_temp[i]);
    }
    uart_printf("\n ");
}



