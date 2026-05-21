#include "init.hpp"
#include "ac_clock_probe_test.hpp"
#include "quartz_test.hpp"
#include "serial.hpp"
#include "timebase.hpp"
#include "ccl_seq_dff_test.hpp"

int main(void)
{
    sys_init();
    Timebase::init();
    Serial.init(1000000);

    QuartzTest::init();
    QuartzTest::report_once();

    CclSeqDffTest::run();
    //AcClockProbeTest::run();

    for (;;)
        __WFI();
}
