#include "tmp117.h"

extern I2C_HandleTypeDef hi2c1;

#define TMP117_ADDR (0x48 << 1)

int16_t TMP117_ReadTemperature(void)
{
    uint8_t reg = 0x00;
    uint8_t rx[2];

    HAL_StatusTypeDef result;

    result = HAL_I2C_Master_Transmit(
        &hi2c1,
        TMP117_ADDR,
        &reg,
        1,
        100
    );

    if (result != HAL_OK)
    {
        return -1000 - result;
    }

    result = HAL_I2C_Master_Receive(
        &hi2c1,
        TMP117_ADDR,
        rx,
        2,
        100
    );

    if (result != HAL_OK)
    {
        return -2000 - result;
    }

    int16_t raw = (int16_t)((rx[0] << 8) | rx[1]);

    return (raw * 10) / 128;
}
