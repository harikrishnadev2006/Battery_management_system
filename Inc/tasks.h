#ifndef TASKS_H
#define TASKS_H

#include "FreeRTOS.h"
#include "timers.h"

void MeasurementTask(void *argument);

void BatteryManagerTask(void *argument);

void UARTTask(void *argument);

void LogTimerCallback(TimerHandle_t xTimer);
#endif
