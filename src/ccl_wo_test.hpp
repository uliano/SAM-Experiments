#ifndef _CCL_WO_TEST_HPP_
#define _CCL_WO_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"

// CCL INSEL=TCC: TCC2 -> LUT2, INSEL1 and INSEL2 verification.
//
// TCC2 is 16-bit, 2 CC channels:
//   PA12 = TCC2 WO[0] = 25%  (CC[0]=468,  mux E)
//   PA13 = TCC2 WO[1] = 75%  (CC[1]=1406, mux E)
//   WO[2] expected to mirror WO[0] (25%) — same CC[0] wraparound as TCC1.
//
// LUT2 OUT → PA25 (mux I=8).

namespace CclWoTest {

static constexpr int K_SLOT = 2;  // change to 2, rebuild and flash

static constexpr uint32_t K_INSEL0 = (K_SLOT == 0) ? 0x8u : 0x0u;
static constexpr uint32_t K_INSEL1 = (K_SLOT == 1) ? 0x8u : 0x0u;
static constexpr uint32_t K_INSEL2 = (K_SLOT == 2) ? 0x8u : 0x0u;
static constexpr uint32_t K_TRUTH  = (K_SLOT == 0) ? 0x02u :
                                      (K_SLOT == 1) ? 0x04u : 0x10u;

static inline void run(void)
{
  Serial.print("\r\nCCL WO slot test (TCC2 -> LUT2)");
  Serial.newline();
  Serial.print("PA12=WO0(25%) PA13=WO1(75%)");
  Serial.newline();
  Serial.print("PA25 = CCL LUT2 OUT");
  Serial.newline();
  if (K_SLOT == 0) Serial.print("Testing INSEL0 -> expect 25%");
  if (K_SLOT == 1) Serial.print("Testing INSEL1 -> expect 75%");
  if (K_SLOT == 2) Serial.print("Testing INSEL2 -> expect 25% (WO2=WO0, 2 CC only)");
  Serial.newline();

  // ── Clocks ──────────────────────────────────────────────────────────────────
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC2 | MCLK_APBCMASK_CCL;

  GCLK->PCHCTRL[TCC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[TCC2_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;
  GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;

  // ── TCC2: NPWM DIV256 @48 MHz → PER=1874 (100 Hz) — 16-bit, 2 CC channels ──
  TCC2->CTRLA.reg = TCC_CTRLA_SWRST;
  while (TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
    ;
  TCC2->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV256;
  TCC2->WAVE.reg  = TCC_WAVE_WAVEGEN_NPWM;
  while (TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE)
    ;
  TCC2->PER.reg = 1874u;
  while (TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_PER)
    ;
  TCC2->CC[0].reg = 468u;   // WO[0] = 25%
  while (TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_CC0)
    ;
  TCC2->CC[1].reg = 1406u;  // WO[1] = 75%
  while (TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_CC1)
    ;
  TCC2->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
    ;

  // ── PA12: TCC2 WO[0] = 25% (mux E=4, even → PMUX[6].PMUXE) ────────────────
  PORT->Group[0].DIRSET.reg     = (1u << 12);
  PORT->Group[0].PINCFG[12].reg = PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[6].bit.PMUXE = 4;

  // ── PA13: TCC2 WO[1] = 75% (mux E=4, odd → PMUX[6].PMUXO) ─────────────────
  PORT->Group[0].DIRSET.reg     = (1u << 13);
  PORT->Group[0].PINCFG[13].reg = PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[6].bit.PMUXO = 4;

  // ── CCL LUT2: one INSEL slot = TCC(0x8), others = MASK(0x0) ────────────────
  CCL->CTRL.reg = 0;
  CCL->LUTCTRL[2].reg =
      (K_TRUTH  << CCL_LUTCTRL_TRUTH_Pos)  |
      (K_INSEL2 << CCL_LUTCTRL_INSEL2_Pos) |
      (K_INSEL1 << CCL_LUTCTRL_INSEL1_Pos) |
      (K_INSEL0 << CCL_LUTCTRL_INSEL0_Pos);
  CCL->CTRL.reg = CCL_CTRL_ENABLE;
  CCL->LUTCTRL[2].reg |= CCL_LUTCTRL_ENABLE;

  // ── PA25: CCL LUT2 OUT (mux I=8, odd → PMUX[12].PMUXO) ────────────────────
  PORT->Group[0].DIRSET.reg     = (1u << 25);
  PORT->Group[0].PINCFG[25].reg = PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[12].bit.PMUXO = 8;

  Serial.print("Running — probe PA25 (LUT2 OUT)");
  Serial.newline();
}

} // namespace CclWoTest

#endif // _CCL_WO_TEST_HPP_
