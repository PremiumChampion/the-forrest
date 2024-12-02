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

static const struct gpio_dt_spec reset_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mycusgpio), gpios, {0});

void join_network(int network_id, int dev_address)
{
    uart::uart_write("AT+BAND=433000000\r\n");
    k_sleep(K_MSEC(1000));
    uart::uart_write("AT+ADDRESS=" + std::to_string(dev_address) + "\r\n");
    k_sleep(K_MSEC(1000));
    uart::uart_write("AT+NETWORKID=" + std::to_string(network_id) + "\r\n");
    k_sleep(K_MSEC(1000));
    uart::uart_write("AT+SEND=2,5,HELLO\r\n");
}

void send_message(int dev_address, std::string message)
{
    uart::uart_write("AT+SEND=" + std::to_string(dev_address) + "," + std::to_string(message.length()) + "," + message + "\r\n");
}

int init_lora_module() {
    if (!gpio_is_ready_dt(&reset_pin))
    {
        return -1;
    }

    gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT);

    k_sleep(K_MSEC(4000));

    gpio_pin_set_dt(&reset_pin, 1);
    k_sleep(K_MSEC(1000));

    // set antenna power
    
    LOG_INF("Setting antenna power");
    uart::uart_write("AT+PARAMETER=9,7,1,12\r\n");
    k_sleep(K_MSEC(1000));
    LOG_INF("Setting RF power");
    uart::uart_write("AT+CRFOP=22\r\n");
    k_sleep(K_MSEC(1000));
    return 0;
}

bool GATEWAY = true;
int GATEWAY_ADDRESS = 420;
int NODE_ADDRESS = 1;

int main(void)
{
    if(init_lora_module() != 0) {
        printk("Failed to initialize LoRa module\n");
        return -1;
    }
    
    if (uart::uart_init() != 0)
    {
        printk("UART initialization failed\n");
        return -1;
    }

    // Join the network
    if (GATEWAY)
    {
        LOG_INF("Joining network as gateway");
        join_network(69, GATEWAY_ADDRESS);
    }
    else
    {
        LOG_INF("Joining network as node");
        join_network(69, NODE_ADDRESS);
    }

    std::string response;
    while (1)
    {
        k_sleep(K_MSEC(2000));

        if (GATEWAY)
        {
            // receive data
            if (uart::uart_read(response) == uart::UART_READ_OK)
            {
                //LOG_INF("%s", response.c_str());
                response = "";
            }
        }
        else
        {
            // send data
            send_message(GATEWAY_ADDRESS, "We <3 embedded");
        }
    }
}