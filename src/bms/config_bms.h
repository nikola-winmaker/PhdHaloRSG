#ifndef CONFIG_BMS_H
#define CONFIG_BMS_H

// Default BMS simulation parameters (can be overridden per-app)
#define BMS_DEFAULT_CHARGING_ENABLED   1
#define BMS_DEFAULT_CURRENT_LIMIT_MA    6000.0f
#define BMS_DEFAULT_VOLTAGE_LIMIT_MV   12500.0f

#include "../battery/battery_config.h" // Battery model parameters shared with BMS

#endif // CONFIG_BMS_H
