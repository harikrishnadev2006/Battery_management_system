#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks.h"
#include "battery_types.h"
#include "rtos_objects.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include "main.h"
#include "timers.h"

#include "tmp117.h"
extern ADC_HandleTypeDef hadc1;

#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

void MeasurementTask(void *argument)
{
    BatteryData data;

    static uint8_t testStage = 0;
    static uint8_t testCount = 0;

    while (1)
    {
        /*
         * REAL battery voltage is measured by ADC
         * in BatteryManagerTask.
         *
         * Current and temperature are simulated
         * for BMS testing.
         */

    	data.cellVoltage[0] = 0;
    	data.cellVoltage[1] = 0;
    	data.cellVoltage[2] = 0;
    	data.cellVoltage[3] = 0;

        data.current = -1800;          // -1.8 A = normal discharge
        data.temperature = TMP117_ReadTemperature();
        data.packVoltage = 0;

        /*
         * TEST SEQUENCE
         *
         * 0 = NORMAL
         * 1 = OVERCURRENT
         * 2 = OVERTEMPERATURE
         *
         * Voltage faults are NOT simulated because
         * battery voltage now comes from the real ADC.
         */

        switch (testStage)
        {
            case 0:
                /* NORMAL OPERATION */
                break;

            case 1:
                /* OVERCURRENT TEST */
                data.current = 4000;       // +4.0 A
                break;

            case 2:
                /* OVERTEMPERATURE TEST */
                    // 65.0°C
                break;

            default:
                testStage = 0;
                break;
        }

        /* Send measurements to BatteryManagerTask */
        xQueueSend(
            measurementQueue,
            &data,
            portMAX_DELAY
        );

        /* Activity indicator */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        /*
         * Each test lasts approximately 5 seconds.
         */
        testCount++;

        if (testCount >= 5)
        {
            testCount = 0;
            testStage++;

            if (testStage > 2)
            {
                testStage = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void BatteryManagerTask(void *argument)
{
    BatteryData data;
    BatteryStatus status;

    status.fault = FAULT_NONE;

    char txBuffer[64];

    BatteryState state = BATTERY_INIT;

    while (1)
    {
        if (xQueueReceive(measurementQueue, &data, portMAX_DELAY) == pdPASS)
        {
            /* =====================================================
             * 1. REAL 1S BATTERY VOLTAGE
             * ===================================================== */

            uint32_t adcValue;

            if (HAL_ADC_Start(&hadc1) == HAL_OK &&
                HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
            {
                adcValue = HAL_ADC_GetValue(&hadc1);

                /*
                 * 10k / 10k voltage divider
                 *
                 * Battery voltage = ADC voltage x 2
                 *
                 * 3.3 V reference:
                 * 4095 ADC counts = 6600 mV battery
                 */
                data.cellVoltage[0] =
                    (adcValue * 6600UL) / 4095UL;
            }
            else
            {
                /*
                 * ADC failure is treated as an unsafe
                 * low-voltage condition.
                 */
                data.cellVoltage[0] = 0;
            }

            HAL_ADC_Stop(&hadc1);

            data.packVoltage = data.cellVoltage[0];


            /* =====================================================
             * 2. FAULT HANDLING
             * ===================================================== */

            /*
             * First determine whether the CURRENT fault
             * has actually recovered.
             */
            switch (status.fault)
            {
                case FAULT_OVERVOLTAGE:

                    if (data.cellVoltage[0] < CELL_OV_RECOVERY_MV)
                    {
                        status.fault = FAULT_NONE;
                    }

                    break;


                case FAULT_UNDERVOLTAGE:

                    if (data.cellVoltage[0] > CELL_UV_RECOVERY_MV)
                    {
                        status.fault = FAULT_NONE;
                    }

                    break;


                case FAULT_OVERCURRENT:

                    if (data.current < OC_RECOVERY_MA &&
                        data.current > -OC_RECOVERY_MA)
                    {
                        status.fault = FAULT_NONE;
                    }

                    break;


                case FAULT_OVERTEMPERATURE:

                    if (data.temperature < OT_RECOVERY_DECIC)
                    {
                        status.fault = FAULT_NONE;
                    }

                    break;


                case FAULT_SENSOR_FAILURE:

                    /*
                     * TMP117 is healthy when it returns
                     * a valid temperature instead of -999.
                     */
                    if (data.temperature != -999)
                    {
                        status.fault = FAULT_NONE;
                    }

                    break;


                case FAULT_NONE:

                default:
                    break;
            }


            /*
             * Now check for a NEW fault.
             *
             * This is deliberately done AFTER recovery.
             * Therefore if one fault clears while another
             * fault is already present, the new fault is
             * detected in the SAME measurement cycle.
             */
            if (status.fault == FAULT_NONE)
            {
                /* Overvoltage */
                if (data.cellVoltage[0] > CELL_OVERVOLTAGE_MV)
                {
                    status.fault = FAULT_OVERVOLTAGE;
                }

                /* Undervoltage */
                else if (data.cellVoltage[0] < CELL_UNDERVOLTAGE_MV)
                {
                    status.fault = FAULT_UNDERVOLTAGE;
                }

                /* Overcurrent */
                else if (data.current > OVERCURRENT_MA ||
                         data.current < -OVERCURRENT_MA)
                {
                    status.fault = FAULT_OVERCURRENT;
                }

                /* Temperature sensor failure */
                else if (data.temperature == -999)
                {
                    status.fault = FAULT_SENSOR_FAILURE;
                }

                /* Overtemperature */
                else if (data.temperature > OVERTEMPERATURE_DECIC)
                {
                    status.fault = FAULT_OVERTEMPERATURE;
                }
            }


            /* =====================================================
             * 3. PROTECTION OUTPUTS
             * ===================================================== */

            /*
             * Safe default:
             * everything enabled when there is no fault.
             */
            status.chargeEnabled = true;
            status.dischargeEnabled = true;

            switch (status.fault)
            {
                case FAULT_OVERVOLTAGE:

                    /* Do not charge an overcharged cell */
                    status.chargeEnabled = false;

                    break;


                case FAULT_UNDERVOLTAGE:

                    /* Do not discharge an undercharged cell */
                    status.dischargeEnabled = false;

                    break;


                case FAULT_OVERCURRENT:
                case FAULT_OVERTEMPERATURE:
                case FAULT_SENSOR_FAILURE:

                    /* Critical fault */
                    status.chargeEnabled = false;
                    status.dischargeEnabled = false;

                    break;


                case FAULT_NONE:

                default:

                    break;
            }


            /* =====================================================
             * 4. CHARGING / DISCHARGING STATUS
             * ===================================================== */

            if (data.current > 0)
            {
                status.charging = true;
            }
            else
            {
                status.charging = false;
            }


            /* =====================================================
             * 5. BALANCING
             * ===================================================== */

            /*
             * This is a 1S battery.
             * Cell balancing is therefore not applicable.
             */
            status.balancing = false;

            for (int i = 0; i < CELL_COUNT; i++)
            {
                status.balanceCell[i] = false;
            }


            /* =====================================================
             * 6. SOC
             * ===================================================== */

            /*
             * Simple 1S Li-ion voltage based SOC:
             *
             * 3.0 V = 0%
             * 4.2 V = 100%
             */

            if (data.packVoltage <= 3000)
            {
                status.soc = 0;
            }
            else if (data.packVoltage >= 4200)
            {
                status.soc = 100;
            }
            else
            {
                status.soc =
                    ((uint32_t)(data.packVoltage - 3000) * 100) / 1200;
            }





            /* =====================================================
             * 8. BATTERY STATE MACHINE
             * ===================================================== */

            switch (state)
            {
                case BATTERY_INIT:

                    if (status.fault != FAULT_NONE)
                    {
                        state = BATTERY_FAULT;
                    }
                    else if (status.charging)
                    {
                        state = BATTERY_CHARGING;
                    }
                    else
                    {
                        state = BATTERY_DISCHARGING;
                    }

                    break;


                case BATTERY_CHARGING:

                    if (status.fault != FAULT_NONE)
                    {
                        state = BATTERY_FAULT;
                    }
                    else if (!status.charging)
                    {
                        state = BATTERY_DISCHARGING;
                    }

                    break;


                case BATTERY_DISCHARGING:

                    if (status.fault != FAULT_NONE)
                    {
                        state = BATTERY_FAULT;
                    }
                    else if (status.charging)
                    {
                        state = BATTERY_CHARGING;
                    }

                    break;


                case BATTERY_FAULT:

                    /*
                     * Only leave FAULT when the fault
                     * has actually recovered.
                     */
                    if (status.fault == FAULT_NONE)
                    {
                        if (status.charging)
                        {
                            state = BATTERY_CHARGING;
                        }
                        else
                        {
                            state = BATTERY_DISCHARGING;
                        }
                    }

                    break;


                default:

                    state = BATTERY_INIT;

                    break;
            }


            /* =====================================================
             * 9. UART OUTPUT
             * ===================================================== */

            snprintf(
                txBuffer,
                sizeof(txBuffer),
                "V:%u SOC:%u T:%d I:%d F:%d State:%d CHG:%d DSG:%d\r\n",
                data.packVoltage,
                status.soc,
                data.temperature,
                data.current,
                status.fault,
                state,
                status.chargeEnabled,
                status.dischargeEnabled

            );


            UARTMessage uartMsg;

            strcpy(
                uartMsg.message,
                txBuffer
            );

            xQueueSend(
                uartQueue,
                &uartMsg,
                portMAX_DELAY
            );
        }
    }
}
void UARTTask(void *argument)
{
    UARTMessage uartMsg;

    while(1)
    {
        if(xQueueReceive(
                uartQueue,
                &uartMsg,
                portMAX_DELAY) == pdPASS)
        {
            HAL_UART_Transmit(
                &huart1,
                (uint8_t*)uartMsg.message,
                strlen(uartMsg.message),
                HAL_MAX_DELAY
            );
        }
    }
}


