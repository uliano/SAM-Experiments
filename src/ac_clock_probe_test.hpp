#ifndef _AC_CLOCK_PROBE_TEST_HPP_
#define _AC_CLOCK_PROBE_TEST_HPP_

#include <stdint.h>

#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"

// AC clock/output-path bench probe.
//
// Purpose:
//   Separate three questions that are easy to mix together:
//   1. Does AC enable/READY need GCLK_AC?
//   2. Does OUT_ASYNC bypass GCLK_AC retiming on the CMP pin?
//   3. Does the CCL INSEL=AC path keep following COMP0 when GCLK_AC is stopped
//      after COMP0 is already ready?
//
// Scope wiring:
//   Jumper PA09 -> PA05, or drive PA05 with the external analog test signal.
//
//   PA05 = AC AIN1 / COMP0 positive input.
//   PA09 = TCC0/WO1 375 kHz 50% digital stimulus.
//   PA12 = AC CMP0 direct pad output.
//   PA19 = CCL LUT0 OUT, internal COMP0 pass-through.
//   PA08 = optional TCC0/WO0 375 kHz 1/8-duty marker.
//
// Scope channels:
//   CH input/reference: PA05 input and/or PA09 stimulus.
//   CH direct AC:       PA12, direct AC/CMP0 pad output.
//   CH internal path:   PA19, CCL LUT0 pass-through of COMP0.
//   Optional marker:    PA08.
//
// Automatic phase sequence, 30 s per phase, repeating forever:
//   1. OUT_SYNC,  GCLK_AC = 375 kHz.
//   2. OUT_ASYNC, GCLK_AC = 375 kHz.
//   3. OUT_ASYNC, GCLK_AC stopped after COMP0 READY0.
//   4. OUT_SYNC,  GCLK_AC = 48 MHz.
//   5. OUT_ASYNC, GCLK_AC = 48 MHz.
//
// At boot, the test also runs a preflight with GCLK_AC disabled during COMP0
// enable. The serial line reports whether AC global enable, COMP0 enable, and
// READY0 complete or time out. This separates "GCLK_AC required for AC setup"
// from "OUT_ASYNC propagation depends on GCLK_AC".
//
// QuartzTest::init() must run first so GCLK1=24 MHz and GCLK2=375 kHz exist.

namespace AcClockProbeTest {

static constexpr uint8_t  kScalerValue = 31;     // VDD * 32/64 = VDD/2
static constexpr uint32_t kPhaseMs     = 30000;  // scope time per phase
static constexpr uint32_t kTimeoutMs   = 20;

enum class AcClock : uint8_t {
    Off,
    Gclk2_375k,
    Gclk0_48m,
};

enum class AcOutput : uint8_t {
    Async,
    Sync,
};

static inline uint32_t now_ms()
{
    return Timebase::millis();
}

static inline void delay_ms(uint32_t ms)
{
    const uint32_t start = now_ms();
    while ((now_ms() - start) < ms)
        ;
}

static inline bool wait_until_clear(volatile uint32_t& reg, uint32_t mask, uint32_t timeout_ms)
{
    const uint32_t start = now_ms();
    while (reg & mask) {
        if ((now_ms() - start) >= timeout_ms)
            return false;
    }
    return true;
}

static inline bool wait_ac_sync(uint32_t mask)
{
    return wait_until_clear(AC->SYNCBUSY.reg, mask, kTimeoutMs);
}

static inline bool wait_ac_ready0(uint32_t timeout_ms)
{
    const uint32_t start = now_ms();
    while (!(AC->STATUSB.reg & AC_STATUSB_READY0)) {
        if ((now_ms() - start) >= timeout_ms)
            return false;
    }
    return true;
}

static inline void print_bool(const char* name, bool value)
{
    Serial.print(name);
    Serial.print(value ? "=yes" : "=no");
}

static inline void print_ac_status()
{
    const bool ready = (AC->STATUSB.reg & AC_STATUSB_READY0) != 0;
    const bool state = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;
    const bool chen  = (GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN) != 0;

    print_bool(" gclk", chen);
    print_bool(" ready", ready);
    Serial.print(" state=");
    Serial.print(state ? "1" : "0");
    Serial.print(" syncbusy=0x");
    Serial.print(AC->SYNCBUSY.reg, PrintBase::Hex);
}

static inline void set_ac_clock(AcClock clock)
{
    uint32_t value = 0;

    if (clock == AcClock::Gclk2_375k) {
        value = GCLK_PCHCTRL_GEN_GCLK2 | GCLK_PCHCTRL_CHEN;
    } else if (clock == AcClock::Gclk0_48m) {
        value = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    }

    GCLK->PCHCTRL[AC_GCLK_ID].reg = value;

    if (value != 0) {
        while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
            ;
    } else {
        while (GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN)
            ;
    }
}

static inline void configure_pins()
{
    // PA08: TCC0/WO0, mux E, optional 1/8 marker.
    PORT->Group[0].DIRSET.reg        = (1u << 8);
    PORT->Group[0].PINCFG[8].reg     = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[4].bit.PMUXE = 4;

    // PA09: TCC0/WO1, mux E, 50% stimulus. Jumper this to PA05.
    PORT->Group[0].DIRSET.reg        = (1u << 9);
    PORT->Group[0].PINCFG[9].reg     = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[4].bit.PMUXO = 4;

    // PA05: COMP0 AIN1, mux B.
    PORT->Group[0].DIRCLR.reg        = (1u << 5);
    PORT->Group[0].PINCFG[5].reg     = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[2].bit.PMUXO = 1;

    // PA12: AC/CMP0 direct output, mux H.
    PORT->Group[0].DIRSET.reg        = (1u << 12);
    PORT->Group[0].PINCFG[12].reg    = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[6].bit.PMUXE = 7;

    // PA19: CCL LUT0 OUT, mux I.
    PORT->Group[0].DIRSET.reg        = (1u << 19);
    PORT->Group[0].PINCFG[19].reg    = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PMUX[9].bit.PMUXO = 8;
}

static inline void configure_tcc0_stimulus()
{
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0;

    GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;

    TCC0->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST)
        ;

    TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1;
    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE)
        ;

    TCC0->PER.reg = 63u;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_PER)
        ;

    TCC0->CC[0].reg = 8u;   // PA08 marker: 1/8 duty.
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC0)
        ;

    TCC0->CC[1].reg = 32u;  // PA09 stimulus: 50% duty.
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_CC1)
        ;

    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE)
        ;
}

static inline void configure_ccl_ac_passthrough()
{
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;

    GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN))
        ;

    CCL->CTRL.reg = 0;
    CCL->SEQCTRL[0].reg = 0;
    CCL->SEQCTRL[1].reg = 0;

    // IN2=AC(COMP0), IN1=0, IN0=0, TRUTH bit 4 set => OUT=IN2.
    CCL->LUTCTRL[0].reg =
        (0x10u << CCL_LUTCTRL_TRUTH_Pos) |
        (0x5u  << CCL_LUTCTRL_INSEL2_Pos) |
        (0x0u  << CCL_LUTCTRL_INSEL1_Pos) |
        (0x0u  << CCL_LUTCTRL_INSEL0_Pos) |
        CCL_LUTCTRL_ENABLE;

    CCL->LUTCTRL[1].reg = 0;
    CCL->LUTCTRL[2].reg = 0;
    CCL->LUTCTRL[3].reg = 0;
    CCL->CTRL.reg = CCL_CTRL_ENABLE;
}

static inline bool reset_ac_with_clock(AcClock clock)
{
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC;
    set_ac_clock(clock);

    AC->CTRLA.reg = AC_CTRLA_SWRST;
    return wait_ac_sync(AC_SYNCBUSY_SWRST);
}

static inline bool configure_comp0(AcOutput output)
{
    const uint32_t out_bits = (output == AcOutput::Async)
        ? AC_COMPCTRL_OUT_ASYNC
        : AC_COMPCTRL_OUT_SYNC;

    AC->COMPCTRL[0].reg &= ~AC_COMPCTRL_ENABLE;
    if (!wait_ac_sync(AC_SYNCBUSY_COMPCTRL0))
        return false;

    AC->SCALER[0].reg = kScalerValue;
    AC->COMPCTRL[0].reg =
        AC_COMPCTRL_MUXPOS_PIN1 |
        AC_COMPCTRL_MUXNEG_VSCALE |
        AC_COMPCTRL_SPEED_HIGH |
        out_bits;
    if (!wait_ac_sync(AC_SYNCBUSY_COMPCTRL0))
        return false;

    AC->CTRLA.reg |= AC_CTRLA_ENABLE;
    if (!wait_ac_sync(AC_SYNCBUSY_ENABLE))
        return false;

    AC->COMPCTRL[0].reg |= AC_COMPCTRL_ENABLE;
    if (!wait_ac_sync(AC_SYNCBUSY_COMPCTRL0))
        return false;

    return wait_ac_ready0(50);
}

static inline void print_scope_setup()
{
    Serial.print("\r\nAC clock/output probe");
    Serial.newline();
    Serial.print("Jumper PA09 -> PA05, or drive PA05 with your analog test signal.");
    Serial.newline();
    Serial.print("Scope: PA05 input, PA09 stimulus, PA12 CMP0 direct, PA19 CCL(COMP0).");
    Serial.newline();
    Serial.print("PA08 is optional 1/8-duty marker. Threshold = VDD/2 via AC scaler.");
    Serial.newline();
    Serial.print("Each phase lasts ");
    Serial.print(kPhaseMs / 1000u);
    Serial.print(" s and repeats forever.");
    Serial.newline();
}

static inline void enable_without_gclk_probe()
{
    Serial.print("\r\nPreflight: try enabling COMP0 with GCLK_AC disabled");
    Serial.newline();

    bool reset_ok = reset_ac_with_clock(AcClock::Gclk0_48m);
    Serial.print(" reset_with_gclk=");
    Serial.print(reset_ok ? "ok" : "timeout");
    Serial.newline();

    set_ac_clock(AcClock::Off);

    AC->SCALER[0].reg = kScalerValue;
    AC->COMPCTRL[0].reg =
        AC_COMPCTRL_MUXPOS_PIN1 |
        AC_COMPCTRL_MUXNEG_VSCALE |
        AC_COMPCTRL_SPEED_HIGH |
        AC_COMPCTRL_OUT_ASYNC;

    AC->CTRLA.reg |= AC_CTRLA_ENABLE;
    const bool global_ok = wait_ac_sync(AC_SYNCBUSY_ENABLE);

    AC->COMPCTRL[0].reg |= AC_COMPCTRL_ENABLE;
    const bool comp_ok = wait_ac_sync(AC_SYNCBUSY_COMPCTRL0);
    const bool ready_ok = wait_ac_ready0(kTimeoutMs);

    Serial.print(" enable_global=");
    Serial.print(global_ok ? "completed" : "timeout");
    Serial.print(" enable_comp0=");
    Serial.print(comp_ok ? "completed" : "timeout");
    Serial.print(" ready0=");
    Serial.print(ready_ok ? "yes" : "no");
    print_ac_status();
    Serial.newline();

    Serial.print("Re-enable GCLK_AC to recover");
    Serial.newline();
    set_ac_clock(AcClock::Gclk0_48m);
    wait_ac_sync(AC_SYNCBUSY_ENABLE | AC_SYNCBUSY_COMPCTRL0);
    reset_ac_with_clock(AcClock::Gclk0_48m);
}

static inline const char* output_name(AcOutput output)
{
    return (output == AcOutput::Async) ? "OUT_ASYNC" : "OUT_SYNC";
}

static inline const char* clock_name(AcClock clock)
{
    if (clock == AcClock::Gclk2_375k)
        return "GCLK2 375 kHz";
    if (clock == AcClock::Gclk0_48m)
        return "GCLK0 48 MHz";
    return "GCLK_AC OFF";
}

static inline void print_phase_header(uint32_t index, const char* title,
                                      AcClock clock, AcOutput output)
{
    Serial.print("\r\nPHASE ");
    Serial.print(index);
    Serial.print(": ");
    Serial.print(title);
    Serial.newline();
    Serial.print("AC clock: ");
    Serial.print(clock_name(clock));
    Serial.print("  output: ");
    Serial.print(output_name(output));
    Serial.newline();
    Serial.print("Measure PA12 and PA19 delay from PA05/PA09 edge.");
    Serial.newline();
}

static inline void run_timed_phase(uint32_t index, const char* title,
                                   AcClock clock, AcOutput output,
                                   bool disable_clock_after_ready)
{
    print_phase_header(index, title, clock, output);

    bool ok = reset_ac_with_clock(clock == AcClock::Off ? AcClock::Gclk0_48m : clock);
    ok = ok && configure_comp0(output);
    configure_ccl_ac_passthrough();

    if (disable_clock_after_ready)
        set_ac_clock(AcClock::Off);

    Serial.print("Configured:");
    Serial.print(ok ? " ok" : " FAILED");
    print_ac_status();
    Serial.newline();
    Serial.flush();

    const uint32_t start = now_ms();
    uint32_t last_print = start;
    bool last_state = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;
    uint32_t toggles = 0;

    while ((now_ms() - start) < kPhaseMs) {
        const bool state = (AC->STATUSA.reg & AC_STATUSA_STATE0) != 0;
        if (state != last_state) {
            last_state = state;
            ++toggles;
        }

        const uint32_t now = now_ms();
        if ((now - last_print) >= 5000u) {
            last_print = now;
            Serial.print("  t=");
            Serial.print((now - start) / 1000u);
            Serial.print("s status_toggles=");
            Serial.print(toggles);
            print_ac_status();
            Serial.newline();
        }
    }
}

static inline void run(void)
{
    print_scope_setup();

    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0 | MCLK_APBCMASK_CCL | MCLK_APBCMASK_AC;

    configure_pins();
    configure_tcc0_stimulus();
    configure_ccl_ac_passthrough();
    enable_without_gclk_probe();

    for (;;) {
        run_timed_phase(1, "SYNC at slow AC clock", AcClock::Gclk2_375k, AcOutput::Sync, false);
        run_timed_phase(2, "ASYNC at same slow AC clock", AcClock::Gclk2_375k, AcOutput::Async, false);
        run_timed_phase(3, "ASYNC after GCLK_AC is stopped", AcClock::Gclk2_375k, AcOutput::Async, true);
        run_timed_phase(4, "SYNC at fast AC clock", AcClock::Gclk0_48m, AcOutput::Sync, false);
        run_timed_phase(5, "ASYNC at fast AC clock", AcClock::Gclk0_48m, AcOutput::Async, false);
    }
}

} // namespace AcClockProbeTest

#endif // _AC_CLOCK_PROBE_TEST_HPP_
