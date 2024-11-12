#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <algorithm>
#include <stdio.h>
#include "uart/uart.hpp"
#include "at/commands.hpp"

LOG_MODULE_REGISTER(main);

int main(void)
{
    if (uart::uart_init() != 0)
    {
        printk("UART initialization failed\n");
        return -1;
    }
    // struct k_poll_event events[1];
    // k_poll_event_init(&events[0],
    //                   K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
    //                   K_POLL_MODE_NOTIFY_ONLY,
    //                   &uart::uart_msgq_rx);

    if (at::commands::at() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not responding\n");
    }

    if (at::commands::cnmp() != at::commands::OK)
    {
        LOG_ERR("Failed to set module to NB-IoT mode\n");
    }

    if (at::commands::nbsc() != at::commands::OK)
    {
        LOG_ERR("Failed to enable scrambling\n");
    }

    while (1)
    {
        k_sleep(K_MSEC(1000));
    }
}
