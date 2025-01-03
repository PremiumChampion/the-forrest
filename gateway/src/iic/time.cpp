#include <zephyr/kernel.h>
#include <time.h>
#include <zephyr/logging/log.h>

#include "iic/bus.hpp"
#include "iic/time.hpp"

LOG_MODULE_REGISTER(iic_time);

namespace iic::time
{
    uint8_t bcd_to_dec(uint8_t bcd);
    uint8_t dec_to_bcd(uint8_t dec);

    // Function to convert the time data from the DS1307 to struct tm
    struct tm convert_time_data_to_tm(uint8_t *time_data)
    {
        struct tm time_info;

        // Convert the time_data to struct tm
        time_info.tm_sec = bcd_to_dec(time_data[0] & 0x7F); // Mask to get the lower 7 bits
        time_info.tm_min = bcd_to_dec(time_data[1] & 0x7F);
        time_info.tm_hour = bcd_to_dec(time_data[2] & 0x3F); // Mask to get the lower 6 bits
        time_info.tm_mday = bcd_to_dec(time_data[4] & 0x3F);
        time_info.tm_mon = bcd_to_dec(time_data[5] & 0x1F) - 1; // Month is 0-11
        time_info.tm_year = bcd_to_dec(time_data[6]) + 100;     // Year since 1900

        // Set tm_wday (day of the week) based on the DS1307 format
        time_info.tm_wday = ((bcd_to_dec(time_data[3]) - 1) % 7); // Convert to 0-6 (0 = Sunday)

        // Set tm_isdst to -1 to indicate that the DST information is not available
        time_info.tm_isdst = -1;

        return time_info;
    }

    uint8_t dec_to_bcd(uint8_t dec)
    {
        return ((dec / 10) << 4) | (dec % 10);
    }

    // Function to convert BCD to decimal
    uint8_t bcd_to_dec(uint8_t bcd)
    {
        return ((bcd >> 4) * 10) + (bcd & 0x0F);
    }

    // Function to get the current time from the DS1307
    struct tm get_current_time()
    {

        uint8_t write_data[1] = {0x00};

        uint8_t time_data[7];

        int ret = iic::bus::write_read(write_data, sizeof(write_data), time_data, sizeof(time_data), 0x68);

        if (ret < 0)
        {
            // Handle error
            LOG_ERR("I2C write/read failed: %d", ret);
            return (struct tm){0};
        }

        struct tm time = convert_time_data_to_tm(time_data);
        return time;
    }

    int set_time(struct tm time)
    {
        // Convert the struct tm to time data for the DS1307
        uint8_t write_data[8] = {
            0x00,
            dec_to_bcd(time.tm_sec),
            dec_to_bcd(time.tm_min),
            (uint8_t)(dec_to_bcd((uint8_t)time.tm_hour) | 0b01000000), // Set the 24-hour mode
            (uint8_t)(time.tm_wday + 1),                               // Convert to 1-7 (1 = Sunday)
            dec_to_bcd(time.tm_mday),
            dec_to_bcd(time.tm_mon + 1),
            dec_to_bcd(time.tm_year - 100)};

        // execute the write/write operation
        int ret = iic::bus::write(write_data, sizeof(write_data), 0x68);

        if (ret < 0)
        {
            // Handle error
            LOG_ERR("I2C write/write failed: %d", ret);
            return ret;
        }

        return 0;
    }

} // namespace iic::time
