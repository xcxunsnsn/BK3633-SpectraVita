/**
 ****************************************************************************************
 *
 * @file ftmps_task.h
 *
 * @brief Header file - Battery Service Server Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */


#ifndef _FTMPS_TASK_H_
#define _FTMPS_TASK_H_


#include "rwprf_config.h"
#if (BLE_FTMP_SERVER)
#include <stdint.h>
#include "rwip_task.h" // Task definitions
/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of FTMP Server task instances
#define FTMPS_IDX_MAX     0x01
///Maximal number of FTMP that can be added in the DB

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the FTMPS task
enum ftmps_state
{
    /// Idle state
    FTMPS_IDLE,
    /// busy state
    FTMPS_BUSY,
    /// Number of defined states.
    FTMPS_STATE_MAX
};

/// Messages for FTMP Server
enum ftmps_msg_id
{
    /// Start the FTMP Server - at connection used to restore bond data
    FTMPS_CREATE_DB_REQ   = TASK_FIRST_MSG(TASK_ID_FTMPS),
    
    /// FFF1 Level Value Update Request
    FTMPS_LEVEL_UPD_REQ,
    /// Inform APP if FFF1 Level value has been notified or not
    FTMPS_LEVEL_UPD_RSP,
    /// Inform APP that FFF1 Level Notification Configuration has been changed - use to update bond data
    FTMPS_LEVEL_NTF_CFG_IND,
    
      FTMPS_WRITER_REQ_IND,
      
      FTMPS_READ_REQ_IND,

      FTMPS_LEVEL_PERIOD_NTF
    
        
};

/// Features Flag Masks
enum ftmps_features
{
    /// FFF1 Level Characteristic doesn't support notifications
    FTMP_NTF_NOT_SUP,
    /// FFF1 Level Characteristic support notifications
    FTMP_NTF_SUP,
};
/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters for the database creation
struct ftmps_db_cfg
{
    /// Number of FTMP to add
    uint16_t ftmp_nb;
    /// Features of each FTMP instance
    uint16_t features;
   };

/// Parameters of the @ref FTMPS_ENABLE_REQ message
struct ftmps_enable_req
{
    /// connection index
    uint8_t  conidx;
    /// Notification Configuration
    uint8_t  ntf_cfg;
    /// Old FFF1 Level used to decide if notification should be triggered
    uint8_t  old_fff1_lvl;
};


///Parameters of the @ref FTMPS_BATT_LEVEL_UPD_REQ message
struct ftmps_level_upd_req
{
    /// BAS instance
    uint8_t conidx;
    
      uint8_t length;
    /// fff1 Level
    uint8_t level[20];
    
      uint16_t uuid_indx;
};

struct ftmps_read_upd_rsp
{
    /// Requested value
    uint8_t uuid_indx;
};

///Parameters of the @ref FTMPS_FFF1_LEVEL_UPD_RSP message
struct ftmps_level_upd_rsp
{
    ///status
    uint8_t status;
};

///Parameters of the @ref BASS_BATT_LEVEL_NTF_CFG_IND message
struct ftmps_level_ntf_cfg_ind
{
    /// connection index
    uint8_t  conidx;
    ///Notification Configuration
    uint8_t  ntf_cfg;
};


/// Parameters of the @ref FTMPS_FFF2_WRITER_REQ_IND message
struct ftmps_writer_ind
{
    /// Alert level
    uint8_t value[20];
    
    uint8_t length;
    /// Connection index
    uint8_t conidx;

    uint8_t flag;
};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */




/**
 * fitness_machine_feature的feature定义
 */
typedef enum
{
    average_speed =0x00000001,
    cadence_supported =0x00000002,
    total_distance =0x00000004,
    inclination_supported =0x00000008,
    elevation_gain =0x00000010,
    pace_supported =0x00000020,
    step_count =0x00000040,
    resistance_level =0x00000080,
    stride_count =0x00000100,
    expended_energy =0x00000200,
    heart_rate_measurement =0x00000400,
    metabolic_equivalent =0x00000800,
    elapsed_time =0x00001000,
    remaining_time =0x00002000,
    power_measurement =0x00004000,
    force_on_belt_and_power_output =0x00008000,
    user_data_retention =0x00010000,
}fitness_machine_feature;


/**
 * target_setting_feature的feature定义
 * 小字节序
 */
typedef enum
{
    speed_target_setting=0x00000001,
    inclination_target_setting =0x00000002,
    resistance_target_setting =0x00000004,
    power_target_setting =0x00000008,
    heart_rate_target_setting =0x00000010,
    targeted_expended_energy_configuration =0x00000020,
    targeted_step_number_configuration =0x00000040,
    targeted_stride_number_configuration =0x00000080,
    targeted_distance_configuration =0x00000100,
    targeted_training_time_configuration =0x00000200,
    targeted_time_in_two_heart_rate_zones_configuration =0x00000400,
    targeted_time_in_three_heart_rate_zones_configuration =0x00000800,
    targeted_time_in_five_heart_rate_zones_configuration =0x00001000,
    indoor_bike_simulation_parameters  =0x00002000,
    wheel_circumference_configuration  =0x00004000,
    spin_down_control  =0x00008000,
    targeted_cadence_configuration  =0x00010000,
} target_setting_feature;


/**
 *fitness_machine_feature的类型
 */
typedef enum
{
    fitness_machine_features  =0x01,
    target_setting_features =0x02,
}fitness_machine_feature_properties;


/**
 * treadmill 的field定义
 */
typedef enum
{
    instantaneous_speed=0x0000, // 必须支持，为0代表支持瞬时速度
    treadmill_average_speed =0x0002,
    treadmill_total_distance =0x0004,
    inclination_and_ramp_angle_setting =0x0008,
    treadmill_elevation_gain =0x0010,
    instantaneous_pace =0x0020,
    average_pace =0x0040,
    treadmill_expended_energy =0x0080,
    heart_rate =0x0100,
    treadmill_metabolic_equivalent =0x0200,
    treadmill_elapsed_time =0x0400,
    treadmill_remaining_time =0x0800,
    treadmill_force_on_belt_and_power_output =0x1000,
} treadmill_data_field;


/**
 *
 */
typedef struct
{
    unsigned short flags;
    unsigned short instantaneousSpeed; // 必须支持,单位：0.01km/h
    unsigned short averageSpeed;
    unsigned int totalDistance; // 必须支持，单位:米
    signed short inclination;
    signed short rampAngle;
    unsigned short positiveElevationGain;
    unsigned short negativeElevationGain;
    unsigned short totalEnergy; // 必须支持，单位kcal        //卡路里
    unsigned short energyPerHour; // 必须填写，不支持填0
    unsigned char energyPerMinute; // 必须填写，不支持填0
    unsigned char heartRate;
    unsigned short elapsedTime; // 必须支持
}treadmill_Data_t;

typedef enum
{
    other =0x00, // 其他未知状态
    idle =0x01, // 空闲,必须支持
    warming_Up =0x02,
    low_intensity_interval =0x03,
    high_intensity_interval =0x04,
    recovery_interval =0x05,
    isometric =0x06,
    heart_rate_control =0x07,
    fitness_test =0x08,
    speed_outside_of_control_region_low =0x09,
    speed_outside_of_control_region_high =0x0A,
    cool_down =0x0B,
    watt_control =0x0C,
    quick_start =0x0D, // 快速开始,必须支持
    pre_workout =0x0E, // 开启前读秒时状态,必须支持
    post_workout =0x0F, // 结束后读秒时状态,必须支持
}training_status;



#define TREADMILL_DATA_LEN            11   // data max 18

typedef struct {
    unsigned int len; // data max 18
    unsigned int data[TREADMILL_DATA_LEN];
}treadmill_data_t;

typedef struct {
    unsigned int len;
    unsigned char flags;
    unsigned char training_status;
}training_status_t;

typedef struct {
    unsigned short minSpeed;
    unsigned short maxSpeed;
    unsigned short minIncrement;
}speed_range_t;

typedef struct {
    signed short minInclination;
    signed short maxInclination;
    unsigned short minIncrement;
}inclination_range_t;


typedef struct {
    signed short minLevel;
    signed short maxLevel;
    unsigned short minIncrement;
}resistance_level_range_t;


typedef struct {
    unsigned char minHeartRate;
    unsigned char maxHeartRate;
    unsigned char minIncrement;
}heart_rate_range_t;


/**
 * Fitness Machine Control Point的行为枚举定义
 */
typedef enum
{
    request_control=0x00, // 请求控制权限
    reset=0x01,
    set_target_speed=0x02,
    set_target_inclination =0x03,
    start=0x07,
    stop=0x08,
    resume=0X15, // 华为扩展
    pause=0x16, // 华为扩展
    response_code =0x80,
}fitness_machine_control_point_operation_code;

/**
 * Fitness Machine Control Point 的命令
 */
typedef struct {
    unsigned char opCode;
    unsigned char parameterLength;
}fitness_machine_control_point_t;

typedef union {
    unsigned short targetSpeed;
    signed short targetInclination;
} control_point_target_union;
/**
 * Fitness Machine Control Point response的resultCode枚举定义
 */
typedef enum
{
    success=0x01,
    op_code_not_supported=0x02,
    invalid_parameter =0x03,
    operation_failed =0x04,
    control_not_permitted=0x05,
}fitness_machine_control_point_response_resultCode;

/**
 * Fitness Machine Control Point 的执行结果答复
 */
typedef struct{
    unsigned char opCode;
    unsigned char resultCode;
    int parameter;
}fitness_machine_control_point_response_t;




/**
 * Fitness Machine status的行为枚举定义
 */
typedef enum
{
    pause_by_user=0x00, // 用户暂停
    status_reset=0x01,  // 参数：无
    stop_by_user=0x02, // 用户停止 参数：无
    stop_by_safety_key=0x03, // 参数：无
    start_by_user =0x04,  // 参数：无
    target_speed_changed=0x05, // 参数：uint16
    target_incline_changed=0x06, // 参数：sint16
    resume_by_user =0x16, // 用户resume
    control_permission_lost =0xFF, // Fitness Machine发送该指令后，不再接收Fitness Machine Control Point
}fitness_machine_status_operation_code;


#define STATUS_DATA_LEN            2   // data max 2
typedef struct {
    unsigned char code;
    unsigned int len; // data max 18
    unsigned char data[STATUS_DATA_LEN];
}fitness_machine_status_t;


typedef enum
{
    unLock_Code =0x01,
    extension_heart_rate=0x02,
    total_energy=0x04,
    dynamic_enerty=0x08,
    extension_step_count=0x10,
}fitness_extension_data_flag;


/**
 *
 */
typedef struct
{
    unsigned short flags;
    char unLockCode[6];                //解锁码
    unsigned char heartRate;        //心率
    unsigned short totalEnergy;        //总卡路里
    unsigned short dynamicEnerty;    //动态卡路里
    unsigned short steps;            //实时步数
}huawei_fitness_extension_data_t;


typedef enum
{
    verify_failed =0x00,
    verify_success=0xFF,
}fitness_verify_code;


/**
 * 读取Fitness Machine Feature
 *
 */
void FitnessMachineFeatureRead(uint32_t featureField,uint32_t targetSettingFeatures);


/**
 * read Treadmill Data
 * 读取设置好的运动数据，返回一个数组
 *
 */
void TreadmillDataNotify(treadmill_Data_t data);

/**
 * 设置Training Status Characteristics 的trainingStatus
 * read and Notification suported
 * trainingStatus见枚举值training_status的定义，传递对应参数即可
 * uuid为fitness_machine_uuids.h的GATT_UUID_TRAINING_STATUS
 *
 */
void TrainingStatusAccessRead(uint8_t trainingStatus);

/**
 * 读取Supported Speed Range
 * param:最大最小值，及最低增量
 *
 */
void SpeedRangeRead(uint16_t minSpeed,uint16_t maxSpeed,uint16_t minIncrement);

/**
 * 读取Supported Inclination Range
 * param:最大最小值，及最低增量
 *
 */
void InclinationRangeRead(uint16_t minInclination,uint16_t maxInclination,uint16_t minIncrement);

/**
 * 读取Supported Resistance Level Range
 * param:最大最小值，及最低增量
 *
 */
void ResistanceLevelRangeRead(uint16_t minLevel,uint16_t maxLevel,uint16_t minIncrement);


/**
 * 读取Supported Heart Rate Range
 * param:最大最小值，及最低增量
 *
 */
void HeartRateRangeRead(uint8_t minHeartRate,uint8_t maxHeartRate,uint8_t minIncrement);


void TrainingStatusRead(uint8_t trainingStatus);
void ftmps_task_init(struct ke_task_desc *task_desc);


/**
 * 解析client传递的Fitness Machine Control Point
 * 参数第一个字节为opCode命令,后面的都为参数，参数长度0-18个字节
 * opCode的命令定义见fitness_machine_control_point_operation_code
 * 使用Fitness Machine Control Point 控制Fitness Machine前，会先发送Op Code = 0x00进行权限请求
 * 通过后才能进行发送命令，否则Fitness Machine不响应
 * 以下三种情况会失去权限控制
 * 1、蓝牙断开 2、client发送Op Code = Reset 3、Fitness Machine发送Control Permission Lost 的Notification
 *
 */
void FitnessMachineControlPointWrite(uint8_t *pBuf,uint8_t length);


/**
 * client传递的Fitness Machine Control Point 的response
 * 参数为该次Fitness Machine Control Point的结构体
 * 在命令执行后发送答复
 * resultCode见枚举值fitness_machine_control_point_response_resultCode的定义
 *
 */
void FitnessMachineControlPointResponse(uint8_t opCode,uint8_t resultCode);



/**
 * Fitness Machine传递的Fitness Machine status
 * Permission:Notify
 * opCode见fitness_machine_status_operation_code的定义，parameter传递实际数值
 * uuid为fitness_machine_uuids.h的GATT_UUID_FITNESS_MACHINE_STATUS
 * 若为以下命令则有参数：target_speed_changed=0x05, // 参数范围：uint16
 * target_incline_changed=0x06, // 参数范围：sint16

 */
void FitnessMachineStatusnotify(uint8_t opCode,uint8_t* parameter,uint8_t length);


/**
 * Fitness Extension Data Characteristic for FTMS
 * 当前支持的数据类型见fitness_extension_data定义，
 * 具体字段的长度见huawei_fitness_extension_data_t
 * 函数将返回一个huawei_fitness_extension_data_t的结构体。
 * uuid为fitness_machine_uuids.h的GATT_UUID_FITNESS_EXTENSION_DATA
 *
 */
void fitnessExtensionDataWrite(uint8_t* pBuf,uint8_t length);

/**
 * 蓝牙断开时需要重置标志位
 * 将会重置请求的控制权限和解锁码的校验状态
 * 在蓝牙断开回调的函数中调用该函数即可
 *
 */


void addTwoOctData(uint8_t* treadmillData,uint16_t data,int dataIndex);

uint8_t HandleControlPointCollision(uint8_t opCode);

void addUnlockCode(huawei_fitness_extension_data_t *extensionData,uint8_t* pBuf,uint8_t length);

void VerifyUnlockCode(uint8_t *unlockCode);

void SetControlPermissionToSuccess(void);

void ResetWhenBluetoothDisconnect(void);

void ResetControlPermission(void);

uint8_t GetFitnessMachineStatus(void);
uint8_t CheckUnlockCode(uint8_t *unlockCode);
#endif // BLE_FTMP_SERVER


#endif /* _FTMPS_TASK_H_ */

