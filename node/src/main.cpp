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
static const struct gpio_dt_spec reset_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mycusgpio), gpios, {0});

std::string response;

static void wakeup_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    wakeup_flag = true;
    LOG_INF("Wake-up signal received");
}

void join_network(int network_id, int dev_address)
{
    LOG_INF("Setting frequency band");
    at::commands::prv::_at("AT+BAND=433000000\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting address");
    at::commands::prv::_at("AT+ADDRESS=" + std::to_string(dev_address) + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Setting network id");
    at::commands::prv::_at("AT+NETWORKID=" + std::to_string(network_id) + "\r\n", response);
    LOG_INF("%s", uart::escape_response(response).c_str());
    response = "";

    LOG_INF("Joining network, sending HELLO");
    at::commands::prv::_at("AT+SEND=2,5,HELLO\r\n", response);
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

    //Keep pin low for 4 seconds to start module correctly according to datasheet
    k_sleep(K_MSEC(4000));

    // Turn on reset pin for module startup
    gpio_pin_set_dt(&reset_pin, 1);
    k_sleep(K_MSEC(1000));

    at::commands::result result;
    result = at::commands::prv::at();
    while (result != at::commands::result::OK)
    {
        LOG_INF("Waiting for module to respond");

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

        result = at::commands::prv::at();
        k_sleep(K_MSEC(1000));
    }

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
    LOG_INF("\nDS3231 ctrl %02x ; ctrl_stat %02x\n",
            maxim_ds3231_ctrl_update(rtc, 0, 0),
            maxim_ds3231_stat_update(rtc, 0, 0));
}

void configure_ds3231_alarm(const struct device *rtc)
{
    struct maxim_ds3231_alarm min_alarm;
    int rc = 0;
    uint32_t now = 0;

    struct rtc_time current_time;
    rc = counter_get_value(rtc, &now);

    time_t raw_time = now;
    struct tm *time_info = gmtime(&raw_time);

    current_time.tm_sec = time_info->tm_sec;
    current_time.tm_min = time_info->tm_min;
    current_time.tm_hour = time_info->tm_hour;
    current_time.tm_mday = time_info->tm_mday;
    current_time.tm_mon = time_info->tm_mon;
    current_time.tm_year = time_info->tm_year;

    if (rc < 0)
    {

        LOG_ERR("Failed to get current time: %d\n", rc);
        return;
    }

    // Set the alarm time to 1 minute in the future
    current_time.tm_min += 1;
    if (current_time.tm_min >= 60)
    {
        current_time.tm_min -= 60;
        current_time.tm_hour += 1;
        if (current_time.tm_hour >= 24)
        {
            current_time.tm_hour -= 24;
        }
    }

    struct tm tm_time;
    tm_time.tm_sec = current_time.tm_sec;
    tm_time.tm_min = current_time.tm_min;
    tm_time.tm_hour = current_time.tm_hour;
    tm_time.tm_mday = current_time.tm_mday;
    tm_time.tm_mon = current_time.tm_mon;
    tm_time.tm_year = current_time.tm_year;

    min_alarm.time = mktime(&tm_time);
    min_alarm.flags = 0 | MAXIM_DS3231_ALARM_FLAGS_IGNDA | MAXIM_DS3231_ALARM_FLAGS_IGNHR | MAXIM_DS3231_ALARM_FLAGS_IGNMN | MAXIM_DS3231_ALARM_FLAGS_IGNSE;

    // Configure ALARM2 for trigger at min_alarm
    rc = maxim_ds3231_set_alarm(rtc, 1, &min_alarm);
    LOG_INF("Set min alarm flags %x at %u ~ %s: %d\n", min_alarm.flags,
            (uint32_t)min_alarm.time, format_time(min_alarm.time, -1), rc);
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
    rc = maxim_ds3231_set(rtc, &sp, &notify);

    printk("\nSet %s at %u ms past: %d\n", format_time(sp.rtc.tv_sec, sp.rtc.tv_nsec),
           syncclock, rc);

    /* Wait for the set to complete */
    rc = k_poll(&sevt, 1, K_FOREVER);

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

    printk("\nCounter at %p\n", ds3231);
    printk("\tMax top value: %u (%08x)\n",
           counter_get_max_top_value(ds3231),
           counter_get_max_top_value(ds3231));
    printk("\t%u channels\n", counter_get_num_of_channels(ds3231));
    printk("\t%u Hz\n", counter_get_frequency(ds3231));

    printk("Top counter value: %u (%08x)\n",
           counter_get_top_value(ds3231),
           counter_get_top_value(ds3231));

    (void)counter_get_value(ds3231, &now);

    LOG_INF("True counter value %d", now);
    LOG_INF("Now %u: %s\n", now, format_time(now, -1));
}

// RTC Test End

bool GATEWAY = true;
int GATEWAY_ADDRESS = 420;
int NODE_ADDRESS = 1;

#define DEVICE_ADDR 0x68
#define REG_ADDR 0x00

const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

int main(void)
{

    if (uart::uart0::uart_init() != 0)
    {
        printk("UART initialization failed\n");
        return -1;
    }

    if (gpio::adc::setup() != 0)
    {
        printk("ADC initialization failed\n");
        return -1;
    }

    if (init_lora_module() != 0)
    {
        printk("Failed to initialize LoRa module\n");
        return -1;
    }

    int rc;
    // Power off test
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

// RTC
#ifdef CONFIG_FLASH_CURRENT_TIMESTAMP
    set_date_time(rtc);
#endif
    show_counter(rtc);

    // Enable interrupt output on SQW pin
    enable_alarm_interrupt(rtc);

    configure_ds3231_alarm(rtc);
    // set_date_time(rtc);

    /* Continuously read the current date and time from the RTC */

    k_sleep(K_MSEC(1000));

    // Join the network

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

    while (1)
    {
        // power off test working sys_poweroff();

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

            send_message(GATEWAY_ADDRESS, std::to_string(NODE_ADDRESS) + "," + std::to_string(voltage0) + "," + std::to_string(voltage1));
            k_sleep(K_MSEC(1000));

            rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
            if (rc < 0)
            {
                printf("Could not resume console (%d)\n", rc);
                return 0;
            }

            while (!wakeup_flag)
            {
                sys_poweroff();
            }

            wakeup_flag = false;

            pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
        }
    }
}