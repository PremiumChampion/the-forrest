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
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>


LOG_MODULE_REGISTER(main);

// I2C Clock test
const struct device *const rtc = DEVICE_DT_GET(DT_NODELABEL(rtc_ds1307));
#define I2C0_NODE DT_NODELABEL(rtc_ds1307_i2c)
static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C0_NODE);

// Lora startup init pin do not touch
static const struct gpio_dt_spec reset_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mycusgpio), gpios, {0});

std::string response;

void join_network(int network_id, int dev_address)
{
    LOG_INF("Setting frequency band");
    at::commands::prv::_at("AT+BAND=433000000\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting address");
    at::commands::prv::_at("AT+ADDRESS=" + std::to_string(dev_address) + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting network id");
    at::commands::prv::_at("AT+NETWORKID=" + std::to_string(network_id) + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Joining network, sending HELLO");
    at::commands::prv::_at("AT+SEND=2,5,HELLO\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

void send_message(int dev_address, std::string message)
{
    at::commands::prv::_at("AT+SEND=" + std::to_string(dev_address) + "," + std::to_string(message.length()) + "," + message + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

int init_lora_module()
{
    if (!gpio_is_ready_dt(&reset_pin))
    {
        return -1;
    }

    gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT);

    k_sleep(K_MSEC(4000));

    gpio_pin_set_dt(&reset_pin, 1);
    k_sleep(K_MSEC(1000));

    at::commands::result result;
    result = at::commands::prv::at();
    while (result != at::commands::result::OK)
    {
        LOG_INF("Waiting for module to respond");

        switch (result)
        {
        case at::commands::result::OK:
            LOG_INF("SUCCESS");
            break;
        case at::commands::result::TIMEOUT:
            LOG_INF("TIMEOUT");
            break;
        default:
            LOG_INF("UNKNOWN");
            break;
        }

        result = at::commands::prv::at();
        k_sleep(K_MSEC(1000));
    }

    LOG_INF("Setting parameters");

    result = at::commands::prv::_at("AT+PARAMETER=9,7,1,12\r\n", response);

    switch (result)
    {
    case at::commands::result::OK:
        LOG_INF("SUCCESS");
        break;
    case at::commands::result::TIMEOUT:
        LOG_INF("TIMEOUT");
        break;
    default:
        LOG_INF("UNKNOWN");
        break;
    }
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting RF power");
    at::commands::prv::_at("AT+CRFOP=22\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
    return 0;
}

// Needs to be called and set again after each receive
void set_smart_transceiver_mode()
{
    // rx time,sleep_time
    at::commands::prv::_at("AT+MODE=2,100,10000\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

void lora_module_sleep()
{
    at::commands::prv::_at("AT+MODE=1\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

void lora_module_wake()
{
    at::commands::prv::_at("AT+MODE=0\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

// RTC Test
static int set_date_time(const struct device *rtc)
{
    int ret = 0;
    struct rtc_time tm = {
        .tm_sec = 0,
        .tm_min = 20,
        .tm_hour = 4,
        .tm_mday = 11,
        .tm_mon = 9 - 1,
        .tm_year = 2001 - 1900,
    };

    if (!device_is_ready(rtc))
    {
        printk("RTC device is not ready\n");
        return -ENODEV;
    }

    ret = rtc_set_time(rtc, &tm);
    if (ret < 0)
    {
        printk("Cannot write date time: %d\n", ret);
        return ret;
    }
    return ret;
}

static int get_date_time(const struct device *rtc)
{
    int ret = 0;
    struct rtc_time tm;

    ret = rtc_get_time(rtc, &tm);
    if (ret < 0)
    {
        LOG_INF("Cannot read date time: %d\n", ret);
        return ret;
    }

    LOG_INF("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
            tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return ret;
}

// RTC Test End

bool GATEWAY = true;
int GATEWAY_ADDRESS = 420;
int NODE_ADDRESS = 1;

#define DEVICE_ADDR 0x68
#define REG_ADDR 0x00

const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

int main(void)
{

    if (uart::uart_init() != 0)
    {
        printk("UART initialization failed\n");
        return -1;
    }

    if (init_lora_module() != 0)
    {
        printk("Failed to initialize LoRa module\n");
        return -1;
    }

    // RTC Test

    if (!device_is_ready(rtc))
    {
        printk("RTC device is not ready\n");
    }

    //i2c_configure(rtc, I2C_SPEED_SET(I2C_SPEED_STANDARD | I2C_MODE_CONTROLLER));

    /* Check if the RTC is ready */
    if (!device_is_ready(rtc))
    {
        LOG_INF("Device is not ready\n");
        return 0;
    }

    //set_date_time(rtc);

    /* Continuously read the current date and time from the RTC */
    // while (get_date_time(rtc) == 0)
    // {
    //     k_sleep(K_MSEC(1000));
    // };

    //Join the network

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

    while (1)
    {

        if (GATEWAY)
        {
            // receive data
            if (uart::uart_read(response) == uart::UART_READ_OK)
            {
                LOG_INF("%s", uart::escape_response(response).c_str());
                if (response.find("RCV") != std::string::npos)
                {
                    lora_module_sleep();
                    k_sleep(K_MSEC(8000));
                    lora_module_wake();
                }
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