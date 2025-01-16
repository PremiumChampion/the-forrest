#ifndef AT_POWER_HPP
#define AT_POWER_HPP

#include "at/common.hpp"
#include "at/sim7000e/common.hpp" // at::commands::sim7000e::_at
#include "gpio/gpio.hpp"

namespace at::commands::sim7000e::power
{
    result wake_up();
    result wait_for_boot();

    enum psm_event_report_mode
    {
        PSM_EVENT_REPORT_DISABLE = 0,
        PSM_EVENT_REPORT_ENABLE = 1
    };
    result set_psm_event_report(psm_event_report_mode mode);

    static const int timing_unit[] = {600, 3600, 36000, 2, 30, 60, (int)1.152e6};

    struct timer_configuration
    {
        int mode;
        int Requested_periodic_RAU;     // not needed to configure
        int Requested_GPRS_ready_timer; // not needed to configure
        uint8_t Requested_periodic_TAU; // T3412_ext
        uint8_t Requested_active_time;  // T3324
    };

    // result get_timer_configuration_by_network();
    result set_slow_clock(bool enable);
    result enable_PSM();
    result set_PSM_timers(uint8_t unit_TAU = 3,      // 1h
                          uint8_t value_TAU = 31,    // 24 units = 24 h
                          uint8_t unit_T3324 = 0,    // 2 sec
                          uint8_t value_T3324 = 10); // 5 units = 10 sec
    result enter_idle_mode();
    result wait_for_enter_psm(k_timeout_t timeout = K_FOREVER);
    result wait_for_exit_psm(k_timeout_t timeout = K_FOREVER);
    result wait_for_enter_exit_psm(bool &entered_psm, k_timeout_t timeout = K_FOREVER);
    result exit_PSM();
    result power_off(bool urgent);
    result set_baudrate(uart::baudrate baudrate);
    result reboot();
} // namespace at::commands::sim7000e::power

#endif // AT_POWER_HPP