#ifndef DATA_STORAGE_HPP
#define DATA_STORAGE_HPP

#include <zephyr/kernel.h>
#include <map>
#include <string>
#include <vector>
#include <ctime>

namespace data
{
    struct data_point
    {
        std::string device_id;
        struct tm timestamp;
        uint32_t battery_voltage_mv;
        uint32_t humidity_voltage_mv;
    };

    typedef struct data_point data_point_t;

    class storage
    {
    private:
        struct k_mutex lock;
        // device_id -> data_point
        std::map<std::string, std::vector<data_point_t>> data;
    public:
        storage();
        ~storage();
        void add_data_point(data_point_t data_point);
        std::string to_json();
        void clear();
    };

    extern storage storage_instance;
} // namespace data
#endif // DATA_STORAGE_HPP