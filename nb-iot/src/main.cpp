#include <zephyr/kernel.h>

#include <fcntl.h>
#include <unistd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>

#include <stdio.h>

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

int main(void)
{
    
}
