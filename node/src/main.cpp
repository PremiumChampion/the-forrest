#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <algorithm>
#include <stdio.h>
#include "uart/uart0.hpp"
#include "at/prv.hpp"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include "gpio/adc.hpp"
#include <zephyr/sys/poweroff.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/rtc/maxim_ds3231.h>
#include <zephyr/drivers/counter.h>

LOG_MODULE_REGISTER(main);
// syspoweroff test
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static const struct gpio_dt_spec wakeup_pin = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback wakeup_cb_data;

static volatile bool wakeup_flag = false;

// I2C Clock test
const struct device *const rtc = DEVICE_DT_GET(DT_NODELABEL(rtc_ds3231));

// Lora startup init pin do not touch
static const struct gpio_dt_spec reset_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(lorareset), gpios, {0});

std::string response;

static void wakeup_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    wakeup_flag = true;
    LOG_INF("Wake-up signal received");
}

void join_network(int network_id, int dev_address)
{
    LOG_INF("Setting address");
    at::commands::prv::_at("AT+ADDRESS=" + std::to_string(dev_address) + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting network id");
    at::commands::prv::_at("AT+NETWORKID=" + std::to_string(network_id) + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

void send_message(int dev_address, std::string message)
{
    at::commands::prv::_at("AT+SEND=" + std::to_string(dev_address) + "," + std::to_string(message.length()) + "," + message + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

int init_lora_module()
{
    if (!gpio_is_ready_dt(&reset_pin))
    {
        return -1;
    }

    gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT);

    // Keep pin low for 4 seconds to start module correctly according to datasheet
    k_sleep(K_MSEC(4000));

    // Turn on reset pin for module startup
    gpio_pin_set_dt(&reset_pin, 1);
    k_sleep(K_MSEC(1000));
    uart::uart0::change_baudrate(uart::Baud115200);
    at::commands::result result;

    do
    {
        result = at::commands::prv::at();
        LOG_INF("Waiting for module to respond...");

        switch (result)
        {
        case at::commands::result::OK:
            LOG_INF("SUCCESS");
            break;
        case at::commands::result::TIMEOUT:
            LOG_INF("TIMEOUT");
            break;
        default:
            LOG_INF("UNKNOWN");
            break;
        }

        k_sleep(K_MSEC(1000));

    } while (result != at::commands::result::OK);

    LOG_INF("Setting parameters");

    result = at::commands::prv::_at("AT+PARAMETER=9,7,1,12\r\n", response);

    switch (result)
    {
    case at::commands::result::OK:
        LOG_INF("SUCCESS");
        break;
    case at::commands::result::TIMEOUT:
        LOG_INF("TIMEOUT");
        break;
    default:
        LOG_INF("UNKNOWN");
        break;
    }
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting RF power");
    at::commands::prv::_at("AT+CRFOP=22\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting frequency band");
    at::commands::prv::_at("AT+BAND=433000000\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    return 0;
}

// Needs to be called and set again after each receive
void set_smart_transceiver_mode()
{
    // rx time,sleep_time
    at::commands::prv::_at("AT+MODE=2,100,10000\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

void lora_module_sleep()
{
    at::commands::prv::_at("AT+MODE=1\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

void lora_module_wake()
{
    at::commands::prv::_at("AT+MODE=0\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";
}

// RTC Test

/* Format times as: YYYY-MM-DD HH:MM:SS DOW DOY */
static const char *format_time(time_t time,
                               long nsec)
{
    static char buf[64];
    char *bp = buf;
    char *const bpe = bp + sizeof(buf);
    struct tm tv;
    struct tm *tp = gmtime_r(&time, &tv);

    bp += strftime(bp, bpe - bp, "%Y-%m-%d %H:%M:%S", tp);
    if (nsec >= 0)
    {
        bp += snprintf(bp, bpe - bp, ".%09lu", nsec);
    }
    bp += strftime(bp, bpe - bp, " %a %j", tp);
    return buf;
}

void enable_alarm_interrupt(const struct device *rtc)
{
    int ret;
    uint8_t ctrl_reg;

    // Read the current control register value
    ctrl_reg = 0;

    // Enable the Alarm1 interrupt (A1IE bit) and the INTCN bit
    ctrl_reg |= BIT(0); // A1IE is bit 0 in the control register (for second precision)
    ctrl_reg |= BIT(1); // A2IE is bit 1 in the control register
    ctrl_reg |= BIT(2); // INTCN is bit 2 in the control register

    // Write back the updated control register value
    ret = maxim_ds3231_ctrl_update(rtc, ctrl_reg, 0);
    if (ret < 0)
    {
        LOG_ERR("Failed to enable Alarm1 interrupt: %d", ret);
        return;
    }
    else
    {
        LOG_INF("Alarm interrupt enabled.");
    }

    /* Show the DS3231 ctrl and ctrl_stat register values */
    LOG_INF("DS3231 ctrl %02x ; ctrl_stat %02x\n",
            maxim_ds3231_ctrl_update(rtc, 0, 0),
            maxim_ds3231_stat_update(rtc, 0, 0));
}

void configure_ds3231_alarm(const struct device *rtc, int NODE_ADDRESS)
{
    struct maxim_ds3231_alarm min_alarm;
    struct maxim_ds3231_alarm reset_alarm;
    int rc = 0;
    uint32_t now = 0;

    rc = counter_get_value(rtc, &now);

    time_t raw_time = now;

    if (rc < 0)
    {

        LOG_ERR("Failed to get current time: %d\n", rc);
        return;
    }

    int offset = 0;

    switch (NODE_ADDRESS)
    {
    case 1:
        offset = 15;
        break;
    case 2:
        offset = 30;
        break;
    case 3:
        offset = 45;
        break;
    case 4:
        offset = 0;
        break;
    default:
        LOG_ERR("Invalid node address for alarm offset");
    }

    min_alarm.time = raw_time + (offset + 60 - (raw_time % 60));
    min_alarm.flags = 0 | MAXIM_DS3231_ALARM_FLAGS_IGNDA | MAXIM_DS3231_ALARM_FLAGS_IGNHR | MAXIM_DS3231_ALARM_FLAGS_IGNMN;

    reset_alarm.time = raw_time - 120;
    reset_alarm.flags = 0;

    // Configure ALARM1 for trigger at min_alarm
    rc = maxim_ds3231_set_alarm(rtc, 0, &min_alarm);
    LOG_INF("Set sec based alarm 1 min in the future flags %x at %u ~ %s: %d\n", min_alarm.flags,
            (uint32_t)min_alarm.time, format_time(min_alarm.time, -1), rc);
    // Turn off ALARM 2 to prevent bugs
    rc = maxim_ds3231_set_alarm(rtc, 1, &reset_alarm);
}

static int set_date_time(const struct device *rtc)
{
    uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(rtc);
    uint32_t syncclock = maxim_ds3231_read_syncclock(rtc);
    struct sys_notify notify;
    struct k_poll_signal ss;
    struct k_poll_event sevt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                                        K_POLL_MODE_NOTIFY_ONLY,
                                                        &ss);
    int rc = 0;

    struct maxim_ds3231_syncpoint sp = {
        .rtc = {
            .tv_sec = CURRENT_UNIX_TIMESTAMP,
            .tv_nsec = (uint64_t)NSEC_PER_SEC * syncclock / syncclock_Hz,
        },
        .syncclock = syncclock,
    };

    uint32_t t0 = k_uptime_get_32();

    int rc_set = 0;
    rc_set = maxim_ds3231_set(rtc, &sp, &notify);

    printk("Set %s at %u ms past: %d\n", format_time(sp.rtc.tv_sec, sp.rtc.tv_nsec),
            syncclock, rc);

    /* Wait for the set to complete */
    rc = k_poll(&sevt, 1, K_FOREVER);

    if (rc_set < 0)
    {
        LOG_ERR("Failed to set time: %d\n", rc_set);
        return rc_set;
    }

    uint32_t t1 = k_uptime_get_32();
    /* Delay so log messages from sync can complete */
	k_sleep(K_MSEC(100));
	printk("Synchronize final: %d %d in %u ms\n", rc, ss.result, t1 - t0);

	rc = maxim_ds3231_get_syncpoint(rtc, &sp);
	printk("wrote sync %d: %u %u at %u\n", rc,
	       (uint32_t)sp.rtc.tv_sec, (uint32_t)sp.rtc.tv_nsec,
	       sp.syncclock);
    return 0;
}

static void show_counter(const struct device *ds3231)
{
    uint32_t now = 0;

    LOG_INF("Counter at %p\n", ds3231);
    LOG_INF("\tMax top value: %u (%08x)\n",
            counter_get_max_top_value(ds3231),
            counter_get_max_top_value(ds3231));
    LOG_INF("\t%u channels\n", counter_get_num_of_channels(ds3231));
    LOG_INF("\t%u Hz\n", counter_get_frequency(ds3231));

    LOG_INF("Top counter value: %u (%08x)\n",
            counter_get_top_value(ds3231),
            counter_get_top_value(ds3231));

    (void)counter_get_value(ds3231, &now);

    LOG_INF("True counter value %d", now);
    LOG_INF("Now %u: %s\n", now, format_time(now, -1));
}

// RTC

bool GATEWAY = false;
int GATEWAY_ADDRESS = 420;
int NODE_ADDRESS = 2;

#define DEVICE_ADDR 0x68
#define REG_ADDR 0x00

const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

int main(void)
{

    if (uart::uart0::uart_init() != 0)
    {
        LOG_INF("UART initialization failed\n");
        return -1;
    }

    if (gpio::adc::setup() != 0)
    {
        LOG_INF("ADC initialization failed\n");
        return -1;
    }

    if (init_lora_module() != 0)
    {
        LOG_INF("Failed to initialize LoRa module\n");
        return -1;
    }

    int rc;
    // Power off
    if (!device_is_ready(cons))
    {
        printf("%s: device not ready.\n", cons->name);
        return 0;
    }

    /* Configure sw0 as input and set up interrupt */
    rc = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
    if (rc < 0)
    {
        printf("Could not configure sw0 GPIO (%d)\n", rc);
        return 0;
    }

    rc = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
    if (rc < 0)
    {
        printf("Could not configure sw0 GPIO interrupt (%d)\n", rc);
        return 0;
    }

    gpio_init_callback(&wakeup_cb_data, wakeup_callback, BIT(wakeup_pin.pin));
    gpio_add_callback(wakeup_pin.port, &wakeup_cb_data);

// RTC time flashing based on kconfig
#ifdef CONFIG_FLASH_CURRENT_TIMESTAMP
    set_date_time(rtc);
    k_sleep(K_MSEC(8000));
#endif

    k_sleep(K_MSEC(1000));

    show_counter(rtc);

    // Join the network depending on the device type

    if (GATEWAY)
    {
        LOG_INF("Joining network as gateway");
        join_network(69, GATEWAY_ADDRESS);
    }
    else
    {
        LOG_INF("Joining network as node");
        join_network(69, NODE_ADDRESS);
    }

    // Enable interrupt output on SQW pin
    enable_alarm_interrupt(rtc);

    configure_ds3231_alarm(rtc, NODE_ADDRESS);

    while (1)
    {

        if (GATEWAY)
        {
            // receive data
            if (uart::uart0::uart_read(response, K_FOREVER) == uart::UART_READ_OK)
            {
                LOG_INF("%s", uart::escape_response(response).c_str());
                // if (response.find("RCV") != std::string::npos)
                // {
                //     lora_module_sleep();
                //     k_sleep(K_MSEC(8000));
                //     lora_module_wake();
                // }
                response = "";
            }
            else
            {
                LOG_INF("Error reading uart");
            }
        }
        else
        {
            // send data read from ADC sensor
            int32_t voltage0 = 0;
            int32_t voltage1 = 0;
            gpio::adc::read_channel(0, voltage0);
            LOG_INF("ADC reading [%u]: %u  ", 4, voltage0);
            gpio::adc::read_channel(1, voltage1);
            LOG_INF("ADC reading [%u]: %u", 5, voltage1);

            send_message(GATEWAY_ADDRESS, "\"" + std::to_string(NODE_ADDRESS) + "," + std::to_string(voltage0) + "," + std::to_string(voltage1) + "\"");
            k_sleep(K_MSEC(10000));

            rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
            if (rc < 0)
            {
                printf("Could not suspend console (%d)\n", rc);
                return 0;
            }
            wakeup_flag = false;
            
            while (!wakeup_flag)
            {
                sys_poweroff();
            }

            

            pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
        }
    }
}