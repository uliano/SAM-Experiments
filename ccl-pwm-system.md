# CCL PWM System Design

Tracks the architecture and open questions for the AC-gated PWM system. Update
in place as decisions are made.

> Status: historical design note. For the current dual-channel design and the
> current DS80000740S errata pass, use `design-dual-channel.md` and
> `llm-wiki/sam/wiki/sources/samc21-errata.md` as the authority.

---

## Requirements

1. **PWM source** — two signals at 375 kHz derived from the 24 MHz external
   crystal (GCLK1), with duty cycles **1/8** (WO0) and **7/8** (WO1).
   Frequency must be exact; duty cycles are fixed.

2. **CCL — 2 LUTs:**
   - LUT0: `COMP0 ? WO0 : WO1` — mux, fully internal (no PCB routing)
   - LUT3: `COMP3 ? WO0 : 0` — gate, fully internal (no PCB routing)

3. **AC positive inputs** shared with ADC — dual-comparator fan-out:
   - Pair 0: **COMP0 and COMP1** both on PA05 → ADC0_AIN5 (MUXPOS=PIN1=AIN1)
   - Pair 1: **COMP2 and COMP3** both on PB05 → ADC1_AIN7 (MUXPOS=PIN2=AIN6)
   - Both comparators in each pair are configured identically (same pin, same threshold).
     This allows signal A to reach LUT0+LUT1 and signal B to reach LUT2+LUT3 via the
     fixed INSEL=AC mapping (LUT_n→COMP_n). COMP1/COMP2 were otherwise unused.
   - Avoid PA03/PA04 (VREF conditioning pins on this board).

4. **Counters** — three independent counters, each ≥ 24 bits.
   - One counter must count **N periods of WO0** (375 kHz), generating:
     - MC interrupt at the (N−1)th period (penultimate)
     - OVF interrupt + OVF event at the Nth period (reset moment)
   - Other two counters: purpose TBD.

---

## LUT and CCL Input Routing

INSEL=AC has a fixed hardware mapping: LUT_n → COMP_n.
INSEL=TCC (0x8) routes TCC_n → LUT_n (with LUT3 wrapping back to TCC0,
since TCC3 does not exist on SAMC21J18A). Both laws confirmed experimentally
(scope, 2026-05-06/07).

### Active LUTs: LUT0 and LUT3

- COMP0 → LUT0 via INSEL=AC; TCC0 → LUT0 via INSEL=TCC → **fully internal**
- COMP3 → LUT3 via INSEL=AC; TCC0 → LUT3 via INSEL=TCC (wrap) → **fully internal**

### Dual-Comparator Fan-Out — AC signal to 4 LUTs

COMP1 and COMP2 are configured identically to COMP0 and COMP3 respectively,
so each AC decision signal is available to two LUTs:

| Signal | Comparators | LUTs reachable |
|--------|-------------|----------------|
| A (PA05) | COMP0 = COMP1 | LUT0, LUT1 |
| B (PB05) | COMP2 = COMP3 | LUT2, LUT3 |

### LUT1/LUT2 Activation Status

| LUT | AC source | WO source | Status |
|-----|-----------|-----------|--------|
| LUT0 | COMP0 | TCC0 (internal) | **Active** |
| LUT1 | COMP1 | TCC1 = period counter — no 375 kHz PWM | Needs PCB WO routing |
| LUT2 | COMP2 | TCC2 (16-bit) can duplicate TCC0 at 375 kHz on GCLK1 | Activatable internally |
| LUT3 | COMP3 | TCC0 wrap (internal) | **Active** |

LUT2 can be fully internal: TCC2 on GCLK1 (PCHCTRL[29]) with PER=63 and the
same CC values as TCC0 gives an identical 375 kHz waveform. LUT1 remains
blocked because TCC1 is allocated as the period counter (COUNTEV mode, no PWM).

---

## Peripheral Constraints — SAMC21J18A-AU

**J-variant (64-pin). Silicon revision: Rev F (DSU.DID=0x11010500). Current
DS80000740S still lists Rev-F-active errata; do not use older "all errata
annulled" notes.**

- **Timers available**: TC0–TC4, TCC0–TCC2. TC5/TC6/TC7 do not exist.
- **TC4 is standalone**: 16-bit only (no TC5 to pair).
- **TCC widths**: TCC0=24-bit, TCC1=24-bit, TCC2=16-bit.
- **≥24-bit counters**: TCC1 (24-bit), TC0+TC1 (32-bit), TC2+TC3 (32-bit).
- **TC2+TC3**: experimentally verified functional in COUNT32 mode.

---

## Clock Tree

| GCLK | Source | Frequency | Used by |
|------|--------|-----------|---------|
| GCLK0 | OSC48M | 48 MHz | CPU, TC0+TC1 (PCHCTRL[30]), TC2+TC3 (PCHCTRL[31]) |
| GCLK1 | XOSC | 24 MHz | TCC0 + TCC1 (PCHCTRL[28], shared) |

TCC0 and TCC1 share PCHCTRL[28]; one write covers both. TCC0 requires GCLK1
for exact 375 kHz. TCC1 (COUNTEV counter) is indifferent to GCLK frequency.

---

## PWM Generation — TCC0

TCC0 is 24-bit, running on GCLK1 (24 MHz XOSC). PER=63 gives exact 375 kHz.
Two CC channels provide WO0 (1/8 duty) and WO1 (7/8 duty), which feed LUT0
and LUT3 internally via INSEL=TCC — **no physical output pins required for
the PWM signals themselves**.

| Parameter | Value |
|-----------|-------|
| Peripheral | TCC0 |
| GCLK | GCLK1 = 24 MHz (XOSC) |
| PCHCTRL | [28] (shared with TCC1) |
| Prescaler | DIV1 |
| PER | 63 |
| f_out | 24 MHz / 64 = **375 kHz** |
| CC[0] | 8 → WO[0] duty = 1/8 **(WO0, narrow pulse)** |
| CC[1] | 56 → WO[1] duty = 7/8 **(WO1, wide pulse)** |

NPWM convention: WO[n] is HIGH while COUNT < CC[n], LOW while COUNT ≥ CC[n].

Physical output pins (optional — only if raw PWM signal needed externally):

| Signal | Pin | PMUX |
|--------|-----|------|
| TCC0/WO[0] | PA08 | E (0x4) |
| TCC0/WO[1] | PA09 | E (0x4) |

---

## CCL — LUT Configuration

### INSEL=TCC Routing (experimentally confirmed)

`INSEL_n = TCC (0x8)` routes WO[n] to IN[n]:
- INSEL0=TCC → IN[0] = WO[0] (narrow, 1/8)
- INSEL1=TCC → IN[1] = WO[1] (wide, 7/8)
- INSEL2=TCC → IN[2] = WO[2]

TCC→LUT pairing: TCC0→LUT0, TCC0→LUT3 (LUT3 wraps back to TCC0, no TCC3).

### LUT0: `COMP0 ? WO0 : WO1`

Mux: COMP0=1 selects WO0 (1/8); COMP0=0 selects WO1 (7/8).

| INSEL2 | INSEL1 | INSEL0 | Description |
|--------|--------|--------|-------------|
| AC (0x5) → COMP0 | TCC (0x8) → WO[1] | TCC (0x8) → WO[0] | all internal |

Truth table — IN[2]=COMP0, IN[1]=WO[1](7/8), IN[0]=WO[0](1/8):

| IN[2] COMP0 | IN[1] WO1 | IN[0] WO0 | OUT |
|-------------|-----------|-----------|-----|
| 0 | 0 | 0 | 0 |
| 0 | 0 | 1 | 0 |
| 0 | 1 | 0 | 1 |
| 0 | 1 | 1 | 1 |
| 1 | 0 | 0 | 0 |
| 1 | 0 | 1 | 1 |
| 1 | 1 | 0 | 0 |
| 1 | 1 | 1 | 1 |

**TRUTH = 0xAC**

Output pin (optional): PA07, mux I (0x8).

> Note: With INSEL=TCC, INSEL1 is forced to WO[1] and INSEL0 to WO[0]. This
> differs from the IO-pin arrangement in the old design (TRUTH was 0xCA there,
> with IN[1]=WO0 and IN[0]=WO1 freely assigned via PCB). TRUTH=0xAC is correct
> for the all-internal INSEL=TCC arrangement.

### LUT3: `COMP3 ? WO0 : 0`

Gate: COMP3=1 passes WO0 (1/8); COMP3=0 outputs 0.

| INSEL2 | INSEL1 | INSEL0 | Description |
|--------|--------|--------|-------------|
| AC (0x5) → COMP3 | MASK (0x0) | TCC (0x8) → WO[0] | all internal |

Truth table — IN[2]=COMP3, IN[1]=0 (MASK), IN[0]=WO[0](1/8):

| IN[2] COMP3 | IN[1] 0 | IN[0] WO0 | OUT |
|-------------|---------|-----------|-----|
| 0 | 0 | 0 | 0 |
| 0 | 0 | 1 | 0 |
| 1 | 0 | 0 | 0 |
| 1 | 0 | 1 | 1 |

**TRUTH = 0x20**

Output pin (optional): PB17, mux I (0x8).

### CCL Init Sequence

```cpp
// APB + GCLK for CCL
MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;
GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

// Disable CCL before configuring
CCL->CTRL.reg = 0;
CCL->SEQCTRL[0].reg = 0;
CCL->SEQCTRL[1].reg = 0;

// LUT0: COMP0 ? WO0 : WO1
CCL->LUTCTRL[0].reg =
    (0xACu                    << CCL_LUTCTRL_TRUTH_Pos)  |
    (CCL_LUTCTRL_INSEL2_AC    << CCL_LUTCTRL_INSEL2_Pos) |  // COMP0
    (CCL_LUTCTRL_INSEL1_TCC   << CCL_LUTCTRL_INSEL1_Pos) |  // WO[1]
    (CCL_LUTCTRL_INSEL0_TCC   << CCL_LUTCTRL_INSEL0_Pos) |  // WO[0]
    CCL_LUTCTRL_ENABLE;

// LUT3: COMP3 ? WO0 : 0
CCL->LUTCTRL[3].reg =
    (0x20u                    << CCL_LUTCTRL_TRUTH_Pos)  |
    (CCL_LUTCTRL_INSEL2_AC    << CCL_LUTCTRL_INSEL2_Pos) |  // COMP3
    (CCL_LUTCTRL_INSEL1_MASK  << CCL_LUTCTRL_INSEL1_Pos) |  // 0
    (CCL_LUTCTRL_INSEL0_TCC   << CCL_LUTCTRL_INSEL0_Pos) |  // WO[0]
    CCL_LUTCTRL_ENABLE;

// Enable CCL only after all LUTCTRL writes (DS80000740S 1.7.3)
CCL->CTRL.reg = CCL_CTRL_ENABLE;
```

---

## Analog Comparator

Two comparators: COMP0 (pair 0) and COMP3 (pair 1).

Each pair uses a distinct set of four AIN pins (pair 0: AIN0–AIN3; pair 1: AIN4–AIN7).
COMP0+ and COMP3+ land on different physical ports and are served by different ADCs.

### AC/ADC Shared Pins

| Comparator | MUXPOS | AC AIN | Pin | ADC channel | Role |
|------------|--------|--------|-----|-------------|------|
| COMP0+ | PIN1 | AIN1 | **PA05** | ADC0_AIN5 | signal A → LUT0 |
| COMP1+ | PIN1 | AIN1 | **PA05** | ADC0_AIN5 | signal A → LUT1 (fan-out) |
| COMP2+ | PIN2 | AIN6 | **PB05** | ADC1_AIN7 | signal B → LUT2 (fan-out) |
| COMP3+ | PIN2 | AIN6 | **PB05** | ADC1_AIN7 | signal B → LUT3 |

Both physical pins use function B (analog). The pin pad is simultaneously
accessible to both the AC input mux and the ADC input mux — no electrical
contention, confirmed. COMP0=COMP1 share the same pad; COMP2=COMP3 share the
same pad — both are valid (separate input mux registers, passive HiZ).
PA04/PA03 are reserved as VREF conditioning pins and must not be used.

### AC Clock

Use the current AC generic clock channel, `AC_GCLK_ID = 40`. The old
`PCHCTRL[34]`/ADC1-clock workaround is not a current Rev-F rule. AC output for
low-latency sampling must be `OUT_ASYNC`; `OUT_SYNC` was measured to add up to
two `GCLK_AC` cycles of lag.

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC;
GCLK->PCHCTRL[AC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
```

### COMPCTRL Values

All four comparators are configured. COMP0 and COMP1 are identical (signal A);
COMP2 and COMP3 are identical (signal B). The duplication is the fan-out
mechanism — same pin, same threshold, hardware delivers the result to two LUTs.

```cpp
// Signal A: PA05 (AIN1 in pair-0). COMP0→LUT0, COMP1→LUT1.
AC->COMPCTRL[0].reg =
    AC_COMPCTRL_MUXPOS_PIN1     // PA05 = pair-0 AIN1
    | AC_COMPCTRL_MUXNEG_VSCALE // VDD scaler (VALUE TBD)
    | AC_COMPCTRL_SPEED_HIGH
    | AC_COMPCTRL_ENABLE;

AC->COMPCTRL[1].reg =           // identical to COMP0 — delivers A to LUT1
    AC_COMPCTRL_MUXPOS_PIN1
    | AC_COMPCTRL_MUXNEG_VSCALE
    | AC_COMPCTRL_SPEED_HIGH
    | AC_COMPCTRL_ENABLE;

// Signal B: PB05 (AIN6 = pair-1 PIN2). COMP2→LUT2, COMP3→LUT3.
AC->COMPCTRL[2].reg =           // identical to COMP3 — delivers B to LUT2
    AC_COMPCTRL_MUXPOS_PIN2     // PB05 = pair-1 AIN6
    | AC_COMPCTRL_MUXNEG_VSCALE
    | AC_COMPCTRL_SPEED_HIGH
    | AC_COMPCTRL_ENABLE;

AC->COMPCTRL[3].reg =
    AC_COMPCTRL_MUXPOS_PIN2     // PB05 = pair-1 AIN6
    | AC_COMPCTRL_MUXNEG_VSCALE
    | AC_COMPCTRL_SPEED_HIGH
    | AC_COMPCTRL_ENABLE;
```

---

## Counter System

Three ≥24-bit counters are available. TCC0 is now the PWM source and is no
longer available as a counter.

| Counter | Width | GCLK | PCHCTRL | Role |
|---------|-------|------|---------|------|
| TCC1 | 24-bit | GCLK1 (24 MHz) | [28] shared w/ TCC0 | **period counter — N periods of WO0** |
| TC0+TC1 | 32-bit | GCLK0 (48 MHz) | [30] | TBD |
| TC2+TC3 | 32-bit | GCLK0 (48 MHz) | [31] | TBD |

### TCC1 — Period Counter

TCC1 counts TCC0.OVF events (375 kHz) via EVSYS. One OVF per WO0 period.

```
TCC1 configuration:
  GCLK:        GCLK1 (24 MHz, PCHCTRL[28] shared with TCC0)
  WAVEGEN:     COUNT only (no PWM output)
  EVCTRL:      TCEI0=1, EVACT0=COUNTEV
  EVCTRL:      OVFEO=1 (OVF event output)
  PER:         N − 1
  CC[0]:       N − 2   (MC0 = one period before OVF)
  INTENSET:    OVF | MC0
```

### Event System

Current DS80000740S still lists EVSYS errata on Rev F. In particular, TCC users
must use `PATH_ASYNCHRONOUS` because errata 1.21.9 says TCC is not compatible
with EVSYS SYNC/RESYNC mode.

| EVSYS channel | Generator | User | Path |
|---------------|-----------|------|------|
| ch0 | TCC0 OVF | TCC1 EV0 | ASYNCHRONOUS |
| ch1 | TCC1 OVF | TBD downstream | TBD |

---

## Peripheral Clock Enable Summary

```cpp
// APB clocks
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0 | MCLK_APBCMASK_TCC1;
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC0  | MCLK_APBCMASK_TC1;
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC2  | MCLK_APBCMASK_TC3;
MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;
MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC;
MCLK->APBCMASK.reg |= MCLK_APBCMASK_ADC0 | MCLK_APBCMASK_ADC1;
MCLK->APBCMASK.reg |= MCLK_APBCMASK_EVSYS;

// GCLK channels
GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN; // TCC0+TCC1
GCLK->PCHCTRL[TC0_GCLK_ID ].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN; // TC0+TC1
GCLK->PCHCTRL[TC2_GCLK_ID ].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN; // TC2+TC3
GCLK->PCHCTRL[AC_GCLK_ID  ].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN; // AC
GCLK->PCHCTRL[CCL_GCLK_ID ].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN; // CCL
```

`TCC0_GCLK_ID == TCC1_GCLK_ID == 28` — one write covers both.
`TC0_GCLK_ID == TC1_GCLK_ID == 30`. `TC2_GCLK_ID == TC3_GCLK_ID == 31`.

---

## Open Questions

| # | Item | Needed for |
|---|------|-----------|
| 1 | TCC0 OVF EVGEN index; TCC1 EV0 USER index (from EVSYS wiki) | EVSYS init code |
| 2 | AC negative input: VDD scaler value vs DAC vs external reference | COMPCTRL configuration |
| 3 | CC[0] = N−2 or N−1 for "penultimate" MC0 interrupt | TCC1 register value |
| 4 | TC0+TC1 and TC2+TC3 purpose | counter init code |
| 5 | LUT0 and LUT3 physical output pins needed? (PA07, PB17) | PCB routing |
| ~~6~~ | ~~COMP3 MUXPOS=PIN2 for pair-1 (AIN6=PB05)~~ | **Resolved**: confirmed via PinAssignments.ods (PB05=AIN6, pair-1 PIN2) |

---

## Design Decisions Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-05-06 | Silicon Rev F confirmed (DSU.DID=0x11010500) | Superseded by 2026-05-18 DS80000740S recheck: Rev F has active errata |
| 2026-05-06 | AC clock via GCLK_AC | Correct channel is `AC_GCLK_ID=40`; keep AC output `OUT_ASYNC` for low-latency paths |
| 2026-05-06 | EVSYS path choice | Superseded by 2026-05-18 recheck: TCC users require ASYNC EVSYS path |
| 2026-05-18 | Errata baseline updated to DS80000740S | Rev-F-active errata affect CCL, EVSYS, ADC, AC, and TCC design rules |
| 2026-05-06 | Period counter → TCC1 (COUNTEV) | Clean N-event counting with CC[0]/OVF |
| 2026-05-07 | PWM source → TCC0 (not TCC2) | INSEL=TCC routes TCC0 to LUT0 and LUT3 internally; TCC2 would only reach LUT2 internally |
| 2026-05-07 | AC pair → COMP0 + COMP3 (not COMP1) | COMP0 (pair 0) shares PA05 with ADC0; COMP3 (pair 1) shares PB05 with ADC1 — two independent ADCs |
| 2026-05-07 | LUT1/LUT2 dropped | INSEL=AC is fixed per LUT; COMP0/COMP3 unreachable on LUT1/LUT2 |
| 2026-05-07 | TC2+TC3 added as third ≥32-bit counter | Experimentally verified functional in COUNT32 mode |
| 2026-05-07 | LUT0/LUT3 fully internal | INSEL=TCC confirmed functional; no PCB loopback for WO0/WO1 |
| 2026-05-07 | TRUTH=0xAC for LUT0 mux | INSEL=TCC forces INSEL1→WO[1] and INSEL0→WO[0]; differs from old IO-routed TRUTH=0xCA |
| 2026-05-07 | TRUTH=0x20 for LUT3 gate | INSEL2=AC, INSEL1=MASK, INSEL0=TCC→WO[0]; OUT=1 at row5={1,0,1} |
| 2026-05-10 | Dual-comparator fan-out: COMP0=COMP1 (PA05), COMP2=COMP3 (PB05) | INSEL=AC is fixed LUT_n→COMP_n; configuring both comparators in a pair identically delivers signal A to LUT0+LUT1 and signal B to LUT2+LUT3; COMP1/COMP2 were unused |
| 2026-05-10 | LUT2 potentially activatable internally via TCC2 | TCC2 (16-bit, PCHCTRL[29]) with GCLK1 and PER=63 duplicates TCC0's 375 kHz; COMP2 provides signal B — no PCB routing needed |
| 2026-05-10 | LUT1 requires PCB WO routing | TCC1 is allocated as period counter (COUNTEV); no 375 kHz PWM available internally for LUT1 |
