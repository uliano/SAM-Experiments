#ifndef _SINGLE_CHANNEL_NODFF_HPP_
#define _SINGLE_CHANNEL_NODFF_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"
#include "bytestream.hpp"
#include "core/pin.hpp"

// Full single-channel noDFF integration (design-single-channel_noDFF.md).
//
// Chain:
//   TCC0 (GCLK1=24 MHz, PER=63) -> three phase-aligned heartbeat PWMs:
//     WO0=HB_1_8 (internal), WO1=HB_7_8 (PA09), WO2=HB_1_2 (PA10).
//   AC COMP2: analog input PA02 (AIN[4]) vs VSCALE (VDD/2), OUT=ASYNC.
//   CCL SEQ1 = DFF:
//     LUT2 D-input = COMP2 via INSEL=AC (the J variant HAS COMP2; the
//                    CCL INSEL=AC map is LUT_n<-COMP_n, datasheet Fig 37-8).
//     LUT3 CLK     = HB_1_2 via INSEL=TCC IN2, FILTSEL=SYNCH + EDGESEL=1
//                    (turns the SEQ "DFF" into a true rising-edge flip-flop).
//     Q (= LUT2 post-SEQ output) -> PA25.
//   LUT1 AND-gate: COUNT_PULSE = Q (LINK from LUT2) AND HB_1_2 (PB10 loopback).
//                  -> PA11; EVSYS gen CCL_LUTOUT_1.
//   LUT0 REF mux:  REF = Q ? HB_1_8 : HB_7_8.  Q via PA25->PA18 jumper (IO).
//                  -> PA07.
//   EVSYS CH0 ASYNC: TCC0_OVF   -> TC0_EVU  (window counter source).
//   EVSYS CH1 ASYNC: CCL_LUTOUT_1 -> TC2_EVU (duty counter source).
//   TC0+TC1 COUNT32 MFRQ, CC[0]=N-1: window counter, OVF IRQ each window.
//   TC2+TC3 COUNT32 NFRQ: duty counter, read+reset in the window ISR.
//
// External jumpers required (2):
//   PA10 -> PB10   (HB_1_2 into LUT1 AND-gate, CCL_IN5)
//   PA25 -> PA18   (Q into LUT0 REF mux, CCL_IN2)
//
// Debug / scope:
//   PA09 HB_7_8, PA10 HB_1_2, PA07 REF, PA11 COUNT_PULSE, PA25 Q,
//   PA13 = ISR-duration marker (high during the window ISR).
//
// Self-test: PA02 is driven by firmware (GPIO out) through a multi-phase
// sequence; the AC analog mux still reads the pad voltage.  Set
// SC_NODFF_SELF_DRIVE to 0 to leave PA02 as an analog input for an external
// signal generator instead.

#ifndef SC_NODFF_SELF_DRIVE
#define SC_NODFF_SELF_DRIVE 1
#endif

namespace SingleChannelNoDFF {

static constexpr uint32_t N_WINDOW   = 8u;     // heartbeat periods per window
static constexpr uint8_t  SCALER_VAL = 31u;    // VSCALE = VDD*32/64 ~ VDD/2
static constexpr uint32_t PHASE_MS   = 5000u;  // dwell per test phase
static constexpr uint32_t PRINT_MS   = 500u;   // serial snapshot cadence

// ISR -> main handoff. inline (not static) so the ISR in interrupts.cpp and
// the loop in main.cpp share a single instance across translation units.
inline volatile uint32_t g_last_duty    = 0;
inline volatile uint8_t  g_last_ac      = 0;
inline volatile uint32_t g_window_count = 0;

using Hb78Pin   = sam::gpio::Pin<sam::gpio::Bank::A, 9>;   // TCC0/WO1, mux E
using Hb12Pin   = sam::gpio::Pin<sam::gpio::Bank::A, 10>;  // TCC0/WO2, mux F
using RefPin    = sam::gpio::Pin<sam::gpio::Bank::A, 7>;   // CCL_OUT0, mux I
using PulsePin  = sam::gpio::Pin<sam::gpio::Bank::A, 11>;  // CCL_OUT1, mux I
using QPin      = sam::gpio::Pin<sam::gpio::Bank::A, 25>;  // CCL_OUT2, mux I
using Hb12InPin = sam::gpio::Pin<sam::gpio::Bank::B, 10>;  // CCL_IN5,  mux I
using QInPin    = sam::gpio::Pin<sam::gpio::Bank::A, 18>;  // CCL_IN2,  mux I
using IsrPin    = sam::gpio::Pin<sam::gpio::Bank::A, 13>;  // ISR-duration marker
using InputPin  = sam::gpio::Pin<sam::gpio::Bank::A, 2>;   // AC AIN[4] / ADC0_AIN0

// Window-boundary ISR (called from irq_handler_tc0). Reads + resets the duty
// counter and latches the comparator state. PA13 frames the ISR for scope.
static inline void on_window_isr(void)
{
    IsrPin::set();
    if (TC0->COUNT32.INTFLAG.reg & TC_INTFLAG_OVF) {
        TC0->COUNT32.INTFLAG.reg = TC_INTFLAG_OVF;  // W1C

        TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
        while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
            ;
        uint32_t duty = TC2->COUNT32.COUNT.reg;
        uint8_t  ac   = (AC->STATUSA.reg & AC_STATUSA_STATE2) ? 1u : 0u;

        // Reset duty counter so the next window starts at 0.
        TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;

        g_last_duty    = duty;
        g_last_ac      = ac;
        g_window_count = g_window_count + 1u;
    }
    IsrPin::clear();
}

static inline void setup(void)
{
    // --- APB clocks ---
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0
                       | MCLK_APBCMASK_TC0 | MCLK_APBCMASK_TC1
                       | MCLK_APBCMASK_TC2 | MCLK_APBCMASK_TC3
                       | MCLK_APBCMASK_AC
                       | MCLK_APBCMASK_CCL
                       | MCLK_APBCMASK_EVSYS;

    // --- PCHCTRL: TCC0 on GCLK1 (24 MHz crystal); the rest on GCLK0 (48 MHz) ---
    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[TC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TC2_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[AC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
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

    // --- Pins ---
    Hb78Pin::set_peripheral(sam::gpio::Peripheral::E);   // TCC0/WO1
    Hb12Pin::set_peripheral(sam::gpio::Peripheral::F);   // TCC0/WO2
    RefPin::set_peripheral(sam::gpio::Peripheral::I);    // CCL_OUT0
    PulsePin::set_peripheral(sam::gpio::Peripheral::I);  // CCL_OUT1
    QPin::set_peripheral(sam::gpio::Peripheral::I);      // CCL_OUT2 (Q)
    Hb12InPin::set_peripheral(sam::gpio::Peripheral::I); // CCL_IN5 (HB_1_2 loopback, INEN)
    QInPin::set_peripheral(sam::gpio::Peripheral::I);    // CCL_IN2 (Q loopback, INEN)
    IsrPin::as_output(false);                            // ISR-duration marker

#if SC_NODFF_SELF_DRIVE
    InputPin::as_output(false);                          // firmware-driven AC input
#else
    InputPin::as_input(sam::gpio::Pull::None, false);    // analog input (digital off)
#endif

    // --- AC COMP2: PA02 (AIN[4]) vs VSCALE, async, read by LUT2 via INSEL=AC ---
    AC->SCALER[2].reg = SCALER_VAL;
    AC->COMPCTRL[2].reg =
        AC_COMPCTRL_MUXPOS_PIN0    // AIN[4] for COMP2/3
        | AC_COMPCTRL_MUXNEG_VSCALE
        | AC_COMPCTRL_SPEED_HIGH
        | AC_COMPCTRL_OUT_ASYNC;   // OUT must be SYNC/ASYNC for CCL-internal use
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL2)
        ;
    AC->CTRLA.reg |= AC_CTRLA_ENABLE;
    AC->COMPCTRL[2].reg |= AC_COMPCTRL_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL2)
        ;
    while (!(AC->STATUSB.reg & AC_STATUSB_READY2))
        ;
    AC->INTFLAG.reg = AC_INTFLAG_COMP2;  // discard spurious enable flag

    // --- CCL (errata 1.7.3: write SEQCTRL/LUTCTRL with CTRL.ENABLE=0; no SWRST) ---
    CCL->CTRL.reg = 0;
    CCL->SEQCTRL[0].reg = 0;
    CCL->SEQCTRL[1].reg = CCL_SEQCTRL_SEQSEL_DFF;

    // LUT0 REF mux: REF = Q ? HB_1_8 : HB_7_8.
    CCL->LUTCTRL[0].reg = CCL_LUTCTRL_TRUTH(0xACu)
        | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_TCC_Val)   // WO0 = HB_1_8
        | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_TCC_Val)   // WO1 = HB_7_8
        | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_IO_Val)    // PA18 = Q (jumper)
        | CCL_LUTCTRL_ENABLE;

    // LUT1 AND-gate: COUNT_PULSE = Q AND HB_1_2.  IN0 LINK = LUT2 post-SEQ Q.
    CCL->LUTCTRL[1].reg = CCL_LUTCTRL_TRUTH(0x20u)
        | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_LINK_Val)  // Q from LUT2
        | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
        | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_IO_Val)    // PB10 = HB_1_2 (jumper)
        | CCL_LUTCTRL_LUTEO
        | CCL_LUTCTRL_ENABLE;

    // LUT2 DFF-D: D = COMP2 read directly via INSEL=AC.
    CCL->LUTCTRL[2].reg = CCL_LUTCTRL_TRUTH(0xAAu)
        | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_AC_Val)    // COMP2
        | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
        | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_MASK_Val)
        | CCL_LUTCTRL_ENABLE;

    // LUT3 DFF-CLK: CLK = HB_1_2 (TCC0/WO2 via INSEL=TCC IN2). FILTSEL+EDGESEL
    // make the SEQ block a true rising-edge DFF.
    CCL->LUTCTRL[3].reg = CCL_LUTCTRL_TRUTH(0xF0u)
        | CCL_LUTCTRL_INSEL0(CCL_LUTCTRL_INSEL0_MASK_Val)
        | CCL_LUTCTRL_INSEL1(CCL_LUTCTRL_INSEL0_MASK_Val)
        | CCL_LUTCTRL_INSEL2(CCL_LUTCTRL_INSEL0_TCC_Val)   // WO2 = HB_1_2
        | CCL_LUTCTRL_FILTSEL_SYNCH
        | CCL_LUTCTRL_EDGESEL
        | CCL_LUTCTRL_ENABLE;

    CCL->CTRL.reg = CCL_CTRL_ENABLE;

    // --- EVSYS (ASYNC; USER before CHANNEL) ---
    EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(1);  // channel 0
    EVSYS->CHANNEL[0].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
    EVSYS->USER[EVSYS_ID_USER_TC2_EVU].reg = EVSYS_USER_CHANNEL(2);  // channel 1
    EVSYS->CHANNEL[1].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_1) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    // --- TC0+TC1 window counter: COUNT32 MFRQ, CC[0]=N-1, OVF IRQ ---
    TC0->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
        ;
    TC0->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
    TC0->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
    TC0->COUNT32.CC[0].reg = N_WINDOW - 1u;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CC0)
        ;
    TC0->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
    TC0->COUNT32.INTENSET.reg = TC_INTENSET_OVF;
    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_EnableIRQ(TC0_IRQn);

    // --- TC2+TC3 duty counter: COUNT32 NFRQ ---
    TC2->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
        ;
    TC2->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
    TC2->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_NFRQ;
    TC2->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;

    // Enable + RETRIGGER (mandatory when EVACT != OFF, §14 finding #7).
    TC2->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
        ;
    TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB)
        ;
    TC0->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
        ;
    TC0->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB)
        ;

    // --- TCC0 heartbeat, enabled last so TC0 doesn't miss the first OVF ---
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
    TCC0->CC[0].reg = 8u;   // HB_1_8
    TCC0->CC[1].reg = 56u;  // HB_7_8
    TCC0->CC[2].reg = 32u;  // HB_1_2
    while (TCC0->SYNCBUSY.reg & (TCC_SYNCBUSY_CC0 | TCC_SYNCBUSY_CC1 | TCC_SYNCBUSY_CC2))
        ;
    TCC0->EVCTRL.reg = TCC_EVCTRL_OVFEO;
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;
}

static inline void run(void)
{
    Serial.print("\r\nSingle-channel noDFF integration (N=");
    Serial.print(N_WINDOW);
    Serial.print(" heartbeat periods/window)");
    Serial.newline();
    Serial.print("  jumpers: PA10->PB10 (HB_1_2), PA25->PA18 (Q)");
    Serial.newline();
    Serial.print("  AC COMP2 on PA02 (AIN4) vs VSCALE; LUT2 reads COMP2 via INSEL=AC");
    Serial.newline();
    Serial.print("  scope: PA10=HB_1_2 PA09=HB_7_8 PA07=REF PA11=COUNT_PULSE PA25=Q PA13=ISR");
    Serial.newline();
#if SC_NODFF_SELF_DRIVE
    Serial.print("  input: PA02 firmware-driven, phases low(exp 0)/high(exp 8) every 5 s");
#else
    Serial.print("  input: PA02 external (analog); expected value unknown");
#endif
    Serial.newline();

    setup();

    // Phase table: PA02 level + expected duty count.
    struct Phase { bool level; uint32_t expected; const char* name; };
    static const Phase phases[] = {
        { false, 0u,        "A low " },
        { true,  N_WINDOW,  "B high" },
    };
    const uint32_t n_phases = sizeof(phases) / sizeof(phases[0]);

    uint32_t phase = 0;
    uint32_t phase_start = Timebase::millis();
    uint32_t last_print  = phase_start;
#if SC_NODFF_SELF_DRIVE
    InputPin::write(phases[0].level);
#endif

    for (;;) {
        uint32_t now = Timebase::millis();

        if ((now - phase_start) >= PHASE_MS) {
            phase = (phase + 1u) % n_phases;
            phase_start = now;
#if SC_NODFF_SELF_DRIVE
            InputPin::write(phases[phase].level);
#endif
        }

        if ((now - last_print) >= PRINT_MS) {
            last_print = now;
            uint32_t duty = g_last_duty;
            uint32_t ac   = g_last_ac;
            uint32_t wc   = g_window_count;

            Serial.print("phase ");
            Serial.print(phases[phase].name);
#if SC_NODFF_SELF_DRIVE
            Serial.print("  PA02=");
            Serial.print(static_cast<uint32_t>(phases[phase].level ? 1u : 0u));
            Serial.print("  expected=");
            Serial.print(phases[phase].expected);
#else
            Serial.print("  PA02=ext");
#endif
            Serial.print("  duty=");
            Serial.print(duty);
            Serial.print("  AC=");
            Serial.print(ac);
            Serial.print("  windows=");
            Serial.print(wc);
            Serial.newline();
        }

        __WFI();
    }
}

} // namespace SingleChannelNoDFF

#endif // _SINGLE_CHANNEL_NODFF_HPP_
