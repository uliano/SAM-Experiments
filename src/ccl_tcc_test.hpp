#ifndef _CCL_TCC_TEST_HPP_
#define _CCL_TCC_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"

// Exhaustive CCL INSEL=TCC mapping.
//
// For every (TCC, WO channel, LUT, INSEL slot) combination:
//   - One TCC WO active at 50% (CC = PER/2); other WOs stuck HIGH (CC > PER).
//   - One LUT configured: INSELslot=TCC(0x8), other 2 slots=MASK, LUTEO=1.
//   - Other 3 LUTs: all INSEL=MASK, TRUTH=0 (output always 0).
//   - LUTn event → EVSYS ch.n async → TCn EVACT=COUNT, 0.5 s window.
//
// TCC0: 4 WO; TCC1/2: 2 WO each. Total: 8 WO × 4 LUT × 3 INSEL = 96 tests.
// Runtime: 96 × 0.5 s ≈ 48 s.
//
// Output: one row per (TCC, WO, LUT) with IN0/IN1/IN2 counts side by side.
// ~5 counts = connected; 0 = not connected; 1 = startup edge only.

namespace CclTccTest {

static inline void delay_ms(uint32_t ms)
{
  uint32_t start = Timebase::millis();
  while ((Timebase::millis() - start) < ms)
    ;
}

// ── TCC ──────────────────────────────────────────────────────────────────────

static inline void reset_tcc(Tcc* tcc)
{
  tcc->CTRLA.reg = TCC_CTRLA_SWRST;
  while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
    ;
}

// NPWM, DIV1024 @ 48 MHz → PER=4686 (~10 Hz period).
// WO[active_wo]: CC=2343 (50% duty). All others: CC=4687 (>PER → stuck HIGH).
static inline void setup_tcc_one_wo(Tcc* tcc, int n_cc, int active_wo)
{
  static const uint32_t cc_sync[4] = {
    TCC_SYNCBUSY_CC0, TCC_SYNCBUSY_CC1,
    TCC_SYNCBUSY_CC2, TCC_SYNCBUSY_CC3
  };
  reset_tcc(tcc);
  tcc->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1024;
  tcc->WAVE.reg  = TCC_WAVE_WAVEGEN_NPWM;
  while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE)
    ;
  tcc->PER.reg = 4686u;
  while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_PER)
    ;
  for (int i = 0; i < n_cc; i++) {
    tcc->CC[i].reg = (i == active_wo) ? 2343u : 4687u;
    while (tcc->SYNCBUSY.reg & cc_sync[i])
      ;
  }
  tcc->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
    ;
}

// ── TC event counters ─────────────────────────────────────────────────────────

static inline void arm_tc_counter(Tc* tc)
{
  tc->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tc->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
    ;
  tc->COUNT16.CTRLA.reg  = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  tc->COUNT16.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
  tc->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (tc->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
    ;
}

static inline uint16_t read_tc16(Tc* tc)
{
  tc->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
  while (tc->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
    ;
  return tc->COUNT16.COUNT.reg;
}

// ── EVSYS ─────────────────────────────────────────────────────────────────────

// TC EVACT=COUNT requires asynchronous path (resynchronized does NOT work).
static inline void setup_evsys()
{
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_EVSYS;
  for (int i = 0; i < 4; i++)
    EVSYS->CHANNEL[i].reg =
        EVSYS_CHANNEL_EVGEN(82u + (uint32_t)i) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
  EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(1);
  EVSYS->USER[EVSYS_ID_USER_TC1_EVU].reg = EVSYS_USER_CHANNEL(2);
  EVSYS->USER[EVSYS_ID_USER_TC2_EVU].reg = EVSYS_USER_CHANNEL(3);
  EVSYS->USER[EVSYS_ID_USER_TC3_EVU].reg = EVSYS_USER_CHANNEL(4);
}

// ── Self-test ─────────────────────────────────────────────────────────────────

// Three-step chain isolation test (run before main loop).
//
// A: TC0 free-run OVF → EVSYS(resync) → TC1 EVACT=COUNT
//    TC0 at DIV1/48MHz: OVF every ~1.37 ms → ~365 OVFs in 500 ms.
//    Tests whether TC EVACT=COUNT works at all.
//
// B: Same with EVSYS PATH_ASYNCHRONOUS — checks which path works.
//
// C: CCL FEEDBACK oscillator → EVSYS(resync) → TC1 EVACT=COUNT
//    LUT0 NOT-feedback loop → combinational toggler → expect count > 0 in 1 ms.
//    Tests CCL→EVSYS link.
static inline void selftest()
{
  Serial.print("Selftest");
  Serial.newline();

  // ── A: TC0(OVF) → EVSYS resync → TC1 COUNT ──────────────────────────
  // TC0: free-running COUNT16 DIV1, OVFEO=1. TC0+TC1 share GCLK (already enabled).
  TC0->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC0->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
    ;
  TC0->COUNT16.CTRLA.reg  = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  TC0->COUNT16.EVCTRL.reg = TC_EVCTRL_OVFEO;
  TC0->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC0->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
    ;

  EVSYS->CHANNEL[4].reg =
      EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TC0_OVF) |
      EVSYS_CHANNEL_PATH_RESYNCHRONIZED;
  EVSYS->USER[EVSYS_ID_USER_TC1_EVU].reg = EVSYS_USER_CHANNEL(5);  // ch4=id5

  TC1->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
    ;
  TC1->COUNT16.CTRLA.reg  = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  TC1->COUNT16.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
  TC1->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
    ;

  delay_ms(500);

  TC1->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
    ;
  uint16_t cnt_a = TC1->COUNT16.COUNT.reg;

  Serial.print("  A TC0_OVF->resync->TC1: count=");
  Serial.print((uint32_t)cnt_a);
  Serial.print(cnt_a >= 100 ? "  PASS" : "  FAIL");
  Serial.newline();

  // ── B: Same but PATH_ASYNCHRONOUS ────────────────────────────────────
  TC1->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
    ;
  EVSYS->CHANNEL[4].reg =
      EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TC0_OVF) |
      EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
  TC1->COUNT16.CTRLA.reg  = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  TC1->COUNT16.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
  TC1->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
    ;

  delay_ms(500);

  TC1->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
    ;
  uint16_t cnt_b = TC1->COUNT16.COUNT.reg;

  Serial.print("  B TC0_OVF->async->TC1:  count=");
  Serial.print((uint32_t)cnt_b);
  Serial.print(cnt_b >= 100 ? "  PASS" : "  FAIL");
  Serial.newline();

  TC0->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  TC1->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;

  // ── C: CCL FEEDBACK oscillator → EVSYS async → TC1 ─────────────────
  EVSYS->CHANNEL[4].reg =
      EVSYS_CHANNEL_EVGEN(82u) |              // LUTOUT0
      EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
  EVSYS->USER[EVSYS_ID_USER_TC1_EVU].reg = EVSYS_USER_CHANNEL(5);

  CCL->CTRL.reg = 0;
  CCL->LUTCTRL[0].reg = (0x01u << CCL_LUTCTRL_TRUTH_Pos)  // NOT IN[0]
                      | (0x1u  << CCL_LUTCTRL_INSEL0_Pos)  // FEEDBACK
                      | CCL_LUTCTRL_LUTEO;
  CCL->CTRL.reg = CCL_CTRL_ENABLE;
  CCL->LUTCTRL[0].reg |= CCL_LUTCTRL_ENABLE;

  TC1->COUNT16.CTRLA.reg  = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  TC1->COUNT16.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
  TC1->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
    ;

  delay_ms(1);  // oscillator: many MHz → count should be >> 0

  TC1->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
  while (TC1->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
    ;
  uint16_t cnt_c = TC1->COUNT16.COUNT.reg;

  Serial.print("  C CCL_osc->resync->TC1: count=");
  Serial.print((uint32_t)cnt_c);
  Serial.print(cnt_c > 0 ? "  PASS" : "  FAIL");
  Serial.newline();

  CCL->CTRL.reg = 0;
  TC1->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;

  // Restore TC1_EVU to CHANNEL(2) for the main test
  EVSYS->USER[EVSYS_ID_USER_TC1_EVU].reg = EVSYS_USER_CHANNEL(2);

  // ── D: TCC0 OVF → CHANNEL[0](ASYNC) → TC0 EVACT=COUNT ───────────────
  // Uses the exact channel+TC pair that the main test uses for LUT0.
  // If D passes: TC0+CH[0]+ASYNC works → issue is CCL LUTOUT not emitting.
  // If D fails:  TC0 or CHANNEL[0] broken with ASYNC.
  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;
  setup_tcc_one_wo(TCC0, 4, 0);  // ~10 Hz, ~5 OVFs per 500ms

  EVSYS->CHANNEL[0].reg =
      EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
      EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
  // TC0_EVU is already CHANNEL(1) from setup_evsys()

  arm_tc_counter(TC0);
  delay_ms(500);
  uint16_t cnt_d = read_tc16(TC0);

  Serial.print("  D TCC0_OVF->async->CH0->TC0: count=");
  Serial.print((uint32_t)cnt_d);
  Serial.print(cnt_d >= 3 ? "  PASS" : "  FAIL");
  Serial.newline();

  // ── E: TCC0 OVF → CHANNEL[4](ASYNC) → TC0 — isolates TC0 vs CHANNEL[0] ──
  // TCC0 still running. CHANNEL[4]=ASYNC already. Change TC0_EVU to CH(5).
  EVSYS->CHANNEL[4].reg =
      EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
      EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
  EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(5);  // ch4

  arm_tc_counter(TC0);
  delay_ms(500);
  uint16_t cnt_e = read_tc16(TC0);
  reset_tcc(TCC0);
  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = 0;

  // Restore for main test
  EVSYS->CHANNEL[0].reg = EVSYS_CHANNEL_EVGEN(82u) | EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
  EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(1);

  Serial.print("  E TCC0_OVF->async->CH4->TC0: count=");
  Serial.print((uint32_t)cnt_e);
  Serial.print(cnt_e >= 3 ? "  PASS" : "  FAIL");
  Serial.newline();
}

// ── CCL ───────────────────────────────────────────────────────────────────────

// TRUTH for "output = IN[slot]" with the other two masked:
//   slot 0 → 0x02, slot 1 → 0x04, slot 2 → 0x10
static constexpr uint8_t k_truth[3] = { 0x02u, 0x04u, 0x10u };

// Configure one test cell without touching CCL->CTRL (keeps EVSYS generators alive).
// Disables each LUT individually (enable-protected fields), rewrites, re-enables.
//   LUT[active_lut]: INSEL[insel_slot]=TCC(0x8), others=MASK, TRUTH=k_truth[slot], LUTEO=1.
//   Other LUTs: all INSEL=MASK, TRUTH=0, LUTEO=1 (output always 0).
static inline void configure_ccl(int active_lut, int insel_slot)
{
  for (int lut = 0; lut < 4; lut++) {
    CCL->LUTCTRL[lut].reg &= ~CCL_LUTCTRL_ENABLE;  // disable before modifying

    uint32_t lutctrl;
    if (lut == active_lut) {
      uint32_t i0 = (insel_slot == 0) ? 0x8u : 0x0u;
      uint32_t i1 = (insel_slot == 1) ? 0x8u : 0x0u;
      uint32_t i2 = (insel_slot == 2) ? 0x8u : 0x0u;
      lutctrl =
          ((uint32_t)k_truth[insel_slot] << CCL_LUTCTRL_TRUTH_Pos) |
          (i2 << CCL_LUTCTRL_INSEL2_Pos) |
          (i1 << CCL_LUTCTRL_INSEL1_Pos) |
          (i0 << CCL_LUTCTRL_INSEL0_Pos) |
          CCL_LUTCTRL_LUTEO;
    } else {
      lutctrl = CCL_LUTCTRL_LUTEO;  // TRUTH=0, all INSEL=MASK → output always 0
    }
    CCL->LUTCTRL[lut].reg = lutctrl;
    CCL->LUTCTRL[lut].reg |= CCL_LUTCTRL_ENABLE;
  }
}

// ── Entry point ───────────────────────────────────────────────────────────────

static inline void run(void)
{
  Serial.print("\r\nCCL INSEL=TCC exhaustive mapping (96 combinations)");
  Serial.newline();
  Serial.print("Format: TCC? WO[?] LUT?: IN0=? IN1=? IN2=?");
  Serial.newline();
  Serial.print("~5 = connected | 0-1 = not connected");
  Serial.newline();

  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0 | MCLK_APBCMASK_TCC1 | MCLK_APBCMASK_TCC2 |
                         MCLK_APBCMASK_TC0  | MCLK_APBCMASK_TC1  |
                         MCLK_APBCMASK_TC2  | MCLK_APBCMASK_TC3  |
                         MCLK_APBCMASK_CCL;

  GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;
  GCLK->PCHCTRL[TC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[TC2_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;

  setup_evsys();

  // CCL GCLK — required for LUTEO with resynchronized EVSYS path
  GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
    ;

  Tc* const tc_list[4] = { TC0, TC1, TC2, TC3 };

  selftest();

  // One-time CCL init. CCL->CTRL stays enabled for the whole experiment so
  // EVSYS generators remain live between configure_ccl() calls.
  CCL->CTRL.reg = 0;
  for (int i = 0; i < 4; i++)
    CCL->LUTCTRL[i].reg = 0;
  CCL->CTRL.reg = CCL_CTRL_ENABLE;

  struct TccInfo { Tcc* tcc; uint32_t gclk_id; const char* name; int n_cc; };
  const TccInfo info[3] = {
    { TCC0, TCC0_GCLK_ID, "TCC0", 4 },
    { TCC1, TCC1_GCLK_ID, "TCC1", 2 },
    { TCC2, TCC2_GCLK_ID, "TCC2", 2 },
  };

  for (int t = 0; t < 3; t++) {
    Serial.print("\r\n=== ");
    Serial.print(info[t].name);
    Serial.print(" ===");
    Serial.newline();

    GCLK->PCHCTRL[info[t].gclk_id].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[info[t].gclk_id].reg & GCLK_PCHCTRL_CHEN))
      ;

    for (int wo = 0; wo < info[t].n_cc; wo++) {
      setup_tcc_one_wo(info[t].tcc, info[t].n_cc, wo);

      for (int lut = 0; lut < 4; lut++) {
        uint16_t counts[3];

        for (int slot = 0; slot < 3; slot++) {
          configure_ccl(lut, slot);
          arm_tc_counter(tc_list[lut]);
          delay_ms(500);
          counts[slot] = read_tc16(tc_list[lut]);
        }

        // One compact row per (TCC, WO, LUT)
        Serial.print("  WO[");
        Serial.print((uint32_t)wo);
        Serial.print("] LUT");
        Serial.print((uint32_t)lut);
        Serial.print(": IN0=");
        Serial.print((uint32_t)counts[0]);
        Serial.print(" IN1=");
        Serial.print((uint32_t)counts[1]);
        Serial.print(" IN2=");
        Serial.print((uint32_t)counts[2]);

        bool any = counts[0] > 1 || counts[1] > 1 || counts[2] > 1;
        if (any) Serial.print("  <--");
        Serial.newline();
      }

      reset_tcc(info[t].tcc);
    }

    GCLK->PCHCTRL[info[t].gclk_id].reg = 0;
  }

  // Tear down
  CCL->CTRL.reg = 0;
  for (int i = 0; i < 4; i++)
    tc_list[i]->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  MCLK->APBCMASK.reg &= ~(MCLK_APBCMASK_TCC0 | MCLK_APBCMASK_TCC1 | MCLK_APBCMASK_TCC2 |
                           MCLK_APBCMASK_TC0  | MCLK_APBCMASK_TC1  |
                           MCLK_APBCMASK_TC2  | MCLK_APBCMASK_TC3);

  Serial.print("\r\nExperiment complete");
  Serial.newline();
}

} // namespace CclTccTest

#endif // _CCL_TCC_TEST_HPP_
