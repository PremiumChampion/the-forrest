#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <algorithm>
#include <stdio.h>
#include "uart/uart.hpp"
#include "at/sim7000e/https.hpp"
#include "gpio/gpio.hpp"
#include "at/sim7000e/time.hpp"
#include "iic/bus.hpp"
#include "iic/time.hpp"

LOG_MODULE_REGISTER(main);

int main_send_mb_iot_msg()
{

    if (uart::uart_init() != 0)
    {
        LOG_ERR("UART initialization failed\n");
        return -1;
    }

    if (iic::bus::setup() != 0)
    {
        LOG_ERR("I2C bus setup failed");
        return -1;
    }

    // k_sleep(K_MSEC(5000));

    while (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not responding");
    }

    at::commands::sim7000e::https::reboot();

    while (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not responding");
    }

    bool ip_not_available = false;
    std::string ip;
    // if (at::commands::sim7000e::https::get_ip(ip) != at::commands::OK)
    // {
    //     LOG_ERR("SIM7000E IP address retrieval failed");
    //     ip_not_available = true;
    // }

    ip_not_available = ip_not_available || ip.empty() || ip == "0.0.0.0";

    while (ip_not_available && at::commands::sim7000e::https::setup_apn("internet") != at::commands::OK)
    {
        LOG_ERR("SIM7000E APN setup failed");
    }

    if (ip_not_available)
    {
        if (at::commands::sim7000e::https::get_ip(ip) != at::commands::OK)
        {
            LOG_ERR("SIM7000E IP address retrieval failed");
        }
        else
        {
            LOG_INF("IP address: %s", ip.c_str());
        }
    }

    if (at::commands::sim7000e::https::ignore_ssl_timestamp() != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL timestamp ignore failed");
    }

    if (at::commands::sim7000e::https::set_ssl_version() != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL version set failed");
    }

    if (at::commands::sim7000e::https::set_server_name_indication("static.woyte.dev") != at::commands::OK)
    {
        LOG_ERR("SIM7000E server name indication set failed");
    }

    if (at::commands::sim7000e::https::trust_all_certificates() != at::commands::OK)
    {
        LOG_ERR("SIM7000E trust all certificates failed");
        return -1;
    }

    struct tm time = iic::time::get_current_time();

    std::string body = "{";
    body += "\"time\": \"";
    // body += "2021-09-01T12:00:00+00:00\","; // current time in ISO 8601 format
    body += ((time.tm_year + 1900) < 9 ? "0" : "") + std::to_string(time.tm_year + 1900) + "-";
    body += (time.tm_mon + 1 < 9 ? "0" : "") + std::to_string(time.tm_mon + 1) + "-";
    body += (time.tm_mday < 9 ? "0" : "") + std::to_string(time.tm_mday) + "T";
    body += (time.tm_hour < 9 ? "0" : "") + std::to_string(time.tm_hour) + ":";
    body += (time.tm_min < 9 ? "0" : "") + std::to_string(time.tm_min) + ":";
    body += (time.tm_sec < 9 ? "0" : "") + std::to_string(time.tm_sec) + "+00:00" + "\",";
    body += "\"temperature\": 25.5,";
    body += "\"humidity\": 50.5";
    body += "}";

    // set_body_length
    if (at::commands::sim7000e::https::set_body_length(1024) != at::commands::OK)
    {
        LOG_ERR("SIM7000E body length set failed");
        return -1;
    }

    // set_header_length
    if (at::commands::sim7000e::https::set_header_length(350) != at::commands::OK)
    {
        LOG_ERR("SIM7000E header length set failed");
        return -1;
    }

    // set domain for Server Name Indication
    if (at::commands::sim7000e::https::set_domain("https://static.woyte.dev") != at::commands::OK)
    {
        LOG_ERR("SIM7000E domain set failed");
        return -1;
    }

    if (at::commands::sim7000e::time::get_time() != at::commands::OK)
    {
        LOG_ERR("SIM7000E time get failed");
    }

    // set time
    if (at::commands::sim7000e::time::set_time() != at::commands::OK)
    {
        LOG_ERR("SIM7000E time set failed");
    }

    // start_ssl_session
    at::commands::result sslstartresult = at::commands::sim7000e::https::start_ssl_session();
    if (sslstartresult != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL session start failed");
        LOG_ERR("REASON: %d", sslstartresult);
        // return;
    }

    // clear header
    if (at::commands::sim7000e::https::clear_header() != at::commands::OK)
    {
        LOG_ERR("SIM7000E header clear failed");
        return -1;
    }

    // set_header
    if (at::commands::sim7000e::https::set_header("Content-Type", "application/json") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header set failed");
        return -1;
    }

    // AT+SHAHEAD="User-Agent","curl/7.47.0"
    if (at::commands::sim7000e::https::set_header("User-Agent", "curl/7.47.0") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header user-agent set failed");
        return -1;
    }

    // AT+SHAHEAD="Cache-control","no-cache"
    if (at::commands::sim7000e::https::set_header("Cache-control", "no-cache") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header cache control set failed");
        return -1;
    }

    // AT+SHAHEAD="Accept","*/*"
    if (at::commands::sim7000e::https::set_header("Accept", "*/*") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header accept set failed");
        return -1;
    }

    // set_body
    if (at::commands::sim7000e::https::set_body(body) != at::commands::OK)
    {
        LOG_ERR("SIM7000E body set failed");
        return -1;
    }

    int http_status_code;
    int length;
    if (at::commands::sim7000e::https::exec("/api/save_to_db", at::commands::sim7000e::https::http_method::POST, http_status_code, length) != at::commands::OK)
    {
        LOG_ERR("SIM7000E exec failed");
        return -1;
    }

    LOG_INF("HTTP status code: %d", http_status_code);
    LOG_INF("Length: %d", length);

    // read response
    std::string response;
    if (at::commands::sim7000e::https::read(response, length) != at::commands::OK)
    {
        LOG_ERR("SIM7000E read failed");
        return -1;
    }

    LOG_INF("Response: %s", response.c_str());

    if (at::commands::sim7000e::https::stop_ssl_session() != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL session stop failed");
        return -1;
    }

    if (at::commands::sim7000e::https::network_disconnect() != at::commands::OK)
    {
        LOG_ERR("SIM7000E network disconnect failed");
        return -1;
    }

    return 0;
}

int main_send_to_sleep()
{
    if (uart::uart_init() != 0)
    {
        LOG_ERR("UART initialization failed");
        return -1;
    }

    if (gpio::init() != gpio::OK)
    {
        LOG_ERR("GPIO initialization failed");
        return -1;
    }

    // module powers on by default when recieving power for the first time
    if (gpio::set(gpio::SIM7000_PWR, gpio::HIGH) != gpio::OK)
    {
        LOG_ERR("SIM7000E GPIO power set failed");
        return -1;
    }

    if (iic::bus::setup() != 0)
    {
        LOG_ERR("I2C bus setup failed");
        return -1;
    }

    k_sleep(K_MSEC(5000));

    if (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
    {
        LOG_INF("SIM7000E not responding");

        // power on the module
        if (gpio::set(gpio::SIM7000_PWR, gpio::HIGH) != gpio::OK)
        {
            return -1;
        }

        k_sleep(K_MSEC(100));

        // wait for the module to boot
        while (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
        {
            LOG_INF("SIM7000E not responding");
        }
    }

    // power off the module

    return 0;
}

int main(void)
{
    main_send_mb_iot_msg();
}