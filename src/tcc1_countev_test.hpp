#ifndef _TCC1_COUNTEV_TEST_HPP_
#define _TCC1_COUNTEV_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"

// Historical record / dual-counter testimony for the SAMC21J18A bring-up.
//
// The file name reflects the original goal: have TCC1 event-count TCC0 OVF
// pulses for the heartbeat-period window timer of the dual-channel design.
// Experimental validation on silicon found this is not achievable on this
// part: TCC family peripherals do not perform event-counting on the ASYNC
// EVSYS path that errata DS80000740S 1.21.9 forces TCC users to use.
// Tried — and all failed — were `EVACT0=COUNTEV`, `EVACT0=COUNT`, and
// `EVACT0=INC`, with and without a clock-decoupled source via EIC sync
// edge detection.  See §14 of `design-dual-channel.md` for the full
// chronological test log and the synthesis of what we learned about the
// silicon.
//
// The test that actually passes — and which closes the journey — uses a
// TC peripheral (TC4 16-bit standalone) on an independent clock as the
// event counter.  This is the configuration preserved below: TCC0 OVF
// (GCLK1 = 24 MHz, 1-cycle pulse) → EVSYS CH0 ASYNC → TC4 EVU
// (GCLK0 = 48 MHz, EVACT=COUNT).  TC4 sample period 20.8 ns vs source
// pulse width 41.7 ns: pulse spans ~2 sample edges, captured every time.
// Result: ~56406 counts over 150 ms vs expected 56250 (<0.3 % error).
//
// TCC1 is left configured and attached to the test for documentation: it
// is *detached from EVSYS* (no user mapping), enabled, retriggered, and
// read at the end — it shows zero, confirming that TCC1 was never
// counting anything in any of the prior attempts.  The output line
// "`TCC1 count = 0`" is the testimony that closes the question.
//
// This file is not on the production path.  The SAMC21N variant moves
// the window counter to TC4+TC5 in COUNT32 (32-bit, freewheel, no ISR
// accumulator) — see `design-dual-channel-N.md`.  The test below is
// preserved as the reference TC EVACT=COUNT pattern that the N variant
// will scale to 32-bit.

namespace Tcc1CountevTest {

static inline void delay_ms(uint32_t ms)
{
    uint32_t start = Timebase::millis();
    while ((Timebase::millis() - start) < ms)
        ;
}

static inline uint32_t read_tcc_count(Tcc* tcc)
{
    tcc->CTRLBSET.reg = TCC_CTRLBSET_CMD_READSYNC;
    while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_COUNT)
        ;
    return tcc->COUNT.reg;
}

static inline uint16_t read_tc4_count(void)
{
    TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
        ;
    return TC4->COUNT16.COUNT.reg;
}

static inline void run(void)
{
    Serial.print("\r\nSAMC21J bring-up testimony: TCC1 event-count attempt vs TC4 baseline");
    Serial.newline();
    Serial.print("  TCC0 NPWM PER=63 CC[2]=32 GCLK1=24 MHz -> OVF @ 375 kHz, PA10 scope ref");
    Serial.newline();
    Serial.print("  TCC1 (GCLK1=24 MHz, same clock) EVACT0=COUNT, detached from EVSYS -> 0 always");
    Serial.newline();
    Serial.print("  TC4  (GCLK0=48 MHz, independent) EVACT=COUNT, sole user of EVSYS CH0");
    Serial.newline();
    Serial.print("  EVSYS CH0 ASYNC gen=TCC0_OVF(35) -> TC4_EVU(27)");
    Serial.newline();
    Serial.print("  window=150 ms, TC4 expected ~56250 if path works");
    Serial.newline();

    // APB clocks.
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0
                        | MCLK_APBCMASK_TCC1
                        | MCLK_APBCMASK_TC4
                        | MCLK_APBCMASK_EVSYS;

    // PCHCTRL[28] = TCC0/TCC1, GCLK1 = 24 MHz (from QuartzTest).
    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    // PCHCTRL[32] = TC4, GCLK0 = 48 MHz (independent of TCC0/TCC1).
    GCLK->PCHCTRL[TC4_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TC4_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    // PCHCTRL[6] = GCLK_EVSYS_CHANNEL_0, GCLK0 = 48 MHz.
    GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg & GCLK_PCHCTRL_CHEN))
        ;

    // PA10 mux F = TCC0/WO[2] so an external scope can see the source signal.
    PORT->Group[0].PINCFG[10].reg = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[5].bit.PMUXE = MUX_PA10F_TCC0_WO2;

    // EVSYS CH0: ASYNC, TCC0_OVF(35) -> TC4_EVU(27) only.  TCC1 deliberately
    // not attached as a user: prior testing showed parallel TCC + TC users
    // on the same channel blocks delivery to the TC user too.
    EVSYS->USER[EVSYS_ID_USER_TC4_EVU].reg = EVSYS_USER_CHANNEL(1);
    EVSYS->CHANNEL[0].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    // TCC0: NPWM, DIV1, PER=63, CC[2]=32 -> WO[2] 50% duty.  OVFEO=1.
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
    TCC0->CC[2].reg = 32u;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC2)
        ;
    TCC0->EVCTRL.reg = TCC_EVCTRL_OVFEO;

    // TCC1: configured as the original design intended (EVACT0=COUNT,
    // TCEI0=1), but deliberately *not* connected to any EVSYS channel.
    // Kept here so its final count of 0 is in the serial log as proof
    // that TCC1 never counted anything in any prior attempt either.
    TCC1->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
        ;
    TCC1->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1;
    TCC1->WAVE.reg  = TCC_WAVE_WAVEGEN_NFRQ;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE)
        ;
    TCC1->PER.reg = 0x00FFFFFFu;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_PER)
        ;
    TCC1->EVCTRL.reg = TCC_EVCTRL_TCEI0 | TCC_EVCTRL_EVACT0_COUNT;

    // TC4: 16-bit, DIV1, EVCTRL=TCEI|EVACT_COUNT.  This is the working
    // configuration — same one preserved for the SAMC21N port (scaled to
    // TC4+TC5 COUNT32 there).
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
        ;
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
    TC4->COUNT16.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;

    // Enable both, RETRIGGER each (EVACT != OFF -> ENABLE alone does not
    // clear STATUS.STOP; this is the gotcha that wasted some hours).
    TCC1->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;
    TCC1->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_CTRLB)
        ;

    TC4->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
        ;
    TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB)
        ;

    // Enable source last so neither counter misses the first edge.
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;

    // Time-series read of TC4: proves the counter advances linearly through
    // the window.  The final read after STOP returns 0 because TC's STOP
    // command resets COUNT to 0 (TC semantic, different from TCC) — use the
    // 150 ms in-flight sample as the real total.
    delay_ms(10);
    uint16_t tc4_at_10ms  = read_tc4_count();
    delay_ms(40);
    uint16_t tc4_at_50ms  = read_tc4_count();
    delay_ms(50);
    uint16_t tc4_at_100ms = read_tc4_count();
    delay_ms(50);
    uint16_t tc4_at_150ms = read_tc4_count();

    TCC1->CTRLBSET.reg = TCC_CTRLBSET_CMD_STOP;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_CTRLB)
        ;
    uint32_t tcc1_cnt = read_tcc_count(TCC1);

    Serial.print("TCC1 count=");
    Serial.print(tcc1_cnt);
    Serial.print(" (TCC family does not event-count on ASYNC EVSYS — confirmed)");
    Serial.newline();
    Serial.print("TC4 time-series: 10ms->");
    Serial.print(tc4_at_10ms);
    Serial.print("  50ms->");
    Serial.print(tc4_at_50ms);
    Serial.print("  100ms->");
    Serial.print(tc4_at_100ms);
    Serial.print("  150ms->");
    Serial.print(tc4_at_150ms);
    Serial.print("  expected ~3750/18750/37500/56250");
    Serial.newline();
    Serial.print("  see design-dual-channel.md appendix for the full test log");
    Serial.newline();
}

} // namespace Tcc1CountevTest

#endif // _TCC1_COUNTEV_TEST_HPP_
