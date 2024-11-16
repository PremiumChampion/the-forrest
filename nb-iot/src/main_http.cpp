#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <algorithm>
#include <stdio.h>
#include "uart/uart.hpp"
#include "at/http.hpp"

LOG_MODULE_REGISTER(main);

int main(void)
{
    if (uart::uart_init() != 0)
    {
        printk("UART initialization failed\n");
        return -1;
    }
    // struct k_poll_event events[1];
    // k_poll_event_init(&events[0],
    //                   K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
    //                   K_POLL_MODE_NOTIFY_ONLY,
    //                   &uart::uart_msgq_rx);

    if (at::commands::sim7000e::http::check_sim7000e_present() != at::commands::OK)
    {
        LOG_ERR("SIM7000E not responding\n");
    }

    // if (at::commands::sim7000e::set_network_mode_nb_iot() != at::commands::OK)
    // {
    //     LOG_ERR("Failed to set module to NB-IoT mode\n");
    // }

    if (at::commands::sim7000e::http::enable_scrambling() != at::commands::OK)
    {
        LOG_ERR("Failed to enable scrambling\n");
    }

    std::string provider = "";
    if (at::commands::sim7000e::http::get_operator(provider) != at::commands::OK)
    {
        LOG_ERR("Failed to get operator\n");
    }

    LOG_INF("Operator: %s", provider.c_str());

    if (at::commands::sim7000e::http::configure_bearer_profile() != at::commands::OK)
    {
        LOG_ERR("Failed to configure bearer profile\n");
    }


    if (at::commands::sim7000e::http::enable_bearer_profile() != at::commands::OK)
    {
        LOG_ERR("Failed to enable bearer profile\n");
    }

    std::string ip_address = "";
    if (at::commands::sim7000e::http::get_ip_address(ip_address) != at::commands::OK)
    {
        LOG_ERR("Failed to get IP address\n");
    }

    k_sleep(K_MSEC(1000));
    uart::flush();

    if (at::commands::sim7000e::http::start_http_session() != at::commands::OK)
    {
        LOG_ERR("Failed to start HTTP session\n");
    }

    if (at::commands::sim7000e::http::set_context_id_1() != at::commands::OK)
    {
        LOG_ERR("Failed to set context ID 1\n");
    }

    if (at::commands::sim7000e::http::allow_redirects() != at::commands::OK)
    {
        LOG_ERR("Failed to allow redirects\n");
    }

    if (at::commands::sim7000e::http::set_url("http:/teststaticfunction.azurewebsites.net") != at::commands::OK)
    {
        LOG_ERR("Failed to set URL\n");
    }

    if (at::commands::sim7000e::http::set_content_type("application/json") != at::commands::OK)
    {
        LOG_ERR("Failed to set content type\n");
    }

    if (at::commands::sim7000e::http::add_body("{\"temp\":24}") != at::commands::OK)
    {
        LOG_ERR("Failed to add body\n");
    }

    uart::flush();

    k_sleep(K_MSEC(1000));

    uart::flush();

    int http_status_code;

    if (at::commands::sim7000e::http::exec_post(http_status_code) != at::commands::OK)
    {
        LOG_ERR("Failed to execute POST\n");
    }

    uart::flush();

    k_sleep(K_MSEC(1000));

    uart::flush();

    if (at::commands::sim7000e::http::stop_http_session() != at::commands::OK)
    {
        LOG_ERR("Failed to stop HTTP session\n");
    }  
      uart::flush();

    k_sleep(K_MSEC(1000));

    uart::flush();

    if (at::commands::sim7000e::http::stop_baeer_profile() != at::commands::OK)
    {
        LOG_ERR("Failed to stop bearer profile\n");
    }

    while (1)
    {
        k_sleep(K_MSEC(1000));
    }
}
