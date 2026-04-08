# Integration

This section covers integrating HALO-generated code into your applications.

## Integrating Generated Code in Your Application

### Step 1: Include HALO Headers

In your Linux application:

```c
#include "halo_api.h"
#include "halo_structs.h"
#include "Linux_init.h"
```

### Step 2: Initialize HALO at Startup

```c
int main(int argc, char *argv[]) {
    // Initialize HALO communication layer
    halo_core1_init_Linux();
    
    printf("HALO initialized on Linux Core1\n");
    
    // Your application code here
    while (1) {
        process_sensor_data();
        send_commands();
        usleep(10000);  // 10ms
    }
    
    return 0;
}
```

### Step 3: Send Data Using HALO API

```c
#include "halo_api.h"

void send_sensor_data(uint32_t sensor_id, uint8_t *payload) {
    SharedData data;
    data.sensorId = sensor_id;
    data.timestamp = get_current_time();
    memcpy(data.payload, payload, sizeof(data.payload));
    
    // Send to FreeRTOS Core2 via LinuxToRTOS connection
    int result = halo_send_SharedData_LinuxToRTOS(&data);
    
    if (result == HALO_OK) {
        printf("Data sent successfully\n");
    } else {
        printf("Send failed with error code: %d\n", result);
    }
}
```

### Step 4: Receive Data Using HALO API

```c
void receive_system_status(void) {
    SystemStatus status;
    int result;
    
    // Try to receive from PL (Hardware Accelerator)
    result = halo_recv_SystemStatus_PLToLinux(&status);
    
    if (result == HALO_OK) {
        printf("Status received:\n");
        printf("  Code: 0x%02X\n", status.statusCode);
        printf("  CPU Load: %d%%\n", status.cpuLoad);
        printf("  Memory Used: %d bytes\n", status.memoryUsed);
    } else if (result == HALO_NODATA) {
        // No data available yet
        printf("No data available\n");
    } else {
        printf("Receive error: %d\n", result);
    }
}
```

### Step 5: Complete Application Example

```c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "halo_api.h"
#include "Linux_init.h"

typedef struct {
    uint32_t sensor_count;
    uint32_t frame_number;
} AppState;

void app_init(AppState *state) {
    state->sensor_count = 0;
    state->frame_number = 0;
    
    // Initialize HALO
    halo_core1_init_Linux();
    printf("Application initialized\n");
}

void app_process_cycle(AppState *state) {
    ControlCommand cmd;
    SystemStatus status;
    
    // Receive commands from FreeRTOS
    if (halo_recv_ControlCommand_RTOSToLinux(&cmd) == HALO_OK) {
        printf("Command received: 0x%02X\n", cmd.commandCode);
        state->sensor_count++;
    }
    
    // Receive status from accelerator
    if (halo_recv_SystemStatus_PLToLinux(&status) == HALO_OK) {
        printf("System status: CPU=%d%%, Mem=%d\n", 
               status.cpuLoad, status.memoryUsed);
    }
    
    // Send sensor data periodically
    if ((state->frame_number % 10) == 0) {
        SensorData data = {
            .sensorId = 1,
            .timestamp = state->frame_number,
        };
        strcpy((char*)data.payload, "Sensor reading from Core1");
        
        int result = halo_send_SensorData_LinuxToRTOS(&data);
        if (result != HALO_OK) {
            printf("Send error: %d\n", result);
        }
    }
    
    state->frame_number++;
}

int main(int argc, char *argv[]) {
    AppState state;
    app_init(&state);
    
    // Main application loop
    for (int i = 0; i < 1000; i++) {
        app_process_cycle(&state);
        usleep(10000);  // 10ms per cycle
    }
    
    printf("Application complete: %u frames, %u sensors\n",
           state.frame_number, state.sensor_count);
    return 0;
}
```

## Error Handling

The HALO API returns status codes:

```c
typedef enum {
    HALO_OK        = 0,   // Operation successful
    HALO_INVALID   = -1,  // Invalid parameters
    HALO_NODATA    = -2,  // No data available
    HALO_OVERFLOW  = -3,  // Buffer overflow
    HALO_UNDERFLOW = -4,  // Buffer underflow
    HALO_CRC_ERROR = -5,  // CRC check failed
    HALO_TIMEOUT   = -6,  // Operation timeout
} halo_status_t;
```

Always check return codes:

```c
int result = halo_send_SensorData_LinuxToRTOS(&data);
switch (result) {
    case HALO_OK:
        printf("Success\n");
        break;
    case HALO_CRC_ERROR:
        printf("Data integrity check failed\n");
        break;
    case HALO_OVERFLOW:
        printf("Send buffer full\n");
        break;
    default:
        printf("Unknown error: %d\n", result);
}
```
