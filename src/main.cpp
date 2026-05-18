#include "init.hpp"
#include "quartz_test.hpp"
#include "serial.hpp"
#include "timebase.hpp"
#include "tcc1_countev_test.hpp"

int main(void)
{
    sys_init();
    Timebase::init();
    Serial.init(1000000);

    QuartzTest::init();
    QuartzTest::report_once();

    Tcc1CountevTest::run();

    for (;;)
        __WFI();
}
