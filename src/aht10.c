#include "aht10.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define AHT10_ADDR 0x38

void aht10_hw_init(void) {
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void aht10_init() {
    uint8_t cmd[3] = {0xE1, 0x08, 0x00};
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, cmd, 3, false);
    sleep_ms(20);
}

bool aht10_read(float *temperature, float *humidity) {
    uint8_t data[6];
    uint8_t trigger[3] = {0xAC, 0x33, 0x00};

    if (i2c_write_blocking(I2C_PORT, AHT10_ADDR, trigger, 3, false) < 0)
        return false;

    sleep_ms(80);

    if (i2c_read_blocking(I2C_PORT, AHT10_ADDR, data, 6, false) < 0)
        return false;

    uint32_t raw_humidity =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        ((data[3] >> 4) & 0x0F);

    uint32_t raw_temperature =
        ((uint32_t)(data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        data[5];

    *humidity = (raw_humidity * 100.0f) / 1048576.0f;
    *temperature = (raw_temperature * 200.0f) / 1048576.0f - 50.0f;

    return true;
}
