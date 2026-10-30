#ifndef BATTERY_TYPES_H
#define BATTERY_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Battery configuration */
#define CELL_COUNT              1

/* Protection thresholds */
#define CELL_OVERVOLTAGE_MV     4200
#define CELL_UNDERVOLTAGE_MV    3000
#define OVERCURRENT_MA          3000
#define OVERTEMPERATURE_DECIC   600     // 60.0°C

/* Balancing */
#define BALANCE_THRESHOLD_MV    50



#define CELL_OV_RECOVERY_MV       4150
#define CELL_UV_RECOVERY_MV       3100
#define OC_RECOVERY_MA            2800
#define OT_RECOVERY_DECIC         550

typedef enum
{
    FAULT_NONE = 0,
    FAULT_OVERVOLTAGE,
    FAULT_UNDERVOLTAGE,
    FAULT_OVERCURRENT,
    FAULT_OVERTEMPERATURE,
    FAULT_SENSOR_FAILURE

} FaultType;

typedef enum
{
    BATTERY_INIT,
    BATTERY_IDLE,
    BATTERY_CHARGING,
    BATTERY_DISCHARGING,
    BATTERY_FAULT

} BatteryState;

typedef struct
{
    char message[64];

} UARTMessage;

typedef struct
{
    int16_t cellVoltage[CELL_COUNT];  // mV
    uint16_t packVoltage;             // mV
    int16_t current;                  // mA
    int16_t temperature;              // 0.1°C

} BatteryData;

typedef struct
{
    uint16_t soc;

    bool charging;
    bool balancing;

    bool chargeEnabled;       // NEW
    bool dischargeEnabled;    // NEW

    FaultType fault;

    bool balanceCell[CELL_COUNT];

} BatteryStatus;

#endif
