#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

#define UART0_DEVICE_NAME DT_NODE_FULL_NAME(DT_NODELABEL(uart0))
#define UART1_DEVICE_NAME DT_NODE_FULL_NAME(DT_NODELABEL(uart1))

static const struct device *uart0_dev;
static const struct device *uart1_dev;

#define BUFFER_SIZE 64
static uint8_t rx_buffer[BUFFER_SIZE];

void uart0_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    switch (evt->type) {
    case UART_RX_RDY:
        // Write received data to UART1
        printk("Received %d bytes\n", evt->data.rx.len);
        uart1_write(uart1_dev, evt->data.rx.buf, evt->data.rx.len);
        break;
    case UART_RX_DISABLED:
        // Re-enable RX
        uart_rx_enable(uart0_dev, rx_buffer, sizeof(rx_buffer), 100);
        break;
    default:
        break;
    }
}

void main(void) {
    uart0_dev = device_get_binding(UART0_DEVICE_NAME);
    uart1_dev = device_get_binding(UART1_DEVICE_NAME);

    if (!uart0_dev || !uart1_dev) {
        printk("Failed to get UART device bindings\n");
        return;
    }

    // Configure UART0 for receiving data
    uart_callback_set(uart0_dev, uart0_callback, NULL);
    uart_rx_enable(uart0_dev, rx_buffer, sizeof(rx_buffer), 100);

    printk("UART0 to UART1 bridge started\n");
}