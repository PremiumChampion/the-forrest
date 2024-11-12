#ifndef AT_COMMANDS_HPP
#define AT_COMMANDS_HPP

#include <string>   // std::string

namespace at::commands{

enum result
{
    OK,
    ERROR,
    TIMEOUT
};


result at();
result cnmp();
result nbsc();


} // namespace at::commands


#endif // AT_COMMANDS_HPP