/********************************** (C) COPYRIGHT *******************************
 * File Name          : broadcaster.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 广播应用程序，初始化广播数据然后一直在广播态一直广播

 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CONFIG.h"
#include "devinfoservice.h"
#include "broadcaster.h"
#include "app_i2c.h"
#include <stdio.h>
#include <string.h>

// =============================================================================
// 配置参数
// =============================================================================

// 广播间隔 (units of 625us, min is 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL 1600 * 2
// 数据采集间隔
#define SBP_PERIODIC_EVT_PERIOD 1600 * 20

// =============================================================================
// 全局变量
// =============================================================================

// ADC 校准值的偏移值
static signed short RoughCalib_Value = 0;
// 电池电压
static uint16_t bat = 0;
// Task ID for internal task/event processing
static uint8_t Broadcaster_TaskID;

// =============================================================================
// 广播数据结构定义
// =============================================================================

// 广播数据包结构分析：
// [0-2]   - Flags (3字节)
// [3-15]  - 设备名称 (13字节)
// [16-28] - BTHome 传感器数据 (13字节)

static uint8_t advertData[] = {
    0x02, // 长度 0
    GAP_ADTYPE_FLAGS, // AD类型 1
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED, // 2
    0x0D, // 长度 3 (13字节设备名称)
    0x09, // AD类型 设备名称 4
    '1', '9', '%', '/', '1', '1', 'C', '/', 'H', ':', '1', '1', '%', // 设备名称 (13字节) 5-17
    0x0D, // 长度 18 (13字节BTHome数据)
    0x16, // AD类型 19
    0xD2, 0xFC, // UUID (BTHome UUID FCD2) 20-21
    0x40,                   // BTHome v2 无加密，定期广播 22
    0x01, 0x00,             // 电量 (占位符) 23-24
    0x02, 0x00, 0x00,       // 温度 (占位符) 25-27
    0x03, 0x00, 0x00,       // 湿度 (占位符) 28
};

// 数据包索引定义
#define FLAGS_LEN_IDX 0
#define FLAGS_TYPE_IDX 1
#define FLAGS_DATA_IDX 2

#define NAME_PKG_LEN_IDX 3
#define NAME_PKG_TYPE_IDX 4
#define NAME_PKG_DATA_IDX 5

#define BTH_PKG_LEN_IDX 18
#define BTH_PKG_TYPE_IDX 19
#define BTH_PKG_UUID_IDX 20
#define BTH_PKG_VERSION_IDX 22
#define BTH_PKG_BAT_IDX 23
#define BTH_PKG_TEMP_IDX 25
#define BTH_PKG_HUMID_IDX 27

// =============================================================================
// 函数声明
// =============================================================================

static void Broadcaster_ProcessTMOSMsg(tmos_event_hdr_t* pMsg);
static void Broadcaster_StateNotificationCB(gapRole_States_t newState);
extern bStatus_t GAP_UpdateAdvertisingData(uint8_t taskID, uint8_t adType, uint16_t dataLen, uint8_t* pAdvertData);

// =============================================================================
// GAP Role Callbacks
// =============================================================================

static gapRolesBroadcasterCBs_t Broadcaster_BroadcasterCBs = {
    Broadcaster_StateNotificationCB, // Profile State Change Callbacks
    NULL
};

// =============================================================================
// 传感器数据读取函数
// =============================================================================

/**
 * @brief 电池电压采样
 * @return 电池电压值 (mV)
 */
__HIGH_CODE
uint16_t sample_battery_voltage()
{
    // VINA 实际电压值 1050±15mV
    const int vref = 1050;

    ADC_InterBATSampInit();

    // 每256次进行一次粗略校准
    static uint8_t calib_count = 0;
    if (++calib_count >> 7) {
        RoughCalib_Value = ADC_DataCalib_Rough();
    }

    ADC_ChannelCfg(CH_INTE_VBAT);
    return (ADC_ExcutSingleConver() + RoughCalib_Value) * vref / 512 - 3 * vref;
}

/**
 * @brief 读取SHT20温湿度传感器数据
 * @param temperature 温度指针 (输出)
 * @param humidity 湿度指针 (输出)
 * @return 1表示成功，0表示失败
 */
__HIGH_CODE
int read_sht20_data(uint16_t* temperature, uint16_t* humidity)
{
    i2c_app_init(0x01);
    
    int ret = sht20_read_temp_humi(temperature, humidity);
    
    if (ret != 0) {
        PRINT("SHT20 read failed: %d\n", ret);
        return 0;
    }
    return 1;
}

// =============================================================================
// 广播数据更新函数
// =============================================================================

/**
 * @brief 更新广播数据中的设备名称
 * @param battery_percent 电池百分比
 * @param temperature 温度值
 * @param humidity 湿度值
 */
__HIGH_CODE
void update_advert_device_name(uint8_t battery_percent, uint16_t temperature, uint16_t humidity)
{
    char name_buffer[22];
    
    snprintf(name_buffer, sizeof(name_buffer), "%d%%/%.0fC/%.0f%%", 
             battery_percent, temperature / 100.0, humidity / 100.0);
    
    // 更新广播数据中的设备名称字段
    int name_len = strlen(name_buffer);
    if (name_len > 13) name_len = 13; // 限制长度为13字节 (5-17位置)
    
    // 更新广播数据
    advertData[NAME_PKG_LEN_IDX] = 13; // 更新长度字段 (13字节设备名称)
    advertData[NAME_PKG_TYPE_IDX] = 0x09; // 设备名称类型
    memcpy(&advertData[NAME_PKG_DATA_IDX], name_buffer, name_len);
    // 用空格填充剩余字节
    for (int i = name_len; i < 13; i++) {
        advertData[NAME_PKG_DATA_IDX + i] = ' ';
    }

    // 更新电池电量
    advertData[BTH_PKG_BAT_IDX] = battery_percent;
    
    advertData[BTH_PKG_TEMP_IDX] = temperature & 0xFF; // 温度低字节
    advertData[BTH_PKG_TEMP_IDX + 1] = (temperature >> 8) & 0xFF; // 温度高字节

    advertData[BTH_PKG_HUMID_IDX] = humidity & 0xFF; // 湿度低字节
    advertData[BTH_PKG_HUMID_IDX + 1] = (humidity >> 8) & 0xFF; // 湿度高字节
}

/**
 * @brief 更新广播数据
 */
__HIGH_CODE
void update_advert_data() 
{
    // 读取传感器数据
    uint16_t temp, humid;
    uint8_t battery_percent;

    int sht20_ret = read_sht20_data(&temp, &humid);
    if (!sht20_ret) {
        temp = 0xffff;
        humid = 0xffff;
    }

    // 读取电池电压
    bat = sample_battery_voltage();
    battery_percent = (uint8_t)((bat-3100)/8);
    if (battery_percent > 100)
        battery_percent = 100;

    update_advert_device_name(battery_percent, temp, humid);
    
    // 打印调试信息
    PRINT("Updated advert data: BAT=%d%%, T=%d, H=%d\n", battery_percent, temp, humid);
    PRINT("Advert data length: %d bytes\n", sizeof(advertData));
    
    // 打印数据包内容用于调试
    PRINT("Advert data: ");
    for (int i = 0; i < sizeof(advertData); i++) {
        PRINT("%02X ", advertData[i]);
    }
    PRINT("\n");
}

// =============================================================================
// 主要功能函数
// =============================================================================

/**
 * @brief 广播应用程序初始化
 * @param task_id - TMOS分配的任务ID
 */
void Broadcaster_Init()
{
    Broadcaster_TaskID = TMOS_ProcessEventRegister(Broadcaster_ProcessEvent);

    // 设置GAP广播角色参数
    {
        uint8_t initial_advertising_enable = TRUE;
        uint8_t initial_adv_event_type = GAP_ADTYPE_ADV_NONCONN_IND;

        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &initial_adv_event_type);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
    }

    // 设置广播间隔
    {
        uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, advInt);
    }

    // 启动设备
    tmos_start_task(Broadcaster_TaskID, SBP_START_DEVICE_EVT, DEFAULT_ADVERTISING_INTERVAL);

    // 设置定时器读取传感器数据并更新广播
    tmos_start_task(Broadcaster_TaskID, SBP_PERIODIC_EVT, 2 * DEFAULT_ADVERTISING_INTERVAL - 320);
}

/**
 * @brief 广播应用程序任务事件处理器
 * @param task_id - TMOS分配的任务ID
 * @param events - 要处理的事件
 * @return 未处理的事件
 */
uint16_t Broadcaster_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if (events & SYS_EVENT_MSG) {
        uint8_t* pMsg;

        if ((pMsg = tmos_msg_receive(Broadcaster_TaskID)) != NULL) {
            Broadcaster_ProcessTMOSMsg((tmos_event_hdr_t*)pMsg);
            tmos_msg_deallocate(pMsg);
        }

        return (events ^ SYS_EVENT_MSG);
    }

    if (events & SBP_START_DEVICE_EVT) {
        // 启动设备
        GAPRole_BroadcasterStartDevice(&Broadcaster_BroadcasterCBs);
        return (events ^ SBP_START_DEVICE_EVT);
    }

    if (events & SBP_PERIODIC_EVT) {
        tmos_start_task(Broadcaster_TaskID, SBP_PERIODIC_EVT, SBP_PERIODIC_EVT_PERIOD);

        // 数据采集并更新广播
        update_advert_data();
        GAP_UpdateAdvertisingData(0, TRUE, sizeof(advertData), advertData);

        return (events ^ SBP_PERIODIC_EVT);
    }

    // 丢弃未知事件
    return 0;
}

// =============================================================================
// 内部函数
// =============================================================================

/**
 * @brief 处理传入的任务消息
 * @param pMsg - 要处理的消息
 */
static void Broadcaster_ProcessTMOSMsg(tmos_event_hdr_t* pMsg)
{
    switch (pMsg->event) {
    default:
        break;
    }
}

/**
 * @brief 配置文件状态变化的通知回调
 * @param newState - 新状态
 */
static void Broadcaster_StateNotificationCB(gapRole_States_t newState)
{
    switch (newState) {
    case GAPROLE_STARTED:
        PRINT("Initialized..\n");
        break;

    case GAPROLE_ADVERTISING:
        PRINT("Advertising..\n");
        break;

    case GAPROLE_WAITING:
        PRINT("Waiting for advertising..\n");
        break;

    case GAPROLE_ERROR:
        PRINT("Error..\n");
        break;

    default:
        break;
    }
}
