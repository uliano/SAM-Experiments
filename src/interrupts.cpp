#include "serial.hpp"
#include "timebase.hpp"
#include "single_channel_nodff.hpp"

extern "C" void irq_handler_sercom5(void)
{
  Serial.irq_handler();
}

extern "C" void irq_handler_sys_tick(void)
{
  Timebase::on_systick_isr();
}

extern "C" void irq_handler_tc0(void)
{
  SingleChannelNoDFF::on_window_isr();
}
