/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "uart/uart0.hpp"
#include "uart/uart1.hpp"

LOG_MODULE_REGISTER(main);

int main(void)
{
	if (uart::uart0::uart_init() != 0)
	{
		LOG_ERR("UART initialization failed\n");
		return -1;
	}

	uart::uart0::sleep();

	// initialize the abstraction and driver
	if (uart::uart1::uart_init() != 0)
	{
		LOG_ERR("UART initialization failed\n");
		return -1;
	}

    // we can change the baudrate 
	int rc = uart::uart1::change_baudrate(uart::baudrate::Baud9600);

	if (rc != 0)
	{
		LOG_ERR("Could not change baudrate: %d\n", rc);
		return -1;
	}

    // write data via uart
	uart::uart1::uart_write("Hello from echo bot!\r\n");

	while (true)
	{
        // we manually put the module to sleep.
        // we can not send data now except we continue with uart::uart1::wakeup
        // the driver autoresumes when there is activity on the bus
        // no state management
		uart::uart1::sleep();

		std::string rx_data;
		// read available data on the bus
        if (uart::uart1::uart_read(rx_data, K_FOREVER) == uart::UART_READ_OK)
		{
            // echo the result back
			uart::uart1::uart_write("echo: " + rx_data);
		}
	}
}
