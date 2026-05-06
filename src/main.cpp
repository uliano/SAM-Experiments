#include "init.hpp"
#include "quartz_test.hpp"
#include "serial.hpp"
#include "timebase.hpp"
#include "ccl_wo_test.hpp"

int main(void)
{
    sys_init();
    Timebase::init();
    Serial.init(1000000);

    QuartzTest::init();
    QuartzTest::report_once();

    CclWoTest::run();

    for (;;)
        __WFI();
}
