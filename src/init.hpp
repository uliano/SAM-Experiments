#ifndef _INIT_HPP_
#define _INIT_HPP_

#include "samc21.h"

// Minimal system clock initialization for board bring-up.
// Main clock: OSC48M @ 48 MHz on GCLK0.
// External crystals are started and reported by QuartzTest with timeouts, so
// boot never hangs on a board with a missing or faulty crystal.
inline void sys_init(void)
{
  // 2 wait states required at 48 MHz.
  NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_RWS(2) | NVMCTRL_CTRLB_MANW;

  // OSC48M: remove reset prescaler /2, run at full 48 MHz.
  OSCCTRL->OSC48MDIV.reg = OSCCTRL_OSC48MDIV_DIV(0);
  while (OSCCTRL->OSC48MSYNCBUSY.reg);

  GCLK->GENCTRL[0].reg =
    GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_OSC48M_Val) |
    GCLK_GENCTRL_GENEN;
  while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL_GCLK0);
}

#endif // _INIT_HPP_
