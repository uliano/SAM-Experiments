#ifndef _DUTY_EVLINK_TEST_HPP_
#define _DUTY_EVLINK_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"
#include "bytestream.hpp"

// Step 2 of design-single-channel.md bring-up: validate the duty-counter
// event link, which includes the CCL combinational stage.
//
// Path under test:
//   PA10 = TCC0/WO[2] (HB_1_2, 375 kHz 50%)  -- external jumper -->  PB10
//   PA08 (CCL_IN3, internal pull = Q surrogate) ----------------------+
//                                                                    v
//   CCL LUT1: TRUTH=0x20, IN0=IO(Q), IN1=MASK, IN2=IO(HB_1_2), LUTEO=1
//     -> EVSYS CH1 ASYNC (gen=CCL_LUTOUT_1 id 83, user=TC2_EVU id 25)
//       -> TC2+TC3 COUNT32 NFRQ, EVCTRL=TCEI|EVACT_COUNT
//
// The DFF described in design §10 is not yet on the bench, so PA08 stays
// muxed as CCL_IN3 and the firmware sets its level via the internal pull
// resistor (PINCFG.PULLEN + PORT.OUT).  When PA08=0 the LUT output is
// constant 0 (gate closed), when PA08=1 the LUT output follows HB_1_2
// (gate open, 375 kHz rising edges).
//
// Cross-check: TC0+TC1 runs in parallel as in step 1 (TCC0_OVF source),
// so we have a free-running reference at the same 375 kHz independent of
// the gate state.
//
// Required external setup: PA10 -- jumper -- PB10.
// SWD pins PA30/PA31 untouched.

namespace DutyEvlinkTest {

static constexpr uint32_t F_CPU_XOSC = 24000000u;
static constexpr uint32_t UART_BAUD  = 1000000u;

static inline void switch_cpu_to_xosc(void)
{
    Serial.flush();

    SERCOM5->USART.CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
    while (SERCOM5->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE)
        ;

    SysTick->CTRL = 0;

    GCLK->GENCTRL[0].reg =
        GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC_Val) | GCLK_GENCTRL_GENEN;
    while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL_GCLK0)
        ;

    SysTick->LOAD = (F_CPU_XOSC / 1000u) - 1u;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;

    const uint64_t br =
        (uint64_t)65536u * (F_CPU_XOSC - 16u * UART_BAUD) / F_CPU_XOSC;
    SERCOM5->USART.BAUD.reg = (uint16_t)br;

    SERCOM5->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
    while (SERCOM5->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE)
        ;
}

// Surrogate DFF.Q on PA08: pin is muxed as CCL_IN3 (input), but the
// internal pull resistor controls the level.  OUT=1 -> pull-up -> "Q=1",
// OUT=0 -> pull-down -> "Q=0".  Settling is ~1 us against the ~40 kohm
// pull and the pad's parasitic capacitance, fast enough for 100 ms phases.
static inline void pa08_q_high(void) { PORT->Group[0].OUTSET.reg = (1ul << 8); }
static inline void pa08_q_low(void)  { PORT->Group[0].OUTCLR.reg = (1ul << 8); }

static inline uint32_t read_tc32(Tc* tc)
{
    tc->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
    while (tc->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
        ;
    return tc->COUNT32.COUNT.reg;
}

static inline void run(void)
{
    Serial.print("\r\nStep 2: duty event-link, CCL LUT1 gated by PA08 (Q surrogate)");
    Serial.newline();
    Serial.print("  required: PA10 -- jumper -- PB10 (HB_1_2 loopback)");
    Serial.newline();
    Serial.print("  PA08 internal pull-up/down stands in for the DFF.Q net");
    Serial.newline();

    switch_cpu_to_xosc();

    Serial.print("CPU clock switched to XOSC 24 MHz");
    Serial.newline();

    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0
                       | MCLK_APBCMASK_TC0
                       | MCLK_APBCMASK_TC1
                       | MCLK_APBCMASK_TC2
                       | MCLK_APBCMASK_TC3
                       | MCLK_APBCMASK_EVSYS
                       | MCLK_APBCMASK_CCL;

    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[TC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TC2_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[EVSYS_GCLK_ID_1].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[EVSYS_GCLK_ID_1].reg & GCLK_PCHCTRL_CHEN))
        ;

    // PA10 mux F = TCC0/WO[2] -> HB_1_2 out (also feeds PB10 via external jumper).
    PORT->Group[0].PINCFG[10].reg = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[5].bit.PMUXE = MUX_PA10F_TCC0_WO2;

    // PB10 mux I = CCL_IN5 -> LUT1/IN2.  Input only; INEN=1; no pull (driven
    // by PA10 via the external loopback jumper).
    PORT->Group[1].DIRCLR.reg = (1ul << 10);
    PORT->Group[1].OUTCLR.reg = (1ul << 10);
    PORT->Group[1].PINCFG[10].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
    PORT->Group[1].PMUX[5].bit.PMUXE = MUX_PB10I_CCL_IN5;

    // PA08 mux I = CCL_IN3 -> LUT1/IN0.  Input; INEN=1; PULLEN=1; OUT bit
    // selects pull-up/down.  Start with pull-down -> "Q=0" -> gate closed.
    PORT->Group[0].DIRCLR.reg = (1ul << 8);
    PORT->Group[0].OUTCLR.reg = (1ul << 8);
    PORT->Group[0].PINCFG[8].reg =
        PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
    PORT->Group[0].PMUX[4].bit.PMUXE = MUX_PA08I_CCL_IN3;

    // EVSYS CH0: TCC0_OVF -> TC0_EVU (reference free-running counter).
    EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(1);
    EVSYS->CHANNEL[0].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    // EVSYS CH1: CCL_LUTOUT_1 -> TC2_EVU (the path under test).
    EVSYS->USER[EVSYS_ID_USER_TC2_EVU].reg = EVSYS_USER_CHANNEL(2);
    EVSYS->CHANNEL[1].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_1) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    // CCL: all four LUTCTRLn and both SEQCTRLn must be written with
    // CTRL.ENABLE=0 (errata DS80000740S 1.7.3).  Do NOT use CTRL.SWRST
    // (errata 1.7.4 reports a PAC error on software reset).
    CCL->CTRL.reg = 0;
    CCL->SEQCTRL[0].reg = 0;
    CCL->SEQCTRL[1].reg = 0;
    CCL->LUTCTRL[0].reg = 0;
    // The CMSIS header only exposes symbolic _Val constants for INSEL0; for
    // INSEL1/INSEL2 use the same numeric values (selector field is identical).
    CCL->LUTCTRL[1].reg = CCL_LUTCTRL_TRUTH(0x20)
                       | CCL_LUTCTRL_INSEL0_IO
                       | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_IO_Val)
                       | CCL_LUTCTRL_LUTEO
                       | CCL_LUTCTRL_ENABLE;
    CCL->LUTCTRL[2].reg = 0;
    CCL->LUTCTRL[3].reg = 0;
    CCL->CTRL.reg = CCL_CTRL_ENABLE;

    // TC0+TC1: COUNT32 NFRQ EVACT=COUNT, fed by EVSYS CH0 (reference).
    TC0->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
        ;
    TC0->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
    TC0->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_NFRQ;
    TC0->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;

    // TC2+TC3: COUNT32 NFRQ EVACT=COUNT, fed by EVSYS CH1 (under test).
    TC2->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
        ;
    TC2->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
    TC2->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_NFRQ;
    TC2->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;

    // TCC0 heartbeat: PER=63 CC[2]=32 -> HB_1_2 at 375 kHz 50% on PA10.
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

    // Both TC pairs: ENABLE then RETRIGGER (mandatory when EVACT != OFF).
    TC0->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
        ;
    TC0->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB)
        ;
    TC2->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
        ;
    TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB)
        ;

    // Source enabled last.
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;

    const uint32_t t0 = Timebase::micros();

    auto wait_until_us = [&](uint32_t target_us) {
        while ((Timebase::micros() - t0) < target_us)
            ;
    };

    struct Sample {
        const char* phase;
        uint32_t deadline_us;
        uint32_t actual_us;
        uint32_t tc0;       // reference (TCC0_OVF)
        uint32_t tc2;       // path under test (CCL_LUTOUT_1)
    };

    // Phase A: Q = 0 for 100 ms.  Expected: TC2 stays at 0, TC0 ~ 37500.
    wait_until_us(100000u);
    Sample s_a{ "A end (Q=0)", 100000u, Timebase::micros() - t0,
                read_tc32(TC0), read_tc32(TC2) };

    // Phase B starts: open the gate.  PA08 -> pull-up.
    pa08_q_high();

    // Sample at 200, 300, 400 ms cumulative.
    wait_until_us(200000u);
    Sample s_b1{ "B 200ms (Q=1)", 200000u, Timebase::micros() - t0,
                 read_tc32(TC0), read_tc32(TC2) };
    wait_until_us(300000u);
    Sample s_b2{ "B 300ms (Q=1)", 300000u, Timebase::micros() - t0,
                 read_tc32(TC0), read_tc32(TC2) };
    wait_until_us(400000u);
    Sample s_b3{ "B 400ms (Q=1)", 400000u, Timebase::micros() - t0,
                 read_tc32(TC0), read_tc32(TC2) };

    // Phase C: close the gate, count must stop advancing.
    pa08_q_low();

    wait_until_us(500000u);
    Sample s_c{ "C end (Q=0)", 500000u, Timebase::micros() - t0,
                read_tc32(TC0), read_tc32(TC2) };

    auto report = [&](const Sample& s)
    {
        Serial.print("  ");
        Serial.print(s.phase);
        Serial.print(": t=");
        Serial.print(s.actual_us);
        Serial.print(" us  TC0=");
        Serial.print(s.tc0);
        Serial.print("  TC2=");
        Serial.print(s.tc2);
        Serial.newline();
    };

    Serial.print("Phases (Q via PA08 pull): A=0..100ms Q=0, B=100..400ms Q=1, C=400..500ms Q=0");
    Serial.newline();
    Serial.print("Reference TC0 counts TCC0_OVF (375 kHz) regardless of Q");
    Serial.newline();
    report(s_a);
    report(s_b1);
    report(s_b2);
    report(s_b3);
    report(s_c);

    // Verifications.
    bool a_gate_closed = s_a.tc2 < 1000u;
    int32_t b_delta12 = (int32_t)s_b2.tc2 - (int32_t)s_b1.tc2;
    int32_t b_delta23 = (int32_t)s_b3.tc2 - (int32_t)s_b2.tc2;
    bool b_gate_open  = b_delta12 > 36000 && b_delta12 < 39000
                     && b_delta23 > 36000 && b_delta23 < 39000;
    int32_t c_held    = (int32_t)s_c.tc2 - (int32_t)s_b3.tc2;
    bool c_gate_held  = c_held >= 0 && c_held < 200;
    bool tc0_running  = s_c.tc0 > 180000u && s_c.tc0 < 195000u;

    Serial.print("Δ TC2 in B (100->200ms slice)=");
    Serial.print(static_cast<uint32_t>(b_delta12));
    Serial.print("  (200->300ms slice)=");
    Serial.print(static_cast<uint32_t>(b_delta23));
    Serial.print("  expected ~37500 each");
    Serial.newline();
    Serial.print("Δ TC2 in C (gate closed)=");
    if (c_held < 0) { Serial.print("-"); c_held = -c_held; }
    Serial.print(static_cast<uint32_t>(c_held));
    Serial.print("  expected ~0");
    Serial.newline();

    Serial.print("RESULT: ");
    if (a_gate_closed && b_gate_open && c_gate_held && tc0_running) {
        Serial.print("OK - gate closed/open/held behave as expected, reference TC0 healthy");
    } else {
        Serial.print("FAIL  a=");
        Serial.print(static_cast<uint32_t>(a_gate_closed));
        Serial.print(" b=");
        Serial.print(static_cast<uint32_t>(b_gate_open));
        Serial.print(" c=");
        Serial.print(static_cast<uint32_t>(c_gate_held));
        Serial.print(" tc0=");
        Serial.print(static_cast<uint32_t>(tc0_running));
    }
    Serial.newline();
}

} // namespace DutyEvlinkTest

#endif // _DUTY_EVLINK_TEST_HPP_
