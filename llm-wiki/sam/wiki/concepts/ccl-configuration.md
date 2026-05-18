---
title: CCL Configuration
type: concept
tags: [ccl, logic, lut, samc21]
sources: [samc21-datasheet-ch37-ccl, samc21-errata]
created: 2026-05-06
updated: 2026-05-18
---

# CCL Configuration

Configuration reference for the Configurable Custom Logic (CCL) peripheral
on the SAMC21, providing four programmable Look-Up Tables (LUT0–LUT3).

## Quick-Start Checklist

1. Enable APB clock: `MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL`
2. Enable GCLK if filter/edge/sequential/events needed: `PCHCTRL[CCL_GCLK_ID]`
3. Disable CCL before configuring: `CCL->CTRL.reg = 0`
4. Write `SEQCTRLx` and full `LUTCTRLn` values while `CTRL.ENABLE=0`
5. Include `LUTCTRLn.ENABLE` in the full `LUTCTRLn` write for active LUTs
6. Enable CCL last: `CCL->CTRL.reg = CCL_CTRL_ENABLE`

## Registers

| Register | Offset | Description |
|----------|--------|-------------|
| CTRL | 0x00 | CCL enable and software reset |
| SEQCTRL | 0x04 | Sequential circuit selection |
| LUTCTRL0 | 0x08 | LUT0 full configuration (32-bit) |
| LUTCTRL1 | 0x0C | LUT1 full configuration |
| LUTCTRL2 | 0x10 | LUT2 full configuration |
| LUTCTRL3 | 0x14 | LUT3 full configuration |

Base address: **0x42005C00**

GCLK PCHCTRL index: **38**

## Input Selection (INSEL) Reference

| Value | Name | Description |
|-------|------|-------------|
| 0x0 | MASK | Constant 0 |
| 0x1 | FEEDBACK | This LUT's own output |
| 0x2 | LINK | LUT(n+1).OUT → LUT(n).IN (higher feeds lower) |
| 0x3 | EVENT | EVSYS event input (LUTEI=1 required) |
| 0x4 | IO | I/O pin, function I |
| 0x5 | AC | AC comparator, fixed mapping (see below) |
| 0x6 | TC | TC WO output (see TC mapping below) |
| 0x7 | ALTTC | Alternative TC WO (see TC mapping below) |
| 0x8 | TCC | Functional on SAMC21J18A and now present in the datasheet: LUT0=TCC0, LUT1=TCC1, LUT2=TCC2, LUT3=TCC0. The input slot selects WO[0]/WO[1]/WO[2]. See TCC→LUT mapping below. |
| 0x9 | SERCOM | SERCOM data output |

## AC Input Mapping (Fixed, Non-Selectable)

| LUT | COMP |
|-----|------|
| LUT0 | COMP0 |
| LUT1 | COMP1 |
| LUT2 | COMP2 |
| LUT3 | COMP3 |

COMP0/COMP1 **cannot** be fed into LUT2/LUT3 via INSEL=AC. To route COMP0/COMP1
to LUT2/LUT3: either (a) use AC COMPCTRL.OUT=1 and route via an AC output pin back
to a CCL function-I input pin (PCB trace), or (b) use LINK if the logic allows.

## TC Input Mapping

Older errata 1.8.3 (TC selection reversed) does not affect the target
SAMC21J18A Rev F according to DS80000740S. Use the current datasheet mapping on
the target hardware.

| LUT | INSEL=TC | INSEL=ALTTC |
|-----|----------|-------------|
| LUT0 | TC0 | TC1 |
| LUT1 | TC1 | TC2 |
| LUT2 | TC2 | TC3 |
| LUT3 | TC3 | TC4 |

## TCC Input Mapping (INSEL=0x8) — Experimentally Verified on SAMC21J18A

INSEL=TCC (0x8) is functional on SAMC21J18A and is present in the current
datasheet/header mapping for the target.

### Oscilloscope-Confirmed (direct pin observation, 2026-05-06/07)

**Two laws confirmed by direct scope measurement:**
1. **TCC_n → LUT_n** (TCC0→LUT0, TCC1→LUT1, TCC2→LUT2)
2. **INSEL_n = TCC routes WO[n]** (INSEL0→WO[0], INSEL1→WO[1], INSEL2→WO[2])

| TCC | LUT | INSEL0 | INSEL1 | INSEL2 | Ref pins |
|-----|-----|--------|--------|--------|----------|
| TCC0 | LUT0 | WO[0]=25% ✓ | WO[1]=75% ✓ | WO[2]=50% ✓ | PA08/PA09/PA10 → PA07 |
| TCC1 | LUT1 | WO[0]=25% ✓ | WO[1]=75% ✓ | WO[2]=25%* ✓ | PA06/PA07 → PA11 |
| TCC2 | LUT2 | WO[0]=25% ✓ | — | — | PA12 → PA25 |
| any  | LUT3 | — | — | — | untested |

\* TCC1 has only 2 CC channels; WO[2] reuses CC[0], so WO[2]=WO[0].

No EVSYS/TC chain involved — all results from direct scope observation of CCL output pins.

### Confirmed TCC→LUT Mapping (scope, 2026-05-06/07) — COMPLETE

| LUT | TCC | INSEL0 | INSEL1 | INSEL2 |
|-----|-----|--------|--------|--------|
| LUT0 | TCC0 | WO[0] ✓ | WO[1] ✓ | WO[2] ✓ |
| LUT1 | TCC1 | WO[0] ✓ | WO[1] ✓ | WO[0] ✓ (WO[2]=WO[0], only 2 CC) |
| LUT2 | TCC2 | WO[0] ✓ | WO[1] ✓ | WO[0] ✓ (WO[2]=WO[0], only 2 CC) |
| LUT3 | TCC0 | WO[0] ✓ | WO[1] ✓ | WO[2] ✓ |

For LUT1/TCC1 and LUT2/TCC2: INSEL2 routes the hardware WO[2] pin, but since
TCC1 and TCC2 have only 2 CC channels, WO[2] is driven by CC[0] — identical
signal to WO[0]. INSEL2 cannot independently select WO[2] on these TCCs.

LUT3 wraps back to TCC0 because TCC3 does not exist on SAMC21J18A.
On SAMC21N (100-pin), TCC3 would presumably route to LUT3 instead.

**INSEL_n selects WO[n]** — INSEL0→WO[0], INSEL1→WO[1], INSEL2→WO[2].
No PCB loopback required for up to three WO channels of the same TCC.

**Design implication — AC gate on WO[0]** (`WO0 & !AC_fault`):
- INSEL2=AC (0x5), INSEL1=MASK, INSEL0=TCC (0x8), TRUTH=0x02... wait, need:
  IN[2]=AC, IN[0]=WO[0]; OUT=1 only when AC=0 and WO[0]=1 → row 2 → TRUTH=0x04
- INSEL2=AC (0x5), INSEL1=MASK, INSEL0=TCC (0x8), TRUTH=0x04
- No physical pin or PCB loopback required for either signal.

**Design implication — mux two WO channels** (`AC ? WO0 : WO1`):
- INSEL2=AC (0x5), INSEL1=TCC→WO[1] (0x8), INSEL0=TCC→WO[0] (0x8), TRUTH=0xCA
- All three inputs sourced internally — no PCB loopback needed at all.

## LINK Direction

LINK inputs flow from higher-numbered LUT to lower-numbered LUT:

```
LUT3.OUT ──LINK──► LUT2.IN[x]
LUT2.OUT ──LINK──► LUT1.IN[x]
LUT1.OUT ──LINK──► LUT0.IN[x]
```

## PCB Routing Requirement

TCC outputs use PMUX function F; TC outputs use PMUX function E; CCL inputs use
PMUX function I. These are distinct peripheral functions. A pin cannot serve as
both a TCC/TC output and a CCL input at the same time.

**Consequence**: TCC WO signals must be physically looped back via PCB traces
to CCL function-I input pins to be used as CCL inputs via INSEL=IO.

## TRUTH Table Computation

Output for row N = bit N of the TRUTH byte, where N = `{IN[2], IN[1], IN[0]}`.

Common patterns:

| Function | TRUTH | Notes |
|----------|-------|-------|
| `AC ? WO0 : WO1` (mux) | 0xCA | IN[2]=AC, IN[1]=WO0, IN[0]=WO1 |
| `AC ? WO0 : 0` (gate) | 0x40 | IN[2]=AC, IN[1]=WO0, IN[0]=MASK |
| AND(IN[2],IN[1]) | 0x80 | Two-input AND, IN[0]=MASK |
| OR(IN[2],IN[1]) | 0xE0 | Two-input OR, IN[0]=MASK |
| XOR(IN[2],IN[1]) | 0x60 | Two-input XOR, IN[0]=MASK |

## Sequential Logic

SEQCTRL.SEQSEL0 = pair LUT0+LUT1; SEQCTRL.SEQSEL1 = pair LUT2+LUT3.

| SEQSEL | Mode |
|--------|------|
| 0x0 | Disabled |
| 0x1 | D latch |
| 0x2 | D flip-flop |
| 0x3 | JK flip-flop |
| 0x4 | RS latch |

> **Errata 1.7.1**: RS latch reset is non-functional on rev A. Workaround:
> disable the LUT (LUTCTRLn.ENABLE=0) to clear the latch state.

Current DS80000740S Rev-F-relevant CCL errata:

| # | Issue | Local rule |
|---|---|---|
| 1.7.2 | Sequential logic can remain under reset after disabling/re-enabling LUT0/LUT2 to clear a latch/FF | If sequential CCL is used, write `CTRL.ENABLE` again after re-enabling the LUT |
| 1.7.3 | `SEQCTRLx` and `LUTCTRLx` are protected by `CTRL.ENABLE` | Configure all `SEQCTRLx`/`LUTCTRLn` while `CCL->CTRL.ENABLE=0`; set global enable last |
| 1.7.4 | Writing `CTRL.SWRST` triggers a PAC protection error | Avoid CCL software reset in production paths |

## Event System

| Generator | EVSYS index | Description |
|-----------|-------------|-------------|
| LUTOUT0 | 82 | LUT0 output event |
| LUTOUT1 | 83 | LUT1 output event |
| LUTOUT2 | 84 | LUT2 output event |
| LUTOUT3 | 85 | LUT3 output event |

Set LUTCTRLn.LUTEO=1 to enable the event output for a LUT.
Set LUTCTRLn.LUTEI=1 to accept an EVSYS event as IN[x] (INSEL=EVENT for that slot).

## Initialization Example (Pure Combinational, No GCLK)

```cpp
// Enable APB clock
MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;

// Disable CCL
CCL->CTRL.reg = 0;

// Configure LUT0: AC0?WO0:WO1 — TRUTH=0xCA.
// Because of errata 1.7.3, include LUTCTRL.ENABLE before CCL->CTRL.ENABLE.
CCL->LUTCTRL[0].reg =
    (0xCAu << CCL_LUTCTRL_TRUTH_Pos) |
    (0x5u  << CCL_LUTCTRL_INSEL2_Pos) |  // INSEL2 = AC (COMP0)
    (0x4u  << CCL_LUTCTRL_INSEL1_Pos) |  // INSEL1 = IO (WO0 via pin)
    (0x4u  << CCL_LUTCTRL_INSEL0_Pos) |  // INSEL0 = IO (WO1 via pin)
    CCL_LUTCTRL_ENABLE;

// Enable CCL after all SEQCTRL/LUTCTRL writes
CCL->CTRL.reg = CCL_CTRL_ENABLE;
```

## See Also

- [[SAMC21 Datasheet Ch.37 CCL]]
- [[SAMC20/C21 Errata (DS80000740S)]]
- [[AC Configuration]]
- [[TCC Configuration]]
- [[TC 32-Bit Paired Mode]]
- [[EVSYS]]
- [[I/O Multiplexing]]
