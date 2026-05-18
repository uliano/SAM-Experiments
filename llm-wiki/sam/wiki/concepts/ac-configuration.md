---
title: AC Configuration
type: concept
tags: [ac, comparator, analog, firmware, samc21]
sources: [samc21-datasheet-ch40-ac, samc21-errata]
created: 2026-05-05
updated: 2026-05-18
---

# AC Configuration

Analog Comparator (AC) on the SAM C21: four comparators in two pairs.
Each comparator is independently configurable for continuous or single-shot
operation, with optional digital filter, hysteresis, window mode, and VDD scaler.

## Basic Init — COMP0, Continuous, AIN1 vs VDD/2

```cpp
// COMP0: AIN1 positive, VDD scaler negative (VDD/2 = 64 levels → VALUE=31)
// Continuous mode, high speed, asynchronous output for low-latency routing

void ac_init(void) {
    // 1. Clocks — AC is on APBC (not APBB)
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC;
    GCLK->PCHCTRL[AC_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

    // 2. Reset
    AC->CTRLA.reg = AC_CTRLA_SWRST;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_SWRST);

    // 3. VDD scaler: VALUE=31 → (31+1)/64 × VDD = VDD/2
    AC->SCALER[0].reg = 31;

    // 4. Configure COMP0
    // OUT_ASYNC (0x1): raw comparator output. Required for low-latency DFF/CCL use.
    //   OUT_SYNC (0x2) also works for CCL but adds GCLK_AC-domain latency.
    //   OUT=OFF (0x0) prevents CCL from reading the comparator.
    // Configure all bits before enabling the comparator.
    AC->COMPCTRL[0].reg =
        AC_COMPCTRL_MUXPOS_PIN1       // AIN1 as positive input
        | AC_COMPCTRL_MUXNEG_VSCALE   // VDD scaler as negative input
        | AC_COMPCTRL_SPEED_HIGH       // fastest response (~40 ns propagation)
        | AC_COMPCTRL_OUT_ASYNC;       // raw output, not retimed by GCLK_AC

    // 5. Enable AC global, then individual comparator
    AC->CTRLA.reg |= AC_CTRLA_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_ENABLE);

    AC->COMPCTRL[0].reg |= AC_COMPCTRL_ENABLE;
    while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0);

    // 6. Wait for comparator to be ready (startup time elapsed)
    while (!(AC->STATUSB.reg & AC_STATUSB_READY0));
}
```

## Continuous vs Single-Shot Mode

**Continuous** (`SINGLE=0`): comparator always running. New sample captured every
GCLK_AC cycle. `STATUSA.STATEx` always holds the latest result.
`STATUSB.READYx` is set after the initial startup delay and stays set.
Hysteresis (`HYSTEN`) is only available in continuous mode.

**Single-shot** (`SINGLE=1`): comparator idle by default. One comparison is triggered
by writing `CTRLB.STARTx = 1` or by an EVSYS input event (`EVCTRL.COMPEIx = 1`).
After startup + comparison, `STATUSB.READYx` is set and the result is in
`STATUSA.STATEx`. No filter is applied in single-shot mode.

```cpp
// COMP1 single-shot triggered by software
AC->COMPCTRL[1].reg =
    AC_COMPCTRL_MUXPOS_PIN2         // AIN2 as positive input
    | AC_COMPCTRL_MUXNEG_DAC        // DAC output as reference
    | AC_COMPCTRL_SINGLE            // single-shot mode
    | AC_COMPCTRL_INTSEL_EOC        // interrupt at end of comparison
    | AC_COMPCTRL_SPEED_HIGH
    | AC_COMPCTRL_OUT_ASYNC;
AC->COMPCTRL[1].reg |= AC_COMPCTRL_ENABLE;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL1);

// Trigger comparison:
AC->CTRLB.reg = AC_CTRLB_START1;

// Wait or use interrupt:
while (!(AC->INTFLAG.reg & AC_INTFLAG_COMP1));
AC->INTFLAG.reg = AC_INTFLAG_COMP1;
bool result = (AC->STATUSA.reg & AC_STATUSA_STATE1) != 0;
```

## Output Routing (COMPCTRLn.OUT)

Controls what signal reaches the CMP[x] I/O pin and — critically — what the CCL sees.

| OUT value | Name  | Pin signal                          | CCL INSEL=AC |
|-----------|-------|-------------------------------------|--------------|
| 0x0       | OFF   | Not routed to pin                   | **Does not work** |
| 0x1       | ASYNC | Raw asynchronous comparator output  | Works |
| 0x2       | SYNC  | GCLK_AC-synchronized output (+ filter if FLEN≠0) | Works |

**For CCL INSEL=AC, OUT must be 0x1 or 0x2** (datasheet §40.8.12 note).
OUT=OFF silently prevents the CCL from seeing the comparator.

**Project rule:** the dual-channel heartbeat/DFF design uses `OUT_ASYNC` only.
`OUT_SYNC` was measured to add up to two `GCLK_AC` cycles of lag on the Rev F
board and is unacceptable for that sampling path. `GCLK_AC` is still required by
the AC peripheral for register/status operation before use.

**Latency:**
- `OUT_ASYNC`: only comparator propagation delay (see Electrical Characteristics;
  ~40–100 ns depending on SPEED). Output changes asynchronously, not clock-aligned.
- `OUT_SYNC`: comparator output is synchronized to GCLK_AC (§40.6.9). The datasheet
  describes it as a "CLK_AC-synchronized version" but does not document the internal
  pipeline depth. §40.6.2.4.1 "sampling rate is the GCLK_AC frequency" refers to
  interrupt edge detection rate, not to the OUT_SYNC output latency.
  **Experimentally verified (scope, 375 kHz, SAM C21 Rev F)**: OUT_SYNC uses a
  2-FF synchronizer — not a single latch. Latency = **1–2 GCLK_AC periods**:
  minimum 1 period (crossing just before a GCLK_AC edge, FF1 captures immediately);
  maximum 2 periods (crossing just after an edge, FF1 misses it and captures one
  cycle later). Proven by switching to OUT_ASYNC: lag dropped to ~40–100 ns
  (propagation delay only), confirming the 1–2 period lag is entirely from the
  2-FF synchronizer. With FLEN=MAJn: total = **(1–2) + (N−1) = N to N+1 periods**.

## Digital Filter (COMPCTRLn.FLEN)

Reduces noise on the comparator output at the cost of latency.
Applied before the synchronizer; applies to OUT_SYNC output only.
Not available in single-shot mode. Not available during sleep with continuous mode.

| FLEN value | Name  | Rule               | Additional latency  |
|------------|-------|--------------------|---------------------|
| 0x0        | OFF   | No filtering       | 0                   |
| 0x1        | MAJ3  | 2-of-3 majority    | +2 GCLK_AC cycles   |
| 0x2        | MAJ5  | 3-of-5 majority    | +4 GCLK_AC cycles   |

Total latency with OUT_SYNC = (1–2) (2-FF sync) + (N−1) (filter) = N to N+1 GCLK_AC periods.

## Window Mode (COMP0 + COMP1 as pair)

```cpp
// Window: shared positive input, COMP0 = upper threshold, COMP1 = lower threshold
// WSTATE0: ABOVE(0x0)/INSIDE(0x1)/BELOW(0x2) in STATUSA.WSTATE0

AC->SCALER[0].reg = 40;   // high threshold: 41/64 × VDD
AC->SCALER[1].reg = 20;   // low threshold:  21/64 × VDD

AC->COMPCTRL[0].reg =
    AC_COMPCTRL_MUXPOS_PIN0 | AC_COMPCTRL_MUXNEG_VSCALE
    | AC_COMPCTRL_OUT_ASYNC | AC_COMPCTRL_ENABLE;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0);

AC->COMPCTRL[1].reg =
    AC_COMPCTRL_MUXPOS_PIN0 | AC_COMPCTRL_MUXNEG_VSCALE
    | AC_COMPCTRL_OUT_ASYNC | AC_COMPCTRL_ENABLE;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL1);

AC->WINCTRL.reg = AC_WINCTRL_WEN0;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_WINCTRL);

AC->INTENSET.reg = AC_INTENSET_WIN0;
```

## Wakeup from STANDBY (async, no GCLK)

```cpp
// COMP0 wakes the CPU from STANDBY when AIN0 exceeds threshold.
// In standby with GCLK_AC stopped, AC works asynchronously.
// When a toggle occurs, GCLK_AC is started temporarily to register the event.
// Filtering is NOT supported in this mode (FLEN must be OFF).
AC->COMPCTRL[0].reg |= AC_COMPCTRL_RUNSTDBY;
while (AC->SYNCBUSY.reg & AC_SYNCBUSY_COMPCTRL0);
```

## MUXPOS / MUXNEG Reference

COMP0/COMP1 (pair 0) use AIN0–AIN3. COMP2/COMP3 (pair 1) use AIN4–AIN7.

| MUXPOS value | COMP0/1 pin | COMP2/3 pin |
|--------------|-------------|-------------|
| 0x0 (PIN0)   | AIN0        | AIN4        |
| 0x1 (PIN1)   | AIN1        | AIN5        |
| 0x2 (PIN2)   | AIN2        | AIN6        |
| 0x3 (PIN3)   | AIN3        | AIN7        |
| 0x4 (VSCALE) | VDD scaler  | VDD scaler  |

| MUXNEG value    | COMP0/1 pin | COMP2/3 pin |
|-----------------|-------------|-------------|
| 0x0 (PIN0)      | AIN0        | AIN4        |
| 0x1 (PIN1)      | AIN1        | AIN5        |
| 0x2 (PIN2)      | AIN2        | AIN6        |
| 0x3 (PIN3)      | AIN3        | AIN7        |
| 0x4 (GND)       | Ground      | Ground      |
| 0x5 (VSCALE)    | VDD scaler  | VDD scaler  |
| 0x6 (INTREF)    | Internal bandgap reference (level set by SUPC.VREF.SEL) ||
| 0x7 (DAC)       | DAC output  | DAC output  |

## VDD Scaler

One independent scaler per comparator (SCALERn registers, offset 0x0C+n).
Output voltage: `V_SCALE = VDD × (VALUE+1) / 64`
Range: VDD/64 (VALUE=0) to VDD (VALUE=63). VALUE=31 → VDD/2.

## Key Facts

- Four comparators: COMP0/COMP1 (pair 0), COMP2/COMP3 (pair 1).
- AC is on **APBC** bus: `MCLK->APBCMASK |= MCLK_APBCMASK_AC` (not APBB).
- **Enable-protection is at two levels**:
  - `EVCTRL` is enable-protected by `CTRLA.ENABLE` (AC global enable).
  - COMPCTRL bits (MUXPOS, MUXNEG, SPEED, FLEN, SWAP, SINGLE, INTSEL) are
    protected by `COMPCTRLn.ENABLE` — configure before enabling the comparator.
- Write-synchronized: `CTRLA.SWRST`, `CTRLA.ENABLE`, `COMPCTRLn.ENABLE`, `WINCTRL`.
  Always poll SYNCBUSY after writing these.
- COMPCTRLn bits marked "not synchronized" (OUT, FLEN, SPEED, etc.) do not require
  a SYNCBUSY wait when written while the comparator is disabled.
- **For CCL INSEL=AC: `COMPCTRLn.OUT` must be 0x1 (ASYNC) or 0x2 (SYNC).**
  OUT=0x0 (OFF) prevents the CCL from reading the comparator output.
- Project heartbeat/DFF path: use `OUT_ASYNC` only. Do not use `OUT_SYNC` in
  that path; its measured latency is too high for the external sampling edge.
- OUT_SYNC: CLK_AC-synchronized output (§40.6.9). Internally uses a **2-FF synchronizer**
  (not documented in datasheet; experimentally verified on Rev F). Latency = **1–2
  GCLK_AC periods** with FLEN=OFF (1 period if crossing just before edge; 2 periods
  if just after). Proven by OUT_ASYNC comparison: lag dropped to ~40–100 ns.
  With FLEN=MAJn: total = N to N+1 periods.
- OUT_ASYNC: output follows comparator asynchronously; latency = propagation
  delay only (see Electrical Characteristics; ~40–100 ns at SPEED_HIGH).
- Hysteresis (`HYSTEN`) only works in continuous mode (SINGLE=0).
- FLEN filter: N−1 additional GCLK_AC cycles delay; not available in single-shot
  mode; not available in continuous sleepwalking mode (RUNSTDBY=1, GCLK stopped).
- SWAP swaps MUXPOS/MUXNEG for offset compensation; configure only while disabled.
- `STATUSB.READYx`: set after startup delay; must be 1 before reading `STATUSA.STATEx`.
- VDD scaler: 64 levels, `V_SCALE = VDD × (VALUE+1)/64`.
- Window WSTATE0/1: ABOVE=0x0, INSIDE=0x1, BELOW=0x2.
- Continuous sleepwalking (RUNSTDBY=1, GCLK_AC stopped): comparator runs async;
  on edge detection, GCLK_AC starts temporarily to register events/interrupts.
  Filtering must be disabled (`FLEN=OFF`) in this configuration.
- INTREF (MUXNEG=0x6): internal bandgap reference; voltage level set by SUPC.VREF.SEL.
  Current errata 1.5.6 can generate a spurious COMP interrupt when enabling AC
  with `MUXNEG=INTREF`; prefer `VSCALE` when suitable or clear/ignore the first
  COMP interrupt after enable.

## Interrupt Sources

- `COMP0`–`COMP3`: change in comparator status per `COMPCTRLn.INTSEL`
  (TOGGLE / RISING / FALLING / EOC for single-shot).
- `WIN0`, `WIN1`: window function change per `WINCTRL.WINTSELx`
  (ABOVE / INSIDE / BELOW / OUTSIDE window).

## Event Outputs (EVCTRL, enable-protected)

- `COMPEOx`: copy of comparator x output state (enable with `EVCTRL.COMPEOx`).
- `WINEOx`: copy of window x inside/outside state.
- `COMPEIx`: input event → starts a single-shot comparison on COMPx.

## See Also

- [[SAMC21 Datasheet Ch.40 AC]] — full register reference
- [[EIC]] — digital edge/level interrupt alternative
- [[EVSYS]] — event-triggered single-shot comparisons
- [[SUPC]] — supply voltage and bandgap reference (INTREF level)
- [[Clock System]] — GCLK_AC setup
- [[CCL Configuration]] — CCL INSEL=AC usage
