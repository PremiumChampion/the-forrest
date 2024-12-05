#ifndef AT_COMMANDS_HPP
#define AT_COMMANDS_HPP

#include <string> // std::string

#include "common.hpp" // result

namespace at::commands::sim7000e::https
{
    enum class http_method
    {
        GET = 1,
        PUT = 2,
        POST = 3,
        PATCH = 4,
        HEAD = 5
    };

    std::string escape_body(std::string body);
    result reboot();
    result check_sim7000e_present();
    result setup_apn(std::string apn);
    result get_ip(std::string &ip);
    result ignore_ssl_timestamp();
    result set_ssl_version();
    result set_server_name_indication(std::string server_name);
    result trust_all_certificates();
    result set_body_length(int length);
    result set_header_length(int length);
    result set_domain(std::string url);
    result set_time();
    result get_time();
    result start_ssl_session();
    result clear_header();
    result set_header(std::string header, std::string value);
    result set_body(std::string escaped_body);
    result exec(std::string url, http_method method, int &http_status_code, int &length);
    result read(std::string &response, int length, int timeout_ms = 10000);
    result stop_ssl_session();
    result network_disconnect();

} // namespace at::commands::sim7000e::http

#endif // AT_COMMANDS_HPP