#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

int main(void)
{
    if (!i2c_dev) {
        // Handle error
        LOG_ERR("I2C device not found");
        return -1;
    }
    LOG_INF("I2C device found");


    int ret = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);

    if (ret < 0) {
        // Handle error
        LOG_ERR("I2C configuration failed: %d", ret);
        return -1;
    }

    LOG_INF("I2C configured");

    // Write to the device
    // 0x00 is the register address

    // 0x00 is the data to write // time start
    // 1 is the length of the data // 1 byte
    // 0x68 is the address of the device // 1307
    uint8_t write_data[1] = {0x00};

    ret = i2c_write(i2c_dev, write_data, sizeof(write_data), 0x68);

    if (ret < 0) {
        // Handle error
        LOG_ERR("I2C write failed: %d", ret);
        return -1;
    }

    LOG_INF("I2C write successful");

    // Read from the device in burst mode
    uint8_t read_data[7];
    
    ret = i2c_read(i2c_dev, read_data, sizeof(read_data), 0x68);

    if (ret < 0) {
        // Handle error
        LOG_ERR("I2C read failed: %d", ret);
        return -1;
    }

    LOG_INF("I2C read successful");

    LOG_INF("0x00: %x", read_data[0]);
    LOG_INF("0x01: %x", read_data[1]);
    LOG_INF("0x02: %x", read_data[2]);
    LOG_INF("0x03: %x", read_data[3]);
    LOG_INF("0x04: %x", read_data[4]);
    LOG_INF("0x05: %x", read_data[5]);
    LOG_INF("0x06: %x", read_data[6]);



    return 0;
}
