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

    enum status_reporting_mode
    {
        STATUS_REPORTING_DISABLE = 0,
        STATUS_REPORTING_ENABLE = 1
    };

    result set_registration_status_reporting();

} // namespace at::commands::sim7000e::power


#endif // AT_POWER_HPP