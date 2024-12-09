/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

#include "gpio/adc.hpp"

int32_t voltage = 0;

LOG_MODULE_REGISTER(main);

int main(void)
{
    gpio::adc::setup();
    while (1)
    {
        gpio::adc::read_channel(0, voltage);
        LOG_INF("ADC reading [%u]: %u  ", 4, voltage);
        gpio::adc::read_channel(1, voltage);
        LOG_INF("ADC reading [%u]: %u", 5, voltage);

        k_sleep(K_MSEC(SLEEP_TIME_MS));
    }
    return 0;
}
