#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel/thread.h>
#include "uart/uart.hpp"

#include <string.h>

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data);
const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

K_WORK_DEFINE(uart_work, uart_work_handler);

const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};

uart_init_return_code uart_init(void){
    int err;
    err = device_is_ready(uart);
    if (!err) {
        return uart_init_return_code::UART_INIT_DEVICE_NOT_READY;
    }

    err = uart_configure(uart, &uart_cfg);

    if (err == -ENOSYS) {
        return uart_init_return_code::UART_INIT_DEVICE_CONFIGURATION_ERROR;
    }

    err = uart_callback_set(uart, uart_cb, NULL);
    if (err) {
        return uart_init_return_code::UART_INIT_CALLBACK_ERROR;
    }

    return uart_init_return_code::UART_INIT_SUCCESS;
}


static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	
	case UART_TX_DONE:
		// do something
		break;

	case UART_TX_ABORTED:
		// do something
		break;
		
	case UART_RX_RDY:
        
        k_work_submit(&uart_work);
		// do something
		break;

	case UART_RX_BUF_REQUEST:
		// do something
		break;

	case UART_RX_BUF_RELEASED:
		// do something
		break;
		
	case UART_RX_DISABLED:
        uart_rx_enable(dev, rx_buf, sizeof(*rx_buf), 100);
		// do something
		break;

	case UART_RX_STOPPED:
		// do something
		break;
		
	default:
		break;
	}
}


void uart_work_handler(struct k_work *work){
}