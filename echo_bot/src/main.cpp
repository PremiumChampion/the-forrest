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

	if (uart::uart1::uart_init() != 0)
	{
		LOG_ERR("UART initialization failed\n");
		return -1;
	}


	int rc = uart::uart1::change_baudrate(uart::baudrate::Baud9600);

	if (rc != 0)
	{
		LOG_ERR("Could not change baudrate: %d\n", rc);
		return -1;
	}

	uart::uart1::uart_write("Hello from echo bot!\r\n");

	while (true)
	{
		uart::uart1::sleep();

		std::string rx_data;
		if (uart::uart1::uart_read(rx_data, K_FOREVER) == uart::UART_READ_OK)
		{
			uart::uart1::uart_write("got: " + rx_data);
		}
	}
}
