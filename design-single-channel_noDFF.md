# Single-Channel Design — SAMC21J18A, no external DFF

ATSAMC21J18A-AU operational design target.
Date: 2026-05-21

This is a variant of [design-single-channel.md](design-single-channel.md) that
**eliminates the external D flip-flop** by emulating it inside the CCL
sequential block. With one channel of input there are 4 CCL LUTs available;
the DFF needs 2 of them via SEQCTRL, leaving exactly 2 for the reference mux
and the AND-gate.

The design fits, but the routing is tight: SAMC21 CCL has fixed mapping rules
that constrain *which* LUT can be used for *which* function, and the LINK
direction (LUT_n reads LUT_{n+1}, not the other way around) further limits
internal signal distribution. This document analyses the resource layout in
detail and converges on the recommended architecture, with sub-options where
silicon behavior is unverified.

## 1. Required Behavior

Same as [design-single-channel.md](design-single-channel.md), with one change:

- The external DFF chip is **removed**. The synchronous sample
  `AC_SYNC = AC_CMP0 sampled on HB_1_2 rising edge` is produced by the CCL
  internal sequential block (SEQ1, DFF mode).

Everything downstream (REF mux, COUNT_PULSE AND-gate, TC0+TC1 window counter,
TC2+TC3 duty counter, ISR boundary handling) is unchanged in intent. The
pin/CCL/EVSYS topology necessarily changes.

## 2. Resource Budget

CCL LUTs needed:

| Function | LUTs |
|---|---:|
| DFF (D-input LUT + CLK-input LUT, combined by SEQCTRL) | 2 |
| REF mux | 1 |
| COUNT_PULSE AND-gate | 1 |
| **Total** | **4** |

SAMC21J has exactly 4 LUTs. **No margin** — the design only fits if every LUT
plays exactly one role. This is the central constraint of the no-DFF design.

## 3. CCL Routing Constraints (SAMC21J18A)

These are the silicon facts that determine the layout. All are documented in
`llm-wiki/sam/wiki/sources/samc21-datasheet-ch37-ccl.md` and validated locally
or via CMSIS.

### 3.1 `INSEL=TCC` per-LUT mapping (fixed)

| LUT | `INSEL=TCC` target |
|---|---|
| LUT0 | TCC0 |
| LUT1 | TCC1 |
| LUT2 | TCC2 |
| LUT3 | TCC0 (wraps because TCC3 doesn't exist) |

We need TCC0 waveforms (`HB_1_8`, `HB_7_8`, `HB_1_2`) inside CCL. Only LUT0
and LUT3 can read them via `INSEL=TCC`. Within a LUT, the input slot
determines which `WO[n]`: IN0→WO[0], IN1→WO[1], IN2→WO[2].

**Consequence:** the REF mux (uses both `HB_1_8` and `HB_7_8`) must live on
LUT0 or LUT3. The DFF CLK input (needs `HB_1_2`) ideally lives on LUT0 or
LUT3 too, to avoid an extra `INSEL=IO` loopback.

### 3.2 `INSEL=AC` per-LUT mapping (fixed)

| LUT | `INSEL=AC` target |
|---|---|
| LUT0 | COMP0 |
| LUT1 | COMP1 |
| LUT2 | COMP2 (does not exist on SAMC21) |
| LUT3 | COMP3 (does not exist on SAMC21) |

SAMC21 AC has only COMP0 and COMP1.

**Consequence:** the DFF D-input (needs `AC_CMP0`) can read AC directly via
`INSEL=AC` only if it lives on LUT0 or LUT1. Otherwise AC must enter the
CCL via `INSEL=IO` (external pad loopback from PA12 = `AC_CMP0` pad) or via
`INSEL=EVENT` (EVSYS routing of `AC_COMP_0` event — depends on COMPEO
semantics, see §3.5).

### 3.3 `INSEL=LINK` direction

`LINK` carries `LUT_{n+1}.OUT` into `LUT_n.IN[x]`:

- LUT0 LINK reads LUT1 OUT
- LUT1 LINK reads LUT2 OUT
- LUT2 LINK reads LUT3 OUT
- LUT3 LINK is undefined (no LUT4)

LINK propagates **downward** in LUT index. Signals generated on a higher LUT
can be consumed combinationally on a lower LUT, but not the reverse.

### 3.4 SEQCTRL pair binding (fixed)

`SEQCTRL.SEQSEL0` controls the LUT0+LUT1 pair (SEQ0).
`SEQCTRL.SEQSEL1` controls the LUT2+LUT3 pair (SEQ1).

In DFF mode, the LUT_even truth (LUT0 for SEQ0, LUT2 for SEQ1) supplies D and
the LUT_odd truth (LUT1 for SEQ0, LUT3 for SEQ1) supplies CLK. The
flip-flop output Q **replaces** the LUT_even output in every downstream
consumer (pad mux, EVSYS LUTOUT_n generator, and LINK input of the next
lower LUT). LUT_odd's output is unchanged — it still carries the CLK
waveform to its pad/EVSYS/LINK consumer.

### 3.5 AC EVSYS event semantics + CCL EVENT-input edge detector (RESOLVED 2026-05-21)

Two coupled silicon facts, both confirmed by the datasheet and by bench
experiment (`src/ac_compeo_test.hpp`):

1. **`AC.COMPEOx` is a level passthrough.** Datasheet §40.6.2.4:
   "Events are generated using the comparator output state, regardless
   of whether the interrupt is enabled or not." `COMPCTRLx.INTSEL`
   selects the interrupt edge condition; it has no effect on the event
   output. So `EVCTRL.COMPEO0=1` puts the continuous AC0 output level
   onto the corresponding EVSYS generator line.

2. **The CCL `INSEL=EVENT` input has a forced rising-edge detector on
   the J variant.** Datasheet §37: "By default, CCL includes an edge
   detector. When the event is received, an internal strobe is generated
   when a rising edge is detected. The pulse duration is one GCLK_CCL
   clock cycle. Writing the LUTCTRLx.INSELy = ASYNCEVENT will disable
   the edge detector (This feature is only available on SAM C20/C21 N
   variants)." On the SAMC21J18A `ASYNCEVENT` is not implemented, so
   any EVSYS event entering a LUT through `INSEL=EVENT` is reduced to
   1-cycle strobes — the level on EVSYS is lost.

Implication for this design: even though `COMPEO` is a level, on the J
the CCL cannot receive it as a level. A DFF D-input requires a continuous
level, so **sub-option A1 (`AC_COMP_0 → CCL_LUTIN_2`, `INSEL=EVENT`) is
not viable on this silicon**. The only workable path is **sub-option A2
(`AC_CMP0` pad on PA12 → external jumper → PA22 / `CCL_IN6`, fed into
LUT2 via `INSEL=IO`)** — `INSEL=IO` does not apply the edge detector and
preserves the level.

The initial bench measurement (2026-05-21) misattributed the symptom —
1-cycle strobes on AC rising edges — to a non-level `COMPEO`. The
datasheet review made clear that `COMPEO` is the level, and the strobes
come from the CCL edge detector on `EVENT`. The final answer (A1 not
viable on J, A2 adopted) is unchanged; the *reason* is.

Note: even on the **N** variant, where `INSEL=ASYNCEVENT` would unlock a
proper level path, the no-DFF design has no application — the N is the
dual-channel target, and the no-DFF approach does not extend to dual
channel (§17.1). So `ASYNCEVENT` is silicon that doesn't help us in the
configurations we actually plan to ship.

See [[AC Configuration]] and [[CCL Configuration]] in the wiki for the
details and cross-references.

## 4. Layout Derivation

The four LUTs must absorb four roles (DFF-D, DFF-CLK, REF-mux, AND-gate).
The constraints above narrow the choice:

1. REF-mux requires LUT0 or LUT3 (only ones with `INSEL=TCC → TCC0`).
2. DFF-D needs `AC_CMP0`. LUT0 (via `INSEL=AC`) is ideal. Otherwise LUT2
   needs an external or EVSYS path.
3. DFF-CLK needs `HB_1_2`. LUT0 or LUT3 reach it directly (`INSEL=TCC IN2`).
4. AND-gate needs `Q` and `HB_1_2`. Q is at the DFF output position.
5. SEQ pairs are fixed: DFF-D and DFF-CLK must be in the same pair (LUT0+LUT1
   = SEQ0, or LUT2+LUT3 = SEQ1).

Two coherent layouts result:

### Layout A — DFF on SEQ1 (LUT2+LUT3)

| LUT | Role | Notes |
|---|---|---|
| LUT0 | REF mux | `INSEL=TCC` IN0,IN1 = `HB_1_8`,`HB_7_8`. IN2 = Q via EVSYS. |
| LUT1 | AND-gate | IN0 LINK = Q (LUT2.OUT after SEQ1). IN2 IO = PB10 (`HB_1_2` loopback). |
| LUT2 | DFF D | Needs `AC_CMP0`; can't use `INSEL=AC` (would read non-existent COMP2). EVSYS or IO loopback. |
| LUT3 | DFF CLK | `INSEL=TCC` IN2 = `HB_1_2` (LUT3 wraps to TCC0). |

Q distribution:
- Q → LUT1 IN0 via `INSEL=LINK` (combinational, ~ns)
- Q → LUT0 IN2 via EVSYS LUTOUT_2 → LUTIN_0 (PATH_ASYNCHRONOUS)

### Layout B — DFF on SEQ0 (LUT0+LUT1)

| LUT | Role | Notes |
|---|---|---|
| LUT0 | DFF D | `INSEL=AC` IN0 = `AC_CMP0` ✓ (direct, no loopback or EVSYS needed). |
| LUT1 | DFF CLK | `INSEL=TCC` reads TCC1 (not TCC0); needs IO loopback PB10 for `HB_1_2`. |
| LUT2 | AND-gate | Needs Q (from LUT0 after SEQ0) and `HB_1_2`. LINK reads LUT3, not LUT0. |
| LUT3 | REF mux | `INSEL=TCC` IN0,IN1 = `HB_1_8`,`HB_7_8`. IN2 = Q via EVSYS. |

Q distribution:
- LUT2 (AND-gate) gets Q via EVSYS LUTOUT_0 → LUTIN_2
- LUT3 (REF mux) gets Q via EVSYS LUTOUT_0 → LUTIN_3 (same channel, two users)
- LUT2 still needs `HB_1_2` somewhere: LINK reads LUT3 (= REF, not useful);
  must add a second IO loopback (e.g., PA10 → PA24 = `CCL_IN8` for LUT2 IN2).

**Layout B costs one extra external jumper** (PA10 → PA24) compared to
Layout A. Layout A uses LINK to distribute Q to LUT1 with no extra
external connection, which is both cleaner and faster (LINK ~ns,
intra-CCL).

**Layout A is recommended.** The rest of this document assumes Layout A.

## 5. AC Routing for Layout A — Decision Locked

LUT2 (the DFF D-input) needs `AC_CMP0` but cannot read it via `INSEL=AC`
(would resolve to non-existent COMP2 — see §3.2). The remaining options
were A1 (via EVSYS) and A2 (external pad loopback). With §3.5 now
resolved, **A1 is not viable on the J variant**: although `COMPEO` is a
continuous level on EVSYS, the CCL's `INSEL=EVENT` input applies a
built-in rising-edge detector that the J variant cannot bypass
(`ASYNCEVENT` is N-only). A DFF D-input requires a level, which the
CCL EVENT slot strips down to 1-cycle strobes. **A2 is the adopted path.**

### A2 — `AC_CMP0` via external pad loopback (adopted)

```text
AC enabled with COMPCTRL0.OUT = ASYNC, AC_CMP0 driven on PA12 mux H
External PCB jumper: PA12 → PA22
PA22 muxed as I = CCL_IN6 (LUT2 IN0)
LUT2 IN0 INSEL = IO → reads AC_CMP0 via PA22 pad
```

Cost: +1 external jumper (2 jumpers total: `PA10→PB10` and `PA12→PA22`).
No EVSYS channel allocated for the AC path.

### A1 — `AC_CMP0` via EVSYS (rejected on J, kept for record)

The previously considered "elegant" variant — `AC_COMP_0 → CCL_LUTIN_2`
via EVSYS, with LUT2 IN0 `INSEL=EVENT` — would have required the LUT to
see the `COMPEO` level. The level is present on EVSYS (datasheet §40
confirms), but the CCL's `INSEL=EVENT` slot applies a forced rising-edge
detector on the J variant and only the N variant supports `ASYNCEVENT`
to bypass it (datasheet §37, see §3.5). On the J part the LUT therefore
sees only 1-cycle strobes on AC rising edges — useless as a DFF D-input.
Listed here only so future work doesn't re-derive the same path without
consulting the silicon-behaviour record.

## 6. Peripheral Allocation (Layout A, sub-option A2)

| Peripheral | Role | Width | Clock | PCHCTRL |
|---|---|---|---|---:|
| TCC0 | Heartbeat (HB_1_8, HB_7_8, HB_1_2) | 24-bit | GCLK1 = 24 MHz | 28 |
| AC COMP0 | Single-channel comparator | — | GCLK0 register; `OUT_ASYNC` | 40 |
| ADC0 | Analog readback PA05 / `ADC0_AIN5` | 12-bit | own | 41 |
| CCL LUT0 | REF mux | — | GCLK0 | 38 |
| CCL LUT1 | COUNT_PULSE AND-gate | — | GCLK0 | 38 |
| CCL LUT2 | DFF D-input (SEQ1) | — | GCLK0 | 38 |
| CCL LUT3 | DFF CLK-input (SEQ1) | — | GCLK0 | 38 |
| TC0+TC1 | Window counter (COUNT32, MFRQ, event-count `TCC0_OVF`) | 32-bit | GCLK0 = 48 MHz | 30 |
| TC2+TC3 | Duty counter (COUNT32, NFRQ, event-count `CCL_LUTOUT_1`) | 32-bit | GCLK0 = 48 MHz | 31 |
| EVSYS CH0 | `TCC0_OVF → TC0_EVU` (ASYNC) | — | — | — |
| EVSYS CH1 | `CCL_LUTOUT_1 → TC2_EVU` (ASYNC) | — | — | — |
| EVSYS CH2 | `CCL_LUTOUT_2 → CCL_LUTIN_0` (ASYNC, Q to REF mux) | — | — | — |
| SERCOM5 | UART monitor, 1 Mbaud | — | own | various |

(Sub-option A1, which would have added EVSYS CH3 routing
`AC_COMP_0 → CCL_LUTIN_2` ASYNC, is no longer in scope — see §5.)

## 7. Clock Plan

Identical to [design-single-channel.md](design-single-channel.md) §4. The
GCLK_AC requirement is needed only for AC register access; the COMPEO event
output does not introduce extra clock requirements.

## 8. Pin Allocation (Layout A, sub-option A2)

Sorted by port and pin.

| Pin | Mux | Dir | Net | Function | Notes |
|---|---|---|---|---|---|
| PA00 | — | analog | `XOSC32K_XIN` | XOSC32K input | optional, build flag |
| PA01 | — | analog | `XOSC32K_XOUT` | XOSC32K output | (same) |
| PA03 | — | analog | `VREF` | VREF conditioning | 22 Ω + 22 nF to GND |
| PA04 | — | analog | `VREF` | VREF conditioning | (same) |
| PA05 | B | analog in | `analog_in` | `AC_AIN1` + `ADC0_AIN5` | unchanged |
| PA07 | I | digital out | `REF` | `CCL_OUT0` (LUT0) | REF mux output |
| PA09 | E | digital out | `HB_7_8` | `TCC0/WO1` | scope/debug |
| PA10 | F | digital out | `HB_1_2` | `TCC0/WO2` | drives external loopback to PB10 |
| PA11 | I | digital out | `COUNT_PULSE` | `CCL_OUT1` (LUT1) | EVSYS source for TC2 duty counter |
| PA12 | H | digital out | `AC_CMP0` | `AC_CMP0` | drives external loopback to PA22 |
| PA14 | — | analog | `XIN` | XOSC | 24 MHz crystal |
| PA15 | — | analog | `XOUT` | XOSC | (same) |
| PA22 | I | digital in | `AC_CMP0` loopback | `CCL_IN6` (LUT2/IN0) | from PA12 |
| PA25 | I | digital out | `DFF.Q` (optional) | `CCL_OUT2` (LUT2 via SEQ1) | scope/debug only |
| PA30 | — | digital | `SWCLK` | SWD | reserved |
| PA31 | — | digital | `SWDIO` | SWD | (same) |
| PB10 | I | digital in | `HB_1_2` loopback | `CCL_IN5` (LUT1/IN2) | from PA10 |
| PB22 | GPIO | digital in | bench button | GPIO input | bench fixture |
| PB23 | GPIO | digital out | bench LED | GPIO output | bench fixture |
| PB30 | D | digital out | UART TX | `SERCOM5/PAD2` | COM5 @ 1 Mbaud |
| PB31 | D | digital in | UART RX | `SERCOM5/PAD3` | (same) |

Freed compared to with-DFF single-channel design:
- PA08 (was `CCL_IN3` for DFF.Q to LUT1/IN0)
- PA18 (was `CCL_IN2` for DFF.Q to LUT0/IN2)
- External DFF package and decoupling

Added:
- PA22 mux I = `CCL_IN6` (LUT2/IN0), receives `AC_CMP0` via PCB jumper
- PA25 mux I = `CCL_OUT2` (LUT2/SEQ1 output) — exposes internal `DFF.Q` for
  scope validation. Optional; leave as GPIO if not muxed.

External jumpers required:
- `PA10 → PB10` (`HB_1_2` into LUT1 AND-gate)
- `PA12 → PA22` (`AC_CMP0` into LUT2 DFF D-input)

## 9. TCC0 Heartbeat Procedure

Unchanged from [design-single-channel.md](design-single-channel.md) §6.

## 10. AC Procedure

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC;
GCLK->PCHCTRL[AC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
```

Comparator COMP0:

| Field | Value | Notes |
|---|---|---|
| Comparator | `COMP0` | only one used; COMP1 unused |
| Positive input | `MUXPOS_PIN1`, PA05 / `AC_AIN1` | unchanged |
| Negative input | `VSCALE` / `INTREF` / external — per analog requirement | unchanged |
| Speed | `SPEED_HIGH` | unchanged |
| Filter | `FLEN_OFF` | unchanged |
| Output | `OUT_ASYNC` | unchanged |
| Pad output | PA12 mux H, `AC_CMP0` | jumpered to PA22 |
| INTSEL | reset default | not on the signal path; `COMPEO` is independent (§3.5) |
| `EVCTRL.COMPEO0` | `0` | not used — AC reaches the CCL through the PA12→PA22 loopback |
| ADC | PA05 / `ADC0_AIN5` | unchanged |

`OUT_ASYNC` is required so that PA12 drives `AC_CMP0` continuously (live
level) into the PA22 loopback. Per
[`ac-configuration.md`](llm-wiki/sam/wiki/concepts/ac-configuration.md),
`COMPCTRLn.OUT=OFF` silently blocks the CCL-via-`INSEL=AC` path; for our
loopback path the pad must be live.

If `INTREF` is selected, account for errata DS80000740S §1.5.6 (spurious
COMP interrupt on enable; clear or use `VSCALE`).

## 11. CCL Procedure

Enable clock and clear protection:

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;
GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
CCL->CTRL.reg = 0;
```

Configure SEQCTRL with `CTRL.ENABLE=0`:

```cpp
CCL->SEQCTRL[0].reg = 0;                           // SEQ0 = DISABLE (LUT0+LUT1 normal)
CCL->SEQCTRL[1].reg = CCL_SEQCTRL_SEQSEL_DFF;      // SEQ1 = DFF on LUT2+LUT3
```

Errata DS80000740S §1.7.3: `SEQCTRLx` and `LUTCTRLn` are enable-protected;
write all of them before `CTRL.ENABLE = 1`. Avoid `CCL->CTRL.SWRST`
(errata §1.7.4).

Truth bit-index convention: `bit_index = (IN2 << 2) | (IN1 << 1) | IN0`.

### LUT0 — REF Mux

`REF = Q ? HB_1_8 : HB_7_8`.

| LUT input | `INSEL` | Source |
|---|---:|---|
| IN0 | `TCC` | TCC0/WO[0] = `HB_1_8` |
| IN1 | `TCC` | TCC0/WO[1] = `HB_7_8` |
| IN2 | `EVENT` | Q via EVSYS LUTIN_0 (channel 2) |

`TRUTH = 0xAC`. `LUTCTRL0.ENABLE = 1`. `LUTEO = 0`.
Output: `CCL_OUT0 → PA07 mux I`.

### LUT1 — COUNT_PULSE AND-Gate

`COUNT_PULSE = Q AND HB_1_2`.

| LUT input | `INSEL` | Source |
|---|---:|---|
| IN0 | `LINK` | LUT2.OUT = DFF.Q (via SEQ1 replacement) |
| IN1 | `MASK` | constant 0 |
| IN2 | `IO` | PB10 / `CCL_IN5` = `HB_1_2` loopback from PA10 |

`TRUTH = 0x20`. `LUTCTRL1.ENABLE = 1`. `LUTCTRL1.LUTEO = 1` (feeds EVSYS).
Output: `CCL_OUT1 → PA11 mux I`. EVSYS generator `LUTOUT_1` (83) routes
to `TC2_EVU` for the duty counter.

### LUT2 — DFF D-Input

`D = AC_CMP0` (passthrough of IN0).

| LUT input | `INSEL` | Source |
|---|---:|---|
| IN0 | `IO` | PA22 / `CCL_IN6` = `AC_CMP0` loopback from PA12 |
| IN1 | `MASK` | constant 0 |
| IN2 | `MASK` | constant 0 |

`TRUTH = 0x02`. `LUTCTRL2.ENABLE = 1`. `LUTCTRL2.LUTEO = 1` (feeds EVSYS
channel 2 for Q distribution; also feeds the LINK path to LUT1).
Output: replaced by SEQ1 DFF Q. Goes to:
- `CCL_OUT2 → PA25 mux I` (optional debug)
- EVSYS generator `LUTOUT_2` (84) → channel 2 → LUTIN_0
- LINK: LUT1 IN0 reads this combinationally

### LUT3 — DFF CLK-Input

`CLK = HB_1_2`.

| LUT input | `INSEL` | Source |
|---|---:|---|
| IN0 | `MASK` | constant 0 |
| IN1 | `MASK` | constant 0 |
| IN2 | `TCC` | TCC0/WO[2] = `HB_1_2` (LUT3 wraps to TCC0) |

`TRUTH = 0xF0`. `LUTCTRL3.ENABLE = 1`. `LUTEO = 0`.
**`FILTSEL = SYNCH` (or `FILTER`) + `EDGESEL = 1` are mandatory** — the
SEQ block sees the LUT_odd output as its `G` gate input, and a level
input would degrade SEQ1 from "edge-triggered DFF" to "transparent
gated latch" (datasheet §37.6.2.7 read together with §37.6.2.5/6,
empirically confirmed 2026-05-21 in `src/ccl_seq_dff_test.hpp`).
With `FILTSEL=SYNCH` + `EDGESEL=1`, LUT3 output is a 1 GCLK_CCL strobe
on each rising edge of `HB_1_2`, which is exactly the clock the
external 74HC74 would provide.

Output: LUT3 truth (not replaced by SEQ since LUT3 is LUT_odd of SEQ1).
This output drives the SEQ1 CLK input; it is also potentially visible on
`CCL_OUT3` if PB17 is muxed, but PB17 should be left as GPIO since the
signal is the same as PA10.

### CCL Enable

After all `LUTCTRLn` and `SEQCTRLn` writes:

```cpp
CCL->CTRL.reg = CCL_CTRL_ENABLE;
```

### Summary Table

| LUT | Role | IN2 | IN1 | IN0 | TRUTH | LUTEO | SEQ |
|---:|---|---|---|---|---:|:---:|:---:|
| 0 | REF mux | TCC `HB_7_8`... wait | TCC `HB_7_8` | TCC `HB_1_8` | `0xAC` | 0 | — |
| 1 | AND | IO `HB_1_2` | MASK | LINK Q | `0x20` | 1 | — |
| 2 | DFF D | MASK | MASK | IO `AC_CMP0` (via PA22) | `0x02` | 1 | SEQ1 D |
| 3 | DFF CLK (FILTSEL=SYNCH, EDGESEL=1) | TCC `HB_1_2` | MASK | MASK | `0xF0` | 0 | SEQ1 CLK |

(LUT0 IN2 source: should read "EVENT Q via EVSYS", not "TCC HB_7_8" — the
inline correction is in the LUT0 detail block above.)

## 12. EVSYS and TC32 Counter Procedure

EVSYS channel allocation:

| EVSYS CH | Generator | ID | User | ID | Path |
|---:|---|---:|---|---:|---|
| 0 | `TCC0_OVF` | 35 | `TC0_EVU` | 23 | `ASYNC` |
| 1 | `CCL_LUTOUT_1` | 83 | `TC2_EVU` | 25 | `ASYNC` |
| 2 | `CCL_LUTOUT_2` | 84 | `CCL_LUTIN_0` | 40 | `ASYNC` |

```cpp
// Channel 0: TCC0_OVF -> TC0
EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(1);
EVSYS->CHANNEL[0].reg =
    EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
    EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

// Channel 1: CCL_LUTOUT_1 -> TC2 (duty counter)
EVSYS->USER[EVSYS_ID_USER_TC2_EVU].reg = EVSYS_USER_CHANNEL(2);
EVSYS->CHANNEL[1].reg =
    EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_1) |
    EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

// Channel 2: CCL_LUTOUT_2 (=Q) -> CCL_LUTIN_0 (REF mux IN2)
EVSYS->USER[EVSYS_ID_USER_CCL_LUTIN_0].reg = EVSYS_USER_CHANNEL(3);
EVSYS->CHANNEL[2].reg =
    EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_2) |
    EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
```

TC0+TC1 (window) and TC2+TC3 (duty) configuration is identical to
[design-single-channel.md](design-single-channel.md) §9 (COUNT32, MFRQ for
TC0+TC1, NFRQ for TC2+TC3, `EVCTRL = TCEI | EVACT_COUNT`, RETRIGGER after
ENABLE per §14 finding #7 of the dual-channel J doc).

ISR at window boundary is also identical — read TC2, RETRIGGER TC2,
TC0 wraps automatically.

## 13. Firmware Initialization Order

1. OSC48M / GCLK0 (existing `sys_init()`).
2. Start the 24 MHz XOSC with timeout, configure GCLK1.
3. APB enables: TCC0, TC0..TC3, AC, CCL, EVSYS, SERCOM5.
4. PCHCTRL channels:
   - `TCC0_GCLK_ID = 28` → GCLK1
   - `TC0_GCLK_ID  = 30` → GCLK0
   - `TC2_GCLK_ID  = 31` → GCLK0
   - `AC_GCLK_ID   = 40` → GCLK0
   - `CCL_GCLK_ID  = 38` → GCLK0
5. Configure all pins (PMUX + INEN where needed).
6. Configure AC COMP0 (`OUT_ASYNC`, `MUXPOS_PIN1`); enable AC; wait
   `READY0`. `EVCTRL.COMPEO0` stays at 0 — see §3.5.
7. Configure CCL while `CTRL.ENABLE = 0`:
   - SEQCTRL[0] = 0 (SEQ0 disabled)
   - SEQCTRL[1] = `SEQSEL_DFF`
   - LUTCTRL0..LUTCTRL3 per §11 (include `ENABLE=1` and `LUTEO` as needed)
8. Configure EVSYS channels 0, 1, 2.
9. Configure TC0+TC1 (COUNT32, MFRQ, `CC[0]=N-1`, `EVCTRL = TCEI|EVACT_COUNT`,
   `INTENSET = OVF`).
10. Configure TC2+TC3 (COUNT32, NFRQ, `EVCTRL = TCEI|EVACT_COUNT`).
11. `CCL->CTRL.ENABLE = 1`.
12. Enable TC2+TC3 (ENABLE then RETRIGGER).
13. Enable TC0+TC1 (ENABLE then RETRIGGER).
14. Configure TCC0 heartbeat (`PER`, `CC[0..2]`, `EVCTRL.OVFEO = 1`),
    then ENABLE.
15. `NVIC_EnableIRQ(TC0_IRQn)`.

## 14. Bring-Up Validation Checklist

Order matters — each step depends on the previous one passing.

1. PA09 (`HB_7_8`) and PA10 (`HB_1_2`) show 375 kHz waveforms at expected
   duty cycles. `HB_1_8` has no pad — it's TCC0/WO[0] routed internally only.
2. PA10 loopback appears on PB10 (clean digital input).
3. PA12 (`AC_CMP0`) follows the comparator output as the analog input
   crosses the threshold.
4. PA22 receives the AC_CMP0 loopback from PA12.
5. PA25 (`DFF.Q`, optional scope probe) updates **only on PA10 rising
   edges** and equals the AC_CMP0 value sampled at that instant.
6. PA07 (`REF`) shows the expected mux behaviour driven by `DFF.Q`:
   - Force AC analog above threshold (DFF.Q = 1): `REF` follows `HB_1_8`.
   - Force AC analog below threshold (DFF.Q = 0): `REF` follows `HB_7_8`.
7. PA11 (`COUNT_PULSE`) shows the expected AND:
   - DFF.Q = 1: `COUNT_PULSE` follows `HB_1_2`.
   - DFF.Q = 0: `COUNT_PULSE` stays low.
8. **TC0+TC1 window standalone**: with TCC0 running and `N=187500`
   (500 ms window), `TC0_IRQn` fires every 500 ms ±30 ppm.
9. **TC2 duty with forced Q = 1**: count at window boundary ≈ `N` ±30 ppm.
10. **TC2 duty with forced Q = 0**: count at window boundary = 0.
11. Closed loop: drive PA05 with a known-duty waveform synchronized to the
    heartbeat; verify the TC2/TC0 ratio matches expectations.
12. Stability: repeated windows show stable readings, no boundary drift,
    no missed `TC0_IRQn`.

If step 5 fails (DFF.Q is not stable level updating only on HB_1_2 edges),
the SEQ1 DFF is mis-configured; check `SEQCTRL[1] = SEQSEL_DFF`, `LUTCTRL2`
and `LUTCTRL3` truths and enables.

If step 7 produces glitches near AC_CMP0 transitions, the runt-pulse race
described in §15 is occurring; consider falling back to the external DFF
design or adding an explicit blanking interval.

## 15. Race Analysis (no external DFF)

In the external-DFF design, `DFF.Q` propagates through a discrete chip
(~15 ns t_CQ for 74HC74) and reaches the LUTs via pads. The AND-gate
sees Q stable well before HB_1_2 propagates to its input.

In the internal-DFF design, `DFF.Q` propagates through CCL routing
(~few ns combinational) and reaches LUT1 via LINK. Meanwhile `HB_1_2`
propagates through the PA10→PB10 loopback (~few ns of pad/PCB delay)
to LUT1 IN2.

At an HB_1_2 rising edge where AC_CMP0 has just transitioned (so Q changes
from 0→1 or 1→0):

- If Q's new value reaches LUT1 IN0 **before** HB_1_2 reaches LUT1 IN2:
  no glitch (the AND output starts from the new, correct Q).
- If Q's new value reaches LUT1 IN0 **after** HB_1_2 reaches LUT1 IN2:
  brief runt pulse equal to the difference in propagation times.

The runt pulse can occur regardless of LINK vs EVSYS routing, since
both LUT inputs have small (ns-scale) propagation. The dominant factor
is whether the SEQ1 DFF resolution and Q propagation are faster than
the PA10→PB10 pad path. This is silicon-dependent and not
predictable from datasheet alone.

The TC counter behind LUT1 (TC2 sampling at GCLK0 = 48 MHz, ~20.8 ns
sample period) may or may not register a runt pulse depending on
its duration. §14 of [design-dual-channel.md](design-dual-channel.md)
confirmed TC counts 1-cycle TCC0_OVF pulses (41.7 ns wide) cleanly,
but did not characterize the lower limit.

**Conclusion:** the no-DFF design is functionally equivalent to the
external-DFF design **except** for AC_CMP0 transitions, which may
produce runt pulses at LUT1's output. In bench validation with a static
input (forced Q), the no-DFF path will behave identically to the
external-DFF design — the difference appears only during AC_CMP0
edges. For a final analytical instrument this matters; for the J
bring-up validation it is acceptable and can be characterised on the
scope at step 7 of §14.

## 16. Open Verifications

1. ~~**COMPEO semantics on this silicon.** Is `AC.EVCTRL.COMPEO0 = 1` a
   level passthrough or an edge pulse? Decides whether sub-option A1
   is viable.~~ **Resolved 2026-05-21**: `COMPEO` *is* a level passthrough
   (datasheet §40). However, the CCL's `INSEL=EVENT` slot applies a
   forced rising-edge detector on the J variant (`ASYNCEVENT` bypass is
   N-only, datasheet §37). On the J part the LUT receives only 1-cycle
   strobes, so A1 is still not viable. A2 adopted. See §3.5 and §5.
   Test: `src/ac_compeo_test.hpp`.
2. ~~**LINK reads SEQ output, not raw LUT truth.**~~ **Resolved
   2026-05-21**: confirmed `INSEL=LINK` on LUT_n reads the post-SEQ
   output of LUT_{n+1}, i.e. the latched Q after SEQ1 substitution —
   not the pre-SEQ truth (D). In the discriminator steps of
   `src/ccl_seq_dff_test.hpp` (D ≠ Q held by the DFF), the LUT1 LINK
   pad tracked Q every time, never D. So Q distribution to LUT1 via
   LINK is the correct path for the AND-gate.
3. ~~**SEQCTRL DFF behaviour timing.**~~ **Resolved 2026-05-21**:
   `SEQCTRL.SEQSEL=DFF` is *not* a true rising-edge DFF on its own —
   on the J part it degenerates to a transparent gated latch when the
   LUT_odd output is a level. To get edge-triggered behaviour the
   LUT_odd (CLK source) must enable `LUTCTRL.FILTSEL` (SYNCH or FILTER)
   **and** `LUTCTRL.EDGESEL=1`; the LUT_odd output then becomes a
   1 GCLK_CCL strobe per rising edge of its input, which drives the
   SEQ gate cleanly. With this configuration the test exhibits textbook
   rising-edge DFF behaviour: Q changes only on rising edges of CLK
   (HB_1_2), D changes with CLK held are ignored. Datasheet basis:
   §37.6.2.5 (Filter), §37.6.2.6 (Edge Detector), §37.6.2.7 (Sequential
   Logic) read together. Test: `src/ccl_seq_dff_test.hpp` with LUT3
   `FILTSEL=SYNCH | EDGESEL=1`. §11 LUT3 entry already updated to make
   these bits mandatory.
4. **Runt pulse duration at LUT1 output on AC_CMP0 transitions.**
   Characterize the worst-case runt at step 7 of §14. Determines
   whether the no-DFF design is acceptable for the production target.
   (Still open — scope test, not seriabilizable.)

## 17. Scope and Migration Path

### 17.1 No-DFF does not scale to the dual-channel target

A two-channel implementation requires 2 × {DFF, REF, AND} = 6 LUTs, but
SAMC21 (J and N alike) has only 4 LUTs. The dual-channel N design
[design-dual-channel-N.md](design-dual-channel-N.md) **must keep both
external DFFs**; the no-DFF approach does not extend to it.

### 17.2 Standalone value of the no-DFF single-channel design

Independent of the dual-channel migration, this single-channel design is
a useful end-product in its own right. The current single-channel
prototype is AVR-based and constrained by the AVR timer width: the
measurement window can only be set to small discrete multiples of the
heartbeat period (e.g., multiples of 25 or 50), limiting the achievable
NPLC granularity and SNR/speed trade-offs.

The SAMC21 design — DFF or no-DFF — replaces that with a **32-bit TC
window counter** (TC0+TC1 in COUNT32 MFRQ mode). The window length is
any value in `[1, 2^32)` heartbeat periods, which at 375 kHz spans from
~2.67 µs to ~11 460 s. NPLC values can be set exactly rather than
rounded to a small-integer multiple. This is a meaningful upgrade for
the single-channel workflow even before the dual-channel hardware
arrives.

The no-DFF variant specifically reduces the BOM (no DFF chip, no
clock fanout, no DFF placement) and frees PA08, PA18, and PA12 on the
package. Whether it is the right choice for the production
single-channel board depends on §15 (runt-pulse characterization) and
§16 (open verifications).

### 17.3 Validation value for the N target

Even though no-DFF doesn't extend to the dual-channel N target, the
validations it forces — CCL SEQ DFF correctness, AC `COMPEO` event
semantics (already resolved, §3.5), LINK reading the SEQ output rather
than raw LUT truth — are useful data points for the broader SAMC21
silicon understanding. They will also be available when planning future
single-channel SAM-based work.

## See Also

- [design-single-channel.md](design-single-channel.md) — sibling design with
  external DFF, simpler routing, no race-condition concern
- [design-dual-channel.md](design-dual-channel.md) — original J attempt and
  §14 empirical-findings appendix
- [design-dual-channel-N.md](design-dual-channel-N.md) — production N target
- [llm-wiki/sam/wiki/sources/samc21-datasheet-ch37-ccl.md](llm-wiki/sam/wiki/sources/samc21-datasheet-ch37-ccl.md)
  — CCL `INSEL`, LINK, SEQ details
- [llm-wiki/sam/wiki/concepts/ac-configuration.md](llm-wiki/sam/wiki/concepts/ac-configuration.md)
  — AC `COMPEO`, `OUT`, INTSEL details
- [llm-wiki/sam/wiki/sources/samc21-errata.md](llm-wiki/sam/wiki/sources/samc21-errata.md)
  — DS80000740S erratas applicable to CCL and EVSYS
