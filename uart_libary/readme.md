# UART libary

## Installation

In order to use this libary we have to merge the following files:

* Kconfig: define options and dependencies for the driver abstraction
* CMakeLists.txt: add files for the build system
* Overlay files: merge the overlay files. ensure you specify the pins you want to use to continue the uart driver

```
&uart0 {
  status = "okay";
};

/ {
  custom_gpios {

    compatible = "gpio-keys";

    uart0_rx: uart0_rx {
      gpios = <&gpio0 8 GPIO_ACTIVE_HIGH>;
      label = "UART0 RX";
    };

  };

  aliases {
    uart0rx = &uart0_rx;
  };
};
```

## Usage

Either use namespace methods directly in a functional way:

```cpp
/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "uart/uart0.hpp"

LOG_MODULE_REGISTER(main);

int main(void)
{
    // initialize the abstraction and driver
	if (uart::uart0::uart_init() != 0)
	{
		LOG_ERR("UART initialization failed\n");
		return -1;
	}

    // we can change the baudrate 
	int rc = uart::uart0::change_baudrate(uart::baudrate::Baud9600);

	if (rc != 0)
	{
		LOG_ERR("Could not change baudrate: %d\n", rc);
		return -1;
	}

    // write data via uart
	uart::uart0::uart_write("Hello from echo bot!\r\n");

	while (true)
	{
        // we manually put the module to sleep.
        // we can not send data now except we continue with uart::uart0::wakeup
        // the driver autoresumes when there is activity on the bus
        // no state management
		uart::uart0::sleep();

		std::string rx_data;
		// read available data on the bus
        if (uart::uart0::uart_read(rx_data, K_FOREVER) == uart::UART_READ_OK)
		{
            // echo the result back
			uart::uart0::uart_write("echo: " + rx_data);
		}
	}
}
```

or use the uart_driver class interface. this allows for easy reuse.

```cpp
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "uart/uart0.hpp"
#include "uart/uart1.hpp"

LOG_MODULE_REGISTER(main);
uart::uart0::uart0_implementation uart0_driver = uart::uart0::uart0_implementation();
uart::uart_driver *device = &uart0_driver; 

int main(void)
{
	// initialize the abstraction and driver
	if (device->uart_init() != 0)
	{
		LOG_ERR("UART initialization failed\n");
		return -1;
	}

    // we can change the baudrate 
	int rc = device->change_baudrate(uart::baudrate::Baud9600);

	if (rc != 0)
	{
		LOG_ERR("Could not change baudrate: %d\n", rc);
		return -1;
	}

    // write data via uart
	device->uart_write("Hello from echo bot!\r\n");

	while (true)
	{
        // we manually put the module to sleep.
        // we can not send data now except we continue with uart::uart1::wakeup
        // the driver autoresumes when there is activity on the bus
        // no state management
		device->sleep();

		std::string rx_data;
		// read available data on the bus
        if (device->uart_read(rx_data, K_FOREVER) == uart::UART_READ_OK)
		{
            // echo the result back
			device->uart_write("echo: " + rx_data);
		}
	}

	return 0;
}

```

## Querks

* We can only reliably wakeup when using a baudrate of 9600 or less.