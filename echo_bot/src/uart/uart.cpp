#include "uart/uart.hpp"

namespace uart
{
	
	std::string escape_response(std::string response)
	{
		return escape_response((char *)response.c_str());
	}

	std::string escape_response(char *response)
	{
		std::string escaped = "";
		for (std::size_t i = 0; response[i] != '\0'; i++)
		{
			switch (response[i])
			{
			case '\0':
				escaped += "\\0";
				break;
			case '\r':
				escaped += "\\r";
				break;
			case '\n':
				escaped += "\\n";
				break;
			default:
				escaped += response[i];
				break;
			}
		}
		return escaped;
	}
} // namespace uart
