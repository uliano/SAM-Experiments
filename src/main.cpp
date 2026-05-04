#include "init.hpp"
#include "quartz_test.hpp"
#include "serial.hpp"
#include "timebase.hpp"

int main(void)
{
    sys_init();
    Timebase::init();
    Serial.init(1000000);

    QuartzTest::init();
    QuartzTest::run_forever();
}
