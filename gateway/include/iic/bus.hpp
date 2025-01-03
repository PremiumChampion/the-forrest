#ifndef IIC_BUS_HPP
#define IIC_BUS_HPP
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

namespace iic::bus
{
    extern const struct device *i2c_dev;

    int setup();
    int write(uint8_t *data, size_t len, uint8_t addr);
    int read(uint8_t *data, size_t len, uint8_t addr);
    int write_read(uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len, uint8_t addr);
    int write_write(uint8_t *write_data1, size_t write_len1, uint8_t *write_data2, size_t write_len2, uint8_t addr);
} // namespace iic::bus

#endif // IIC_BUS_HPP