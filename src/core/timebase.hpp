#ifndef _TIMEBASE_HPP_
#define _TIMEBASE_HPP_

#include <stdint.h>

class Timebase
{
public:
  static void init(void);
  static uint32_t millis(void);
  static uint32_t micros(void);
  static void on_systick_isr(void);

private:
  static uint32_t systick_ticks_per_ms(void);
  static uint32_t lock_irq(void);
  static void unlock_irq(uint32_t state);

  static inline volatile uint32_t ms_count_ = 0;
  static inline bool initialized_ = false;
};

#endif // _TIMEBASE_HPP_
