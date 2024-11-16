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

    while (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not responding\n");
    }


    if (at::commands::sim7000e::https::setup_apn("internet") != at::commands::OK)
    {
        LOG_ERR("SIM7000E APN setup failed\n");
    }

    std::string ip;
    if (at::commands::sim7000e::https::get_ip(ip) != at::commands::OK)
    {
        LOG_ERR("SIM7000E IP address retrieval failed\n");
    }
    else
    {
        LOG_INF("IP address: %s", ip.c_str());
    }

    if (at::commands::sim7000e::https::ignore_ssl_timestamp() != at::commands::OK)  {
        LOG_ERR("SIM7000E SSL timestamp ignore failed\n");
    }

    if (at::commands::sim7000e::https::set_ssl_version() != at::commands::OK)  {
        LOG_ERR("SIM7000E SSL version set failed\n");
    }
    

    if (at::commands::sim7000e::https::set_server_name_indication("static.woyte.dev") != at::commands::OK)  {
        LOG_ERR("SIM7000E server name indication set failed\n");
    }

    if (at::commands::sim7000e::https::trust_all_certificates() != at::commands::OK)  {
        LOG_ERR("SIM7000E trust all certificates failed\n");
    }

    std::string body = at::commands::sim7000e::https::escape_body("{\"key\":\"value\"}");

    // set_body_length
    if (at::commands::sim7000e::https::set_body_length(1024) != at::commands::OK)  {
        LOG_ERR("SIM7000E body length set failed\n");
    }

    // set_header_length
    if (at::commands::sim7000e::https::set_header_length(350) != at::commands::OK)  {
        LOG_ERR("SIM7000E header length set failed\n");
    }

    // set_domain
    if (at::commands::sim7000e::https::set_domain("https://static.woyte.dev") != at::commands::OK)  {
        LOG_ERR("SIM7000E domain set failed\n");
    }

    // start_ssl_session
    if (at::commands::sim7000e::https::start_ssl_session() != at::commands::OK)  {
        LOG_ERR("SIM7000E SSL session start failed\n");
    }

    // clear header
    if (at::commands::sim7000e::https::clear_header() != at::commands::OK)  {
        LOG_ERR("SIM7000E header clear failed\n");
    }

    // set_header
    if (at::commands::sim7000e::https::set_header("Content-Type", "application/json") != at::commands::OK)  {
        LOG_ERR("SIM7000E header set failed\n");
    }

    // AT+SHAHEAD="User-Agent","curl/7.47.0"
    if (at::commands::sim7000e::https::set_header("User-Agent", "curl/7.47.0") != at::commands::OK)  {
        LOG_ERR("SIM7000E header set failed\n");
    }

    // AT+SHAHEAD="Cache-control","no-cache"
    if (at::commands::sim7000e::https::set_header("Cache-control", "no-cache") != at::commands::OK)  {
        LOG_ERR("SIM7000E header set failed\n");
    }

    // AT+SHAHEAD="Accept","*/*"
    if (at::commands::sim7000e::https::set_header("Accept", "*/*") != at::commands::OK)  {
        LOG_ERR("SIM7000E header set failed\n");
    }

    // set_body
    if (at::commands::sim7000e::https::set_body(body) != at::commands::OK)  {
        LOG_ERR("SIM7000E body set failed\n");
    }

    int http_status_code;
    int length;
    if (at::commands::sim7000e::https::exec("/api/save_to_db", at::commands::sim7000e::https::http_method::POST, http_status_code, length) != at::commands::OK)  {
        LOG_ERR("SIM7000E exec failed\n");
    }

    LOG_INF("HTTP status code: %d", http_status_code);
    LOG_INF("Length: %d", length);

    // read response
    std::string response;
    if (at::commands::sim7000e::https::read(response, length) != at::commands::OK)  {
        LOG_ERR("SIM7000E read failed\n");
    }

    LOG_INF("Response: %s", response.c_str());

    if (at::commands::sim7000e::https::stop_ssl_session() != at::commands::OK)  {
        LOG_ERR("SIM7000E SSL session stop failed\n");
    }

    if (at::commands::sim7000e::https::network_disconnect() != at::commands::OK)  {
        LOG_ERR("SIM7000E network disconnect failed\n");
    }
}