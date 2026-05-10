#ifndef ICC_H
#define ICC_H 

#include "memory_layout.h"
#include "stdint.h"

typedef enum
{
    channel_sensorFrame = 0,
    channel_chargeCommand,
    channel_ChargeStatus,
    channel_SafetyState,
    channel_OperatorCommand
}t_Channel;

typedef struct
{
    uint64_t* p_dataStart;
    uint32_t dataSize; 
}t_memoryMap;

typedef struct
{
    unsigned int writeCnt;
    unsigned int readCnt;
}t_iccInstance;

//Specific data 
typedef struct SensorFrameData {
    uint32_t battery_voltage_mv;
    int32_t charge_current_ma;
    float battery_temp_c;
    uint8_t breaker_closed;
    uint32_t fault_flags;
} SensorFrameData;

typedef struct OperatorCommandData {
    uint32_t command_id;
    int32_t command_param;
} OperatorCommandData;

typedef struct SafetyStateData {
    uint8_t safe_mode;
    uint8_t breaker_open;
    uint8_t charging_allowed;
    uint32_t heartbeat_counter;
} SafetyStateData;

typedef struct ChargeStatusData {
    char charger_state[5];
    uint32_t requested_current_ma;
    uint32_t requested_voltage_mv;
    uint32_t fault_state;
} ChargeStatusData;

typedef struct ChargeCommandData {
    uint8_t enable_charging;
    uint32_t current_limit_ma;
    uint32_t voltage_limit_mv;
    char charging_mode[7];
} ChargeCommandData;

int writeDataVirtual(t_Channel channel,void* payload,uint32_t size);
int writeData(t_Channel channel,void* payload,uint32_t size);

int readData(t_Channel channel,void* payload,uint32_t size);
int readDataVirtual(t_Channel channel,void* payload,uint32_t size);

#endif
