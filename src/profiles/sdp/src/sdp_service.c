/**
 ****************************************************************************************
 *
 * @file sdp_service.c
 *
 * @brief Sdp Service Client implementation.
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @addtogroup SDP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "rwprf_config.h"
#if (BLE_SDP_CLIENT)
#include "ke_mem.h"
#include "gap.h"
//#include "ke_env.h"
#include "sdp_service.h"
#include "sdp_service_task.h"
#include "co_math.h"
#include "uart.h"
#include <stdint.h>             // standard definition
#include <stdbool.h>            // standard boolean
#include "arch.h"               // architecture
#include "co_math.h"            // computation utilities
#include "ke_mem.h"             // kernel memory
//#include "RomCallFlash.h"
#if CLI_CONSOLE
#include "cli.h"
#endif
#include "sdp_service.h"
#include "app.h"
#include "app_task.h"
#include "app_init.h"
#include "app_sec.h"


struct sdp_env_init_t sdp_env_init;
///struct sdp_env_init_t sdp_env_init_save[BLE_CONNECTION_MAX];
/*
uint8_t db_ram_b[4][250];
uint8_t count_b=0;
uint8_t db_ram_l[6][60];
uint8_t count_l=0;
*/
/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void sdp_service_init(void)
{
    memset((uint8_t *)&sdp_env_init,0,sizeof(struct sdp_env_init_t));

    for(uint8_t i = 0; i < BLE_NB_PROFILES_ADD_MAX ; i++)
    {
        struct prf_sdp_db_env *prf_sdp_db = NULL;
        struct sdp_content *sdp_cont = NULL;
        sdp_env_init.used_status[i] = FREE_STATUS; 
        sdp_env_init.sdp_env[i].conidx = 0xff;
        prf_sdp_db = ke_malloc(sizeof(struct prf_sdp_db_env),KE_MEM_NON_RETENTION);
       // memset(prf_sdp_db, 0, sizeof(struct prf_sdp_db_env));

        sdp_cont = ke_malloc(sizeof(struct sdp_content),KE_MEM_NON_RETENTION);
        //memset(sdp_cont, 0, sizeof(struct sdp_content));
        sdp_cont->chars_descs_inf.chars_inf=NULL;
        sdp_cont->chars_descs_inf.descs_inf=NULL;
        sdp_env_init.sdp_env[i].prf_db_env  = prf_sdp_db;
        prf_sdp_db->sdp_cont = sdp_cont;
                
    }
   
}
void sdp_service_status_reset(uint8_t conidx)
{
    struct prf_sdp_db_env *prf_db_env;
    for(uint8_t i = 0; i < BLE_NB_PROFILES_ADD_MAX ; i++)
    {
        if(sdp_env_init.sdp_env[i].conidx == conidx)
        {
            sdp_env_init.used_status[i] = FREE_STATUS;  
            prf_db_env = sdp_env_init.sdp_env[i].prf_db_env;

            prf_db_env->sdp_cont->chars_nb=0;

            prf_db_env->sdp_cont->descs_nb=0;

            if(prf_db_env->sdp_cont->chars_descs_inf.chars_inf != NULL)
            {
                uart_printf("ke_free chars------------------++++++++ \r\n");
                ke_free(prf_db_env->sdp_cont->chars_descs_inf.chars_inf);
            }
            if(prf_db_env->sdp_cont->chars_descs_inf.descs_inf !=NULL)
            {
                uart_printf("ke_free descs------------------++++++++ \r\n");
                ke_free(prf_db_env->sdp_cont->chars_descs_inf.descs_inf);
            }
            prf_db_env->sdp_cont->chars_descs_inf.chars_inf=NULL;
            prf_db_env->sdp_cont->chars_descs_inf.descs_inf=NULL;
        }
    }  
    prf_reset(conidx);
}

uint8_t sdp_service_free_env_find(void)
{
    uint8_t free_idx=0;
    for(free_idx = 0; free_idx < BLE_NB_PROFILES_ADD_MAX ; free_idx++)
    {   
        //uart_printf("sdp_env_init.used_status[%x]=%x \r\n",free_idx,sdp_env_init.used_status[free_idx]);
        if(FREE_STATUS == sdp_env_init.used_status[free_idx])
        {
            return free_idx;
        }
    }
    return free_idx;
}


static uint8_t sdp_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uart_printf("%s,app_task:0x%x \r\n",__func__,app_task);

    struct prf_sdp_db_env *prf_db_env = (struct prf_sdp_db_env *)params;
    
    //-------------------- allocate memory required for the profile  ---------------------
    struct sdp_env_tag* sdp_env = &sdp_env_init.sdp_env[prf_db_env->prf_idx];
    
    // allocate  required environment variable
    env->env = (prf_env_t*) sdp_env;
    sdp_env->prf_env.app_task = app_task
                                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    sdp_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);
    // initialize environment variable
    env->role                   = PRF_CLIENT;
    env->id                     = KE_BUILD_ID(TASK_ID_SDP,prf_db_env->prf_idx);
    *start_hdl = sdp_env->prf_db_env->sdp_cont->svc.shdl;
    sdp_task_init(&(env->desc));   
    sdp_env->operation = NULL;
    // service is ready, go into an Idle state
    ke_state_set(KE_BUILD_ID(env->task, sdp_env->conidx), SDP_IDLE);
  //  uart_printf("prf_db_env->sdp_cont->descs_nb=%x,%x\r\n",prf_db_env->sdp_cont->descs_nb,sdp_env->prf_env.prf_task);

    
   // if(0 != prf_db_env->sdp_cont->descs_nb)
    {  
        prf_register_atthdl2gatt(&(sdp_env->prf_env),sdp_env->conidx, &sdp_env->prf_db_env->sdp_cont->svc);      
    }

    sdp_enable_all_server_ntf_ind(sdp_env->conidx,1);
   
   // uart_printf("%s end!!\r\n",__func__);
    return GAP_ERR_NO_ERROR;
}
/**
 ****************************************************************************************
 * @brief Destruction of the BASC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void sdp_destroy(struct prf_task_env* env)
{
    uart_printf("sdp_destroy START env->id = 0x%x,role = %x\r\n",env->id,env->role);

    struct sdp_env_tag* sdp_env = (struct sdp_env_tag*) env->env;
    
    
   // uart_printf("used_status = 0x%x,prf_idx:%d\r\n",sdp_env_init.used_status[sdp_env->prf_db_env->prf_idx],sdp_env->prf_db_env->prf_idx);
    //sdp_env_init.used_status[sdp_env->prf_db_env->prf_idx] = INVALID_STATUS;////

    // cleanup environment variable for each task instances
             
    if(sdp_env->operation != NULL)
    {
        uart_printf("sdp_destroy sdp_env->operation = 0x%x\r\n",sdp_env->operation);
        sdp_env->operation = NULL;
    }        
    // free profile environment variables
    uart_printf("sdp_destroy end\r\n");
}
/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void sdp_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put BAS Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), SDP_IDLE);
}
/**
 ****************************************************************************************
 * @brief Handles Disconnection
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */

static void sdp_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{

    //uart_printf("sdp_cleanup env->id = 0x%x,role = %x,conidx = %x reason = 0x%x\r\n",env->id,env->role,conidx,reason);
   
    struct sdp_env_tag* sdp_env = (struct sdp_env_tag*) env->env;
    
    if(sdp_env->conidx != conidx)
    {
       // uart_printf("sdp_cleanup not cur connected!!\r\n");
        ke_state_set(KE_BUILD_ID(env->task, conidx), SDP_FREE);
        return;
    }
    //env->id = TASK_ID_INVALID;

    if(sdp_env->operation != NULL)
    {
        struct ke_msg *msg = ke_msg2param(sdp_env->operation);
        uart_printf("operation = 0x%08x\r\n",(uint32_t)sdp_env->operation);
        uart_printf("msgid = 0x%02x,dest_id = 0x%02x,src_id = 0x%02x\r\n",msg->id,msg->dest_id,msg->src_id);                
        ke_free(sdp_env->operation);
        sdp_env->operation = NULL;
                
    }

    uart_printf("sdp_cleanup end\r\n");
    /* uart_printf BAS Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), SDP_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/// BASC Task interface required by profile manager
const struct prf_task_cbs sdp_itf =
{
    sdp_init,
    sdp_destroy,
    sdp_create,
    sdp_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
const struct prf_task_cbs* sdp_prf_itf_get(void)
{
    return &sdp_itf;
}

void sdp_discover_all_service(uint8_t conidx)
{
    uart_printf("sdp_discover_all_service conidx:%x\r\n",conidx);
    
    struct gattc_sdp_svc_disc_cmd * svc_req = KE_MSG_ALLOC_DYN(GATTC_SDP_SVC_DISC_CMD,
            KE_BUILD_ID(TASK_GATTC,conidx), KE_BUILD_ID(TASK_APP,conidx),
            gattc_sdp_svc_disc_cmd, ATT_UUID_16_LEN);
    //gatt request type: by UUID
    svc_req->operation         = GATTC_SDP_DISC_SVC_ALL;
    //start handle;
    svc_req->start_hdl        = ATT_1ST_REQ_START_HDL;
    //end handle
    svc_req->end_hdl          = ATT_1ST_REQ_END_HDL;
    // UUID search
    svc_req->uuid_len = ATT_UUID_16_LEN;
    /// operation sequence number
    //  svc_req->seq_num = 2;
    //set the first two bytes to the value array, LSB to MSB:Health Thermometer Service UUID first
    co_write16p(&(svc_req->uuid[0]), 0x0000);
    
    //send the message to GATT, which will send back the response when it gets it
    ke_msg_send(svc_req);
   
}



//extern struct prf_env_tag prf_env;

uint8_t sdp_extract_svc_info(uint8_t conidx,struct gattc_sdp_svc_ind const *ind)
{

    struct prf_sdp_db_env *prf_db_env;
    uint8_t free_env_idx;
    uint16_t malloc_size;  
/*    
    uart_printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
    uart_printf("%s\r\n",__func__);
    uart_printf("service uuid len = 0x%x,uuid = 0x",ind->uuid_len);
   


    for(int i = 0; i < ind->uuid_len; i++)
    {
        uart_printf("%02x",ind->uuid[ind->uuid_len - i - 1]);
    }
    uart_printf("\r\n");
    

    uart_printf("start_hdl:%02d[0x%02x],end_hdl:%02d[0x%02x]\r\n",ind->start_hdl,ind->start_hdl,ind->end_hdl,ind->end_hdl);
 */   
    //Counters
    uint8_t svc_char,svc_char_cnt;
    uint8_t svc_desc,svc_desc_cnt;
    uint8_t fnd_att;
    /**  search svc_char_cnt and desc_cnt for alloc memory **/
    svc_char_cnt = 0;
    svc_desc_cnt = 0;
    for (fnd_att=0; fnd_att< (ind->end_hdl - ind->start_hdl); fnd_att++)
    {
        if(ind->info[fnd_att].att_type == GATTC_SDP_ATT_CHAR)
        {
            uint16_t val_hdl  = ind->info[fnd_att].att_char.handle;
            // check that value handle is in a valid range
            if((val_hdl <= ind->end_hdl) && (val_hdl > (ind->start_hdl + fnd_att)))
            {
                //Look over requested characteristics
                for (svc_char=0; svc_char < ind->end_hdl -ind->start_hdl ; svc_char++)
                {
                    //uart_printf("svccnt:%d\n",svc_char_cnt);
                    svc_char_cnt = (svc_char_cnt + 1) % SDP_NB_CHAR_INSTANCES_MAX;
                    // find end of characteristic handle and discover descriptors
                    do
                    {
                        fnd_att++;
                        // found a descriptor
                        if(ind->info[fnd_att].att_type == GATTC_SDP_ATT_DESC)
                        {
                            //Retrieve characteristic descriptor handle using UUID
                            for(svc_desc = 0; svc_desc<ind->end_hdl - ind->start_hdl; svc_desc++)
                            {
                                // check if it's expected descriptor
                                if ( svc_desc == svc_char)
                                {
                                     //uart_printf("descnt:%d\n",svc_desc_cnt);
                                     svc_desc_cnt = (svc_desc_cnt + 1 ) % SDP_NB_DESC_INSTANCES_MAX;
                                    // search for next descriptor
                                      break;
                                }
                            }
                        }
                    }
                    while(((ind->start_hdl+ 1 + fnd_att) <= ind->end_hdl)
                            && (ind->info[fnd_att].att_type != GATTC_SDP_ATT_CHAR)
                            && (ind->info[fnd_att].att_type != GATTC_SDP_INC_SVC));
                    // return to previous valid value
                    fnd_att--;
                    // search next characteristic
                    break;
                }
            }
        }
    }
    /**************************end search ************************************/
    
    malloc_size = ((sizeof(struct prf_char_inf) * svc_char_cnt) + (sizeof(struct prf_char_desc_inf) * svc_desc_cnt));
    if(check_enough_mem_add_service(malloc_size))
    {   
      uart_printf("not enough mem to store profile!!!!!\r\n");
        return GAP_ERR_INSUFF_RESOURCES;
    }
    
    free_env_idx = sdp_service_free_env_find();
  //  uart_printf("free_env_idx = %d\r\n",free_env_idx);
    if(free_env_idx >= BLE_NB_PROFILES_ADD_MAX)
    {
        uart_printf("not enough env to store profile!!!!!\r\n");
        return GAP_ERR_INSUFF_RESOURCES;                                                         
    }
    
 //   uart_printf("total need = 0x%x\r\n",sizeof(struct  prf_char_inf ) * svc_char_cnt + svc_desc_cnt * sizeof( struct prf_char_desc_inf ));
    /*************************alloc memory *******************************/
    //uart_printf("chars need = 0x%x\r\n",sizeof(struct prf_char_inf) * svc_char_cnt);
    struct prf_char_inf *chars = (struct prf_char_inf *) ke_malloc(sizeof(struct prf_char_inf) * svc_char_cnt,KE_MEM_NON_RETENTION);
    //uart_printf("descs need = 0x%x\r\n",sizeof(struct prf_char_desc_inf) * svc_desc_cnt);
    struct prf_char_desc_inf *descs = (struct prf_char_desc_inf *)ke_malloc(sizeof(struct prf_char_desc_inf) * svc_desc_cnt ,KE_MEM_ATT_DB);
 //   uart_printf("chars_inf = 0x%08x,descs_inf = 0x%08x\r\n",chars,descs);
 //   uart_printf("alloc memory END\r\n");
   // add_svc_flag = 0;
    //if(true == 1)   

    #if 0
    if(sizeof(struct prf_char_desc_inf) * svc_desc_cnt>60)
    {     
        descs= db_ram_b[count_b++];
        
    }else
    {
        descs= db_ram_l[count_l++];
    }
    #endif
 
    {
       
        prf_db_env = sdp_env_init.sdp_env[free_env_idx].prf_db_env;
        sdp_env_init.sdp_env[free_env_idx].conidx = conidx;
        sdp_env_init.used_status[free_env_idx] = USED_STATUS;
        prf_db_env->prf_idx = free_env_idx;
        prf_db_env->sdp_cont->chars_descs_inf.chars_inf = chars;
        prf_db_env->sdp_cont->chars_descs_inf.descs_inf = descs;
        prf_db_env->sdp_cont->svc.shdl = ind->start_hdl;
        prf_db_env->sdp_cont->svc.ehdl = ind->end_hdl;
        prf_db_env->sdp_cont->svc.uuid_len = ind->uuid_len;
        memcpy(prf_db_env->sdp_cont->svc.uuid,ind->uuid,ind->uuid_len);
                       
        svc_char_cnt = 0;
        svc_desc_cnt = 0;
        prf_db_env->sdp_cont->chars_nb = 0;
        prf_db_env->sdp_cont->char_idx = 0;
        prf_db_env->sdp_cont->descs_nb = 0;
        for (fnd_att=0; fnd_att< (ind->end_hdl - ind->start_hdl); fnd_att++)
        {
            if(ind->info[fnd_att].att_type == GATTC_SDP_ATT_CHAR)
            {
                uint16_t char_hdl = ind->start_hdl + 1 + fnd_att;
                uint16_t val_hdl  = ind->info[fnd_att].att_char.handle;
                uint8_t  val_prop = ind->info[fnd_att].att_char.prop;
                uint8_t  char_idx = fnd_att;
                // check that value handle is in a valid range
                if((val_hdl <= ind->end_hdl) && (val_hdl > (ind->start_hdl + fnd_att)))
                {
                    // retrieve value index
                    uint8_t val_idx = (val_hdl - ind->start_hdl - 1);
                    //Look over requested characteristics
                    for (svc_char=0; svc_char < ind->end_hdl -ind->start_hdl ; svc_char++)
                    {
                        #if 0
                        uart_printf("-----------------------------------------------------\r\n");
                        uart_printf("char uuid len = 0x%x ,svc_char = %d ",ind->info[val_idx].att.uuid_len,svc_char);
                      //  UART_PRINTF("char uuid = 0x%04x \r\n",attm_convert_to16((uint8_t *)ind->info[val_idx].att.uuid,ind->info[val_idx].att.uuid_len));
                        uart_printf("char uuid = 0x");
                        for(int i = 0; i < ind->info[val_idx].att.uuid_len; i++)
                        {
                            uart_printf("%02x",ind->info[val_idx].att.uuid[ind->info[val_idx].att.uuid_len - i - 1]);
                        }
                        uart_printf("\r\n\r\n");
                        uart_printf("char_hdl:%d[0x%02x],val_hdl:%d[0x%02x],fnd_att = 0x%x ,val_idx = 0x%x,val_prop = 0x%x\r\n",char_hdl,char_hdl,val_hdl,val_hdl,fnd_att,val_idx,val_prop);
                        #endif
                        if(0)
                        {
                            uart_printf("Characteristic Properties: 0x%02x\r\n",val_prop);
                            uart_printf("Broadcast :                %d\r\n",PERM_GET(val_prop << 8,BROADCAST));
                            uart_printf("Read :                     %d\r\n",PERM_GET(val_prop << 8,RD));
                            uart_printf("Write Without Response :   %d\r\n",PERM_GET(val_prop << 8,WRITE_COMMAND));
                            uart_printf("Write :                    %d\r\n",PERM_GET(val_prop << 8,WRITE_REQ));
                            uart_printf("Notify :                   %d\r\n",PERM_GET(val_prop << 8,NTF));
                            uart_printf("Indicate :                 %d\r\n",PERM_GET(val_prop << 8,IND));
                            uart_printf("Write Signed :             %d\r\n",PERM_GET(val_prop << 8,WRITE_SIGNED));
                            uart_printf("Extended properties :      %d\r\n",PERM_GET(val_prop << 8,EXT));
                        };
       
                        //Save properties and handles
                        chars[svc_char_cnt].uuid_len = ind->info[val_idx].att.uuid_len;
                        memcpy(chars[svc_char_cnt].uuid,ind->info[val_idx].att.uuid,ind->info[val_idx].att.uuid_len);
                                        
                        chars[svc_char_cnt].char_hdl       = char_hdl;
                        chars[svc_char_cnt].val_hdl        = val_hdl;
                        chars[svc_char_cnt].prop           = val_prop;
                       
                        //uart_printf("uuid_len:%d\n",ind->info[val_idx].att.uuid_len);
                        // find end of characteristic handle and discover descriptors
                        do
                        {
                            fnd_att++;
                         //   uart_printf("fnd_att=%x,ind->info[fnd_att].att_type=%x\n",fnd_att,ind->info[fnd_att].att_type);
                            // found a descriptor
                            if(ind->info[fnd_att].att_type == GATTC_SDP_ATT_DESC)
                            {
                                //Retrieve characteristic descriptor handle using UUID
                                for(svc_desc = 0; svc_desc<ind->end_hdl - ind->start_hdl; svc_desc++)
                                {
                                    // check if it's expected descriptor
                                    if ( svc_desc == svc_char)
                                    {
                                        
                                      //  uart_printf("svc_desc = 0x%x,desc uuid = 0x%02x uuid_len = 0x%x ",svc_desc,
                                     //   attm_convert_to16((uint8_t*)ind->info[fnd_att].att.uuid,ind->info[fnd_att].att.uuid_len),ind->info[fnd_att].att.uuid_len);
                                        
                                        descs[svc_desc_cnt].uuid_len = ind->info[fnd_att].att.uuid_len;
                                        memcpy(descs[svc_desc_cnt].uuid,ind->info[fnd_att].att.uuid,ind->info[fnd_att].att.uuid_len);
                            
                                        descs[svc_desc_cnt].desc_hdl = ind->start_hdl + 1 + fnd_att;
                                        descs[svc_desc_cnt].char_code =  svc_char_cnt;
                                   //     uart_printf("desc_hdl:%d[0x%02x] desc_char_cnt = 0x%x,val_idx = 0x%x\r\n",descs[svc_desc_cnt].desc_hdl,descs[svc_desc_cnt].desc_hdl,svc_char_cnt - 1,val_idx);
                                        // search for next descriptor
                                        svc_desc_cnt++;
                                        prf_db_env->sdp_cont->descs_nb = svc_desc_cnt;
                                        //break;
                                    }
                                }
                            }
                        } while(((ind->start_hdl+ 1 + fnd_att) <= ind->end_hdl)
                                && (ind->info[fnd_att].att_type != GATTC_SDP_ATT_CHAR)
                                && (ind->info[fnd_att].att_type != GATTC_SDP_INC_SVC));
                        // return to previous valid value
                        fnd_att--;
                        // previous handle was end of the characteristic
                        chars[svc_char_cnt].char_ehdl_off    = fnd_att - char_idx;

                        svc_char_cnt++;
                        prf_db_env->sdp_cont->chars_nb = svc_char_cnt;

                       // uart_printf("svc_char_cnt:%d,0x%x\n",svc_char_cnt,&chars[svc_char_cnt].char_ehdl_off);

                        // search next characteristic
                        break;
                    }
                }
            }
        }
        
       //uart_printf("===========================svc_char_cnt2:%d,%d\r\n",svc_char_cnt,svc_desc_cnt);
     //   uart_printf("%s  end\r\n",__func__);  
     //   uart_printf("sdp_env_init.used_status[%x]=%x\r\n",free_env_idx,sdp_env_init.used_status[free_env_idx]);  
        
        sdp_add_profiles(conidx,prf_db_env);
    }
    
    return 0;
}

void sdp_add_profiles(uint8_t conidx,struct prf_sdp_db_env *db_env)
{
    uart_printf("%s ,conidx:%x\r\n",__func__,conidx);
    // return;
    struct prf_sdp_db_env *env;
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                            TASK_GAPM, KE_BUILD_ID(TASK_APP,conidx),
                                            gapm_profile_task_add_cmd, sizeof(struct prf_sdp_db_env));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = 0;//PERM(SVC_AUTH, ENABLE);
    req->prf_task_id = KE_BUILD_ID(TASK_ID_SDP,db_env->prf_idx);
    req->app_task = KE_BUILD_ID(TASK_APP,conidx);
    req->start_hdl = 0; //req->start_hdl = 0; dynamically allocated
    req->role = PRF_CLIENT;
    env = (struct prf_sdp_db_env *)req->param ;
    env->prf_idx  = db_env->prf_idx;
    env->sdp_cont  = db_env->sdp_cont;

    //uart_printf("env->sdp_cont.chars_nb = %d,prf_idx :%d,%x\r\n",env->sdp_cont->chars_nb,env->prf_idx,TASK_GAPM);
    ke_msg_send(req);
    
    
}
void sdp_enable_rsp_send(struct sdp_env_tag *sdp_env, uint8_t conidx, uint8_t status)
{
    // Send APP the details of the discovered attributes on
    struct sdp_enable_rsp * rsp = KE_MSG_ALLOC(SDP_ENABLE_RSP,
                                  prf_dst_task_get(&(sdp_env->prf_env) ,conidx),
                                  prf_src_task_get(&(sdp_env->prf_env) ,conidx),
                                  sdp_enable_rsp);
    rsp->status = status;
    ke_msg_send(rsp);
} 
uint8_t check_enough_mem_add_service(uint32_t size)
{
    // KE_MEM_BLOCK_MAX
    if( ke_check_malloc(size,KE_MEM_NON_RETENTION))
    {
        return 0;
    }
    else
    {
        return 1;
    }
    
}
static uint8_t sdp_write_service_ntf_cfg_req(uint8_t conidx,uint16_t handle,uint16_t ntf_cfg,uint16_t seq_num)
{
    uart_printf("%s,state = %x\r\n",__func__,ke_state_get(KE_BUILD_ID(TASK_APP,conidx)));
    uart_printf("conidx:%d,handle = 0x%04x,ntf_cfg = 0x%x\r\n",conidx,handle,ntf_cfg);
    
    uint8_t ret = APPM_ERROR_NO_ERROR;
    if((ke_state_get(KE_BUILD_ID(TASK_APP,conidx)) != APPC_LINK_IDLE))
    {
        struct sdp_write_ntf_cfg_req *req = KE_MSG_ALLOC(SDP_WRITE_NTF_CFG_REQ,
                                        prf_get_task_from_id(KE_BUILD_ID(TASK_ID_SDP,conidx)), KE_BUILD_ID(TASK_APP,conidx),
                                        sdp_write_ntf_cfg_req);
       // uart_printf("dest = 0x%04x\r\n",prf_get_task_from_id(KE_BUILD_ID(TASK_ID_SDP,conidx)));
        // Fill in the parameter structure
        req->conidx = conidx;
        req->handle = handle;
        req->ntf_cfg = ntf_cfg;
        req->seq_num = seq_num;
        // Send the message
        ke_msg_send(req);
    }
    else
    {
        ret = APPM_ERROR_STATE;
        uart_printf("ke_state_get(TASK_APPC) = %x\r\n",ke_state_get(KE_BUILD_ID(TASK_APP,conidx)));
    }
    
    return ret;
}
uint8_t sdp_enable_all_server_ntf_ind(uint8_t conidx,uint8_t  reset)
{
    uart_printf(" %s,conidx:%d,reset:%d\r\n",__func__,conidx,reset);
    uint8_t more_enable = false;
    static uint8_t server_num = 0,chars_num = 0;
    if(reset == 1)
    {
        server_num = 0;
        chars_num = 0;
    }
    

    for(; server_num < SDP_NB_SERVICE_INSTANCES_MAX;)
    {
        //uart_printf("server_num :%d,chars_num:%d,condix:%d\r\n",server_num,chars_num,sdp_env_init.sdp_env[conidx][server_num].conidx);
        //uart_printf("server_num = 0x%x,use_status = 0x%x\r\n",server_num,sdp_env_init.used_status[server_num] );
        //uart_printf("p_use_status = 0x%x\r\n",&Sdp_env[server_num].use_status);
                  
        if( (sdp_env_init.used_status[server_num] == USED_STATUS)&&(sdp_env_init.sdp_env[server_num].conidx == conidx))
        {
                
      //      uart_printf("server_num = %d,chars_nb = %d\r\n",server_num,sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_nb);
            for(; chars_num < sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_nb;)
            {
        //        uart_printf("server_num = %d,chars_num = %d,prop = 0x%x\r\n",server_num,chars_num,sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_descs_inf.chars_inf[chars_num].prop);
                if(sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_descs_inf.chars_inf[chars_num].prop & ATT_CHAR_PROP_IND)
                {
                    sdp_write_service_ntf_cfg_req(conidx,sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_descs_inf.chars_inf[chars_num].val_hdl,PRF_CLI_START_IND,0xaa);
                    
                    more_enable = true;
                    chars_num++;
                    return more_enable;
                }
                else if(sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_descs_inf.chars_inf[chars_num].prop & ATT_CHAR_PROP_NTF)
                {
     //               uart_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~*******************\n");
                    sdp_write_service_ntf_cfg_req(conidx,sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_descs_inf.chars_inf[chars_num].val_hdl,PRF_CLI_START_NTF,0xaa);                   
                    more_enable = true;
                    chars_num++;
                    return more_enable;
                }
                else
                {
                    if(sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_descs_inf.chars_inf[chars_num].prop & ATT_CHAR_PROP_RD)
                    {
                        appm_read_info_req(conidx,sdp_env_init.sdp_env[server_num].prf_db_env->sdp_cont->chars_descs_inf.chars_inf[chars_num].val_hdl);
                    }
                    chars_num++;
                }
            }  
            server_num++;
            chars_num = 0;
        }
        else
        {
            more_enable = false;
            server_num++;    
            //return more_enable;
        }
    }
    
    return more_enable;
}

void sdp_prf_register_all_atthdl2gatt(uint8_t conidx)
{
    uint8_t idx = 0;
    for(idx = 0;idx < SDP_NB_SERVICE_INSTANCES_MAX;idx++)
    {
        if( sdp_env_init.used_status[idx] == USED_STATUS)
        {
            struct sdp_env_tag* sdp_env = (struct sdp_env_tag*)&sdp_env_init.sdp_env[idx];

            if((0 != sdp_env->prf_db_env->sdp_cont->descs_nb)&& (sdp_env->conidx == conidx) )         
            {               
                uart_printf("prf_register_atthdl2gatt start_hdl = 0x%x,end_hdl = 0x%x\r\n",sdp_env->prf_db_env->sdp_cont->svc.shdl,sdp_env->prf_db_env->sdp_cont->svc.ehdl);
                prf_register_atthdl2gatt(&(sdp_env->prf_env),sdp_env->conidx, &sdp_env->prf_db_env->sdp_cont->svc);
            }
        }
    }
}

int sdp_get_handle_from_uuid(uint8_t conidx,uint8_t uuid_len,uint8_t* uuid)
{
    uint8_t char_idx;
    uint16_t handle = 0;
    //uint16_t val_prop;
    //uint8_t desc_nb;
    struct sdp_env_tag *sdp_env = NULL;

    sdp_env =(struct sdp_env_tag *) prf_env_get_from_uuid(conidx,uuid_len,uuid);
    if(sdp_env != NULL)
    {
        char_idx = sdp_env->prf_db_env->sdp_cont->char_idx;
        //desc_nb = sdp_env->prf_db_env->sdp_cont->descs_nb;
        handle = sdp_env->prf_db_env->sdp_cont->chars_descs_inf.chars_inf[char_idx].val_hdl;
        //val_prop = sdp_env->prf_db_env->sdp_cont->chars_descs_inf.chars_inf[char_idx].prop;
    }
    uart_printf("func %s,handle=%x\r\n",__func__,handle);
    return handle;
}
#endif /* (BLE_CLIENT) */
/// @} SDP
