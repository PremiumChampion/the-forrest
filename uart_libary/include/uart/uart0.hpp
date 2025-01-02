#ifndef UART0_HPP
#define UART0_HPP

#include "uart/uart.hpp"

namespace uart::uart0
{

    extern struct k_msgq uart_msgq_rx;
    void uart_cb(const struct device *dev, void *user_data);
    uart::read_result uart_read(std::string &result, k_timeout_t timeout = K_MSEC(100)); // read from the UART
    uart::write_result uart_write(std::string data);                                     // write to the UART

    int uart_init(); // initialize the UART
    void flush();
    int sleep();
    int wakeup();
    int change_baudrate(uart::baudrate baudrate);

    class uart0_implementation : public uart_driver
    {
    public:
        uart0_implementation();
        uart::read_result uart_read(std::string &result, k_timeout_t timeout = K_MSEC(100)) override; // read from the UART
        uart::write_result uart_write(std::string data) override;
        int change_baudrate(uart::baudrate baudrate) override;
        int uart_init() override;
        int sleep() override;
        int wakeup() override;
        void flush() override;
    };
} // namespace uart

#endif // UART_HPP