# Dual-Channel Design — SAMC21N18A target

ATSAMC21N18A-AU operational design target.
Date: 2026-05-20

This is the N-variant rework of [design-dual-channel.md](design-dual-channel.md).
Same functional requirements, restructured to take advantage of the SAMC21N's
extra TC pairs (TC4+TC5, TC6+TC7) so that **all counters run in hardware
freewheel during the measurement window**. The CPU only intervenes at the
window boundary to read totals and reset for the next window.

The J variant was constrained by having only 2 × 32-bit TC pairs (both
consumed by the channel duty counters), forcing a software accumulator or
free-running cycle counter for the window. N has 4 × 32-bit TC pairs, exactly
matching the requirement.

## 1. Required Behavior (unchanged from J design)

Common hardware:

- 24 MHz crystal drives the heartbeat time base.
- TCC0 generates three phase-aligned PWM signals at 375 kHz:
  - `HB_1_8`: duty 1/8
  - `HB_7_8`: duty 7/8
  - `HB_1_2`: duty 1/2
- A common window counter counts heartbeat periods. At the end of N periods it
  raises an overflow interrupt; ISR reads the channel duty totals and resets.
- PA30 (SWCLK) and PA31 (SWDIO) are reserved for SWD debug.

Per channel:

- One analog input observed by one AC comparator and the ADC.
- AC comparator output routed to an external D flip-flop, clocked by `HB_1_2`.
- DFF output `AC_SYNC = DFF.Q sampled on the active edge of HB_1_2`.
- CCL LUT selects channel reference: `REF_OUT = AC_SYNC ? HB_1_8 : HB_7_8`.
- CCL LUT generates the counted pulse stream: `COUNT_PULSE = AC_SYNC & HB_1_2`.
- Each `COUNT_PULSE` is counted by a 32-bit TC pair.

## 2. Why SAMC21N specifically

| Resource | SAMC21J18A | SAMC21N18A |
|---|---|---|
| 32-bit TC pairs | 2 (TC0+TC1, TC2+TC3) | **4** (+ TC4+TC5, TC6+TC7) |
| Total TC instances | TC0..TC4 (TC4 standalone) | TC0..TC7 |
| TCC instances | TCC0..TCC2 | TCC0..TCC2 (identical) |
| CCL LUTs | 4 | 4 (identical) |
| CCL INSEL options | up to 0x9 (SERCOM) | + 0xA (ALT2TC: TC4..TC7) + 0xB (ASYNCEVENT) |
| EIC features | base | + interrupt pin debouncing |
| Pin count | 64 (J) | 100 (N), includes Port C |

For this design the critical change is the **third 32-bit TC pair** — needed
to host the window counter without sacrificing the channel duty counters or
relying on software accumulators.

The TCC family limitations carried over from J (PCHCTRL[28] shared by
TCC0+TCC1, TCC EVACT broken on ASYNC EVSYS per silicon evidence, errata
1.21.9) are silicon-level behaviors not addressed by the variant change.
The N architecture works *around* them by avoiding TCC user roles entirely
and using TC pairs for all event-counting roles.

## 3. Peripheral Allocation

| Peripheral | Role | Width | Clock | PCHCTRL |
|---|---|---|---|---|
| **TCC0** | Heartbeat generator (HB_1_8, HB_7_8, HB_1_2) | 24-bit | GCLK1 = 24 MHz | 28 |
| TCC1 | unused (PCHCTRL[28] shared, available for secondary timer if needed) | 24-bit | (GCLK1, shared) | 28 |
| TCC2 | unused (available for future) | 16-bit | own | 29 |
| **AC COMP0** | Channel A comparator | — | GCLK0 for register access, OUT_ASYNC for output | 40 |
| **AC COMP1** | Channel B comparator | — | (shared with COMP0) | 40 |
| **TC0+TC1** | **Channel A duty counter** (COUNT32, event-count CCL LUTOUT1) | 32-bit | GCLK0 = 48 MHz | 30 |
| **TC2+TC3** | **Channel B duty counter** (COUNT32, event-count CCL LUTOUT2) | 32-bit | GCLK0 = 48 MHz | 31 |
| **TC4+TC5** | **Window event counter** (COUNT32, event-count TCC0_OVF) | 32-bit | GCLK0 = 48 MHz | 32 |
| TC6+TC7 | spare 32-bit pair | 32-bit | available | 33 |
| **CCL LUT0** | Channel A mux: `REF_A = Q_A ? HB_1_8 : HB_7_8` | — | GCLK0 | 38 |
| **CCL LUT1** | Channel A count pulse: `COUNT_A = Q_A AND HB_1_2` | — | (shared) | 38 |
| **CCL LUT2** | Channel B count pulse: `COUNT_B = Q_B AND HB_1_2` | — | (shared) | 38 |
| **CCL LUT3** | Channel B mux: `REF_B = Q_B ? HB_1_8 : HB_7_8` | — | (shared) | 38 |
| ADC0 | Channel A + Channel B sequential ADC | — | own | 41 |
| EIC | available (used only for SWD diagnostic / debounce if needed) | — | optional | 2 |
| SERCOM5 | UART monitor (existing pattern) | — | own | various |

**Freewheel guarantee**: TC0+TC1, TC2+TC3, TC4+TC5 all run as event counters
during the window. No software intervention. TC4+TC5 OVF fires the window
boundary interrupt; the ISR reads the channel totals and resets for the next
window.

## 4. Clock Plan

| Generator | Source | Frequency | Consumed by |
|---|---|---|---|
| GCLK0 | OSC48M | 48 MHz | CPU, AC, ADC, CCL, TC0+TC1, TC2+TC3, **TC4+TC5** |
| GCLK1 | XOSC (24 MHz crystal) | 24 MHz | TCC0 (heartbeat — needs phase-stable reference) |
| GCLK2 | (unused unless needed for AC or alternative) | — | — |
| GCLK3..GCLK8 | spare | — | — |

Note: TC4+TC5 sampling at GCLK0=48 MHz vs TCC0_OVF (1-cycle pulse on
GCLK1=24 MHz, width 41.7 ns) gives a 2:1 sample/pulse ratio — race-free
capture, as already proven empirically on the J variant.

## 5. Pin Allocation Plan

### 5.1 Master pin table

Sorted by port and pin number for quick cross-check against the Xplained Pro
N pinout. Every line is a pin our design needs (mux configured) — anything
not listed is either reserved by silicon (power, reset, NC) or free for
the board to use.

| Pin | Mux | Dir | Net | Function | Notes / verification |
|---|---|---|---|---|---|
| PA00 | — | analog | XOSC32K_XIN | XOSC32K input | optional, only if `SAM_BOARD_TEST_XOSC32K=1` |
| PA01 | — | analog | XOSC32K_XOUT | XOSC32K output | (same) |
| PA02 | — | — | — | spare | free |
| PA03 | — | analog | VREF_A | VREF conditioning | 22 Ω + 22 nF to GND |
| PA04 | — | analog | VREF_B | VREF conditioning | 22 Ω + 22 nF to GND |
| PA05 | B | analog in | analog_A | `AC_AIN1` + `ADC0_AIN5` | Channel A analog input |
| PA06 | B | analog in | analog_B | `AC_AIN2` + `ADC0_AIN6` | Channel B analog input |
| PA07 | I | digital out | `REF_A` | `CCL_OUT0` (LUT0 output) | Channel A reference-steering output |
| PA08 | I | digital in | DFF_A.Q | `CCL_IN3` (LUT1/IN0) | TCC0/WO[0] **not exposed**; HB_1_8 stays internal via INSEL=TCC |
| PA09 | E | digital out | `HB_7_8` | `TCC0/WO[1]` | scope/debug, fast-ref |
| PA10 | F | digital out | `HB_1_2` / PIN_Clock | `TCC0/WO[2]` | drives external DFF clocks + CCL loopback (PB10, PA24) |
| PA11 | I | digital out | `COUNT_A` | `CCL_OUT1` (LUT1 output) | EVSYS source for TC0+TC1; optional debug pin |
| PA12 | H | digital out | DFF_A.D src | `AC_CMP0` | comparator output routed to external DFF_A.D |
| PA13 | H | digital out | DFF_B.D src | `AC_CMP1` | comparator output routed to external DFF_B.D |
| PA14 | — | analog | XIN | XOSC | 24 MHz crystal |
| PA15 | — | analog | XOUT | XOSC | (same) |
| PA18 | I | digital in | DFF_A.Q | `CCL_IN2` (LUT0/IN2) | mux LUT mux-select input |
| PA22 | I | digital in | DFF_B.Q | `CCL_IN6` (LUT2/IN0) | and-gate LUT signal input |
| PA24 | I | digital in | `HB_1_2` loopback | `CCL_IN8` (LUT2/IN2) | from PA10, and-gate clock input |
| PA25 | I | digital out | `COUNT_B` | `CCL_OUT2` (LUT2 output) | EVSYS source for TC2+TC3; optional debug pin |
| PA30 | — | digital | SWCLK | SWD | reserved for Atmel-ICE / EDBG |
| PA31 | — | digital | SWDIO | SWD | (same) |
| PB10 | I | digital in | `HB_1_2` loopback | `CCL_IN5` (LUT1/IN2) | from PA10, and-gate clock input |
| PB16 | I | digital in | DFF_B.Q | `CCL_IN11` (LUT3/IN2) | mux LUT mux-select input |
| PB17 | I | digital out | `REF_B` | `CCL_OUT3` (LUT3 output) | Channel B reference-steering output |
| PB22 | GPIO | digital in | bench button | GPIO input | candidate for relocation to Xplained board button |
| PB23 | GPIO | digital out | bench LED | GPIO output | candidate for relocation to Xplained board LED |
| PB30 | D | digital out | UART TX | `SERCOM5/PAD2` | verify EDBG VCOM mapping on Xplained N |
| PB31 | D | digital in | UART RX | `SERCOM5/PAD3` | (same) |
| **PC[*]** | — | — | — | spare | entire Port C is N-variant exclusive, **available for relocation if PA/PB conflicts arise** |

**Total assigned: 28 pins** out of 84 GPIO available on SAMC21N18A. The
remaining 56 pins (most of Port C + free PA/PB) are available as
fallbacks or for future extensions.

### 5.2 Critical pins by function (for design intent reference)

The same pins grouped by role, for when you need to understand *why* a
particular pin was chosen rather than *what* uses it.

#### Heartbeat outputs (must be free on Xplained N)

| Net | MCU pin | Mux | Function | Xplained N? |
|---|---|---|---|---|
| `HB_1_8` (internal to LUT0/LUT3) | PA08 | I | `CCL_IN3` (DFF_A.Q → LUT1 IN0); WO[0] not exposed | check |
| `HB_7_8` | PA09 | E | `TCC0/WO1` (debug / scope) | check |
| `HB_1_2` | PA10 | F | `TCC0/WO2` (DFF clock, CCL loopback source) | check |

Alternative pins for `TCC0/WO[0..2]` are available on N (see datasheet Table
6-1 column F for mux F) if the Xplained N board uses PA08/09/10 for its own
purposes:
- WO[0] alt: PA12 (F), PA15 (F), PA22 (F)
- WO[1] alt: PA13 (F), PA14 (F), PA23 (F)
- WO[2] alt: PA18 (F)

#### AC comparator and ADC analog inputs

| Net | MCU pin | Mux | Function |
|---|---|---|---|
| Channel A analog | PA05 | B | `AC_AIN1`, `ADC0_AIN5` |
| Channel B analog | PA06 | B | `AC_AIN2`, `ADC0_AIN6` |
| Channel A comparator output | PA12 | H | `AC_CMP0` → DFF_A.D |
| Channel B comparator output | PA13 | H | `AC_CMP1` → DFF_B.D |

#### CCL function-I inputs (PCB loopback targets)

| Net | MCU pin | Mux | CCL slot |
|---|---|---|---|
| DFF_A.Q → LUT0 (mux input) | PA18 | I | LUT0/IN2 (`CCL_IN2`) |
| DFF_A.Q → LUT1 (and input) | PA08 | I | LUT1/IN0 (`CCL_IN3`) |
| DFF_B.Q → LUT3 (mux input) | PB16 | I | LUT3/IN2 (`CCL_IN11`) |
| DFF_B.Q → LUT2 (and input) | PA22 | I | LUT2/IN0 (`CCL_IN6`) |
| HB_1_2 → LUT1 (clock input) | PB10 | I | LUT1/IN2 (`CCL_IN5`) |
| HB_1_2 → LUT2 (clock input) | PA24 | I | LUT2/IN2 (`CCL_IN8`) |

#### CCL outputs (LUTOUT)

| Net | MCU pin | Mux | Function |
|---|---|---|---|
| LUT0 OUT = `REF_A` | PA07 | I | drives external ref-steering for channel A |
| LUT1 OUT = `COUNT_A` | PA11 | I | debug + EVSYS source for TC0+TC1 |
| LUT2 OUT = `COUNT_B` | PA25 | I | debug + EVSYS source for TC2+TC3 |
| LUT3 OUT = `REF_B` | PB17 | I | drives external ref-steering for channel B |

#### Clock / debug / utility pins

| Net | MCU pin | Function |
|---|---|---|
| XOSC XIN | PA14 | 24 MHz crystal |
| XOSC XOUT | PA15 | 24 MHz crystal |
| XOSC32K XIN (optional) | PA00 | populated only if SAM_BOARD_TEST_XOSC32K=1 |
| XOSC32K XOUT (optional) | PA01 | populated only if SAM_BOARD_TEST_XOSC32K=1 |
| SWCLK | PA30 | Atmel-ICE / Atmel-EDBG SWD |
| SWDIO | PA31 | (same) |
| VREF | PA03 | 22 Ω + 22 nF conditioning |
| VREF | PA04 | (same) |
| UART TX | PB30 | SERCOM5 PAD2 mux D |
| UART RX | PB31 | SERCOM5 PAD3 mux D |
| Bench LED | (Xplained board LED) | check Xplained allocation |
| Bench button | (Xplained board button) | check Xplained allocation |

### 5.3 Pins newly free on N vs J

The third 32-bit TC pair (TC4+TC5) is the window counter; **it consumes NO
pins** (it only reads EVSYS events internally). So compared to J, no extra
pin is needed for this role. The freewheel architecture is essentially
"free" in pin terms.

The PCB loopback for HB_1_2 (PA10 → PB10, PA24) and the channel B analog
(PA06 → AC_CMP1 → DFF_B.D → fanout to PB16, PA22) are unchanged.

## 6. EVSYS Channel Assignment

| EVSYS CH | Generator | Generator ID | User | User ID | Path |
|---:|---|---:|---|---:|---|
| 0 | TCC0 OVF | 35 | TC4_EVU (= TC4+TC5 event input) | 27+1=? *(see note)* | ASYNC |
| 1 | CCL LUTOUT1 | 83 | TC0_EVU | 23 | ASYNC |
| 2 | CCL LUTOUT2 | 84 | TC2_EVU | 25 | ASYNC |

> **Note on user ID for TC4+TC5**: on N variant we need to verify the exact
> `EVSYS_ID_USER_TCx_EVU` for the TC4+TC5 pair. On J the TC4 EVU is at
> index 27. The N variant adds TC5_EVU, TC6_EVU, TC7_EVU at subsequent
> indices (likely 47–49 per the J reference table in §29.6.3); the N
> CMSIS header is authoritative.

EVSYS path: ASYNC throughout, per errata DS80000740S 1.21.9 and the
empirically-validated working pattern from the J variant (TC4 16-bit test
gave perfect event capture on async with independent clock).

## 7. TC4+TC5 Window Counter Procedure

Sketch — confirms freewheel:

```cpp
// Enable APB clocks
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC4 | MCLK_APBCMASK_TC5;

// GCLK0 = 48 MHz to TC4 PCHCTRL
GCLK->PCHCTRL[TC4_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[TC4_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

// Configure TC4 as COUNT32 master (TC5 follows automatically).
// SAMC21 keeps WAVEGEN in a separate WAVE register, unlike SAMD21
// where it sits in CTRLA — applies to both J and N variants.
TC4->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
while (TC4->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST);
TC4->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
TC4->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
TC4->COUNT32.CC[0].reg = N_periods_per_window - 1;  // MFRQ: TOP = CC0
while (TC4->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CC0);

// Event input: count TCC0 OVF events
TC4->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;

// Enable overflow interrupt (window boundary)
TC4->COUNT32.INTENSET.reg = TC_INTENSET_OVF;
NVIC_EnableIRQ(TC4_IRQn);

// Enable
TC4->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
while (TC4->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE);
TC4->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
while (TC4->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB);
```

ISR at window boundary:

```cpp
extern "C" void irq_handler_tc4(void)
{
    if (TC4->COUNT32.INTFLAG.reg & TC_INTFLAG_OVF) {
        TC4->COUNT32.INTFLAG.reg = TC_INTFLAG_OVF;  // W1C

        // Read channel duty totals (COUNT32 readsync)
        TC0->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
        while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_COUNT);
        const uint32_t duty_a = TC0->COUNT32.COUNT.reg;

        TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
        while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_COUNT);
        const uint32_t duty_b = TC2->COUNT32.COUNT.reg;

        // Reset for next window
        TC0->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
        TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;

        // TC4+TC5 auto-wraps in MFRQ mode — next window starts immediately

        // Hand totals to application layer
        on_window_complete(duty_a, duty_b);
    }
}
```

**No ISR fires during the window itself.** The ISR rate is `1 / window_time`
= 5 Hz for 200 ms windows = trivial.

## 8. Window Sizing

| Window | Periods (375 kHz heartbeat) | TC4+TC5 PER value | Fits 32-bit? |
|---|---|---|---|
| 1 NPLC @ 50 Hz | 7 500 | 7 499 | yes |
| 10 NPLC @ 50 Hz | 75 000 | 74 999 | yes |
| 100 NPLC @ 50 Hz | 750 000 | 749 999 | yes |
| 1000 NPLC @ 50 Hz | 7 500 000 | 7 499 999 | yes |
| Max | 4 294 967 296 / 375 000 = 11 460 s | 0xFFFFFFFE | yes (~3 hour windows possible) |

Effectively unbounded for practical NPLC values.

## 9. Open Items for SAMC21N Xplained Pro Verification

Things to check against the Xplained Pro N user guide before committing:

1. **PA08, PA09, PA10**: heartbeat WO output pins. Xplained boards
   sometimes route these to onboard LEDs or buttons. If contested, move to
   alt mux F pins (see §5).
2. **PA05, PA06**: analog inputs for AC and ADC. If contested, alt analog
   pins on Port B or C exist.
3. **PA12, PA13**: AC comparator outputs (mux H). On N these are unique to
   these pins — verify they're free.
4. **PA07, PA11, PA25, PB17**: CCL LUTOUT pins. If any is contested, the
   CCL outputs can route to other pins of the same LUT (check Table 37-4
   on N variant CCL chapter).
5. **PA18, PA08, PA22, PA24, PB10, PB16**: CCL function-I inputs (PCB
   loopback targets). Some flexibility per LUT/slot if alternative pins
   exist.
6. **PA30, PA31**: SWD — verify the Xplained EDBG routes SWD here (it
   should, but the board might have a header that provides alternative
   access).
7. **PB30, PB31**: SERCOM5 UART. The Xplained EDBG typically provides a
   virtual COM port via SERCOM somewhere — verify it's compatible or
   choose a different SERCOM.
8. **TC6+TC7 pair**: not used in this design but available — verify it
   exists per N CMSIS header.

## 10. Bring-up Validation Plan

In order of escalating complexity:

1. **Quartz + serial bring-up** — same as J, verify XOSC starts, UART
   prints at 1 Mbaud.
2. **TCC0 heartbeat alone** — scope confirms PA08/09/10 generate the
   expected duties at 375 kHz.
3. **TC4+TC5 window event counter alone** — replicate the J variant's
   working pattern (TC4 16-bit + EVCTRL TCEI|EVACT_COUNT) but in COUNT32
   mode on TC4+TC5. Expected: 187 500 events in 500 ms with TCC0_OVF as
   source. Confidence high (J variant TC4 16-bit already proven).
4. **AC + DFF + CCL chain** — same as J validation §12, scope-validate
   each LUT output.
5. **Full system** — TC0+TC1 and TC2+TC3 count channel-gated events
   during a 200 ms window, TC4+TC5 wraps, ISR reads totals, restart.

## 11. Migration Notes from J

When porting firmware from the J prototype:

- Change `__SAMC21J18A__` → `__SAMC21N18A__` in `platformio.ini` and CMSIS
- Pin allocation table in §5 is the new authoritative source
- `TC4_GCLK_ID` is index 32 on both J and N (per shared family datasheet)
- TC5_MASTER bit is 0 (slave to TC4); the 32-bit COUNT register is accessed
  through `TC4->COUNT32.*`
- `EVSYS_ID_USER_TC4_EVU` index — verify in the N CMSIS header (may
  differ from J's value of 27 since N adds users for TC5, TC6, TC7)
- The TCC1 free-running window-cycle pattern from the J workaround is no
  longer needed — drop it
- The PA10↔PA11 EIC loopback patch from the J workaround is no longer
  needed — drop it
- The TCC1 event-counting code (never validated on silicon, found broken
  empirically) is dropped — TC4+TC5 takes that role with the validated
  TC EVACT=COUNT pattern

## 12. What Stays the Same as J

For reference / sanity:

- CCL LUT TRUTH tables (0xAC for mux, 0x20 for AND gate)
- AC `OUT_ASYNC` requirement (per errata 1.5.6 considerations)
- ADC0 sequential channel A/B sampling
- External DFF chip and clock routing
- VREF conditioning (22 Ω + 22 nF on PA03 and PA04)
- SWD pins (PA30/PA31) reservation
- UART debug pattern via SERCOM5
- The errata-driven `PATH_ASYNCHRONOUS` choice for all EVSYS channels
- The heartbeat math: TCC0 PER=63, CC[0]=8, CC[1]=56, CC[2]=32 → 375 kHz

## See Also

- [design-dual-channel.md](design-dual-channel.md) — original J variant
  design, retained as reference for the constraints encountered
- [llm-wiki/sam/wiki/sources/samc21-errata.md](llm-wiki/sam/wiki/sources/samc21-errata.md)
  — SAM C20/C21 errata DS80000740S, applies to both variants
- SAMC21N18A datasheet — for definitive pin tables and CCL/EVSYS chapter
  variations relative to J
