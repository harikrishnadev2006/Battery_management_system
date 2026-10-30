#include "ina219.h"

extern I2C_HandleTypeDef hi2c1;

#define INA219_ADDR       (0x40 << 1)

#define INA219_REG_CONFIG 0x00
#define INA219_REG_SHUNT  0x01
#define INA219_REG_BUS    0x02
#define INA219_REG_CAL    0x05
#define INA219_REG_CURRENT 0x04

static void INA219_WriteRegister(uint8_t reg, uint16_t value)
{
    uint8_t data[3];

    data[0] = reg;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value & 0xFF);

    HAL_I2C_Master_Transmit(
        &hi2c1,
        INA219_ADDR,
        data,
        3,
        HAL_MAX_DELAY
    );
}

static uint16_t INA219_ReadRegister(uint8_t reg)
{
    uint8_t rx[2];

    HAL_I2C_Master_Transmit(
        &hi2c1,
        INA219_ADDR,
        &reg,
        1,
        HAL_MAX_DELAY
    );

    HAL_I2C_Master_Receive(
        &hi2c1,
        INA219_ADDR,
        rx,
        2,
        HAL_MAX_DELAY
    );

    return ((uint16_t)rx[0] << 8) | rx[1];
}

void INA219_Init(void)
{
    /*
     * 32V bus range
     * ±320mV shunt range
     * Continuous bus + shunt measurement
     */

    uint16_t config = 0x399F;

    INA219_WriteRegister(INA219_REG_CONFIG, config);

    /*
     * 0.1 ohm shunt
     * Current LSB = 1 mA
     */

    INA219_WriteRegister(INA219_REG_CAL, 409);
}

uint16_t INA219_ReadBusVoltage_mV(void)
{
    uint16_t raw;

    raw = INA219_ReadRegister(INA219_REG_BUS);

    raw >>= 3;

    return raw * 4;
}

int16_t INA219_ReadCurrent_mA(void)
{
    uint16_t raw;

    raw = INA219_ReadRegister(INA219_REG_CURRENT);

    return (int16_t)raw;
}
