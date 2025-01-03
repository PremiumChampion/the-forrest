#include <cstdint>
#ifndef ADC_HPP
#define ADC_HPP

namespace gpio::adc
{
    int setup();
    void read_channel(int index, int32_t &voltage);
}

#endif // ADC_HPP
