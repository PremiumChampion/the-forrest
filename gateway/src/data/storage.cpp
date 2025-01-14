#include "data/storage.hpp"
#include <ctime>
namespace data
{
    storage storage_instance = storage();

    storage::storage()
    {
        k_mutex_init(&lock);
    }

    storage::~storage()
    {
        k_mutex_unlock(&lock);
    }

    void storage::add_data_point(data_point_t data_point)
    {
        k_mutex_lock(&lock, K_FOREVER);
        data.push_back(data_point);
        k_mutex_unlock(&lock);
    }

    void storage::clear()
    {
        k_mutex_lock(&lock, K_FOREVER);
        data.clear();
        k_mutex_unlock(&lock);
    }

    std::string storage::to_json(int &included_data_point_count)
    {
        k_mutex_lock(&lock, K_FOREVER);
        // max length 1024

        std::string json = "{\"data\":[";

        for (auto &data_point : data)
        {
            std::string data_point_string;
            data_point_string += "{";
            data_point_string += "\"id\":\"" + data_point.device_id + "\",";
            data_point_string += "\"t\":" + std::to_string(std::mktime(&data_point.timestamp)) + ",";
            data_point_string += "\"mV_2\":" + std::to_string(data_point.battery_voltage_mv) + ",";
            data_point_string += "\"mV_1\":" + std::to_string(data_point.humidity_voltage_mv) + "";
            data_point_string += "}";

            if(data_point_string.size() + json.size() + 3 > 1024){
                break;
            }
            if(included_data_point_count > 0){
                json += ",";
            }
            json += data_point_string;
            included_data_point_count++;
        }
        json += "]}";
        k_mutex_unlock(&lock);
        return json;
    }
    void storage::remove_first(int count)
    {
        k_mutex_lock(&lock, K_FOREVER);
        if (data.size() > count)
        {
            data.erase(data.begin(), data.begin() + count);
        }
        else
        {
            data.clear();
        }
        k_mutex_unlock(&lock);
    }

    int storage::size(){
        k_mutex_lock(&lock, K_FOREVER);
        int size = data.size();
        k_mutex_unlock(&lock);
        return size;
    }

} // namespace data