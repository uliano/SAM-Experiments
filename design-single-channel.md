# Single-Channel Design — SAMC21J18A target

ATSAMC21J18A-AU operational design target.
Date: 2026-05-21

This document specifies a **single-channel** version of the heartbeat / AC / CCL /
TC counting chain, designed to fit on the J variant we already have on the
bench. Its purpose is to prove on real silicon — before committing to a SAMC21N
board build — that every piece of the chain works: TCC0 heartbeat, external DFF
sampling, CCL mux/and-gate, gated TC32 event counting, and an independent TC32
window counter driven by TCC0 OVF.

Once this single-channel design is validated on the J, the full dual-channel
design [design-dual-channel-N.md](design-dual-channel-N.md) becomes a
mechanical extension (one extra channel, identical patterns) on the N.

## 1. Required Behavior

- 24 MHz crystal drives the heartbeat time base.
- TCC0 generates three phase-aligned PWM signals at 375 kHz:
  - `HB_1_8`: duty 1/8 — internal only (no pad), routed via `INSEL=TCC` into LUT0
  - `HB_7_8`: duty 7/8 — `TCC0/WO1` on PA09
  - `HB_1_2`: duty 1/2 — `TCC0/WO2` on PA10, drives external DFF clock and CCL loopback
- One analog input is observed by `AC COMP0` and by `ADC0`.
- `AC_CMP0` clocks an external D flip-flop whose data input is the comparator
  output. The DFF is clocked by `HB_1_2`:

  ```text
  AC_SYNC = DFF.Q sampled on the rising edge of HB_1_2
  ```

- CCL LUT0 generates the channel reference waveform:

  ```text
  REF_OUT = AC_SYNC ? HB_1_8 : HB_7_8
  ```

- CCL LUT1 generates the gated count pulse stream:

  ```text
  COUNT_PULSE = AC_SYNC AND HB_1_2
  ```

- `TC2+TC3` COUNT32 counts rising edges of `COUNT_PULSE` (event-counted via
  EVSYS ASYNC). This is the **duty counter**.
- `TC0+TC1` COUNT32 counts `TCC0_OVF` events (one per heartbeat period). At
  `COUNT == N-1` it wraps and raises the window-boundary interrupt. This is
  the **window counter**.
- At each window boundary the ISR reads the duty counter, resets it, and
  hands the total to the application layer. The window counter auto-wraps in
  MFRQ mode and starts the next window immediately.
- PA30 (SWCLK) and PA31 (SWDIO) are reserved for the Atmel-ICE SWD interface
  and remain unmuxed.

## 2. Why This Validates the N Migration

The dual-channel design on J fails for one reason: **TCC family does not
event-count on ASYNC EVSYS** (empirical evidence in
[design-dual-channel.md](design-dual-channel.md) §14). On J the only available
extra event counter for the window role was TCC1, which is broken in that
configuration.

This single-channel design avoids the broken path entirely: the window
counter is `TC0+TC1` COUNT32, identical in pattern to the duty counter and
identical to the working TC4-16bit test that validated the path in iteration
#8 of §14. There are exactly **two TC32 pairs on J — both are used here**.
The single-channel design therefore exercises every silicon mechanism that
the N dual-channel design relies on:

| Mechanism | Validated by | Status going into this design |
|---|---|---|
| TCC0 NPWM heartbeat at 375 kHz, three phase-aligned outputs | dual-channel J bring-up | already proven on bench |
| External DFF sampling AC_CMP on HB_1_2 edge | not yet exercised on full chain | **to validate here** |
| CCL LUT mux (TRUTH=0xAC, INSEL=TCC + IO) | not yet exercised | **to validate here** |
| CCL LUT AND-gate (TRUTH=0x20, MASK + IO + IO) | not yet exercised | **to validate here** |
| EVSYS ASYNC `CCL_LUTOUT_n → TCx_EVU`, TC `EVACT_COUNT` | §10 plan from dual J (never executed) | **to validate here** |
| EVSYS ASYNC `TCC0_OVF → TCx_EVU`, TC `EVACT_COUNT` | §14 iteration #8 (TC4 16-bit, perfect count) | **scale to TC0+TC1 COUNT32 here** |
| TC32 `READSYNC` + reset in ISR | standard TC pattern, partially exercised | **to validate boundary handling** |

If everything in this design works on the J, the N migration is purely
mechanical: add channel B (a second AC + DFF + LUT0/LUT3 + LUT2 + TC pair)
and move the window counter from TC0+TC1 to TC4+TC5. Every individual
mechanism has already been proven.

## 3. Peripheral Allocation

| Peripheral | Role | Width | Clock | PCHCTRL |
|---|---|---|---|---:|
| TCC0 | Heartbeat generator (HB_1_8, HB_7_8, HB_1_2) | 24-bit | GCLK1 = 24 MHz | 28 |
| AC COMP0 | Single-channel comparator | — | GCLK0 register access; `OUT_ASYNC` output | 40 |
| ADC0 | Analog readback (PA05 / `ADC0_AIN5`) | 12-bit | own | 41 |
| CCL LUT0 | Reference mux: `REF = Q ? HB_1_8 : HB_7_8` | — | GCLK0 | 38 |
| CCL LUT1 | Count pulse: `COUNT = Q AND HB_1_2` | — | GCLK0 (shared) | 38 |
| TC0+TC1 | Window counter (COUNT32, MFRQ, event-count `TCC0_OVF`) | 32-bit | GCLK0 = 48 MHz | 30 |
| TC2+TC3 | Duty counter (COUNT32, NFRQ, event-count `CCL_LUTOUT1`) | 32-bit | GCLK0 = 48 MHz | 31 |
| EVSYS CH0 | `TCC0_OVF` → `TC0_EVU` (ASYNC) | — | — | — |
| EVSYS CH1 | `CCL_LUTOUT_1` → `TC2_EVU` (ASYNC) | — | — | — |
| SERCOM5 | UART monitor, 1 Mbaud (existing pattern) | — | own | various |

LUT2, LUT3, TCC1, TCC2, TC4, EIC are **unused** and available for ancillary
debug.

## 4. Clock Plan

| Generator | Source | Frequency | Consumed by |
|---|---|---|---|
| GCLK0 | OSC48M | 48 MHz | CPU, AC, ADC, CCL, **TC0+TC1**, **TC2+TC3** |
| GCLK1 | XOSC (24 MHz crystal) | 24 MHz | TCC0 |
| GCLK2..GCLK8 | — | — | unused in this design |

The critical relationship: **TC0+TC1 and TC2+TC3 sample at GCLK0 = 48 MHz**,
while their event sources (TCC0 on GCLK1, CCL combinational) are slower or
unrelated. This gives the race-free capture proven by §14 iteration #8/#10
(TC4 sampling TCC0_OVF at GCLK0 = 48 MHz, perfect accuracy).

No GCLK3 (EIC) is needed — the EIC bypass trick from the dual-channel J
design is dropped along with the TCC1 window counter.

## 5. Pin Allocation

Sorted by port and pin. Anything not listed is free.

| Pin | Mux | Dir | Net | Function | Notes |
|---|---|---|---|---|---|
| PA00 | — | analog | `XOSC32K_XIN` | XOSC32K input | optional, only if `SAM_BOARD_TEST_XOSC32K=1` |
| PA01 | — | analog | `XOSC32K_XOUT` | XOSC32K output | (same) |
| PA03 | — | analog | `VREF` | VREF conditioning | 22 Ω + 22 nF to GND |
| PA04 | — | analog | `VREF` | VREF conditioning | 22 Ω + 22 nF to GND |
| PA05 | B | analog in | `analog_in` | `AC_AIN1` + `ADC0_AIN5` | Single-channel analog input |
| PA07 | I | digital out | `REF` | `CCL_OUT0` (LUT0 output) | Reference-steering output |
| PA08 | I | digital in | `DFF.Q` | `CCL_IN3` (LUT1/IN0) | TCC0/WO[0] not exposed; `HB_1_8` stays internal via `INSEL=TCC` |
| PA09 | E | digital out | `HB_7_8` | `TCC0/WO1` | scope / debug, fast-ref |
| PA10 | F | digital out | `HB_1_2` | `TCC0/WO2` | drives external DFF clock + CCL loopback (PB10) |
| PA11 | I | digital out | `COUNT_PULSE` | `CCL_OUT1` (LUT1 output) | EVSYS source for TC2+TC3; optional scope probe |
| PA12 | H | digital out | `AC_CMP0` | `AC_CMP0` | comparator output → DFF.D |
| PA14 | — | analog | `XIN` | XOSC | 24 MHz crystal |
| PA15 | — | analog | `XOUT` | XOSC | (same) |
| PA18 | I | digital in | `DFF.Q` | `CCL_IN2` (LUT0/IN2) | LUT0 mux-select input |
| PA30 | — | digital | `SWCLK` | SWD | reserved for Atmel-ICE |
| PA31 | — | digital | `SWDIO` | SWD | (same) |
| PB10 | I | digital in | `HB_1_2` loopback | `CCL_IN5` (LUT1/IN2) | from PA10, and-gate clock input |
| PB22 | GPIO | digital in | bench button | GPIO input | existing bench fixture |
| PB23 | GPIO | digital out | bench LED | GPIO output | existing bench fixture |
| PB30 | D | digital out | UART TX | `SERCOM5/PAD2` | COM5 @ 1 Mbaud |
| PB31 | D | digital in | UART RX | `SERCOM5/PAD3` | (same) |

The DFF.Q net must fan out to **two MCU input pins** (PA08 and PA18) because
LUT0/IN2 and LUT1/IN0 are separate CCL cells with no internal "reuse this IO
input in another LUT" path. This is the same constraint per channel as in
the dual-channel design.

The `HB_1_2` loopback from PA10 to PB10 is a single external jumper.

## 6. TCC0 Heartbeat Procedure

Unchanged from the J dual-channel design. Already validated on bench.

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC0;
GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[TCC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
```

Configure:

| Register | Value | Result |
|---|---:|---|
| `CTRLA.PRESCALER` | `DIV1` | TCC0 runs at 24 MHz |
| `WAVE.WAVEGEN` | `NPWM` | Single-slope PWM |
| `PER` | 63 | 64 ticks per heartbeat period |
| `CC[0]` | 8 | `HB_1_8`, high for 8/64 ticks |
| `CC[1]` | 56 | `HB_7_8`, high for 56/64 ticks |
| `CC[2]` | 32 | `HB_1_2`, high for 32/64 ticks |
| `EVCTRL.OVFEO` | 1 | TCC0 overflow event marks one heartbeat period |

Pins:

```text
PA09 mux E = TCC0/WO1
PA10 mux F = TCC0/WO2
PA08 mux I = CCL_IN3   (DFF.Q input; HB_1_8 has no physical output)
```

Expected waveform:

```text
f_heartbeat = 24 MHz / 64 = 375 kHz
T_heartbeat = 2.666666 us
HB_1_8 high time = 333.333 ns
HB_7_8 high time = 2.333333 us
HB_1_2 high time = 1.333333 us
```

Enable TCC0 last in the bring-up order (§11), after all event consumers are
configured and ready to receive `TCC0_OVF`.

## 7. AC and ADC Procedure

Single comparator, single ADC channel.

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC;
GCLK->PCHCTRL[AC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[AC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
```

`GCLK_AC` is required for AC register access. It is **not** used to retime the
comparator output (`OUT_ASYNC` keeps the comparator output combinational).

| Field | Value |
|---|---|
| Comparator | `COMP0` |
| Positive input | `MUXPOS_PIN1`, PA05 / `AC_AIN1` |
| Negative input | Project threshold: `VSCALE`, `INTREF`, or DAC/reference path |
| Speed | `SPEED_HIGH` |
| Filter | `FLEN_OFF` |
| Output | `OUT_ASYNC` |
| Pad output | PA12 mux H, `AC_CMP0` → DFF.D |
| ADC | PA05 / `ADC0_AIN5` |

`OUT_ASYNC` is required — `OUT_SYNC` was measured on this silicon to add up
to two `GCLK_AC` cycles of lag, breaking the DFF setup timing.

If `INTREF` is the negative input, account for errata DS80000740S §1.5.6: a
spurious COMP interrupt may occur at enable. Discard/clear the first interrupt
or use `VSCALE` when it satisfies the analog requirement.

ADC scheduling is not specified here; the application layer drives sequential
ADC reads of PA05 at whatever cadence makes sense.

## 8. CCL Procedure

Only LUT0 and LUT1 are used. LUT2 and LUT3 are left disabled.

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;
GCLK->PCHCTRL[CCL_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[CCL_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
CCL->CTRL.reg = 0;
CCL->SEQCTRL[0].reg = 0;
CCL->SEQCTRL[1].reg = 0;
```

Per errata DS80000740S §1.7.3, `SEQCTRLx` and `LUTCTRLx` are enable-protected:
write all four `LUTCTRLn` (and both `SEQCTRLn`) **before** setting
`CTRL.ENABLE`. Do not use `CCL->CTRL.SWRST` (errata §1.7.4 reports a PAC
protection error on software reset).

Truth-table bit index: `bit_index = (IN2 << 2) | (IN1 << 1) | IN0`.

### LUT0 — Reference Mux

Purpose: `REF = Q ? HB_1_8 : HB_7_8`.

| LUT input | `INSEL` | Source |
|---|---:|---|
| IN0 | `TCC` | TCC0/WO0 = `HB_1_8` |
| IN1 | `TCC` | TCC0/WO1 = `HB_7_8` |
| IN2 | `IO` | PA18 / `CCL_IN2` = DFF.Q |

`TRUTH = 0xAC` (if instead the polarity is `Q ? HB_7_8 : HB_1_8`, use `0xCA`).

Output: `LUT0 → CCL_OUT0 → PA07 mux I = REF`.

Required bits: `LUTCTRL0.ENABLE = 1`. `LUTEO` not required (the LUT output
is consumed only as a pad signal, not as an EVSYS generator).

### LUT1 — Count Pulse

Purpose: `COUNT_PULSE = Q AND HB_1_2`.

| LUT input | `INSEL` | Source |
|---|---:|---|
| IN0 | `IO` | PA08 / `CCL_IN3` = DFF.Q |
| IN1 | `MASK` | Constant 0 |
| IN2 | `IO` | PB10 / `CCL_IN5` = external loopback from PA10 `HB_1_2` |

`TRUTH = 0x20` (with IN1 masked to 0, only the IN2·IN0 product term survives).

Required bits: `LUTCTRL1.ENABLE = 1`, `LUTCTRL1.LUTEO = 1` (LUT1 feeds EVSYS).

Output: `LUT1 → CCL_OUT1 → PA11 mux I` (optional scope/debug pin).
Event:  `LUT1 → EVSYS_ID_GEN_CCL_LUTOUT_1 = 83`.

### CCL Enable

After all four `LUTCTRLn` and both `SEQCTRLn` writes:

```cpp
CCL->CTRL.reg = CCL_CTRL_ENABLE;
```

### Summary Table

| LUT | Function | IN2 | IN1 | IN0 | Truth | LUTEO |
|---:|---|---|---|---|---:|:---:|
| 0 | `REF` mux | PA18 `Q` | TCC0/WO1 `HB_7_8` | TCC0/WO0 `HB_1_8` | `0xAC` | 0 |
| 1 | `COUNT_PULSE` and | PB10 `HB_1_2` | MASK | PA08 `Q` | `0x20` | 1 |
| 2 | disabled | — | — | — | — | — |
| 3 | disabled | — | — | — | — | — |

## 9. EVSYS and TC32 Counter Procedure

Two EVSYS channels, both ASYNC.

```cpp
MCLK->APBCMASK.reg |= MCLK_APBCMASK_EVSYS
                    | MCLK_APBCMASK_TC0
                    | MCLK_APBCMASK_TC1
                    | MCLK_APBCMASK_TC2
                    | MCLK_APBCMASK_TC3;
GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
GCLK->PCHCTRL[TC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
```

### EVSYS Routes

| EVSYS CH | Generator | ID | User | ID | Path |
|---:|---|---:|---|---:|---|
| 0 | `TCC0_OVF` | 35 | `TC0_EVU` | 23 | `ASYNC` |
| 1 | `CCL_LUTOUT_1` | 83 | `TC2_EVU` | 25 | `ASYNC` |

```cpp
EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(1);  // channel 0 + 1
EVSYS->CHANNEL[0].reg =
    EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_TCC0_OVF) |
    EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

EVSYS->USER[EVSYS_ID_USER_TC2_EVU].reg = EVSYS_USER_CHANNEL(2);  // channel 1 + 1
EVSYS->CHANNEL[1].reg =
    EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_CCL_LUTOUT_1) |
    EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
```

`ASYNC` is required by errata DS80000740S §1.21.9 and is the empirically-proven
working path for TC event counters (§14 finding #2).

### TC0+TC1 — Window Counter (COUNT32, MFRQ)

| Register | Value | Notes |
|---|---|---|
| `CTRLA.MODE` | `COUNT32` | TC0 is master, TC1 slave |
| `CTRLA.PRESCALER` | `DIV1` | clock not used for counting; events bypass prescaler |
| `WAVE.WAVEGEN` | `MFRQ` | TOP = `CC[0]`, counter wraps to 0 on match. On SAMC21 `WAVEGEN` is in a separate `WAVE` register, **not** in `CTRLA` as on SAMD21. |
| `CC[0]` | `N - 1` | `N` = number of heartbeat periods per window |
| `EVCTRL` | `TCEI \| EVACT_COUNT` | one increment per `TCC0_OVF` event |
| `INTENSET` | `OVF` | fires at window boundary |

```cpp
TC0->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST);
TC0->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
TC0->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
TC0->COUNT32.CC[0].reg = N - 1;
while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CC0);
TC0->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
TC0->COUNT32.INTENSET.reg = TC_INTENSET_OVF;
NVIC_EnableIRQ(TC0_IRQn);
```

### TC2+TC3 — Duty Counter (COUNT32, NFRQ)

| Register | Value | Notes |
|---|---|---|
| `CTRLA.MODE` | `COUNT32` | TC2 is master, TC3 slave |
| `CTRLA.PRESCALER` | `DIV1` | events bypass prescaler |
| `WAVE.WAVEGEN` | `NFRQ` | free-running up-counter; no wrap during window. Same `WAVE`-register caveat as TC0+TC1 above. |
| `EVCTRL` | `TCEI \| EVACT_COUNT` | one increment per `CCL_LUTOUT_1` rising edge |
| `INTENSET` | 0 | no interrupt; CPU reads at window boundary |

```cpp
TC2->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST);
TC2->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1;
TC2->COUNT32.WAVE.reg = TC_WAVE_WAVEGEN_NFRQ;
TC2->COUNT32.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT;
```

### Enable Sequence

For both pairs, `ENABLE=1` then `RETRIGGER`:

```cpp
TC0->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE);
TC0->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
while (TC0->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB);

TC2->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_ENABLE);
TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_CTRLB);
```

`RETRIGGER` after `ENABLE` is required when `EVCTRL.EVACT != OFF` — without
it the counter stays in `STATUS.STOP` and silently ignores events (§14
finding #7).

### Window-Boundary ISR

```cpp
extern "C" void irq_handler_tc0(void)
{
    if (TC0->COUNT32.INTFLAG.reg & TC_INTFLAG_OVF) {
        TC0->COUNT32.INTFLAG.reg = TC_INTFLAG_OVF;  // W1C

        // Snapshot the duty counter
        TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
        while (TC2->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_COUNT);
        const uint32_t duty = TC2->COUNT32.COUNT.reg;

        // Reset duty counter for the next window
        TC2->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
        // TC0+TC1 auto-wraps in MFRQ mode — next window starts immediately

        on_window_complete(duty);
    }
}
```

**Boundary precision note.** TC `RETRIGGER` resets `COUNT` to 0 (§14 finding
#4). Events generated by CCL between the TC0 wrap and the TC2 retrigger are
lost. For the first bring-up this is acceptable (the loss is one or two
events per ~75 000, < 30 ppm). If higher precision is needed later, the
options are:

1. Use a TCC0_OVF-driven hardware reset of TC2 (no firmware path on J:
   TC2 already consumes its single event input from CCL).
2. Add a blanking interval inside the heartbeat where the comparator is
   forced low between windows.

Both are deferred until the basic chain is proven.

### Window Sizing

| Window | Periods | TC0 CC[0] | TC2 max count | Fits 32-bit? |
|---|---:|---:|---:|:---:|
| 1 NPLC @ 50 Hz | 7 500 | 7 499 | 7 500 | yes |
| 10 NPLC @ 50 Hz | 75 000 | 74 999 | 75 000 | yes |
| 100 NPLC @ 50 Hz | 750 000 | 749 999 | 750 000 | yes |
| 1000 NPLC @ 50 Hz | 7 500 000 | 7 499 999 | 7 500 000 | yes |
| Max | 4 294 967 296 | 0xFFFFFFFE | depends on duty | yes (~11 460 s window) |

## 10. External DFF

Use one D-type flip-flop (e.g., half of a 74HC74 or faster voltage-compatible
equivalent).

| DFF pin | Connection |
|---|---|
| `D` | PA12 (`AC_CMP0`) |
| `CLK` | PA10 (`HB_1_2`) |
| `Q` | fan out to PA08 (LUT1/IN0) AND PA18 (LUT0/IN2) |
| `/PRE` | pulled high (inactive) |
| `/CLR` | pulled high (inactive) |

`Q` updates on the rising edge of `HB_1_2`.

## 11. Firmware Initialization Order

1. Start OSC48M / GCLK0 as in current `sys_init()`.
2. Start the 24 MHz XOSC with timeout and configure GCLK1.
3. Enable APB clocks: APBC for EVSYS, TCC0, TC0..TC3, AC, CCL, and APBA for SERCOM5.
4. Configure PCHCTRL channels:
   - `TCC0_GCLK_ID = 28` → GCLK1
   - `TC0_GCLK_ID  = 30` → GCLK0
   - `TC2_GCLK_ID  = 31` → GCLK0
   - `AC_GCLK_ID   = 40` → GCLK0
   - `CCL_GCLK_ID  = 38` → GCLK0
5. Configure all pins (mux + INEN) before enabling their peripheral outputs.
6. Configure AC COMP0 (`OUT_ASYNC`), enable, wait `READY0`.
7. Configure CCL LUT0/LUT1/SEQCTRL with `CTRL.ENABLE=0`; include `LUTEO` on LUT1.
8. Configure EVSYS channel 0 (`TCC0_OVF` → `TC0_EVU`, ASYNC) and channel 1
   (`CCL_LUTOUT_1` → `TC2_EVU`, ASYNC): write `USER` first, then `CHANNEL`.
9. Configure TC0+TC1 (COUNT32, MFRQ, `CC[0]=N-1`, `EVCTRL=TCEI|EVACT_COUNT`,
   `INTENSET=OVF`).
10. Configure TC2+TC3 (COUNT32, NFRQ, `EVCTRL=TCEI|EVACT_COUNT`).
11. Set `CCL->CTRL.ENABLE = 1`.
12. Enable TC2+TC3 (`ENABLE` then `RETRIGGER`).
13. Enable TC0+TC1 (`ENABLE` then `RETRIGGER`).
14. Configure TCC0 heartbeat (`PER`, `CC[0..2]`, `EVCTRL.OVFEO=1`), then
    `ENABLE` (TCC0 starts on enable since its `EVACT` is OFF).
15. `NVIC_EnableIRQ(TC0_IRQn)`.

`TCC0` is enabled last so that `TC0+TC1` is ready to count from the very
first `TCC0_OVF` event.

## 12. Bring-Up Validation Checklist

Scope checks in order:

1. PA09 = 375 kHz at 7/8 duty. PA10 = 375 kHz at 1/2 duty. PA08 carries
   DFF.Q (not WO[0]). `HB_1_8` has no physical output.
2. PA10 loopback appears on PB10 as a clean digital input.
3. PA12 follows `AC_CMP0` (drive the analog input through the comparator
   threshold to toggle the output).
4. DFF.Q only changes on PA10 rising edges, matches AC_CMP0 sampled at
   `HB_1_2` rising edge.
5. PA07 (`REF`) shows the expected mux behaviour:
   - For DFF.Q = 1: `REF` follows `HB_1_8` (375 kHz, 1/8 duty)
   - For DFF.Q = 0: `REF` follows `HB_7_8` (375 kHz, 7/8 duty)
6. PA11 (`COUNT_PULSE`) shows the expected AND:
   - For DFF.Q = 1: `COUNT_PULSE` follows `HB_1_2` (375 kHz, 1/2 duty)
   - For DFF.Q = 0: `COUNT_PULSE` is low
7. **TC0+TC1 window counter standalone**: with TCC0 running and a known
   window (e.g., `N = 187 500`, 500 ms), `TC0_IRQn` fires every 500 ms.
   Cross-check on PB23 (toggle in ISR) with scope.
8. **TC2+TC3 duty counter with forced Q = 1**: `TC2->COUNT32.COUNT` at
   window boundary ≈ `N`. Within 30 ppm of TC0's expected value.
9. **TC2+TC3 duty counter with forced Q = 0**: `TC2->COUNT32.COUNT == 0`
   at window boundary.
10. **Closed loop with comparator**: drive PA05 with a known duty-cycle
    waveform synchronized to the heartbeat. TC2/TC0 ratio matches the
    forced duty within expected accuracy.
11. Repeated windows show stable readings (no boundary drift, no missed
    interrupts).

## 13. Migration Path to N (after this validates)

If §12 passes end-to-end on the J:

- Add channel B: AC COMP1 on PA06, DFF_B, LUT3 (mux), LUT2 (and-gate),
  EVSYS CH2 (`CCL_LUTOUT_2` → `TC*_EVU`).
- Move the window counter from TC0+TC1 to **TC4+TC5** on N (newly
  available 32-bit pair).
- Free TC0+TC1 for channel A duty; TC2+TC3 takes channel B duty.
- All other patterns (clocks, LUTs, EVSYS ASYNC, RETRIGGER, ISR) carry
  over unchanged.

The N is then a mechanical extension of a proven design, not a leap of
faith.

## See Also

- [design-dual-channel.md](design-dual-channel.md) — original J attempt and
  §14 empirical-findings appendix (the source of all silicon-behaviour
  facts cited here)
- [design-dual-channel-N.md](design-dual-channel-N.md) — target dual-channel
  N design that this single-channel J validates
- [llm-wiki/sam/wiki/sources/samc21-errata.md](llm-wiki/sam/wiki/sources/samc21-errata.md)
  — SAM C20/C21 errata DS80000740S, applies to both variants
