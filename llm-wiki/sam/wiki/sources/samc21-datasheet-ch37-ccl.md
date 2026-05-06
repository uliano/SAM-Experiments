---
title: SAMC21 Datasheet Ch.37 CCL
type: source
tags: [ccl, logic, lut, samc21, datasheet]
sources: [samc21-datasheet-ch37-ccl]
created: 2026-05-06
updated: 2026-05-06
---

# SAMC21 Datasheet Ch.37 CCL

Summary of Chapter 37 — Configurable Custom Logic (CCL), pages 849–867.

## Abstract

The CCL peripheral provides four Look-Up Tables (LUT0–LUT3), each with three
inputs and one output. Each LUT is an 8-bit combinational function of its
three inputs, defined by a TRUTH register. Sequential elements (D/JK/RS
latches) and edge/filter options are available. The CCL can be combined with
the Event System to route outputs and synchronize across peripherals.

## Key Facts

- **Four LUTs**: LUT0–LUT3, each with IN[2:0] inputs and one output.
- **TRUTH register**: 8-bit value; output = bit `{IN[2],IN[1],IN[0]}` of TRUTH.
- **Base address**: 0x42005C00 (APB-C bridge).
- **APB mask**: MCLK_APBCMASK_CCL — disabled at reset; must enable explicitly.
- **GCLK**: PCHCTRL[38]. Required only for filter/edge/sequential/event features;
  pure combinational operation requires no GCLK.
- **CTRLA** (offset 0x00): SWRST[0], ENABLE[1]. Enable-protected for LUTs.
- **SEQCTRL** (offset 0x04): sequential circuit selection per LUT pair (0/1 and 2/3).
  Values: 0x0=disabled, 0x1=D latch, 0x2=D FF, 0x3=JK FF, 0x4=RS latch.
- **LUTCTRLn** (offset 0x08+4n, 32-bit): configures each LUT fully.

### LUTCTRLn Bit Fields

| Bits | Field | Description |
|------|-------|-------------|
| [31:24] | TRUTH | 8-bit truth table |
| [22] | LUTEO | LUT event output enable |
| [21] | LUTEI | LUT event input enable |
| [20] | INVEI | Invert event input |
| [19:16] | INSEL2 | Input 2 source select |
| [15:12] | INSEL1 | Input 1 source select |
| [11:8] | INSEL0 | Input 0 source select |
| [7] | EDGESEL | Edge selection (requires GCLK) |
| [5:4] | FILTSEL | Filter selection (0=none, 1=sync, 2=filter) |
| [1] | ENABLE | LUT enable |

### INSEL Values

| Value | Name | Description |
|-------|------|-------------|
| 0x0 | MASK | Constant 0 (input masked) |
| 0x1 | FEEDBACK | Output of this LUT fed back as input |
| 0x2 | LINK | Output of the next-higher LUT (LUT(n+1).OUT → LUT(n).IN) |
| 0x3 | EVENT | Linked event input (LUTEI) |
| 0x4 | IO | I/O pin (function I in Table 6-2) |
| 0x5 | AC | Analog Comparator output — fixed mapping per LUT (see below) |
| 0x6 | TC | TC WO output — datasheet says TC0/TC1/TC2/TC3 for LUT0/1/2/3 |
| 0x7 | ALTTC | Alternative TC — datasheet says TC1/TC2/TC3/TC4 for LUT0/1/2/3 |
| 0x8 | TCC | Datasheet says reserved on C20/C21 — **experimentally confirmed functional on SAMC21J18A**. Routes TCC WO[0] to the LUT input. All three INSEL slots (IN[0]/IN[1]/IN[2]) receive the same WO[0]; different WO channels of the same TCC cannot be selected this way. See [[CCL Configuration]] for the full verified mapping. |
| 0x9 | SERCOM | SERCOM data output |
| 0xA | ALT2TC | N-series only |
| 0xB | ASYNCEVENT | N-series only |

### INSEL=AC Fixed Mapping (per Datasheet)

| LUT | Comparator |
|-----|-----------|
| LUT0 | COMP0 |
| LUT1 | COMP1 |
| LUT2 | COMP2 |
| LUT3 | COMP3 |

This mapping is **fixed** in hardware — it is not selectable. COMP0/COMP1 cannot
be routed into LUT2/LUT3 via INSEL=AC.

### INSEL=TC Mapping (per Datasheet — see Errata 1.8.3)

| LUT | INSEL=TC | INSEL=ALTTC |
|-----|----------|-------------|
| LUT0 | TC0 | TC1 |
| LUT1 | TC1 | TC2 |
| LUT2 | TC2 | TC3 |
| LUT3 | TC3 | TC4 |

> **WARNING — Errata 1.8.3**: On rev A silicon the actual hardware mapping is
> TC4/TC0/TC1/TC2 for INSEL=TC and TC0/TC1/TC2/TC3 for INSEL=ALTTC.
> See [[SAMC21 Errata]] for details.

### LINK Direction

LINK routes **LUT(n+1).OUT into LUT(n).IN**:
- LUT1.OUT → LUT0.IN[x] when LUT0 has INSEL=LINK
- LUT2.OUT → LUT1.IN[x] when LUT1 has INSEL=LINK
- LUT3.OUT → LUT2.IN[x] when LUT2 has INSEL=LINK

LUT3 has no higher LUT to link from; LINK on LUT3 is undefined/unused.

### CCL Event System Integration

| Generator | EVSYS index | Description |
|-----------|-------------|-------------|
| LUTOUT0 | 82 | LUT0 output event |
| LUTOUT1 | 83 | LUT1 output event |
| LUTOUT2 | 84 | LUT2 output event |
| LUTOUT3 | 85 | LUT3 output event |

LUT event input (LUTEI) is an EVSYS user. LUTEO enables the output as a generator.

### TRUTH Table Examples

**Mux: `AC ? WO0 : WO1`** — IN[2]=AC, IN[1]=WO0, IN[0]=WO1:

| IN[2] AC | IN[1] WO0 | IN[0] WO1 | OUT |
|---------|----------|----------|-----|
| 0 | 0 | 0 | 0 |
| 0 | 0 | 1 | 1 |
| 0 | 1 | 0 | 0 |
| 0 | 1 | 1 | 1 |
| 1 | 0 | 0 | 0 |
| 1 | 0 | 1 | 0 |
| 1 | 1 | 0 | 1 |
| 1 | 1 | 1 | 1 |

**TRUTH = 0xCA**

**Gate: `AC ? WO0 : 0`** — IN[2]=AC, IN[1]=WO0, IN[0]=MASK(0):

| IN[2] AC | IN[1] WO0 | IN[0]=0 | OUT |
|---------|----------|---------|-----|
| 0 | 0 | 0 | 0 |
| 0 | 1 | 0 | 0 |
| 1 | 0 | 0 | 0 |
| 1 | 1 | 0 | 1 |

**TRUTH = 0x40** (only row 3 is active; IN[0] is always 0)

### CCL I/O Pin Assignments (Function I)

CCL inputs use peripheral function I in the PORT PMUX table. TCC/TC outputs use
function E or F. These are **different functions** — the same pin cannot serve
both simultaneously. PCB routing is required to bring TCC WO signals back to
CCL input pins.

> TBD: exact function-I pin assignments for LUT0–LUT3 IN[0..2] (from Table 6-2).

### Sequential Logic

SEQCTRL.SEQSEL0 controls LUT0/LUT1 pair; SEQCTRL.SEQSEL1 controls LUT2/LUT3.
With RS latch (0x4): LUT0.OUT = Set, LUT1.OUT = Reset (for pair 0).

> **Errata 1.7.1**: RS latch reset function is non-functional on rev A silicon.
> Workaround: disable the LUT to clear the latch. See [[SAMC21 Errata]].

## Entities Mentioned

- [[SAMC21J18A]] (target MCU)

## Concepts Mentioned

- [[CCL Configuration]]
- [[EVSYS]]
- [[AC Configuration]]
- [[TCC Configuration]]
- [[TC 32-Bit Paired Mode]]
- [[I/O Multiplexing]]

## Open Questions / Gaps

- Exact function-I pin assignments for each LUT[n].IN[m] input (Table 6-2).
- Verify actual pin numbers for LUT0–LUT3 outputs on the J/64-pin package.

## See Also

- [[CCL Configuration]]
- [[SAMC21 Errata]]
- [[AC Configuration]]
- [[EVSYS]]
- [[I/O Multiplexing]]
