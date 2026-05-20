#ifndef _TCC1_COUNTEV_TEST_HPP_
#define _TCC1_COUNTEV_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"

// TCC1 event-count via asynchronous EVSYS path — design validation.
//
// Verifies that TCC1 EVACT0 correctly counts TCC0 overflow events delivered
// through an asynchronous EVSYS channel.  This is the exact path used by the
// dual-channel design heartbeat window counter:
//
//   TCC0 OVF → EVSYS CH0 (ASYNC) → TCC1_EV0 → EVACT0=COUNT
//
// EVACT0 selection rationale (the reason this file changed):
//   - The first revision used EVACT0=COUNTEV (0x2 "Count on event") and read
//     back count=0.  COUNTEV expects an EDGSEL-detected edge from EVSYS, but
//     errata DS80000740S 1.21.9 forces PATH_ASYNCHRONOUS for TCC users on
//     Rev F, and the async path is purely combinational with EDGSEL forced
//     to NO_EVT_OUTPUT — there is no edge for COUNTEV to count.
//   - EVACT0=COUNT (0x5, "Count on active state of asynchronous event") is
//     the value explicitly intended for async event sources.  Counter ticks
//     at GCLK_TCC1 rate while EV0 is asserted; the TCC0 OVF pulse is one
//     GCLK1 cycle wide and TCC0/TCC1 share GCLK1, so each event yields
//     exactly one count.
//
// TCC0: GCLK1=24 MHz XOSC, DIV1, PER=63 → 375 kHz OVF rate, OVFEO=1.
// TCC1: same GCLK1 (shared PCHCTRL[28]), NFRQ, PER=0xFFFFFF, EVACT0=COUNT.
// Window: 500 ms → expected ~187 500 counts (375 000 Hz × 0.5 s).
// PASS: count in [150 000, 220 000].
//
// Pre-condition: QuartzTest::init() has already configured GCLK1=24 MHz XOSC.

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

static inline void run(void)
{
    Serial.print("\r\nTCC1 EVACT0=COUNT via ASYNC EVSYS — design path validation");
    Serial.newline();
    Serial.print("TCC0 GCLK1=24 MHz DIV1 PER=63 -> 375 kHz OVF  OVFEO=1");
    Serial.newline();
    Serial.print("EVSYS CH0 ASYNC: gen=TCC0_OVF(35) -> user=TCC1_EV0(15)");
    Serial.newline();
    Serial.print("TCC1 EVACT0=COUNT EV0  window=500 ms  expected~187500");
    Serial.newline();

    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0
                        | MCLK_APBCMASK_TCC1
                        | MCLK_APBCMASK_EVSYS;

    // TCC0+TCC1 share PCHCTRL[28]; GCLK1 already running from QuartzTest.
    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;

    // TCC0: NPWM, DIV1, PER=63 → 375 kHz, overflow event enabled.
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
    TCC0->EVCTRL.reg = TCC_EVCTRL_OVFEO;

    // EVSYS CH0: ASYNC, TCC0_OVF (gen=35) → TCC1_EV0 (user=15).
    // §29.6.2.2 requires USER written before CHANNEL.
    EVSYS->USER[EVSYS_ID_USER_TCC1_EV_0].reg = EVSYS_USER_CHANNEL(1); // ch0+1
    EVSYS->CHANNEL[0].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    // TCC1: NFRQ, DIV1, PER=max 24-bit (no overflow in window), EVACT0=COUNT.
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
    // EVACT0=COUNT (0x5), not COUNTEV (0x2): COUNTEV requires EVSYS-side edge
    // detection (EDGSEL), which PATH_ASYNCHRONOUS does not provide.  COUNT
    // operates on the active level of the async event and yields one tick per
    // 1-GCLK1-wide TCC0 OVF pulse — see file header for full rationale.
    TCC1->EVCTRL.reg = TCC_EVCTRL_TCEI0 | TCC_EVCTRL_EVACT0_COUNT;

    // Enable TCC1 before TCC0 so no events are missed at start.
    TCC1->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;

    delay_ms(500);

    // Stop TCC1 before reading.
    TCC1->CTRLBSET.reg = TCC_CTRLBSET_CMD_STOP;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_CTRLB)
        ;

    uint32_t count = read_tcc_count(TCC1);

    // Diagnostic: confirm TCC0 is actually running.  If TCC0 isn't counting,
    // no OVF events are produced and the EVSYS→TCC1 path can't be evaluated.
    // TCC0 keeps running here (only TCC1 was stopped), so READSYNC returns a
    // momentary snapshot.  A value in [0..63] (= PER range) means the
    // heartbeat is alive; a stuck 0 or value outside the PER range points to
    // a TCC0 clock/config problem rather than an EVSYS/TCC1 path issue.
    uint32_t tcc0_count = read_tcc_count(TCC0);

    // Capture additional TCC0/TCC1/clock state to discriminate between
    // "TCC0 not enabled", "TCC0 stopped", "GCLK channel disabled", etc.
    // All snapshots are taken before tear-down so the registers still reflect
    // the post-run state.
    uint32_t tcc0_ctrla    = TCC0->CTRLA.reg;
    uint32_t tcc0_status   = TCC0->STATUS.reg;
    uint32_t tcc0_syncbusy = TCC0->SYNCBUSY.reg;
    uint32_t tcc0_wave     = TCC0->WAVE.reg;
    uint32_t tcc0_per      = TCC0->PER.reg;
    uint32_t tcc0_evctrl   = TCC0->EVCTRL.reg;
    uint32_t tcc1_ctrla    = TCC1->CTRLA.reg;
    uint32_t tcc1_status   = TCC1->STATUS.reg;
    uint32_t tcc1_evctrl   = TCC1->EVCTRL.reg;
    uint32_t pchctrl28     = GCLK->PCHCTRL[TCC0_GCLK_ID].reg;
    uint32_t genctrl1      = GCLK->GENCTRL[1].reg;

    bool pass = (count >= 150000u) && (count <= 220000u);

    Serial.print("count=");
    Serial.print(count);
    Serial.print("  expected~187500  ");
    Serial.print(pass ? "PASS" : "FAIL");
    Serial.newline();
    Serial.print("  tcc0.count=");
    Serial.print(tcc0_count);
    Serial.print(" (0..63 = heartbeat alive)");
    Serial.newline();
    Serial.print("  tcc0: ctrla=0x");
    Serial.print(tcc0_ctrla, PrintBase::Hex);
    Serial.print(" status=0x");
    Serial.print(tcc0_status, PrintBase::Hex);
    Serial.print(" syncbusy=0x");
    Serial.print(tcc0_syncbusy, PrintBase::Hex);
    Serial.print(" wave=0x");
    Serial.print(tcc0_wave, PrintBase::Hex);
    Serial.print(" per=0x");
    Serial.print(tcc0_per, PrintBase::Hex);
    Serial.print(" evctrl=0x");
    Serial.print(tcc0_evctrl, PrintBase::Hex);
    Serial.newline();
    Serial.print("  tcc1: ctrla=0x");
    Serial.print(tcc1_ctrla, PrintBase::Hex);
    Serial.print(" status=0x");
    Serial.print(tcc1_status, PrintBase::Hex);
    Serial.print(" evctrl=0x");
    Serial.print(tcc1_evctrl, PrintBase::Hex);
    Serial.newline();
    Serial.print("  clk:  pchctrl[28]=0x");
    Serial.print(pchctrl28, PrintBase::Hex);
    Serial.print(" genctrl[1]=0x");
    Serial.print(genctrl1, PrintBase::Hex);
    Serial.newline();

    if (!pass) {
        if (count == 0u) {
            Serial.print("  -> zero: EVSYS path or TCC1 EVCTRL not working");
        } else if (count < 150000u) {
            Serial.print("  -> low: possible missed events or wrong clock");
        } else {
            Serial.print("  -> high: GCLK mismatch or double-counting");
        }
        Serial.newline();
    }

    // Tear down.
    TCC1->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
        ;
    TCC0->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
        ;
    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = 0;
    EVSYS->CHANNEL[0].reg = 0;
    EVSYS->USER[EVSYS_ID_USER_TCC1_EV_0].reg = 0;
    MCLK->APBCMASK.reg &= ~(MCLK_APBCMASK_TCC0 | MCLK_APBCMASK_TCC1);
}

} // namespace Tcc1CountevTest

#endif // _TCC1_COUNTEV_TEST_HPP_
