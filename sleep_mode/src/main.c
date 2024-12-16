/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "retained.h"

#include <inttypes.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>

static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/* Enumeration for system states */
typedef enum {
    MODE_IDLE,
    MODE_SLEEP
} system_mode_t;

/* Current system mode */
static volatile system_mode_t current_mode = MODE_IDLE;

/* Button callback function */
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	int rc;
    if (current_mode == MODE_IDLE) {
        printf("Switching to Sleep Mode\n");
		rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
		if (rc < 0) {
			printf("Could not suspend console (%d)\n", rc);
			return 0;
		}
        current_mode = MODE_SLEEP;
    } else {
        printf("Switching to Idle Mode\n");
		rc = pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
		if (rc < 0) {
			printf("Could not resume console (%d)\n", rc);
			return 0;
		}
        current_mode = MODE_IDLE;
    }
}

static struct gpio_callback button_cb_data;

int main(void)
{
    int rc;

    if (!device_is_ready(cons)) {
        printf("%s: device not ready.\n", cons->name);
        return 0;
    }

    if (IS_ENABLED(CONFIG_APP_USE_NRF_RETENTION) || IS_ENABLED(CONFIG_APP_USE_RETAINED_MEM)) {
        bool retained_ok = retained_validate();

        /* Increment for this boot attempt and update. */
        retained.boots += 1;
        retained_update();

        printf("Retained data: %s\n", retained_ok ? "valid" : "INVALID");
        printf("Boot count: %u\n", retained.boots);
        printf("Off count: %u\n", retained.off_count);
        printf("Active Ticks: %" PRIu64 "\n", retained.uptime_sum);
    } else {
        printf("Retained data not supported\n");
    }

    /* Configure sw0 as input and set up interrupt */
    rc = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
    if (rc < 0) {
        printf("Could not configure sw0 GPIO (%d)\n", rc);
        return 0;
    }

    rc = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_RISING);
    if (rc < 0) {
        printf("Could not configure sw0 GPIO interrupt (%d)\n", rc);
        return 0;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(sw0.pin));
    gpio_add_callback(sw0.port, &button_cb_data);

	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
	if (rc < 0) {
		printf("Could not resume console (%d)\n", rc);
		return 0;
	}
    printf("Press sw0 to switch modes\n");

	printf("Do Stuff!\n");
	k_sleep(K_MSEC(1000));
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	if (rc < 0) {
		printf("Could not resume console (%d)\n", rc);
		return 0;
	}
	sys_poweroff();
	printf("Done Stuff!\n");
	return 0;
	/*
    while (1) {
		k_sleep(K_MSEC(5000));
        if (current_mode == MODE_SLEEP) {
            printf("Entering Sleep Mode\n");

            if (IS_ENABLED(CONFIG_APP_USE_NRF_RETENTION) || IS_ENABLED(CONFIG_APP_USE_RETAINED_MEM)) {
                retained.off_count += 1;
                retained_update();
            }

            sys_poweroff();
        } else {
            printf("In Idle Mode\n");
            k_sleep(K_MSEC(1000));
        }
    }
	*/
    return 0;
	
}
