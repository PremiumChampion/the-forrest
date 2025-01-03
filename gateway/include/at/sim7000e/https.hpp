#ifndef AT_COMMANDS_HPP
#define AT_COMMANDS_HPP

#include <string> // std::string
#include <tuple> // std::tuple
#include <vector> // std::vector

#include "at/common.hpp" // result
#include "at/sim7000e/common.hpp" // at::commands::sim7000e::_at

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

    result ignore_ssl_timestamp();
    result set_ssl_version();
    result set_server_name_indication(std::string server_name);
    result trust_all_certificates();
    result set_body_length(int length);
    result set_header_length(int length);
    result set_domain(std::string url);
    result start_ssl_session();
    result clear_header();
    result start_header();
    result set_header(std::string header, std::string value);
    result set_body(std::string escaped_body);
    result exec(std::string url, http_method method, int &http_status_code, int &length);
    result read(std::string &response, int length, int timeout_ms = 10000);
    result stop_ssl_session();

    struct web_request
    {
        // domain for the request including https://
        // e.g. https://static.woyte.dev
        std::string domain;
        // server name indication domain excluing https://
        // e.g. static.woyte.dev
        std::string server_name;
        // method to use
        http_method method;
        // path to request excluding domain
        // e.g. /api/save_to_db
        std::string url;
        // headers to send
        // e.g. {{"Cache-control", "no-cache"}, {"Accept", "*/*"}}
        std::vector<std::tuple<std::string, std::string>> headers;
        // body to send
        std::string body;

        // the status code returned by the server
        int http_status_code;
        // the resoponse of the server
        std::string response;
    };

    result execute_web_request(web_request &request);

} // namespace at::commands::sim7000e::http

#endif // AT_COMMANDS_HPP