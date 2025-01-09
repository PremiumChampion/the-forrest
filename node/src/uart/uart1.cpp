#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <errno.h>
#include <string>
#include <span>
#include <tuple>
#include "uart/uart1.hpp"

LOG_MODULE_REGISTER(uart1);

namespace uart::uart1
{
	// start and end index of the message
	typedef struct
	{
		std::size_t start;
		std::size_t size;
		std::size_t end;
	} uart_msg;

	uart1_implementation uart1_driver = uart1_implementation();

	static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

	static const struct gpio_dt_spec uart1_rx_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(uart1rx), gpios);

	static struct gpio_callback uart_gpio_cb_data;
	static void uart_interrupt(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

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
	// indicates if the uart device is initialized
	static bool is_initialized = false;
	// indicates if the uart device is sleeping
	static bool is_sleeping = false;

	// this is the queue where we transmitt messages to the application
	K_MSGQ_DEFINE(uart1_msgq_rx, sizeof(uart_msg), 128, 4);

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
				if (k_msgq_put(&uart1_msgq_rx, &msg, K_NO_WAIT) == -EAGAIN)
				{
					LOG_ERR("RX message dropped");
					free_msg_in_buffer(msg);
				}
			}
		}
	}

	write_result uart_write(std::string data)
	{
		if (!is_initialized)
		{
			LOG_ERR("UART device not initialized!");
			return write_result::UART_WRITE_ERROR;
		}

		if (is_sleeping)
		{
			LOG_ERR("UART device is sleeping!");
			return write_result::UART_WRITE_ERROR;
		}

		for (std::size_t i = 0; i < data.size(); i++)
		{
			uart_poll_out(uart, data[i]);
		}

		return write_result::UART_WRITE_OK;
	}

	read_result uart_read(std::string &response, k_timeout_t timeout)
	{
		if (!is_initialized)
		{
			LOG_ERR("UART device not initialized!");
			return read_result::UART_READ_ERROR;
		}

		uart_msg msg{0, 0};
		std::string _response = "";

		if (k_msgq_get(&uart1_msgq_rx, &msg, timeout) == 0)
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

		if (K_TIMEOUT_EQ(timeout, K_FOREVER))
		{
		}

		return read_result::UART_READ_TIMEOUT;
	}

	int uart_init()
	{
		if (is_initialized)
		{
			LOG_ERR("UART device already initialized!");
			return -1;
		}

		if (!device_is_ready(uart))
		{
			LOG_ERR("UART device not found!");
			return -1;
		}

		/* configure interrupt and callback to receive data */
		int rc = uart_irq_callback_user_data_set(uart, uart_cb, NULL);

		if (rc < 0)
		{
			if (rc == -ENOTSUP)
			{
				LOG_ERR("Interrupt-driven UART API support not enabled\n");
			}
			else if (rc == -ENOSYS)
			{
				LOG_ERR("UART device does not support interrupt-driven API\n");
			}
			else
			{
				LOG_ERR("Error setting UART callback: %d\n", rc);
			}
			return 0;
		}
		uart_irq_rx_enable(uart);

		/* Configure uart1_rx_gpio as input and set up interrupt */
		rc = gpio_pin_configure_dt(&uart1_rx_gpio, GPIO_INPUT);
		if (rc < 0)
		{
			LOG_ERR("Could not configure uart1_rx_gpio GPIO (%d)\n", rc);
			return 0;
		}

		gpio_init_callback(&uart_gpio_cb_data, uart_interrupt, BIT(uart1_rx_gpio.pin));

		rc = gpio_pin_interrupt_configure_dt(&uart1_rx_gpio, GPIO_INT_EDGE_FALLING);
		if (rc < 0)
		{
			LOG_ERR("Could not configure uart1_rx_gpio GPIO interrupt (%d)\n", rc);
			return rc;
		}

		is_initialized = true;
		is_sleeping = false;

		return 0;
	}

	/* Button callback function */
	static void uart_interrupt(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
	{
		int rc;
		rc = wakeup();
		rc = gpio_remove_callback(uart1_rx_gpio.port, &uart_gpio_cb_data);
	}

	int sleep()
	{
		int rc;

		rc = pm_device_action_run(uart, PM_DEVICE_ACTION_SUSPEND);
		if (rc < 0)
		{
			LOG_ERR("Could not suspend UART (%d)\n", rc);
			return rc;
		}

		rc = gpio_add_callback(uart1_rx_gpio.port, &uart_gpio_cb_data);
		if (rc < 0)
		{
			LOG_ERR("Could not add callback on uart1_rx_gpio GPIO (%d)\n", rc);
			return rc;
		}

		is_sleeping = true;

		return 0;
	}

	int wakeup()
	{
		int rc = pm_device_action_run(uart, PM_DEVICE_ACTION_RESUME);
		if (rc < 0)
		{
			LOG_ERR("Could not resume UART (%d)\n", rc);
			return rc;
		}
		is_sleeping = false;
		return rc;
	}

	void flush()
	{
		if (!is_initialized)
		{
			LOG_ERR("UART device not initialized!");
			return;
		}

		uart_msg buf{};
		while (k_msgq_get(&uart1_msgq_rx, &buf, K_NO_WAIT) == 0)
		{
			free_msg_in_buffer(buf);
			// do nothing
		}
	}

	int change_baudrate(baudrate baudrate)
	{
		if (!is_initialized)
		{
			LOG_ERR("UART device not initialized!");
			return -1;
		}

		if (is_sleeping)
		{
			LOG_ERR("UART device is sleeping!");
			return -1;
		}

		struct uart_config cfg;
		int rc;
		rc = uart_config_get(uart, &cfg);
		if (rc)
		{
			LOG_ERR("UART config get error %d", rc);
			return rc;
		}

		cfg.baudrate = baudrate;

		rc = uart_configure(uart, &cfg);
		if (rc)
		{
			LOG_ERR("UART configure error %d", rc);
		}

		rc = uart_config_get(uart, &cfg);
		if (rc)
		{
			LOG_ERR("UART config get error %d", rc);
			return rc;
		}

		LOG_INF("UART configured at %d baud", cfg.baudrate);
		return rc;
	}

	int get_baudrate(baudrate &baudrate){
		if (!is_initialized)
		{
			LOG_ERR("UART device not initialized!");
			return -1;
		}

		struct uart_config cfg;
		int rc;
		rc = uart_config_get(uart, &cfg);
		if (rc)
		{
			LOG_ERR("UART config get error %d", rc);
			return rc;
		}

		baudrate = (uart::baudrate)cfg.baudrate;

		return 0;
	}

	uart1_implementation::uart1_implementation() : uart_driver()
	{
		this->uart_msgq_rx = &uart::uart1::uart1_msgq_rx;
	}

	read_result uart1_implementation::uart_read(std::string &result, k_timeout_t timeout)
	{
		return uart::uart1::uart_read(result, timeout);
	}

	write_result uart1_implementation::uart_write(std::string data)
	{
		return uart::uart1::uart_write(data);
	}

	int uart1_implementation::change_baudrate(uart::baudrate baudrate)
	{
		return uart::uart1::change_baudrate(baudrate);
	}

	int uart1_implementation::get_baudrate(uart::baudrate &baudrate)
	{
		return uart::uart1::get_baudrate(baudrate);
	}

	int uart1_implementation::uart_init()
	{
		return uart::uart1::uart_init();
	}

	int uart1_implementation::sleep()
	{
		return uart::uart1::sleep();
	}

	int uart1_implementation::wakeup()
	{
		return uart::uart1::wakeup();
	}

	void uart1_implementation::flush()
	{
		uart::uart1::flush();
	}

} // namespace uart::uart1