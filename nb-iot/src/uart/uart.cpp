#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <string>

#include "uart/uart.hpp"

LOG_MODULE_REGISTER(uart);

namespace uart
{

	const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

	#define UART_MAX_MSG_SIZE 128
	using uart_msg = std::string*;

	K_MSGQ_DEFINE(uart_msgq_rx, sizeof(uart_msg), 10, 4);

	/* receive buffer used in UART ISR callback */
	std::string* recieved;
	

	void uart_cb(const struct device *dev, void *user_data)
	{

		if (!uart_irq_update(dev))
		{
			return;
		}

		if (!uart_irq_rx_ready(dev))
		{
			return;
		}

		uint8_t c;

		/* read until FIFO empty */
		while (uart_fifo_read(dev, &c, 1) == 1)
		{
			if (recieved->length() < UART_MAX_MSG_SIZE)
			{
				(*recieved) += c;
			}

			// check if we have reached the end of the buffer
			if (recieved->length() == UART_MAX_MSG_SIZE || c == '\n')
			{
				// if queue is full, message is silently dropped
				k_msgq_put(&uart_msgq_rx, &recieved, K_NO_WAIT);
				uart_msg buf = recieved;
				recieved = new std::string();
			}
		}
	}

	write_result uart_write(std::string data)
	{
		for (std::size_t i = 0; i < data.size(); i++)
		{
			uart_poll_out(uart, data[i]);
		}

		return write_result::UART_WRITE_OK;
	}

	read_result uart_read(std::string &response)
	{
		uart_msg buf;
		if (k_msgq_get(&uart_msgq_rx, &buf, K_MSEC(100)) == 0)
		{
			response += (*buf).c_str();
			LOG_INF("RX: %s", (*buf).c_str());
			delete buf;
			return read_result::UART_READ_OK;
		}
		return read_result::UART_READ_TIMEOUT;
	}

	int uart_init()
	{
		if (!device_is_ready(uart))
		{
			LOG_ERR("UART device not found!");
			return -1;
		}

		// enable wakeup sources
		// if (!pm_device_wakeup_enable(uart, true))
		// {
		// 	LOG_ERR("Failed to enable UART wakeup sources");
		// 	return -1;
		// }
		recieved = new std::string();

		/* configure interrupt and callback to receive data */
		int ret = uart_irq_callback_user_data_set(uart, uart_cb, NULL);

		if (ret < 0)
		{
			if (ret == -ENOTSUP)
			{
				LOG_ERR("Interrupt-driven UART API support not enabled\n");
			}
			else if (ret == -ENOSYS)
			{
				LOG_ERR("UART device does not support interrupt-driven API\n");
			}
			else
			{
				LOG_ERR("Error setting UART callback: %d\n", ret);
			}
			return 0;
		}
		uart_irq_rx_enable(uart);

		return 0;
	}

	void flush()
	{
		uart_msg buf;
		while (k_msgq_get(&uart_msgq_rx, &buf, K_NO_WAIT) == 0)
		{
			delete buf;
			// do nothing
		}
	}

	std::string escape_response(std::string response)
	{
		std::string escaped = "";
		for (std::size_t i = 0; i < response.size(); i++)
		{
			switch (response[i])
			{
			case '\r':
				escaped += "\\r";
				break;
			case '\n':
				escaped += "\\n";
				break;
			default:
				escaped += response[i];
				break;
			}
		}
		return escaped;
	}

	std::string escape_response(char *response)
	{
		std::string escaped = "";
		for (std::size_t i = 0; response[i] != '\0'; i++)
		{
			switch (response[i])
			{
			case '\r':
				escaped += "\\r";
				break;
			case '\n':
				escaped += "\\n";
				break;
			default:
				escaped += response[i];
				break;
			}
		}
		return escaped;
	}
} // namespace uart