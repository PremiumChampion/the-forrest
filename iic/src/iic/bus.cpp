#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include "iic/bus.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(iic_bus);

namespace iic::bus
{
    const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    int setup()
    {
        if (!i2c_dev)
        {
            // Handle error
            LOG_ERR("I2C device not found");
            return -1;
        }
        LOG_INF("I2C device found");

        int ret = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);

        if (ret < 0)
        {
            // Handle error
            LOG_ERR("I2C configuration failed: %d", ret);
            return -1;
        }

        LOG_INF("I2C configured");
        return 0;
    }
    int write(uint8_t *data, size_t len, uint8_t addr){
        int ret = i2c_write(i2c_dev, data, len, addr);
        if (ret < 0)
        {
            // Handle error
            LOG_ERR("I2C write failed: %d, [addr=%x]", ret, addr);
        }
        return ret;
    }
    int read(uint8_t *data, size_t len, uint8_t addr){
        int ret = i2c_read(i2c_dev, data, len, addr);
        if (ret < 0)
        {
            // Handle error
            LOG_ERR("I2C read failed: %d, [addr=%x]", ret, addr);
        }
        return ret;
    }
    int write_read(uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len, uint8_t addr){
        int ret = i2c_write_read(i2c_dev, addr, write_data, write_len, read_data, read_len);
        if (ret < 0)
        {
            // Handle error
            LOG_ERR("I2C write-read failed: %d, [addr=%x]", ret, addr);
            return ret;
        }

        return ret;
    }
} // namespace iic::bus
