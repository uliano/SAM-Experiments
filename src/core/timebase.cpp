#include "timebase.hpp"

#include "samc21.h"

static_assert((F_CPU / 1000ul) > 0ul, "F_CPU must be >= 1000 Hz");
static_assert((F_CPU / 1000ul) <= 0x1000000ul, "SysTick reload exceeds 24-bit range");

void Timebase::init(void)
{
  ms_count_ = 0;
  initialized_ = false;

  // SysTick_Config expects ticks-per-interrupt (not reload value).
  if (0 == SysTick_Config((uint32_t)(F_CPU / 1000ul)))
    initialized_ = true;
}

uint32_t Timebase::millis(void)
{
  if (!initialized_)
    return 0;

  uint32_t state = lock_irq();
  uint32_t ms = ms_count_;
  unlock_irq(state);
  return ms;
}

uint32_t Timebase::micros(void)
{
  if (!initialized_)
    return 0;

  uint32_t state = lock_irq();
  uint32_t ms = ms_count_;
  uint32_t val = SysTick->VAL;
  uint32_t ctrl = SysTick->CTRL;
  uint32_t pending = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;

  // If the tick elapsed but ISR did not run yet, account for the next millisecond.
  if (((ctrl & SysTick_CTRL_COUNTFLAG_Msk) || pending) && (val > 0))
    ++ms;

  unlock_irq(state);

  uint32_t ticks_per_ms = systick_ticks_per_ms();
  uint32_t elapsed_ticks = ticks_per_ms - val;
  uint32_t us = (elapsed_ticks * 1000ul) / ticks_per_ms;

  return (ms * 1000ul) + us;
}

void Timebase::on_systick_isr(void)
{
  if (initialized_)
    ms_count_ = ms_count_ + 1u;
}

uint32_t Timebase::systick_ticks_per_ms(void)
{
  return (SysTick->LOAD & SysTick_LOAD_RELOAD_Msk) + 1u;
}

uint32_t Timebase::lock_irq(void)
{
  uint32_t state = __get_PRIMASK();
  __disable_irq();
  return state;
}

void Timebase::unlock_irq(uint32_t state)
{
  if (0 == (state & 1u))
    __enable_irq();
}
