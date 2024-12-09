#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <string>

#include "uart/uart.hpp"

LOG_MODULE_REGISTER(uart);

#define UART_MAX_MSG_SIZE 128

namespace uart
{

	const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

	K_MSGQ_DEFINE(uart_msgq_rx, UART_MAX_MSG_SIZE, 10, 4);

	/* receive buffer used in UART ISR callback */
	char rx_buf[UART_MAX_MSG_SIZE] = {0};
	size_t rx_buf_pos = 0;

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

			if (rx_buf_pos < (sizeof(rx_buf) - 1))
			{
				rx_buf[rx_buf_pos++] = c;
			}

			// check if we have reached the end of the buffer
			if (rx_buf_pos == UART_MAX_MSG_SIZE - 1 || c == '\n')
			{
				rx_buf[rx_buf_pos] = '\0';
				//LOG_INF("Received: %s", escape_response(rx_buf).c_str());
				// if queue is full, message is silently dropped
				k_msgq_put(&uart_msgq_rx, &rx_buf, K_NO_WAIT);
				// reset the buffer (it was copied to the msgq)
				rx_buf_pos = 0;
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
		char buf[UART_MAX_MSG_SIZE];
		if (k_msgq_get(&uart_msgq_rx, &buf, K_MSEC(100)) == 0)
		{
			response += std::string(buf);
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

		// // enable wakeup sources
		// if (!pm_device_wakeup_enable(uart, true))
		// {
		// 	LOG_ERR("Failed to enable UART wakeup sources");
		// 	return -1;
		// }

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
		char buf[UART_MAX_MSG_SIZE];
		while (k_msgq_get(&uart_msgq_rx, &buf, K_NO_WAIT) == 0)
		{
			// do nothing
		}
	}
} // namespace uart