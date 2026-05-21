#ifndef _WINDOW_EVLINK_TEST_HPP_
#define _WINDOW_EVLINK_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"
#include "bytestream.hpp"

// Step 1 of design-single-channel.md bring-up: validate that the
// window-counter event link works.
//
// Path under test:
//   TCC0_OVF (GCLK1 = 24 MHz, PER=63 -> 375 kHz, 1-cycle pulse)
//     -> EVSYS CH0 ASYNC
//       -> TC0+TC1 COUNT32 EVU, EVACT=COUNT, NFRQ free-running.
//
// Iteration 2: to remove the OSC48M tolerance from the measurement, the
// test switches GCLK0 (CPU/SysTick/SERCOM5/TC0 clock) from OSC48M to the
// 24 MHz crystal before measuring, and uses SysTick directly via
// Timebase::micros() (sub-ms resolution, ~42 ns granularity at 24 MHz).
// Source and time-base are then driven by the same crystal, so any
// remaining residual is the crystal's intrinsic accuracy (<~50 ppm) plus
// the µs-quantization of micros().

namespace WindowEvlinkTest {

static constexpr uint32_t F_CPU_XOSC = 24000000u;
static constexpr uint32_t UART_BAUD  = 1000000u;

static inline void switch_cpu_to_xosc(void)
{
    Serial.flush();

    // Disable SERCOM5 before its clock source changes underneath it.
    SERCOM5->USART.CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
    while (SERCOM5->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE)
        ;

    // Stop SysTick while we change its input clock.
    SysTick->CTRL = 0;

    // Repoint GCLK0: OSC48M -> XOSC (24 MHz, already up via QuartzTest).
    GCLK->GENCTRL[0].reg =
        GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC_Val) | GCLK_GENCTRL_GENEN;
    while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL_GCLK0)
        ;

    // Reconfigure SysTick so one tick = 1 real-ms at the new CPU rate.
    // Timebase::micros() reads LOAD at runtime, so it adapts automatically.
    SysTick->LOAD = (F_CPU_XOSC / 1000u) - 1u;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;

    // Recompute SERCOM5 BAUD for the new core clock; same fractional formula
    // used by UartINT::init.
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

static inline uint32_t read_tc0_count(void)
{
    TC0->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_COUNT)
        ;
    return TC0->COUNT32.COUNT.reg;
}

static inline void run(void)
{
    Serial.print("\r\nStep 1 rev2: window event-link, CPU on XOSC, micros() timebase");
    Serial.newline();
    Serial.print("  TCC0 NPWM PER=63 CC[2]=32 GCLK1=24 MHz -> OVF @ 375 kHz, PA10 scope ref");
    Serial.newline();
    Serial.print("  TC0+TC1 COUNT32 NFRQ GCLK0=24 MHz (XOSC), EVACT=COUNT, free-running");
    Serial.newline();
    Serial.print("  EVSYS CH0 ASYNC gen=TCC0_OVF(35) -> TC0_EVU(23)");
    Serial.newline();

    switch_cpu_to_xosc();

    Serial.print("CPU clock switched to XOSC 24 MHz; SysTick + UART rebaud OK");
    Serial.newline();

    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0
                       | MCLK_APBCMASK_TC0
                       | MCLK_APBCMASK_TC1
                       | MCLK_APBCMASK_EVSYS;

    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;
    GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[EVSYS_GCLK_ID_0].reg & GCLK_PCHCTRL_CHEN))
        ;

    // PA10 mux F = TCC0/WO[2] for scope visibility of the source signal.
    PORT->Group[0].PINCFG[10].reg = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[5].bit.PMUXE = MUX_PA10F_TCC0_WO2;

    EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(1);
    EVSYS->CHANNEL[0].reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    // SAMC21 keeps WAVEGEN in a separate WAVE register, unlike SAMD21
    // (the §9 snippet of the design doc follows the SAMD21 convention).
    TC0->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
        ;
    TC0->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
    TC0->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_NFRQ;
    TC0->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;

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

    // ENABLE then RETRIGGER: when EVACT != OFF, ENABLE alone leaves
    // STATUS.STOP set and events are silently dropped (§14 finding #7).
    TC0->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE)
        ;
    TC0->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
    while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB)
        ;

    // Source enabled last so TC0 cannot miss the first edge.
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;

    // Baseline micros() right after TCC0 enable so the residual setup
    // latency (a handful of µs) does not bias the very first sample.
    const uint32_t t0 = Timebase::micros();

    // Sample at fixed absolute deadlines relative to t0.  Using absolute
    // targets makes every sample independent of the previous wait's rounding.
    auto wait_until = [&](uint32_t target_us) {
        while ((Timebase::micros() - t0) < target_us)
            ;
    };

    wait_until(10000u);   uint32_t c_10ms  = read_tc0_count();
    uint32_t s_10ms  = Timebase::micros() - t0;
    wait_until(50000u);   uint32_t c_50ms  = read_tc0_count();
    uint32_t s_50ms  = Timebase::micros() - t0;
    wait_until(100000u);  uint32_t c_100ms = read_tc0_count();
    uint32_t s_100ms = Timebase::micros() - t0;
    wait_until(200000u);  uint32_t c_200ms = read_tc0_count();
    uint32_t s_200ms = Timebase::micros() - t0;
    wait_until(500000u);  uint32_t c_500ms = read_tc0_count();
    uint32_t s_500ms = Timebase::micros() - t0;

    auto report = [&](const char* label, uint32_t target_us,
                      uint32_t actual_us, uint32_t count)
    {
        // Expected = 0.375 events/µs × actual_us = (3 * actual_us + 4) / 8.
        // Use the actual elapsed µs (which carries the µs quantization)
        // rather than the nominal target, so the comparison is fair.
        uint32_t expected = (3u * actual_us) / 8u;
        Serial.print("  ");
        Serial.print(label);
        Serial.print(": target=");
        Serial.print(target_us);
        Serial.print(" us  actual=");
        Serial.print(actual_us);
        Serial.print(" us  count=");
        Serial.print(count);
        Serial.print("  expected=");
        Serial.print(expected);
        int32_t diff = (int32_t)count - (int32_t)expected;
        Serial.print("  diff=");
        if (diff < 0) { Serial.print("-"); diff = -diff; }
        else          { Serial.print("+"); }
        Serial.print((uint32_t)diff);
        Serial.newline();
    };

    Serial.print("TC0+TC1 results (count expected at 0.375 events/us = 375 kHz crystal):");
    Serial.newline();
    report(" 10 ms", 10000u,  s_10ms,  c_10ms);
    report(" 50 ms", 50000u,  s_50ms,  c_50ms);
    report("100 ms", 100000u, s_100ms, c_100ms);
    report("200 ms", 200000u, s_200ms, c_200ms);
    report("500 ms", 500000u, s_500ms, c_500ms);

    bool span_ok   = c_500ms > c_200ms && c_200ms > c_100ms
                  && c_100ms > c_50ms && c_50ms  > c_10ms;
    bool above_16b = c_200ms > 65535u;

    Serial.print("RESULT: ");
    if (span_ok && above_16b) {
        Serial.print("OK - monotonic, exceeds 16-bit; residual diff should be in single digits");
    } else {
        Serial.print("FAIL - span_ok=");
        Serial.print(static_cast<uint32_t>(span_ok));
        Serial.print(" above_16b=");
        Serial.print(static_cast<uint32_t>(above_16b));
    }
    Serial.newline();
}

} // namespace WindowEvlinkTest

#endif // _WINDOW_EVLINK_TEST_HPP_
