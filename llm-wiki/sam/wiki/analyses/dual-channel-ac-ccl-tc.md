---
title: Dual-Channel AC+CCL+TC Architecture
type: analysis
tags: [ac, ccl, tcc, tc, evsys, architecture, multislope, samc21j18a]
sources: [samc21-datasheet-ch37-ccl, samc21-datasheet-ch40-ac, samc21-datasheet-ch35-tc, samc21-datasheet-ch36-tcc, samc21-datasheet-ch29-evsys]
created: 2026-05-10
updated: 2026-05-18
---

# Dual-Channel AC+CCL+TC Architecture

> **DEPRECATED** — moved to `SAM-Experiments/design-dual-channel.md` (project root).
> This copy is out of date. Do not use it for current errata or AC output
> decisions; see [[SAMC20/C21 Errata (DS80000740S)]] and [[AC Configuration]].

Design document for a two-channel analog front-end using AC comparators, CCL LUTs,
and 32-bit TC pairs on the ATSAMC21J18A-AU. Each channel captures the high-time
fraction of an AC comparator output, gated by a common 375 kHz square-wave clock,
and steers a reference waveform based on comparator state.

---

## System Overview

```
                 ┌──────────────────────────────────────────────────────┐
                 │  TCC0 "Heartbeat"  375 kHz  (GCLK1 = 24 MHz)        │
                 │  WO[0] duty 1/8  ─── slow-ref                       │
                 │  WO[1] duty 7/8  ─── fast-ref                       │
                 │  WO[2] duty 1/2  ─── PIN_Clock (375 kHz square wave)│
                 └────────────┬────────────────────┬─────────────────────┘
                              │ PIN_Clock           │ WO[0], WO[1]
          ┌───────────────────┤                     │
          │                   ▼                     ▼
          │         Channel A                              Channel B
          │
  [PA05]──┼──► AC COMP0 ──► Pin_dig1_A ──► DFF_A.D         [PAxx]──┼──► AC COMP1 ──► Pin_dig1_B ──► DFF_B.D
  analog  │              (async ~50ns)    DFF_A.CLK ◄── PIN_Clock    analog  │         (async)       DFF_B.CLK ◄── PIN_Clock
          │                               DFF_A.Q = Pin_dig2_A                │                       DFF_B.Q = Pin_dig2_B
          │                                    │                              │                            │
          │                          ┌─────────┴─────────┐                   │                  ┌────────┴────────┐
          │                          │                   │                   │                  │                 │
          │                          ▼                   ▼                   │                  ▼                 ▼
          │                   LUT0: mux_A         LUT1: and_A                │           LUT3: mux_B      LUT2: and_B
          │                 Q ? WO1 : WO0       Q AND PIN_Clock              │         Q ? WO1 : WO0    Q AND PIN_Clock
          │                       │                    │                     │               │                │
          │                  Pin_dig3_A           Pin_dig4_A                 │          Pin_dig3_B       Pin_dig4_B
          │                  (ref steer)          (debug)                    │          (ref steer)      (debug)
          │                                       EVSYS→TC0+TC1              │                          EVSYS→TC2+TC3
          │
  [PA05]──┼──► ADC0 (same pin as AC input)                          [PAxx]──┼──► ADC0
          └────────────────────────────────────────────────────────────────────

    TCC1 "Window": counts N heartbeat periods → ISR at N−1, OVF at N + event output
```

**Key signal chain per channel:**

| Step | Signal | Source | Destination |
|------|--------|--------|-------------|
| 1 | Analog input | external | AC comparator + ADC |
| 2 | Pin_dig1 | AC ASYNC output (CMP[n] pad) | DFF.D input |
| 3 | PIN_Clock | TCC0/WO[2] (PA10, mux F) | DFF.CLK input |
| 4 | Pin_dig2 | DFF.Q output | LUT_mux (INSEL=IO) + LUT_and (INSEL=IO) |
| 5 | Pin_dig3 | LUT_mux output | reference steering (external) |
| 6 | Pin_dig4 | LUT_and output | debug pin + EVSYS → TC32 counter |

**Pin_dig1 ≠ Pin_dig2.** The DFF registers the AC output on PIN_Clock rising edges,
ensuring Pin_dig2 transitions only at clock edges — eliminating partial-pulse glitches
in the AND gate and providing a clean signal for TC counting.

---

## Global Resources

### GCLK Generators (existing, set up by QuartzTest)

| Generator | Source | Frequency | Used by |
|-----------|--------|-----------|---------|
| GCLK0 | OSC48M | 48 MHz | CPU, CCL |
| GCLK1 | XOSC | 24 MHz | TCC0, TCC1 |
| GCLK2 | XOSC/64 | 375 kHz | Historical only; current design uses AC `OUT_ASYNC`, not `OUT_SYNC` |

### TCC0 "Heartbeat" — PCHCTRL[28], GCLK1=24 MHz

| Register | Value | Result |
|----------|-------|--------|
| CTRLA.PRESCALER | DIV1 | clock = 24 MHz |
| PER | 63 | period = 64 cycles = 375 kHz |
| CC[0] | 8 | WO[0] HIGH while COUNT<8 → duty 1/8 (333 ns HIGH) |
| CC[1] | 56 | WO[1] HIGH while COUNT<56 → duty 7/8 (2333 ns HIGH) |
| CC[2] | 32 | WO[2] HIGH while COUNT<32 → duty 1/2 = **PIN_Clock** |
| WAVE | NPWM | normal PWM |

**Output pins:**

| Signal | Pin | Mux | Notes |
|--------|-----|-----|-------|
| WO[0] slow-ref 1/8 | PA08 | E (0x4) | PMUX[4].PMUXE=4 |
| WO[1] fast-ref 7/8 | PA09 | E (0x4) | PMUX[4].PMUXO=4 |
| WO[2] PIN_Clock 1/2 | **PA10** | **F (0x5)** | PMUX[5].PMUXE=5 — note mux F not E |

### TCC1 "Window" — PCHCTRL[28], GCLK1=24 MHz

Counts N heartbeat periods (N×64 GCLK1 cycles). Generates:
- Compare match at count (N−1)×64: ISR prepares for window end.
- Overflow at N×64: ISR reads TC counts + event output resets/restarts window.

| Register | Value | Notes |
|----------|-------|-------|
| PER | N×64 − 1 | window length (N heartbeat periods) |
| CC[0] | (N−1)×64 − 1 | compare match at N−1 heartbeat periods |
| WAVE | NPWM | — |
| INTENSET | OVF + MC[0] | overflow ISR + compare ISR |
| EVCTRL | OVFEO=1 | OVF event output (for downstream trigger if needed) |

No pin output required. TCC0 and TCC1 share PCHCTRL[28]; both run on GCLK1=24 MHz.
Their COUNT registers start independently — if phase alignment is needed, retrigger
TCC1 from TCC0 OVF via EVSYS (EVACT=RETRIGGER on TCC1 event input).

---

## Per-Channel Structure (×2)

### AC Comparator — OUT=ASYNC, SPEED=HIGH, FLEN=OFF

| | Channel A | Channel B |
|--|-----------|-----------|
| Comparator | **COMP0** (pair 0) | **COMP1** (pair 0) |
| GCLK_AC | GCLK2 = 375 kHz | GCLK2 = 375 kHz |
| Analog input pin | PA05 = AIN1 (mux B) | PA06 = AIN2 or PA07 = AIN3 (mux B) |
| MUXNEG | VSCALE or INTREF | VSCALE or INTREF |
| OUT mode | **ASYNC** | **ASYNC** |
| Lag AC→Pin_dig1 | ~40–100 ns (propagation only) | ~40–100 ns |
| Pin_dig1 (DFF.D) | CMP[0] → ⚠ verify pin (mux G/H) | CMP[1] → ⚠ verify pin (mux G/H) |
| ADC channel | ADC0/AIN1 = PA05 | ADC0/AIN2 = PA06 (or AIN3) |

**Comparator choice is now free**: because the CCL mux LUTs read Pin_dig2 via
INSEL=IO (physical DFF output pin), not via INSEL=AC (fixed COMP→LUT mapping).
The LUT→COMP constraint is removed. COMP0+COMP1 are used here (both pair 0,
convenient for shared VSCALE reference), but any two comparators work.

Both pair-0 comparators share the pair-0 VDD scaler: AC->SCALER[0] for COMP0,
AC->SCALER[1] for COMP1. Each has an independent SCALER register.

---

## CCL LUT Allocation — PCHCTRL[38], GCLK0=48 MHz

### LUT0 — `mux_A`  (Channel A reference steering)

| Slot | INSEL | Source | IN value |
|------|-------|--------|----------|
| INSEL2 | 0x4 (IO) | Pin_dig2_A (DFF_A.Q, physical pad → mux I) | Q_A |
| INSEL1 | 0x8 (TCC) | TCC0/WO[1] (experimentally verified) | fast-ref |
| INSEL0 | 0x8 (TCC) | TCC0/WO[0] (experimentally verified) | slow-ref |

**TRUTH = 0xCA** — `Q_A ? WO[1] : WO[0]`

```
IN[2]=AC  IN[1]=WO1  IN[0]=WO0  │  OUT
   0          0          0      │   0   (AC=0 → select WO0=0)
   0          0          1      │   1   (AC=0 → select WO0=1)
   0          1          0      │   0   (AC=0 → select WO0=0)
   0          1          1      │   1   (AC=0 → select WO0=1)
   1          0          0      │   0   (AC=1 → select WO1=0)
   1          0          1      │   0   (AC=1 → select WO1=0)
   1          1          0      │   1   (AC=1 → select WO1=1)
   1          1          1      │   1   (AC=1 → select WO1=1)
```

Output: → **Pin_dig3_A** (reference steering, drives external DAC/switch);
EVSYS LUTOUT0 if needed.

---

### LUT3 — `mux_B`  (Channel B reference steering)

Identical structure to LUT0. LUT3 also maps to TCC0 (TCC3 absent on J-variant,
LUT3 wraps to TCC0 — confirmed experimentally, see [[CCL Configuration]]).

| Slot | INSEL | Source | IN value |
|------|-------|--------|----------|
| INSEL2 | 0x4 (IO) | Pin_dig2_B (DFF_B.Q, physical pad → mux I) | Q_B |
| INSEL1 | 0x8 (TCC) | TCC0/WO[1] | fast-ref |
| INSEL0 | 0x8 (TCC) | TCC0/WO[0] | slow-ref |

**TRUTH = 0xCA** (identical to LUT0)

Output: → **Pin_dig3_B**

---

### LUT1 — `and_A`  (Channel A gated counter clock)

LUT1 natively maps to TCC1 and COMP1 — neither is needed here. Both inputs
are sourced from physical I/O pins via INSEL=IO (0x4).

| Slot | INSEL | Source | IN value |
|------|-------|--------|----------|
| INSEL2 | 0x4 (IO) | PIN_Clock loopback pin (⚠ see below) | CLK |
| INSEL1 | 0x0 (MASK) | constant 0 | — |
| INSEL0 | 0x4 (IO) | COMP0 output loopback pin (⚠ see below) | AC_A |

**TRUTH = 0x20** — `IN[2] AND IN[0]` = `CLK AND AC_A`

```
IN[2]=CLK  IN[1]=0  IN[0]=AC_A  │  OUT
   0          0         0       │   0
   0          0         1       │   0
   1          0         0       │   0
   1          0         1       │   1   ← only when CLK=1 AND AC_A=1
```

Output: → **Pin_dig4_A** (debug) + **EVSYS LUTOUT1** → TC0+TC1 COUNT event.

---

### LUT2 — `and_B`  (Channel B gated counter clock)

Identical structure to LUT1, using COMP3 and PIN_Clock loopback pins.

| Slot | INSEL | Source | IN value |
|------|-------|--------|----------|
| INSEL2 | 0x4 (IO) | PIN_Clock loopback pin (same trace, different CCL IO slot) | CLK |
| INSEL1 | 0x0 (MASK) | constant 0 | — |
| INSEL0 | 0x4 (IO) | COMP3 output loopback pin | AC_B |

**TRUTH = 0x20** (identical to LUT1)

Output: → **Pin_dig4_B** (debug) + **EVSYS LUTOUT2** → TC2+TC3 COUNT event.

---

### CCL LUT Summary

| LUT | Function | IN[2] source | IN[1] source | IN[0] source | TRUTH |
|-----|----------|-------------|-------------|-------------|-------|
| LUT0 | mux_A | Pin_dig2_A (IO) | TCC0/WO[1] (TCC) | TCC0/WO[0] (TCC) | 0xCA |
| LUT1 | and_A | PIN_Clock (IO) | MASK | Pin_dig2_A (IO) | 0x20 |
| LUT2 | and_B | PIN_Clock (IO) | MASK | Pin_dig2_B (IO) | 0x20 |
| LUT3 | mux_B | Pin_dig2_B (IO) | TCC0/WO[1] (TCC) | TCC0/WO[0] (TCC) | 0xCA |

All AC-derived signals enter via INSEL=IO (physical DFF output pads). The COMP→LUT
fixed mapping constraint does not apply — comparator choice is free.

---

## Physical Net Connections

The DFF is an **external component** (e.g. SN74LVC1G74 dual D-FF or equivalent),
one per channel. Its clock must be driven directly by PA10/TCC0/WO[2] so that
Pin_dig2 transitions are phase-aligned to PIN_Clock. Using GCLK2 (also 375 kHz)
as the DFF clock would introduce a fixed but unknown phase offset — unacceptable.

| Net | Source | Destination(s) |
|-----|--------|---------------|
| Pin_dig1_A | CMP[0] pad (⚠ verify, mux G/H) | DFF_A.D |
| Pin_dig1_B | CMP[1] pad (⚠ verify, mux G/H) | DFF_B.D |
| PIN_Clock | PA10 (TCC0/WO[2], mux F) | DFF_A.CLK, DFF_B.CLK, LUT1/INSEL2 IO, LUT2/INSEL2 IO |
| Pin_dig2_A | DFF_A.Q pad | LUT0/INSEL2 IO (mux_A), LUT1/INSEL0 IO (and_A) |
| Pin_dig2_B | DFF_B.Q pad | LUT3/INSEL2 IO (mux_B), LUT2/INSEL0 IO (and_B) |

⚠ **Action required**: verify exact CCL IO input pin assignments for all INSEL=IO
slots from Ch.37 Table 37-4 (per-pin mux, function I = 0x8).
Verify CMP[0] and CMP[1] output pins from Ch.5/Ch.6 (mux G or H).

---

## TC 32-Bit Counter Pairs

| | Channel A | Channel B |
|--|-----------|-----------|
| TC pair | **TC0+TC1** (PCHCTRL[30]) | **TC2+TC3** (PCHCTRL[31]) |
| Master | TC0 (COUNT32) | TC2 (COUNT32) |
| Slave | TC1 | TC3 |
| GCLK | GCLK1 = 24 MHz | GCLK1 = 24 MHz |
| Event input | LUTOUT1 via EVSYS | LUTOUT2 via EVSYS |
| EVACT | COUNT (increment on LUT event) | COUNT |

TC counts = number of PIN_Clock pulses (at 375 kHz) while AC was HIGH.
At window end (TCC1 OVF): ISR reads and resets TC0 and TC2 counts.

⚠ **TC2+TC3 errata §35.6.2.4**: avoid in new designs. Experimentally observed
to work in COUNT32 mode but not recommended. Verify on target silicon.

---

## EVSYS Channel Assignment

| EVSYS CH | Generator | Generator ID | User | User ID | Path |
|----------|-----------|-------------|------|---------|------|
| 0 | CCL LUTOUT1 | 83 | TC0 event input | EVSYS_ID_USER_TC0_EVU | ASYNC |
| 1 | CCL LUTOUT2 | 84 | TC2 event input | EVSYS_ID_USER_TC2_EVU | ASYNC |
| 2 | TCC0 OVF | — | TCC1 retrigger (optional) | EVSYS_ID_USER_TCC1_EV_0 | ASYNC |
| 3 | TCC1 OVF | — | spare (CPU event or TC reset) | — | — |

Use ASYNC path for LUTOUT→TC to minimize event latency.

---

## Pin Allocation Summary

### Committed (no conflicts)

| Pin | Function | Mux | Notes |
|-----|----------|-----|-------|
| PA08 | TCC0/WO[0] slow-ref 1/8 | E | scope ref |
| PA09 | TCC0/WO[1] fast-ref 7/8 | E | |
| PA10 | TCC0/WO[2] PIN_Clock 1/2 | **F** | loopback source for LUT1+LUT2 |
| PA05 | AC COMP0 AIN1 / ADC0 AIN1 | B | Channel A analog input |
| PBxx | AC COMP3 AIN4–7 / ADC AINx | B | Channel B analog input — ⚠ verify Ch.5 |
| PB30 | SERCOM5 TX | D | UART (existing) |
| PB31 | SERCOM5 RX | D | UART (existing) |
| PA14 | XOSC XIN | — | crystal |
| PA15 | XOSC XOUT | — | crystal |
| PA03 | VREF | — | 22 Ω + 22 nF, leave at reset |
| PA04 | VREF | — | 22 Ω + 22 nF, leave at reset |
| PB22 | Bench button | GPIO | |
| PB23 | Bench LED | GPIO | |

### To verify from Ch.5/Ch.6/Ch.37

| Signal | Notes |
|--------|-------|
| CMP[0] output pin | AC COMP0 debug out, Pin_dig1_A — mux G or H |
| CMP[3] output pin | AC COMP3 debug out, Pin_dig1_B — mux G or H |
| LUT0 OUT (mux I) | Pin_dig3_A — reference steering output for Channel A |
| LUT1 OUT (mux I) | Pin_dig4_A — debug + EVSYS source |
| LUT2 OUT (mux I) | Pin_dig4_B — debug + EVSYS source |
| LUT3 OUT (mux I) | Pin_dig3_B — reference steering output for Channel B |
| LUT1/INSEL2 IO pin | PIN_Clock loopback target for and_A clock input |
| LUT1/INSEL0 IO pin | COMP0 loopback target for and_A AC input |
| LUT2/INSEL2 IO pin | PIN_Clock loopback target for and_B clock input |
| LUT2/INSEL0 IO pin | COMP3 loopback target for and_B AC input |
| Channel B ADC pin | Same pin as COMP3 analog input — check ADC channel number |

---

## APB Clocks Required

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0
                    | MCLK_APBCMASK_TCC1
                    | MCLK_APBCMASK_TC0
                    | MCLK_APBCMASK_TC1
                    | MCLK_APBCMASK_TC2   // ⚠ errata §35.6.2.4
                    | MCLK_APBCMASK_TC3   // ⚠ errata
                    | MCLK_APBCMASK_AC
                    | MCLK_APBCMASK_CCL
                    | MCLK_APBCMASK_EVSYS;
```

---

## Open Items

1. **Channel B analog pin**: confirm which AIN4–7 pin maps to COMP3 MUXPOS and
   which ADC channel shares it. Check Ch.5 signal descriptions and Ch.6 mux table.
2. **CMP[n] output pins**: determine physical pins for CMP[0] and CMP[3] (function
   G or H). Required for PCB loopback traces to LUT1/LUT2 IO inputs.
3. **CCL IO pin table**: read Ch.37 Table 37-4 to identify the specific physical
   pins for INSEL=IO on each LUT/slot. Needed to route loopback traces.
4. **TC2+TC3 errata**: decide whether to accept TC2+TC3 or use an alternative
   (e.g., software timestamping via AC EVSYS capture on TC0/TC1 dual-capture).
5. **TCC1 phase alignment**: decide whether TCC1 needs retriggering from TCC0 OVF,
   or if free-running (both on same GCLK1) is accurate enough for the window timing.
6. **AC MUXNEG**: choose VSCALE value or INTREF level for each channel's threshold.
7. **SEQCTRL**: LUT0+LUT1 pair and LUT2+LUT3 pair share SEQCTRL — ensure SEQSEL
   remains 0x0 (disabled) for both pairs so the LUTs operate as pure combinational logic.

---

## See Also

- [[CCL Configuration]] — LUT INSEL values, TCC→LUT mapping, TRUTH table computation
- [[AC Configuration]] — COMP0/COMP3 init, OUT_ASYNC latency (~40–100 ns), pair mapping
- [[TCC Configuration]] — TCC0 NPWM, CC registers, WO output pins
- [[TC 32-Bit Paired Mode]] — TC0+TC1, TC2+TC3 COUNT32 mode, EVACT=COUNT
- [[EVSYS]] — channel setup, LUTOUT generator IDs (82–85), ASYNC path
- [[I/O Multiplexing]] — function letter map, TCC0 WO[2] on PA10 (mux F)
