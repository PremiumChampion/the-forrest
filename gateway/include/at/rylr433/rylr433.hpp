#ifndef AT_RYL433_HPP
#define AT_RYL433_HPP
#include "at/prv.hpp"

namespace at::commands::rylr433
{
    void join_network(int network_id, int dev_address);
    void init_lora_module();
    uart::uart_driver *get_uart_driver();
}
#endif