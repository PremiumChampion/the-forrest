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
    enum baudrate
    {
        Baud9600 = uint32_t{9600},
        Baud19200 = uint32_t{19200},
        Baud38400 = uint32_t{38400},
        Baud57600 = uint32_t{57600},
        Baud115200 = uint32_t{115200},
    };
    std::string escape_response(std::string response);
    std::string escape_response(char *response);

} // namespace uart

#endif // UART_HPP