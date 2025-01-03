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
        if (data.find(data_point.device_id) == data.end())
        {
            data[data_point.device_id] = std::vector<data_point_t>();
        }
        data[data_point.device_id].push_back(data_point);
        k_mutex_unlock(&lock);
    }

    void storage::clear()
    {
        k_mutex_lock(&lock, K_FOREVER);
        data.clear();
        k_mutex_unlock(&lock);
    }

    std::string storage::to_json()
    {
        k_mutex_lock(&lock, K_FOREVER);
        
        std::string json = "{";
        
        for (auto it = data.begin(); it != data.end(); it++)
        {
            json += "\"" + it->first + "\":[";
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
            {
                json += "{";
                json += "\"device_id\":\"" + it2->device_id + "\",";

                char _timeString[std::size("yyyy-mm-ddThh:mm:ssZ")];
                std::strftime(std::data(_timeString), std::size(_timeString), "%FT%TZ", &it2->timestamp);
                
                json += std::string("\"timestamp\":\"") + _timeString + "\",";
                json += "\"battery_voltage_mv\":" + std::to_string(it2->battery_voltage_mv) + ",";
                json += "\"humidity_voltage_mv\":" + std::to_string(it2->humidity_voltage_mv);
                json += "},";
            }
            json.pop_back();
            json += "],";
        }
        
        json.pop_back();
        json += "}";
        
        k_mutex_unlock(&lock);
        
        return json;
    }
} // namespace data