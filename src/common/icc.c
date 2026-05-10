#include "icc.h"
extern void * get_external_buffer( uint32_t offset );

static void memcpy_internal(void* payload,void* src,int size)
{
    for (uint32_t cnt = 0; cnt<size;cnt++)
    {
        ((uint8_t*)payload)[cnt] = ((uint8_t*)src)[cnt];
    }
}

t_memoryMap virtualMemoryMap[5] =
{
    {(uint64_t*)0,                       SENSOR_FRAME_SIZE}, //SensorFrameIf
    {(uint64_t*)0,                       CHARGE_COMMAND_SIZE},
    {(uint64_t*)VIRTUAL_CHARGE_STATUS,   CHARGE_STATUS_SIZE},
    {(uint64_t*)VIRTUAL_SAFETY_STATE,    SAFETY_STATE_SIZE},
    {(uint64_t*)VIRTUAL_OPERATOR_COMMAND,OPERATOR_COMMAND_SIZE}
};

t_memoryMap memoryMap[5] =
{
    {(uint64_t*)SENSOR_FRAME_BASE,    SENSOR_FRAME_SIZE}, //SensorFrameIf
    {(uint64_t*)CHARGE_COMMAND_BASE,  CHARGE_COMMAND_SIZE},
    {(uint64_t*)CHARGE_STATUS_BASE,   CHARGE_STATUS_SIZE},
    {(uint64_t*)SAFETY_STATE_BASE,    SAFETY_STATE_SIZE},
    {(uint64_t*)OPERATOR_COMMAND_BASE,OPERATOR_COMMAND_SIZE}
};

int writeDataVirtual( t_Channel channel,void* payload,uint32_t size)
{
    int ret = 0;
    t_iccInstance* iccInstance = (t_iccInstance*)&virtualMemoryMap[channel].p_dataStart[0];
    if (iccInstance->readCnt == iccInstance->writeCnt)
    {
        //first uint64 reserved for pointers
        uint64_t* dest = &virtualMemoryMap[channel].p_dataStart[1];
        //only in this case we can prepare palyoad
        memcpy_internal(dest,payload,size);

        iccInstance->writeCnt++;

        ret = 1;
    }

    return ret;
}

int writeData(t_Channel channel,void* payload,uint32_t size)
{
    int ret = 0;
    t_iccInstance* iccInstance = (t_iccInstance*)&memoryMap[channel].p_dataStart[0];
    if (iccInstance->readCnt == iccInstance->writeCnt)
    {
        //first uint64 reserved for pointers
        uint64_t* dest = &memoryMap[channel].p_dataStart[1];
        //only in this case we can prepare palyoad
        memcpy_internal(dest,payload,size);

        iccInstance->writeCnt++;

        ret = 1;
    }

    return ret;
}

int readData(t_Channel channel,void* payload,uint32_t size)
{
    int ret = 0;
    t_iccInstance* iccInstance = (t_iccInstance*)&memoryMap[channel].p_dataStart[0];
    //is there something to read
    if (iccInstance->readCnt != iccInstance->writeCnt)
    {
        uint64_t* src = &memoryMap[channel].p_dataStart[1];
        //new data 
        memcpy_internal(payload,src,size);
        iccInstance->readCnt++;
        ret = 1;
    }

    return ret; 
}

int readDataVirtual(t_Channel channel,void* payload,uint32_t size)
{
    int ret = 0;
    t_iccInstance* iccInstance = &(((uint64_t*)get_external_buffer(virtualMemoryMap[channel].p_dataStart))[0]);
    
    //is there something to read
    if (iccInstance->readCnt != iccInstance->writeCnt)
    {
        uint64_t* src = &((uint64_t*)get_external_buffer(virtualMemoryMap[channel].p_dataStart))[1];
        //new data 
        memcpy_internal(payload,src,size);
        iccInstance->readCnt++;
        ret = 1;
    }

    return ret; 
}