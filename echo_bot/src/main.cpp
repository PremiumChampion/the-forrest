#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "uart/uart0.hpp"
#include "uart/uart1.hpp"

LOG_MODULE_REGISTER(main);
uart::uart0::uart0_implementation uart0_driver = uart::uart0::uart0_implementation();
uart::uart1::uart1_implementation uart1_driver = uart::uart1::uart1_implementation();
uart::uart_driver *device = &uart0_driver;

int main(void)
{
	if (uart::uart1::uart_init() != 0)
	{
		LOG_ERR("UART initialization failed\n");
		return -1;
	}

	int rc;

	uart::uart1::sleep();

	// initialize the abstraction and driver
	if (uart::uart0::uart_init() != 0)
	{
		LOG_ERR("UART initialization failed\n");
		return -1;
	}

	// we can change the baudrate
	rc = uart::uart0::change_baudrate(uart::baudrate::Baud9600);

	if (rc != 0)
	{
		LOG_ERR("Could not change baudrate: %d\n", rc);
		return -1;
	}

	bool sleep = false;

	while (true)
	{
		// we manually put the module to sleep.
		// we can not send data now except we continue with uart::uart0::wakeup
		// the driver autoresumes when there is activity on the bus
		// no state management
		if (sleep)
		{
			uart::uart0::sleep();
		}

		std::string rx_data;
		// read available data on the bus
		if (uart::uart0::uart_read(rx_data, K_FOREVER) == uart::UART_READ_OK)
		{
			if(rx_data.starts_with("sleep")){
				sleep = true;
			}
			if(rx_data.starts_with("wakeup")){
				sleep = false;
			}
		}
	}

	return 0;
}
