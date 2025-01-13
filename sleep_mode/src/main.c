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

typedef enum {
    MODE_IDLE,
    MODE_SLEEP
} system_mode_t;

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
            return;
        }
        current_mode = MODE_SLEEP;
    } else {
        printf("Switching to Idle Mode\n");
        rc = pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
        if (rc < 0) {
            printf("Could not resume console (%d)\n", rc);
            return;
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

    /* Configure sw0 as input and set up interrupt */
    rc = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
    if (rc < 0) {
        printf("Could not configure sw0 GPIO (%d)\n", rc);
        return 0;
    }

    rc = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
    if (rc < 0) {
        printf("Could not configure sw0 GPIO interrupt (%d)\n", rc);
        return 0;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(sw0.pin));
    gpio_add_callback(sw0.port, &button_cb_data);

    printf("Press sw0 to switch modes. Press and hold to power off.\n");

    /* Main loop */
    while (1) {
        if (current_mode == MODE_IDLE) {
            printf("System in IDLE mode...\n");
        } else {
            printf("System in SLEEP mode...\n");
        }
        k_sleep(K_SECONDS(1));

        /* Simulate a condition to power off, e.g., holding the button */
        if (/* Condition to power off */ false) {
            printf("Powering off the system...\n");

            /* Ensure peripherals are suspended before power off */
            rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
            if (rc < 0) {
                printf("Failed to suspend console before power off (%d)\n", rc);
            }

            /* Power off the system */
            sys_poweroff();
        }
    }

    return 0;
}
