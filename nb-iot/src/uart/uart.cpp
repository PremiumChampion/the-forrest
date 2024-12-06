#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <string>
#include <span>
#include <tuple>
#include "uart/uart.hpp"

LOG_MODULE_REGISTER(uart);

namespace uart
{
	// start and end index of the message
	typedef struct
	{
		std::size_t start;
		std::size_t size;
		std::size_t end;
	} uart_msg;
	const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

#define UART_MAX_MESSAGE_SIZE 128
#define UART_MSG_BUFFER_SIZE 1024

	// this is the buffer where we store the incoming messages
	char uart_msg_ring_buffer[UART_MSG_BUFFER_SIZE];
	// this indicates the occupied space in the buffer
	std::size_t uart_msg_buffer_left = 0;
	// this indicates the next position we will write to
	std::size_t uart_msg_buffer_right = 0;
	// indicates the space occupied in the buffer
	std::size_t uart_buffer_filled = 0;
	// this indicates the start of the current message
	std::size_t uart_current_message = 0;
	// this indicates the size of the current message
	std::size_t uart_current_message_size = 0;

	// this is the queue where we transmitt messages to the application
	K_MSGQ_DEFINE(uart_msgq_rx, sizeof(uart_msg), 128, 4);

	uart_msg finalize_msg_in_buffer()
	{
		uart_msg msg{uart_current_message, uart_current_message_size, uart_msg_buffer_right};
		uart_current_message = uart_msg_buffer_right;
		uart_current_message_size = 0;
		return msg;
	}

	void free_msg_in_buffer(uart_msg msg)
	{
		std::size_t start = msg.start;
		std::size_t size = msg.size;
		if (start != uart_msg_buffer_left)
		{
			return;
		}

		uart_msg_buffer_left = (uart_msg_buffer_left + size) % UART_MSG_BUFFER_SIZE;
		uart_buffer_filled -= size;
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

			// check if the buffer is full
			if (uart_buffer_filled == UART_MSG_BUFFER_SIZE)
			{
				LOG_ERR("RX buffer overflow, check if you want to increase the buffer size");
				// reset the buffer discarding the last message

				uart_msg_buffer_right = (uart_msg_buffer_right - uart_current_message_size) % UART_MSG_BUFFER_SIZE;
				uart_buffer_filled -= uart_current_message_size;
				uart_current_message_size = 0;

				return;
			}

			// add the character to the buffer
			uart_msg_ring_buffer[uart_msg_buffer_right] = c;
			uart_msg_buffer_right = (uart_msg_buffer_right + 1) % UART_MSG_BUFFER_SIZE;
			uart_current_message_size++;
			uart_buffer_filled++;

			// check if we have received a newline character
			if (c == '\n' || uart_current_message_size > UART_MAX_MESSAGE_SIZE || uart_buffer_filled == UART_MSG_BUFFER_SIZE)
			{
				uart_msg msg = finalize_msg_in_buffer();

				// if queue is full, drop the message
				if (k_msgq_put(&uart_msgq_rx, &msg, K_NO_WAIT) == -EAGAIN)
				{
					LOG_ERR("RX message dropped");
					free_msg_in_buffer(msg);
				}
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
		uart_msg msg{0, 0};
		std::string _response = "";
		if (k_msgq_get(&uart_msgq_rx, &msg, K_MSEC(100)) == 0)
		{
			// LOG_INF("MSG: [start=%d] [end=%d] [size=%d]", msg.start, msg.end, msg.size);
			if (msg.start < msg.end)
			{
				_response = std::string(uart_msg_ring_buffer + msg.start, uart_msg_ring_buffer + msg.end);
			}
			else
			{
				_response = std::string(uart_msg_ring_buffer + msg.start, uart_msg_ring_buffer + UART_MSG_BUFFER_SIZE) + std::string(uart_msg_ring_buffer, uart_msg_ring_buffer + msg.end);
			}

			free_msg_in_buffer(msg);

			LOG_INF("RX: %s", escape_response(_response).c_str());

			response += _response;

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
		if (!pm_device_wakeup_enable(uart, true))
		{
			LOG_ERR("Failed to enable UART wakeup sources");
			return -1;
		}

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
		uart_msg buf{};
		while (k_msgq_get(&uart_msgq_rx, &buf, K_NO_WAIT) == 0)
		{
			free_msg_in_buffer(buf);
			// do nothing
		}
	}

	std::string escape_response(std::string response)
	{
		return escape_response((char *)response.c_str());
	}

	std::string escape_response(char *response)
	{
		std::string escaped = "";
		for (std::size_t i = 0; response[i] != '\0'; i++)
		{
			switch (response[i])
			{
			case '\0':
				escaped += "\\0";
				break;
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