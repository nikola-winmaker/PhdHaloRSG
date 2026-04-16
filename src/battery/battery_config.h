#ifndef BATTERY_CONFIG_H
#define BATTERY_CONFIG_H

// Battery model parameters (shared with BMS)
#define BATTERY_VOLTAGE_MIN           10000.0f
#define BATTERY_VOLTAGE_MAX           13600.0f
#define BATTERY_TEMP_MAX              60.0f
#define BATTERY_TEMP_MIN              0.0f
#define BATTERY_OVERCURRENT           12000.0f
#define BATTERY_OVERVOLTAGE           12700.0f
#define BATTERY_UNDERVOLTAGE          9900.0f
#define BATTERY_TEMP_RISE_PER_A       0.000001f
#define BATTERY_TEMP_COOL_PER_TICK    0.2f
#define BATTERY_CAPACITY_MAH          5000.0f

#endif // BATTERY_CONFIG_H
