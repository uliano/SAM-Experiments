#ifndef _TCC1_COUNTEV_TEST_HPP_
#define _TCC1_COUNTEV_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"

// TCC1 COUNTEV via asynchronous EVSYS path — design validation.
//
// Verifies that TCC1 EVACT0=COUNTEV correctly counts TCC0 overflow events
// delivered through an asynchronous EVSYS channel.  This is the exact path
// used by the dual-channel design heartbeat window counter:
//
//   TCC0 OVF → EVSYS CH0 (ASYNC) → TCC1_EV0 → EVACT0=COUNTEV
//
// TCC0: GCLK1=24 MHz XOSC, DIV1, PER=63 → 375 kHz OVF rate, OVFEO=1.
// TCC1: same GCLK1 (shared PCHCTRL[28]), NFRQ, PER=0xFFFFFF, COUNTEV EV0.
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
    Serial.print("\r\nTCC1 COUNTEV via ASYNC EVSYS — design path validation");
    Serial.newline();
    Serial.print("TCC0 GCLK1=24 MHz DIV1 PER=63 -> 375 kHz OVF  OVFEO=1");
    Serial.newline();
    Serial.print("EVSYS CH0 ASYNC: gen=TCC0_OVF(35) -> user=TCC1_EV0(15)");
    Serial.newline();
    Serial.print("TCC1 COUNTEV EV0  window=500 ms  expected~187500");
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
    EVSYS->CHANNEL[0].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
    EVSYS->USER[EVSYS_ID_USER_TCC1_EV_0].reg = EVSYS_USER_CHANNEL(1); // ch0+1

    // TCC1: NFRQ, DIV1, PER=max 24-bit (no overflow in window), COUNTEV EV0.
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
    TCC1->EVCTRL.reg = TCC_EVCTRL_TCEI0 | TCC_EVCTRL_EVACT0_COUNTEV;

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
    bool pass = (count >= 150000u) && (count <= 220000u);

    Serial.print("count=");
    Serial.print(count);
    Serial.print("  expected~187500  ");
    Serial.print(pass ? "PASS" : "FAIL");
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
