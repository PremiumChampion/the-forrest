#ifndef AT_POWER_HPP
#define AT_POWER_HPP

#include "at/common.hpp"
#include "at/prv.hpp"
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

    result fix_baud_rate();

    static const int timing_unit[] = {600, 3600, 36000, 2, 30, 60, (int)1.152e6};

    struct timer_configuration
    {
        int mode;
        int Requested_periodic_RAU; // not needed to configure
        int Requested_GPRS_ready_timer; // not needed to configure
        uint8_t Requested_periodic_TAU; // T3412_ext
        uint8_t Requested_active_time; // T3324
    };

    // result get_timer_configuration_by_network();

    result enable_PSM();
    result wait_for_enter_psm();
    result wait_for_exit_psm();
    result exit_PSM();

} // namespace at::commands::sim7000e::power


#endif // AT_POWER_HPP