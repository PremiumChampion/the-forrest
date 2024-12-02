#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <algorithm>
#include <stdio.h>
#include "uart/uart.hpp"
#include "at/https.hpp"

LOG_MODULE_REGISTER(main);

int main(void)
{
    if (uart::uart_init() != 0)
    {
        printk("UART initialization failed\n");
        return -1;
    }

    k_sleep(K_MSEC(5000));

    while (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not responding");
    }

    at::commands::sim7000e::https::reboot();

    k_sleep(K_MSEC(1000));

    while (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not responding");
    }

    k_sleep(K_MSEC(100));

    bool ip_not_available = false;
    std::string ip;
    if (at::commands::sim7000e::https::get_ip(ip) != at::commands::OK)
    {
        LOG_ERR("SIM7000E IP address retrieval failed");
        ip_not_available = true;
    }

    ip_not_available = ip_not_available || ip.empty() || ip == "0.0.0.0";

    if (ip_not_available && at::commands::sim7000e::https::setup_apn("internet") != at::commands::OK)
    {
        LOG_ERR("SIM7000E APN setup failed");
        return -1;
    }

    k_sleep(K_MSEC(1000));
    if (!ip_not_available)
    {
        if (at::commands::sim7000e::https::get_ip(ip) != at::commands::OK)
        {
            LOG_ERR("SIM7000E IP address retrieval failed");
            return -1;
        }
        else
        {
            LOG_INF("IP address: %s", ip.c_str());
        }
    }

    if (at::commands::sim7000e::https::ignore_ssl_timestamp() != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL timestamp ignore failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

    if (at::commands::sim7000e::https::set_ssl_version() != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL version set failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

    if (at::commands::sim7000e::https::set_server_name_indication("static.woyte.dev") != at::commands::OK)
    {
        LOG_ERR("SIM7000E server name indication set failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

    if (at::commands::sim7000e::https::trust_all_certificates() != at::commands::OK)
    {
        LOG_ERR("SIM7000E trust all certificates failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

    std::string body = "{\"temp\":69, \"humidity\":420}";

    // set_body_length
    if (at::commands::sim7000e::https::set_body_length(1024) != at::commands::OK)
    {
        LOG_ERR("SIM7000E body length set failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

    // set_header_length
    if (at::commands::sim7000e::https::set_header_length(350) != at::commands::OK)
    {
        LOG_ERR("SIM7000E header length set failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

    // set domain for Server Name Indication
    if (at::commands::sim7000e::https::set_domain("https://static.woyte.dev") != at::commands::OK)
    {
        LOG_ERR("SIM7000E domain set failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

    if(at::commands::sim7000e::https::get_time() != at::commands::OK)
    {
        LOG_ERR("SIM7000E time get failed");
    }

    k_sleep(K_MSEC(100));
    // set time
    if (at::commands::sim7000e::https::set_time() != at::commands::OK)
    {
        LOG_ERR("SIM7000E time set failed");
    }

    k_sleep(K_MSEC(100));
    // start_ssl_session
    if (at::commands::sim7000e::https::start_ssl_session() != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL session start failed");
        // return;
    }

    k_sleep(K_MSEC(100));
    // clear header
    if (at::commands::sim7000e::https::clear_header() != at::commands::OK)
    {
        LOG_ERR("SIM7000E header clear failed");
        return -1;
    }

    k_sleep(K_MSEC(100));
    // set_header
    if (at::commands::sim7000e::https::set_header("Content-Type", "application/json") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header set failed");
        return -1;
    }

    k_sleep(K_MSEC(100));
    // AT+SHAHEAD="User-Agent","curl/7.47.0"
    if (at::commands::sim7000e::https::set_header("User-Agent", "curl/7.47.0") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header user-agent set failed");
        return -1;
    }

    k_sleep(K_MSEC(100));
    // AT+SHAHEAD="Cache-control","no-cache"
    if (at::commands::sim7000e::https::set_header("Cache-control", "no-cache") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header cache control set failed");
        return -1;
    }

    k_sleep(K_MSEC(100));
    // AT+SHAHEAD="Accept","*/*"
    if (at::commands::sim7000e::https::set_header("Accept", "*/*") != at::commands::OK)
    {
        LOG_ERR("SIM7000E header accept set failed");
        return -1;
    }

    k_sleep(K_MSEC(100));
    // set_body
    if (at::commands::sim7000e::https::set_body(body) != at::commands::OK)
    {
        LOG_ERR("SIM7000E body set failed");
        return -1;
    }
    k_sleep(K_MSEC(100));

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
    k_sleep(K_MSEC(100));
    std::string response;
    if (at::commands::sim7000e::https::read(response, length) != at::commands::OK)
    {
        LOG_ERR("SIM7000E read failed");
        return -1;
    }

    LOG_INF("Response: %s", response.c_str());

    k_sleep(K_MSEC(100));
    if (at::commands::sim7000e::https::stop_ssl_session() != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL session stop failed");
        return -1;
    }

    k_sleep(K_MSEC(100));
    if (at::commands::sim7000e::https::network_disconnect() != at::commands::OK)
    {
        LOG_ERR("SIM7000E network disconnect failed");
        return -1;
    }
}