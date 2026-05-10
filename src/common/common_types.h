typedef struct {
    unsigned int battery_voltage_mv;
    int charge_current_ma;
    float battery_temp_c;
    unsigned char breaker_closed;
    unsigned int fault_flags;  
}SensorFrame;

typedef struct {
    unsigned char enable_charging;
    unsigned int current_limit_ma;
    unsigned int voltage_limit_mv;
    unsigned char charging_mode;
}ChargeCommand;

typedef struct {
    unsigned char charger_state;        
    unsigned int requested_current_ma;
    unsigned int requested_voltage_mv;
    unsigned int fault_state;
}ChargeStatus;

typedef struct {
    unsigned char safe_mode;           
    unsigned char breaker_open;        
    unsigned char charging_allowed;    
    unsigned int heartbeat_counter;
}SafetyState;

typedef struct {
    unsigned int command_id;
    int command_param;   
}OperatorCommand;