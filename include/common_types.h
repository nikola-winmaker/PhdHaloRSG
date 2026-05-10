typedef struct {
    unsigned int battery_voltage_mv = 0;
    int charge_current_ma = 0;
    float battery_temp_c = 0.0;
    bool breaker_closed = false;
    unsigned int fault_flags = 0;  
}SensorFrame;

typedef struct {
    bool enable_charging = False;
    unsigned int current_limit_ma = 0;
    unsigned int voltage_limit_mv = 0;
    string charging_mode = "normal";
}ChargeCommand;

typedef struct {
    string charger_state = "idle";        
    unsigned int requested_current_ma = 0;
    unsigned int requested_voltage_mv = 0;
    unsigned int fault_state = 0;
}ChargeStatus;

typedef struct {
    bool safe_mode = False;           
    bool breaker_open = False;        
    bool charging_allowed = False;    
    unsigned int heartbeat_counter = 0;
}SafetyState;

typedef struct {
    unsigned int command_id = 0;
    int command_param = 0;   
}OperatorCommand;
