#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <algorithm>
#include <stdio.h>
#include "uart/uart.hpp"
#include "at/prv.hpp"
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(main);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mycusgpio), gpios, {0});

void join_network(int network_id, int dev_address) {
        uart::uart_write("AT+BAND=433000000\r\n");
        k_sleep(K_MSEC(1000));
        uart::uart_write("AT+ADDRESS=" + std::to_string(dev_address) + "\r\n");
        k_sleep(K_MSEC(1000));
        uart::uart_write("AT+NETWORKID=" + std::to_string(network_id) + "\r\n");
        k_sleep(K_MSEC(1000));
        uart::uart_write("AT+SEND=2,5,HELLO\r\n");

}

void send_message(int dev_address, std::string message) {
    uart::uart_write("AT+SEND=" + std::to_string(dev_address) + "," + std::to_string(message.length()) + "," + message + "\r\n");
}

int main(void)
{
    if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

    gpio_pin_configure_dt(&led, GPIO_OUTPUT);

    k_sleep(K_MSEC(4000));

    gpio_pin_set_dt(&led, 1);
    if (uart::uart_init() != 0)
    {
        printk("UART initialization failed\n");
        return -1;
    }


    
    k_sleep(K_MSEC(1000));
    at::commands::prv::at();

    //Join the network

    join_network(69, 1);

    while(1) {
        k_sleep(K_MSEC(2000));
        send_message(2, "We <3 embedded not fuck this shit");
    }
    

}

