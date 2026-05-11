#ifndef _AC_SYNC_TEST_HPP_
#define _AC_SYNC_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"

// AC synchronization experiment — COMP0 lag measurement, no phase-lock.
//
// TCC0: GCLK1=24 MHz XOSC, PER=63 → 375 kHz, free-running.
//   PA08 = WO[0] duty 1/8 (high 333 ns)  — scope ch1, trigger reference
//   PA09 = WO[1] duty 7/8 (high 2.33 µs) — scope ch2
//
// COMP0 (pair-0): PA05=AIN1 positive; negative = VDD scaler VALUE=K_SCALER_VALUE.
//   V_threshold = VDD × (K_SCALER_VALUE + 1) / 64   (VALUE=31 → VDD/2 = 2.5 V at 5 V)
//
// CCL LUT0: INSEL2=AC(COMP0), TRUTH=0x10 — pass-through of COMP0 state.
//   PA19 = LUT0 OUT (mux I=8) — scope ch3: COMP0 state
//
// GCLK_AC = GCLK2 = XOSC/64 = 375 kHz (still needed for CCL clock).
// AC OUT_ASYNC + FLEN=OFF: lag = comparator propagation delay only (~40-100 ns).
// Measure on scope: PA05 threshold crossing → PA19 edge.

namespace AcSyncTest {

// COMP0 negative input: VDD scaler VALUE.
// V_neg = VDD × (VALUE + 1) / 64   (31 → VDD/2 = 2.5 V at VDD=5 V)
static constexpr uint8_t K_SCALER_VALUE = 31;

static inline void delay_ms(uint32_t ms)
{
    uint32_t start = Timebase::millis();
    while ((Timebase::millis() - start) < ms)
        ;
}

static inline void run(void)
{
    Serial.print("\r\nAC lag test — COMP0 OUT_ASYNC, FLEN=OFF");
    Serial.newline();
    Serial.print("TCC0 GCLK1=24 MHz PER=63 -> 375 kHz free-running (period 2.667 us)");
    Serial.newline();
    Serial.print("  PA08 = WO[0] duty 1/8, high=333 ns  (scope ch1 trigger)");
    Serial.newline();
    Serial.print("  PA09 = WO[1] duty 7/8, high=2333 ns (scope ch2)");
    Serial.newline();
    Serial.print("COMP0: PA05=AIN1 input, threshold VDD*(");
    Serial.print((uint32_t)(K_SCALER_VALUE + 1u));
    Serial.print("/64) = ~");
    Serial.print(5000u * (uint32_t)(K_SCALER_VALUE + 1u) / 64u);
    Serial.print(" mV at VDD=5V");
    Serial.newline();
    Serial.print("CCL LUT0 OUT: PA19 (mux I) — COMP0 state (scope ch3)");
    Serial.newline();
    Serial.print("Expected: PA19 transitions within 1 GCLK2 period (2.67 us) of PA05 crossing");
    Serial.newline();

    // ── APB clocks ──────────────────────────────────────────────────────────
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0
                        | MCLK_APBCMASK_CCL | MCLK_APBCMASK_AC;

    // ── GCLK assignments ────────────────────────────────────────────────────
    // GCLK1 = 24 MHz XOSC (QuartzTest) → TCC0+TCC1 (PCHCTRL[28]).
    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;

    // GCLK2 = 375 kHz → AC (PCHCTRL[40]).
    // OUT_SYNC aligns AC state changes to GCLK2 edges = TCC0 COUNT≈0 after phase-lock.
    GCLK->PCHCTRL[AC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK2 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;

    // GCLK0 = 48 MHz → CCL.
    GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;

    // ── TCC0: NPWM, GCLK1=24 MHz, PER=63 → 375 kHz, free-running ───────────
    TCC0->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
        ;
    TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1;
    TCC0->WAVE.reg  = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE)
        ;
    TCC0->PER.reg = 63u;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_PER)
        ;
    TCC0->CC[0].reg = 8u;    // WO[0] HIGH while COUNT < 8 → duty 1/8
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC0)
        ;
    TCC0->CC[1].reg = 56u;   // WO[1] HIGH while COUNT < 56 → duty 7/8
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC1)
        ;
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;

    // ── PA08: TCC0 WO[0] (mux E=4, even → PMUX[4].PMUXE) ──────────────────
    PORT->Group[0].DIRSET.reg         = (1u << 8);
    PORT->Group[0].PINCFG[8].reg      = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[4].bit.PMUXE  = 4;

    // ── PA09: TCC0 WO[1] (mux E=4, odd → PMUX[4].PMUXO) ───────────────────
    PORT->Group[0].DIRSET.reg         = (1u << 9);
    PORT->Group[0].PINCFG[9].reg      = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[4].bit.PMUXO  = 4;

    // ── PA05: COMP0 analog input AIN1 (mux B=1, odd → PMUX[2].PMUXO) ───────
    PORT->Group[0].PINCFG[5].reg      = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[2].bit.PMUXO  = 1;   // mux B = analog

    // ── PA19: CCL LUT0 OUT (mux I=8, odd → PMUX[9].PMUXO) ──────────────────
    PORT->Group[0].DIRSET.reg         = (1u << 19);
    PORT->Group[0].PINCFG[19].reg     = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[9].bit.PMUXO  = 8;

    // ── Self-test: LUT0 forced HIGH (TRUTH=0xFF) for 500 ms ─────────────────
    // PA19 must go HIGH. If it stays LOW: wrong LUT0 output pin.
    Serial.print("Self-test: forcing LUT0=HIGH for 500 ms, PA19 should be HIGH");
    Serial.newline();
    Serial.flush();

    CCL->CTRL.reg = 0;
    CCL->LUTCTRL[0].reg = (0xFFu << CCL_LUTCTRL_TRUTH_Pos);  // always 1, all INSEL=MASK
    CCL->CTRL.reg = CCL_CTRL_ENABLE;
    CCL->LUTCTRL[0].reg |= CCL_LUTCTRL_ENABLE;

    delay_ms(500);

    // Brief LOW phase to show the transition on scope.
    CCL->LUTCTRL[0].reg &= ~CCL_LUTCTRL_ENABLE;
    CCL->CTRL.reg = 0;
    CCL->LUTCTRL[0].reg = (0x00u << CCL_LUTCTRL_TRUTH_Pos);
    CCL->CTRL.reg = CCL_CTRL_ENABLE;
    CCL->LUTCTRL[0].reg |= CCL_LUTCTRL_ENABLE;

    delay_ms(100);

    Serial.print("Self-test done — PA19 was HIGH? If yes: pin mux OK.");
    Serial.newline();

    // ── AC ───────────────────────────────────────────────────────────────────
    AC->CTRLA.reg = AC_CTRLA_SWRST;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_SWRST)
        ;
    AC->SCALER[0].reg = K_SCALER_VALUE;
    AC->COMPCTRL[0].reg =
        AC_COMPCTRL_MUXPOS_PIN1     // PA05 = pair-0 AIN1
        | AC_COMPCTRL_MUXNEG_VSCALE // VDD scaler (K_SCALER_VALUE)
        | AC_COMPCTRL_SPEED_HIGH
        | AC_COMPCTRL_OUT_ASYNC;    // async output: propagation delay only (~40-100 ns)
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0)
        ;
    AC->CTRLA.reg |= AC_CTRLA_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_ENABLE)
        ;
    AC->COMPCTRL[0].reg |= AC_COMPCTRL_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0)
        ;
    while (!(AC->STATUSB.reg & AC_STATUSB_READY0))
        ;

    // ── CCL LUT0: COMP0 pass-through ─────────────────────────────────────────
    // INSEL2=AC(0x5) → COMP0 state; TRUTH=0x10 → OUT = IN[2].
    CCL->LUTCTRL[0].reg &= ~CCL_LUTCTRL_ENABLE;
    CCL->CTRL.reg = 0;
    CCL->LUTCTRL[0].reg =
        (0x10u << CCL_LUTCTRL_TRUTH_Pos)  |
        (0x5u  << CCL_LUTCTRL_INSEL2_Pos) |   // COMP0
        (0x0u  << CCL_LUTCTRL_INSEL1_Pos) |
        (0x0u  << CCL_LUTCTRL_INSEL0_Pos);
    CCL->CTRL.reg = CCL_CTRL_ENABLE;
    CCL->LUTCTRL[0].reg |= CCL_LUTCTRL_ENABLE;

    bool state0 = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;
    Serial.print("COMP0 initial: ");
    Serial.print(state0 ? "HIGH (PA05 > threshold)" : "LOW  (PA05 < threshold)");
    Serial.newline();
    Serial.print("Scope: trigger PA08 rising, observe PA19 vs PA05 crossing.");
    Serial.newline();
    Serial.print("Expected AC lag: ~40-100 ns (propagation delay only, no GCLK sync).");
    Serial.newline();
    Serial.print("--- polling COMP0 ---");
    Serial.newline();
    Serial.flush();

    // ── Diagnostic loop ──────────────────────────────────────────────────────
    bool     last_state = state0;
    uint32_t toggles    = 0;
    uint32_t last_tick  = Timebase::millis();

    for (;;) {
        bool state = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;

        if (state != last_state) {
            last_state = state;
            ++toggles;
            Serial.print("COMP0=");
            Serial.print(state ? "1" : "0");
            Serial.print("  toggles=");
            Serial.print(toggles);
            Serial.newline();
        }

        uint32_t now = Timebase::millis();
        if (now - last_tick >= 1000u) {
            last_tick = now;
            Serial.print("tick COMP0=");
            Serial.print(state ? "1" : "0");
            Serial.print("  t=");
            Serial.print(toggles);
            Serial.newline();
        }
    }
}

} // namespace AcSyncTest

#endif // _AC_SYNC_TEST_HPP_
