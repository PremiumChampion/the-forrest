#ifndef UART_HPP
#define UART_HPP

#include <zephyr/kernel.h>
#include <string>

namespace uart
{
    enum read_result
    {
        UART_READ_OK,
        UART_READ_TIMEOUT,
        UART_READ_ERROR
    };

    enum write_result
    {
        UART_WRITE_OK,
        UART_WRITE_TIMEOUT,
        UART_WRITE_ERROR
    };

    extern struct k_msgq uart_msgq_rx;
    void uart_cb(const struct device *dev, void *user_data);
    read_result uart_read(std::string &result); // read from the UART
    write_result uart_write(std::string data);  // write to the UART
    int uart_init();                            // initialize the UART
    void flush();
    std::string escape_response(std::string response);
    std::string escape_response(char *response);

} // namespace uart

#endif // UART_HPP