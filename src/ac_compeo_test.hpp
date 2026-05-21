#ifndef _AC_COMPEO_TEST_HPP_
#define _AC_COMPEO_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"
#include "bytestream.hpp"

// Originally written as: "does AC.EVCTRL.COMPEO0 carry a level or an edge?"
// After datasheet review (2026-05-21), this turned out to be the wrong
// question — see header notes below.  Kept as the bench evidence backing
// the resolution of noDFF §16.1.
//
// Path exercised:
//   PA05 (driven by firmware as plain GPIO output 0V / VDD)
//     -> AC COMP0 (MUXPOS=PIN1, MUXNEG=VSCALE@VDD/2, OUT_ASYNC, INTSEL=TOGGLE)
//       -> EVSYS CH2 ASYNC (gen=AC_COMP_0 id 73)
//          -> CCL_LUTIN_2 (id 42): LUT2 IN0 INSEL=EVENT, LUTEI=1, TRUTH=0xAA
//             -> CCL_OUT2 (PA25 mux I) — readback via PORT.IN
//          -> TC4_EVU (id 27): TC4 COUNT16 NFRQ EVACT=COUNT — edge counter
//
// PA05 is driven digitally (high/low) — the analog mux still sees the pin
// voltage, so AC compares VDD or 0V against VSCALE (VDD/2 with SCALER=31).
//
// What we actually observe on the SAMC21J18A:
//   - PA25 reads LOW even when AC is held HIGH for 5 ms.
//   - TC4 counts +1 per AC rising edge, +0 per AC falling edge.
//
// Datasheet-grounded interpretation:
//   - `COMPEO` is a **continuous level** copy of the comparator output
//     (datasheet §40.6.2.4: "Events are generated using the comparator
//     output state, regardless of whether the interrupt is enabled or
//     not."); `INTSEL` only configures the interrupt source.
//   - The CCL `INSEL=EVENT` input has a **built-in rising-edge detector**
//     on the J variant (datasheet §37: 1 GCLK_CCL strobe per detected
//     rising edge; `INSEL=ASYNCEVENT` to bypass is N-variant only).
//   - Consequently the LUT2 input sees only 1-cycle strobes on AC rises,
//     the LUT2 output collapses back to 0 between strobes, and PA25 reads
//     LOW most of the time.  TC4 listens to the EVSYS line directly (no
//     CCL in its path), but a level signal still produces only one rising
//     edge per AC rise, so its delta is also 10 per 10 toggle cycles.
//   - This test alone cannot discriminate "level on EVSYS + CCL edge
//     detector on J" from "edge on EVSYS"; the datasheet is the
//     tie-breaker.
//
// Net result for noDFF §16.1: A1 not viable on J, A2 (external pad
// loopback `PA12 → PA22` with `INSEL=IO`) is the only path; see
// `design-single-channel_noDFF.md`.
//
// External hardware requirements:
//   - Nothing else on PA05 (no analog signal source).  Firmware drives it.
//   - PA25 not connected to anything that could overpower the CCL output.
//   - SWD lines untouched.
// Optional probes (not required for the test to work):
//   - Scope on PA12 (AC_CMP0 pad, mux H) to see the comparator output.
//   - Scope on PA25 (CCL_OUT2 pad, mux I) to see the EVSYS-routed signal.

namespace AcCompeoTest {

static constexpr uint32_t F_CPU_XOSC   = 24000000u;
static constexpr uint32_t UART_BAUD    = 1000000u;
static constexpr uint8_t  SCALER_VALUE = 31u;   // VDD * 32/64 ≈ VDD/2
static constexpr uint32_t N_TOGGLES    = 10u;   // 10 cycles -> 10 rises + 10 falls
static constexpr uint32_t SETTLE_US    = 5000u; // AC + propagation settling
static constexpr uint32_t TOGGLE_US    = 100u;  // dwell per half-cycle

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

static inline void wait_us(uint32_t us)
{
    uint32_t start = Timebase::micros();
    while ((Timebase::micros() - start) < us)
        ;
}

static inline void pa05_high(void) { PORT->Group[0].OUTSET.reg = (1ul << 5); }
static inline void pa05_low(void)  { PORT->Group[0].OUTCLR.reg = (1ul << 5); }

static inline bool pa25_read(void)
{
    return (PORT->Group[0].IN.reg & (1ul << 25)) != 0u;
}

static inline uint32_t read_tc4_count(void)
{
    TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
        ;
    return TC4->COUNT16.COUNT.reg;
}

static inline void run(void)
{
    Serial.print("\r\nAC COMPEO / CCL EVENT-edge-detector test (noDFF design §16.1)");
    Serial.newline();
    Serial.print("  PA05 driven as GPIO out (firmware-controlled), AC reads it as analog");
    Serial.newline();
    Serial.print("  EVSYS ASYNC: AC_COMP_0 (73) -> { CCL_LUTIN_2 (42), TC4_EVU (27) }");
    Serial.newline();
    Serial.print("  CCL LUT2 passthrough (TRUTH=0xAA, LUTEI=1) -> CCL_OUT2 on PA25");
    Serial.newline();

    switch_cpu_to_xosc();

    Serial.print("CPU clock switched to XOSC 24 MHz");
    Serial.newline();

    MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC
                       | MCLK_APBCMASK_CCL
                       | MCLK_APBCMASK_EVSYS
                       | MCLK_APBCMASK_TC4;

    GCLK->PCHCTRL[AC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[TC4_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TC4_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[EVSYS_GCLK_ID_2].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[EVSYS_GCLK_ID_2].reg & GCLK_PCHCTRL_CHEN))
        ;

    // PA05: plain GPIO output, driven LOW initially.  AC's analog mux still
    // reads the pin voltage independently of the digital mux.
    PORT->Group[0].PINCFG[5].reg = 0;
    PORT->Group[0].DIRSET.reg    = (1ul << 5);
    PORT->Group[0].OUTCLR.reg    = (1ul << 5);

    // PA25 mux I = CCL_OUT2.  INEN=1 so PORT.IN reflects the pad level for
    // firmware readback.
    PORT->Group[0].PINCFG[25].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
    PORT->Group[0].PMUX[12].bit.PMUXO = MUX_PA25I_CCL_OUT2;

    // PA12 mux H = AC_CMP0 (optional scope probe).
    PORT->Group[0].PINCFG[12].reg = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[6].bit.PMUXE = MUX_PA12H_AC_CMP0;

    // CCL: only LUT2 used.  TRUTH=0xAA passes IN0 (the event input) through
    // regardless of IN1/IN2 (which are MASK = 0).
    CCL->CTRL.reg = 0;
    CCL->SEQCTRL[0].reg = 0;
    CCL->SEQCTRL[1].reg = 0;
    CCL->LUTCTRL[0].reg = 0;
    CCL->LUTCTRL[1].reg = 0;
    CCL->LUTCTRL[2].reg = CCL_LUTCTRL_TRUTH(0xAAu)
                       | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_EVENT_Val)
                       | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_MASK_Val)
                       | CCL_LUTCTRL_LUTEI
                       | CCL_LUTCTRL_ENABLE;
    CCL->LUTCTRL[3].reg = 0;
    CCL->CTRL.reg = CCL_CTRL_ENABLE;

    // EVSYS CH2 ASYNC: AC_COMP_0 -> two users (CCL_LUTIN_2 + TC4_EVU).
    EVSYS->USER[EVSYS_ID_USER_CCL_LUTIN_2].reg = EVSYS_USER_CHANNEL(3);
    EVSYS->USER[EVSYS_ID_USER_TC4_EVU].reg     = EVSYS_USER_CHANNEL(3);
    EVSYS->CHANNEL[2].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_AC_COMP_0) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    // TC4: 16-bit, DIV1, NFRQ, EVCTRL=TCEI|EVACT_COUNT, ENABLE + RETRIGGER.
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
        ;
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
    TC4->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_NFRQ;
    TC4->COUNT16.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
    TC4->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
        ;
    TC4->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (TC4->COUNT16.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB)
        ;

    // AC COMP0: PA05 vs VSCALE@VDD/2, async output, INTSEL=TOGGLE,
    // COMPEO0=1 to drive the EVSYS event line.
    AC->SCALER[0].reg = SCALER_VALUE;
    AC->COMPCTRL[0].reg =
        AC_COMPCTRL_MUXPOS_PIN1 |
        AC_COMPCTRL_MUXNEG_VSCALE |
        AC_COMPCTRL_SPEED_HIGH |
        AC_COMPCTRL_OUT_ASYNC |
        AC_COMPCTRL_INTSEL_TOGGLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0)
        ;
    AC->EVCTRL.reg = AC_EVCTRL_COMPEO0;
    AC->CTRLA.reg |= AC_CTRLA_ENABLE;
    AC->COMPCTRL[0].reg |= AC_COMPCTRL_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0)
        ;
    while (!(AC->STATUSB.reg & AC_STATUSB_READY0))
        ;
    AC->INTFLAG.reg = AC_INTFLAG_COMP0;   // discard spurious enable INTFLAG

    // Phase A: PA05 LOW, AC stable LOW.  Let any startup transients pass.
    wait_us(SETTLE_US);
    const uint32_t cnt_a   = read_tc4_count();
    const bool     pad_a   = pa25_read();
    const bool     state_a = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;

    // Phase B: PA05 -> HIGH, AC stable HIGH.  Decisive level/edge probe
    // via PA25.
    pa05_high();
    wait_us(SETTLE_US);
    const uint32_t cnt_b   = read_tc4_count();
    const bool     pad_b   = pa25_read();
    const bool     state_b = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;

    // Phase C: toggle 10 cycles (= 10 falls + 10 rises of AC output).
    for (uint32_t i = 0; i < N_TOGGLES; ++i) {
        pa05_low();
        wait_us(TOGGLE_US);
        pa05_high();
        wait_us(TOGGLE_US);
    }
    const uint32_t cnt_c = read_tc4_count();

    // Phase D: PA05 -> LOW final, settle, last readouts.
    pa05_low();
    wait_us(SETTLE_US);
    const uint32_t cnt_d   = read_tc4_count();
    const bool     pad_d   = pa25_read();
    const bool     state_d = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;

    auto bit = [](bool b) -> uint32_t { return b ? 1u : 0u; };

    Serial.print("Phase A  (PA05=0, settled): TC4=");
    Serial.print(cnt_a);
    Serial.print("  PA25=");
    Serial.print(bit(pad_a));
    Serial.print("  AC.STATE0=");
    Serial.print(bit(state_a));
    Serial.newline();

    Serial.print("Phase B  (PA05=1, settled): TC4=");
    Serial.print(cnt_b);
    Serial.print("  PA25=");
    Serial.print(bit(pad_b));
    Serial.print("  AC.STATE0=");
    Serial.print(bit(state_b));
    Serial.print("   <-- level/edge probe");
    Serial.newline();

    const uint32_t delta_ab = cnt_b - cnt_a;
    const uint32_t delta_bc = cnt_c - cnt_b;
    const uint32_t delta_cd = cnt_d - cnt_c;

    Serial.print("Phase C  (after ");
    Serial.print(N_TOGGLES);
    Serial.print(" toggle cycles): TC4=");
    Serial.print(cnt_c);
    Serial.print("  ΔBC=");
    Serial.print(delta_bc);
    Serial.newline();

    Serial.print("Phase D  (PA05=0 final): TC4=");
    Serial.print(cnt_d);
    Serial.print("  PA25=");
    Serial.print(bit(pad_d));
    Serial.print("  AC.STATE0=");
    Serial.print(bit(state_d));
    Serial.print("  ΔCD=");
    Serial.print(delta_cd);
    Serial.newline();

    Serial.print("Counts: ΔAB(first rise)=");
    Serial.print(delta_ab);
    Serial.print(" expected 1; ΔBC(toggles) LEVEL=");
    Serial.print(N_TOGGLES);
    Serial.print(" or EDGE=");
    Serial.print(2u * N_TOGGLES);
    Serial.print("; ΔCD(final fall) LEVEL=0 EDGE=1");
    Serial.newline();

    // What we are matching against the expected J-variant pattern:
    //   AC level on EVSYS + CCL EVENT-input edge detector (1 GCLK_CCL strobe
    //   per AC rising edge, falling edges silent, no level on the LUT pad).
    // PA25 should be 0 in all three sampled phases; ΔBC ~= N_TOGGLES (one
    // strobe per AC rise); ΔCD = 0 (the final fall produces no event).
    const bool expected_j_pattern =
           !pad_a && !pad_b && !pad_d
        && (delta_bc >= N_TOGGLES - 1u)
        && (delta_bc <= N_TOGGLES + 1u)
        && (delta_cd <= 1u);

    Serial.print("VERDICT: ");
    if (expected_j_pattern) {
        Serial.print("OK -- matches J pattern: level COMPEO + CCL EVENT edge detector");
        Serial.newline();
        Serial.print("        noDFF sub-option A1 is NOT viable on J; use A2 (PA12->PA22)");
    } else {
        Serial.print("ANOMALY -- inspect counts and pad readbacks above");
    }
    Serial.newline();
}

} // namespace AcCompeoTest

#endif // _AC_COMPEO_TEST_HPP_
