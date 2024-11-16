#ifndef AT_COMMANDS_HPP
#define AT_COMMANDS_HPP

#include <string> // std::string

#include "common.hpp" // result

namespace at::commands::sim7000e::http
{
    result check_sim7000e_present();
    result set_network_mode_nb_iot();
    result enable_scrambling();
    result get_operator(std::string &provider);
    result configure_bearer_profile();
    result enable_bearer_profile();
    result get_ip_address(std::string &ip_address);
    result start_http_session();
    result set_context_id_1();
    result allow_redirects();
    result set_url(std::string url);
    result set_content_type(std::string content_type);
    result add_body(std::string body);
    result exec_post(int &http_status_code);
    result stop_http_session();
    result stop_baeer_profile();
} // namespace at::commands::sim7000e::http

#endif // AT_COMMANDS_HPP