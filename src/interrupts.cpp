#include "serial.hpp"
#include "timebase.hpp"

extern "C" void irq_handler_sercom5(void)
{
  Serial.irq_handler();
}

extern "C" void irq_handler_sys_tick(void)
{
  Timebase::on_systick_isr();
}
