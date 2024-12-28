#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fcntl.h>
#include <unistd.h>
// uart
#include "uart/uart.hpp"
// gpio
#include "gpio/gpio.hpp"
// sim7000e
#include "at/sim7000e/https.hpp"
#include "at/sim7000e/time.hpp"
#include "at/sim7000e/network_configuration.hpp"
#include "at/sim7000e/power.hpp"
// iic
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

    if (gpio::init() != gpio::OK)
    {
        LOG_ERR("GPIO initialization failed");
        return -1;
    }

    if (gpio::set(gpio::SIM7000_PWR, gpio::HIGH) != gpio::OK)
    {
        LOG_ERR("SIM7000E power on failed");
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

    while (ip_not_available && at::commands::sim7000e::network_configuration::setup_apn("internet") != at::commands::OK)
    {
        LOG_ERR("SIM7000E APN setup failed");
    }

    if (ip_not_available)
    {
        if (at::commands::sim7000e::network_configuration::get_ip(ip) != at::commands::OK)
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

    std::string time_request = "";

    if (at::commands::sim7000e::time::get_time(time_request) != at::commands::OK)
    {
        LOG_ERR("SIM7000E time get failed");
    }

    if (time_request.find("+CCLK: \"80") != std::string::npos)
    {
        LOG_INF("Need to set time");
        // set_time
        if (at::commands::sim7000e::time::set_time() != at::commands::OK)
        {
            LOG_ERR("SIM7000E time set failed");
            return -1;
        }
    }

    // start_ssl_session
    at::commands::result sslstartresult = at::commands::sim7000e::https::start_ssl_session();
    if (sslstartresult != at::commands::OK)
    {
        LOG_ERR("SIM7000E SSL session start failed");
        LOG_ERR("REASON: %d", sslstartresult);
        // return;
    }

    // // start_header
    // if (at::commands::sim7000e::https::start_header() != at::commands::OK)
    // {
    //     LOG_ERR("SIM7000E header start failed");
    //     return -1;
    // }

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

    // if (at::commands::sim7000e::network_configuration::network_disconnect() != at::commands::OK)
    // {
    //     LOG_ERR("SIM7000E network disconnect failed");
    //     return -1;
    // }

    return 0;
}

int send_to_sleep()
{
    // power off the module

    // Enable PSM event reporting
    if (at::commands::sim7000e::power::set_psm_event_report(at::commands::sim7000e::power::PSM_EVENT_REPORT_ENABLE) != at::commands::OK)
    {
        LOG_ERR("SIM7000E PSM event report enable failed");
        return -1;
    }
    // fix baud rate
    if (at::commands::sim7000e::power::fix_baud_rate() != at::commands::OK)
    {
        LOG_ERR("SIM7000E fix baud rate failed");
        return -1;
    }

    // enable slow clock
    if (at::commands::sim7000e::power::set_slow_clock(true) != at::commands::OK)
    {
        LOG_ERR("SIM7000E enable slow clock failed");
        return -1;
    }

    // disconnect from networt to enter PSM
    if (at::commands::sim7000e::network_configuration::network_disconnect() != at::commands::OK)
    {
        LOG_ERR("SIM7000E network disconnect failed");
    }

    if (at::commands::sim7000e::power::enter_idle_mode() != at::commands::OK)
    {
        LOG_ERR("SIM7000E enter idle mode failed");
        return -1;
    }

    // Inquiry timers configured by network. (optional)
    // Enable PSM mode and set the specific T3412_ext and T3324
    if (at::commands::sim7000e::power::enable_PSM() != at::commands::OK)
    {
        LOG_ERR("SIM7000E enable PSM failed");
        return -1;
    }

    // wait for wait_for_enter_psm
    if (at::commands::sim7000e::power::wait_for_enter_psm() != at::commands::OK)
    {
        LOG_ERR("SIM7000E wait for enter PSM failed");
        // k_sleep(K_MSEC(1000));
    }
    else
    {
        LOG_INF("Entering PSM successfully");
    }

    // wait for wait_for_exit_psm
    if (at::commands::sim7000e::power::wait_for_exit_psm() != at::commands::OK)
    {
        LOG_ERR("SIM7000E wait for exit PSM failed");
    }
    else
    {
        LOG_INF("Exited PSM successfull");
    }

    // disable slow clock
    if (at::commands::sim7000e::power::set_slow_clock(false) != at::commands::OK)
    {
        LOG_ERR("SIM7000E disable slow clock failed");
        return -1;
    }

    k_sleep(K_MSEC(10000));
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
    // we set it to low to power on the module from deep sleep
    if (gpio::set(gpio::SIM7000_PWR, gpio::LOW) != gpio::OK)
    {
        LOG_ERR("SIM7000E GPIO power set failed");
        return -1;
    }

    if (iic::bus::setup() != 0)
    {
        LOG_ERR("I2C bus setup failed");
        return -1;
    }

    // gpio::gpio_state state = gpio::LOW;

    // for (size_t i = 0; i < 10; i++)
    // {
    //     if(gpio::set(gpio::SIM7000_PWR, state))
    //     {
    //         LOG_ERR("SIM7000E GPIO power set failed");
    //         return -1;
    //     }
    //     k_sleep(K_MSEC(1000));
    //     state = state == gpio::LOW ? gpio::HIGH : gpio::LOW;
    // }

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
            k_sleep(K_MSEC(1000));
        }
    }

    while (1)
    {
        bool ip_not_available = false;
        std::string ip;
        if (at::commands::sim7000e::network_configuration::get_ip(ip) != at::commands::OK)
        {
            LOG_ERR("SIM7000E IP address retrieval failed");
            ip_not_available = true;
        }

        ip_not_available = ip_not_available || ip.empty() || ip == "0.0.0.0";

        while (ip_not_available && at::commands::sim7000e::network_configuration::setup_apn("internet") != at::commands::OK)
        {
            LOG_ERR("SIM7000E APN setup failed");
            k_sleep(K_MSEC(1000));
        }

        if (ip_not_available)
        {
            if (at::commands::sim7000e::network_configuration::get_ip(ip) != at::commands::OK)
            {
                LOG_ERR("SIM7000E IP address retrieval failed");
            }
            else
            {
                LOG_INF("IP address: %s", ip.c_str());
            }
        }

        return 0;
    }
    send_to_sleep();
}

int main_power_off()
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

    // POWER THE MODULE
    // we set it to low to power on the module from deep sleep
    if (gpio::set(gpio::SIM7000_PWR, gpio::LOW) != gpio::OK)
    {
        LOG_ERR("SIM7000E GPIO power set failed");
        return -1;
    }

    if (iic::bus::setup() != 0)
    {
        LOG_ERR("I2C bus setup failed");
        return -1;
    }

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
    bool ip_not_available = false;
    std::string ip;
    if (at::commands::sim7000e::network_configuration::get_ip(ip) != at::commands::OK)
    {
        LOG_ERR("SIM7000E IP address retrieval failed");
        ip_not_available = true;
    }

    ip_not_available = ip_not_available || ip.empty() || ip == "0.0.0.0";

    while (ip_not_available && at::commands::sim7000e::network_configuration::setup_apn("internet") != at::commands::OK)
    {
        LOG_ERR("SIM7000E APN setup failed");
        k_sleep(K_MSEC(1000));
    }

    if (ip_not_available)
    {
        if (at::commands::sim7000e::network_configuration::get_ip(ip) != at::commands::OK)
        {
            LOG_ERR("SIM7000E IP address retrieval failed");
        }
        else
        {
            LOG_INF("IP address: %s", ip.c_str());
        }
    }

    // power off the module
    if (at::commands::sim7000e::power::power_off(false) != at::commands::OK)
    {
        LOG_ERR("SIM7000E power off failed");
        return -1;
    }

    LOG_INF("SIM7000E powered off");
    gpio::set(gpio::SIM7000_PWR, gpio::LOW);

    // k_sleep(K_MSEC(10000));

    // LOG_INF("SIM7000E power on");
    // at::commands::result present_check_result = at::commands::sim7000e::https::check_sim7000e_present();
    // if (present_check_result != at::commands::TIMEOUT)
    // {
    //     LOG_INF("SIM7000E not responding");
    // }
    // else if (present_check_result == at::commands::OK)
    // {
    //     LOG_ERR("SIM7000E is responding");
    // }

    // // power on the module
    // if (gpio::set(gpio::SIM7000_PWR, gpio::LOW) != gpio::OK)
    // {
    //     return -1;
    // }

    // // while (at::commands::sim7000e::https::check_sim7000e_present() != at::commands::OK)
    // // {
    // //     LOG_INF("SIM7000E not responding");
    // //     k_sleep(K_MSEC(1000));
    // // }

    // if (at::commands::sim7000e::power::wait_for_boot() != at::commands::OK)
    // {
    //     LOG_ERR("SIM7000E wait for boot failed");
    //     return -1;
    // }

    // LOG_INF("SIM7000E powered on and ready");

    while (1)
    {
        k_sleep(K_MSEC(1000));
    }
    return 0;
}

int main(void)
{
    main_send_mb_iot_msg();
    // power off the module
    if (at::commands::sim7000e::power::power_off(false) != at::commands::OK)
    {
        LOG_ERR("SIM7000E power off failed");
        return -1;
    }

    gpio::set(gpio::SIM7000_PWR, gpio::LOW);

    while (1)
    {
        uart::read_result uart_res;
        std::string response = "";
        uart_res = uart::uart_read(response);
        if (uart_res == uart::read_result::UART_READ_OK)
        {
            LOG_INF("Response: %s", response.c_str());
        }
        if (uart_res == uart::read_result::UART_READ_ERROR)
        {
            LOG_ERR("UART read error");
        }
    }

    return 0;
}