enum uart_init_return_code {
    UART_INIT_SUCCESS = 0,
    UART_INIT_ERROR = -1,
    UART_INIT_DEVICE_NOT_READY = -2,
    UART_INIT_DEVICE_CONFIGURATION_ERROR = -3,
    UART_INIT_CALLBACK_ERROR = -4
};

uart_init_return_code uart_init(void);