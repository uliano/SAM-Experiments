#ifndef _CCL_SCOPE_TEST_HPP_
#define _CCL_SCOPE_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"

// Oscilloscope test for CCL INSEL=TCC.
//
// Probe PA06: TCC1 WO0 at 10 Hz, 50% duty (reference signal).
// Probe PA11: CCL LUT1 output (INSEL0=TCC 0x8, pass-through TRUTH=0x02).
//
// Expected: PA11 follows PA06 if INSEL=TCC routes TCC1→LUT1.
// Expected: PA11 flat if INSEL=TCC is not connected.

namespace CclScopeTest {

static inline void run(void)
{
  Serial.print("\r\nCCL scope test");
  Serial.newline();
  Serial.print("PA06 = TCC1 WO0 reference (100 Hz 50% exact)");
  Serial.newline();
  Serial.print("PA11 = CCL LUT1 OUT (INSEL0=TCC 0x8)");
  Serial.newline();
  Serial.print("Expect PA11 = PA06 if TCC1->LUT1 connected");
  Serial.newline();

  // ── Clocks ──────────────────────────────────────────────────────────────────
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC1 | MCLK_APBCMASK_CCL;

  GCLK->PCHCTRL[TCC1_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[TCC1_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;
  GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;

  // ── TCC1: NPWM DIV256 @48 MHz → PER=1874 (100 Hz exact), CC0=937 (50%) ─────
  TCC1->CTRLA.reg = TCC_CTRLA_SWRST;
  while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
    ;
  TCC1->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV256;
  TCC1->WAVE.reg  = TCC_WAVE_WAVEGEN_NPWM;
  while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE)
    ;
  TCC1->PER.reg = 1874u;
  while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_PER)
    ;
  TCC1->CC[0].reg = 937u;
  while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_CC0)
    ;
  TCC1->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
    ;

  // ── PA06: TCC1 WO0 output (PMUX E = 4, even pin → PMUXE) ───────────────────
  PORT->Group[0].DIRSET.reg   = (1u << 6);
  PORT->Group[0].PINCFG[6].reg = PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[3].bit.PMUXE = 4;  // PA06 even → PMUX[3].PMUXE, mux E

  // ── CCL LUT1: INSEL0=TCC(0x8), others MASK, TRUTH=0x02 (pass IN[0]) ─────────
  CCL->CTRL.reg = 0;
  CCL->LUTCTRL[1].reg =
      (0x02u << CCL_LUTCTRL_TRUTH_Pos)   |  // OUT = IN[0]
      (0x8u  << CCL_LUTCTRL_INSEL0_Pos);    // IN[0] = TCC (0x8)
  CCL->CTRL.reg = CCL_CTRL_ENABLE;
  CCL->LUTCTRL[1].reg |= CCL_LUTCTRL_ENABLE;

  // ── PA11: CCL LUT1 OUT (PMUX I = 8, odd pin → PMUXO) ───────────────────────
  PORT->Group[0].DIRSET.reg    = (1u << 11);
  PORT->Group[0].PINCFG[11].reg = PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[5].bit.PMUXO = 8;  // PA11 odd → PMUX[5].PMUXO, mux I

  Serial.print("Running — probe PA06 and PA11");
  Serial.newline();
}

} // namespace CclScopeTest

#endif // _CCL_SCOPE_TEST_HPP_
