/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/uart.h>

#include <string.h>

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define UART1_DEVICE_NODE DT_NODELABEL(uart1)

#define MSG_SIZE 32

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);
K_MSGQ_DEFINE(uart1_msgq, MSG_SIZE, 10, 4);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
static const struct device *const uart1_dev = DEVICE_DT_GET(UART1_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

static char rx1_buf[MSG_SIZE];
static int rx1_buf_pos;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb0(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb1(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart1_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart1_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart1_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx1_buf_pos > 0) {
			/* terminate string */
			rx1_buf[rx1_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart1_msgq, &rx1_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			rx1_buf_pos = 0;
		} else if (rx1_buf_pos < (sizeof(rx1_buf) - 1)) {
			rx1_buf[rx1_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart0(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void print_uart1(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart1_dev, buf[i]);
	}
}
struct k_poll_event events[2];
int main(void)
{
	char tx_buf[MSG_SIZE];

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return 0;
	}

	if (!device_is_ready(uart1_dev)) {
		printk("UART1 device not found!");
		return 0;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb0, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}
	uart_irq_rx_enable(uart_dev);

	/* configure interrupt and callback to receive data */
	ret = uart_irq_callback_user_data_set(uart1_dev, serial_cb1, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}
	uart_irq_rx_enable(uart1_dev);

	print_uart0("Hello! I'm your echo bot.\r\n");
	print_uart0("Tell me something and press enter:\r\n");

	k_poll_event_init(&events[0],
					K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&uart_msgq);

	k_poll_event_init(&events[1],
					K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&uart1_msgq);

	while (1) {
		int rc = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		if (rc == 0) {
			if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
				ret = k_msgq_get(&uart_msgq, &tx_buf, K_NO_WAIT);
				// handle data
			} else if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
				ret = k_msgq_get(&uart1_msgq, &tx_buf, K_NO_WAIT);
				// handle data
			}

			print_uart0("Echo: ");
			print_uart0(tx_buf);
			print_uart0("\n");
			print_uart1("Echo: ");
			print_uart1(tx_buf);
			print_uart1("\n");

			events[0].state = K_POLL_STATE_NOT_READY;
        	events[1].state = K_POLL_STATE_NOT_READY;
		}else {
        // handle timeout
    	}

    }

	return 0;
}
