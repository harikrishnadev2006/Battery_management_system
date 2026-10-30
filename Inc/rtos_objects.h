#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H



#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"
extern EventGroupHandle_t batteryEvents;

extern TimerHandle_t logTimer;

extern QueueHandle_t measurementQueue;

extern SemaphoreHandle_t uartMutex;
extern QueueHandle_t uartQueue;

#endif
