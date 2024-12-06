#ifndef IIC_TIME_HPP
#define IIC_TIME_HPP

#include "iic/bus.hpp"

namespace iic::time
{
    struct tm convert_time_data_to_tm(uint8_t *time_data);
    struct tm get_current_time();
    
} // namespace iic::time


#endif // IIC_TIME_HPP