#ifndef INA219_H
#define INA219_H

#include "main.h"

void INA219_Init(void);
uint16_t INA219_ReadBusVoltage_mV(void);
int16_t INA219_ReadCurrent_mA(void);

#endif
