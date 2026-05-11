# Dual-Channel AC+CCL+TC Architecture
# ATSAMC21J18A-AU — Design Document
# Created: 2026-05-10

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
  [PA05]──┼──► AC COMP0 ──► Pin_dig1_A ──► DFF_A.D         [PA06]──┼──► AC COMP1 ──► Pin_dig1_B ──► DFF_B.D
  analog  │              (~40-100 ns)    DFF_A.CLK◄─PIN_Clock analog │         (~40-100 ns)    DFF_B.CLK◄─PIN_Clock
          │                               DFF_A.Q = Pin_dig2_A       │                          DFF_B.Q = Pin_dig2_B
          │                                    │                     │                               │
          │                          ┌─────────┴──────────┐          │                    ┌──────────┴──────────┐
          │                          ▼                    ▼          │                    ▼                     ▼
          │                   LUT0: mux_A          LUT1: and_A       │             LUT3: mux_B         LUT2: and_B
          │                 Q ? WO1 : WO0        Q AND PIN_Clock     │           Q ? WO1 : WO0       Q AND PIN_Clock
          │                       │                     │            │                 │                    │
          │                  Pin_dig3_A            Pin_dig4_A        │            Pin_dig3_B           Pin_dig4_B
          │                  (ref steer)           (debug)           │            (ref steer)          (debug)
          │                                        EVSYS→TC0+TC1     │                                 EVSYS→TC2+TC3
          │
  [PA05]──┼──► ADC0/AIN1                                   [PA06]──┼──► ADC0/AIN2
          └──────────────────────────────────────────────────────────┘

    TCC1 "Window": counts N heartbeat periods → ISR at N−1, OVF at N + event output
```

**Signal chain per channel:**

| Step | Signal | From | To |
|------|--------|------|----|
| 1 | Analog input | external circuit | AC comparator + ADC |
| 2 | Pin_dig1 | AC ASYNC (CMP[n] pad, ~40-100 ns lag) | DFF.D |
| 3 | PIN_Clock | TCC0/WO[2] (PA10, mux F) | DFF.CLK |
| 4 | Pin_dig2 | DFF.Q (synchronized to PIN_Clock edges) | LUT_mux + LUT_and |
| 5 | Pin_dig3 | LUT_mux output | reference steering (external) |
| 6 | Pin_dig4 | LUT_and output | debug pin + EVSYS → TC32 |

**Pin_dig1 ≠ Pin_dig2.** The DFF registers the AC output on PIN_Clock rising edges.
Pin_dig2 transitions only at clock edges — no partial pulses in the AND gate, clean
TC counting. DFF clock = TCC0/WO[2] directly (not GCLK2 which has unknown phase offset).

---

## Global Resources

### GCLK Generators

| Generator | Source | Frequency | Used by |
|-----------|--------|-----------|---------|
| GCLK0 | OSC48M | 48 MHz | CPU, CCL |
| GCLK1 | XOSC | 24 MHz | TCC0, TCC1 |
| GCLK2 | XOSC/64 | 375 kHz | AC GCLK_AC |

### TCC0 "Heartbeat" — PCHCTRL[28], GCLK1=24 MHz

| Register | Value | Result |
|----------|-------|--------|
| CTRLA.PRESCALER | DIV1 | 24 MHz |
| PER | 63 | 64 cycles = 375 kHz period |
| CC[0] | 8 | WO[0] HIGH while COUNT<8 → duty 1/8 (333 ns) |
| CC[1] | 56 | WO[1] HIGH while COUNT<56 → duty 7/8 (2333 ns) |
| CC[2] | 32 | WO[2] HIGH while COUNT<32 → duty 1/2 = PIN_Clock |
| WAVE | NPWM | |

| Signal | Pin | Mux |
|--------|-----|-----|
| WO[0] slow-ref 1/8 | PA08 | E (0x4) |
| WO[1] fast-ref 7/8 | PA09 | E (0x4) |
| WO[2] PIN_Clock 1/2 | **PA10** | **F (0x5)** — not E |

### TCC1 "Window" — PCHCTRL[28], GCLK1=24 MHz

| Register | Value |
|----------|-------|
| PER | N×64 − 1 |
| CC[0] | (N−1)×64 − 1 |
| WAVE | NPWM |
| INTENSET | OVF + MC[0] |
| EVCTRL | OVFEO=1 |

Counts N heartbeat periods. ISR at N−1 (prepare), OVF at N (read TC counts + restart).
No pin output. TCC1 phase alignment: optionally retrigger from TCC0 OVF via EVSYS.

---

## AC Comparators — OUT=ASYNC, SPEED=HIGH, FLEN=OFF

| | Channel A | Channel B |
|--|-----------|-----------|
| Comparator | COMP0 (pair 0) | COMP1 (pair 0) |
| GCLK_AC | GCLK2 = 375 kHz | GCLK2 = 375 kHz |
| Analog input | PA05 = AIN1 (mux B) | PA06 = AIN2 (mux B) |
| MUXNEG | VSCALE / INTREF | VSCALE / INTREF |
| AC→Pin_dig1 lag | ~40–100 ns | ~40–100 ns |
| Pin_dig1 (DFF.D) | CMP[0] pad — ⚠ verify mux G/H | CMP[1] pad — ⚠ verify mux G/H |
| ADC | ADC0/AIN1 = PA05 | ADC0/AIN2 = PA06 |

---

## D Flip-Flops (external component, ×2)

One DFF per channel (e.g. SN74LVC1G74 or half of a dual-DFF package).

| Pin | Channel A | Channel B |
|-----|-----------|-----------|
| D | Pin_dig1_A = CMP[0] | Pin_dig1_B = CMP[1] |
| CLK | PA10 / TCC0/WO[2] | PA10 / TCC0/WO[2] |
| Q | Pin_dig2_A | Pin_dig2_B |

---

## CCL LUT Allocation — PCHCTRL[38], GCLK0=48 MHz

All AC-derived signals enter the CCL via INSEL=IO (DFF output pads).
The fixed COMP→LUT constraint does not apply. TCC0/WO[0,1] remain internal
(INSEL=TCC) and require LUT0 and LUT3 (only LUTs that map to TCC0).

### LUT0 — mux_A (Channel A reference steering)

| Slot | INSEL | Source | IN |
|------|-------|--------|----|
| INSEL2 | 0x4 IO | Pin_dig2_A (DFF_A.Q, mux I) | Q_A |
| INSEL1 | 0x8 TCC | TCC0/WO[1] | fast-ref |
| INSEL0 | 0x8 TCC | TCC0/WO[0] | slow-ref |

**TRUTH = 0xCA** — `Q_A ? WO[1] : WO[0]`

```
IN[2]=Q  IN[1]=WO1  IN[0]=WO0  │  OUT
   0        0          0       │   0
   0        0          1       │   1   (Q=0 → pass WO0)
   0        1          0       │   0
   0        1          1       │   1   (Q=0 → pass WO0)
   1        0          0       │   0
   1        0          1       │   0   (Q=1 → pass WO1)
   1        1          0       │   1   (Q=1 → pass WO1)
   1        1          1       │   1
```

Output → Pin_dig3_A (reference steering)

### LUT3 — mux_B (Channel B reference steering)

LUT3 maps to TCC0 (TCC3 absent on J-variant — verified experimentally).

| Slot | INSEL | Source | IN |
|------|-------|--------|----|
| INSEL2 | 0x4 IO | Pin_dig2_B (DFF_B.Q, mux I) | Q_B |
| INSEL1 | 0x8 TCC | TCC0/WO[1] | fast-ref |
| INSEL0 | 0x8 TCC | TCC0/WO[0] | slow-ref |

**TRUTH = 0xCA** (identical to LUT0)

Output → Pin_dig3_B (reference steering)

### LUT1 — and_A (Channel A gated counter clock)

| Slot | INSEL | Source | IN |
|------|-------|--------|----|
| INSEL2 | 0x4 IO | PIN_Clock (PA10/WO[2], mux I) | CLK |
| INSEL1 | 0x0 MASK | — | 0 |
| INSEL0 | 0x4 IO | Pin_dig2_A (DFF_A.Q, mux I) | Q_A |

**TRUTH = 0x20** — `CLK AND Q_A`

```
IN[2]=CLK  IN[1]=0  IN[0]=Q_A  │  OUT
    0         0        0       │   0
    0         0        1       │   0
    1         0        0       │   0
    1         0        1       │   1   ← CLK=1 AND Q_A=1
```

Output → Pin_dig4_A (debug) + EVSYS LUTOUT1 → TC0+TC1 COUNT event

### LUT2 — and_B (Channel B gated counter clock)

| Slot | INSEL | Source | IN |
|------|-------|--------|----|
| INSEL2 | 0x4 IO | PIN_Clock (PA10/WO[2], mux I) | CLK |
| INSEL1 | 0x0 MASK | — | 0 |
| INSEL0 | 0x4 IO | Pin_dig2_B (DFF_B.Q, mux I) | Q_B |

**TRUTH = 0x20** (identical to LUT1)

Output → Pin_dig4_B (debug) + EVSYS LUTOUT2 → TC2+TC3 COUNT event

### CCL Summary

| LUT | Function | IN[2] | IN[1] | IN[0] | TRUTH |
|-----|----------|-------|-------|-------|-------|
| LUT0 | mux_A | Pin_dig2_A (IO) | TCC0/WO[1] (TCC) | TCC0/WO[0] (TCC) | 0xCA |
| LUT1 | and_A | PIN_Clock (IO) | MASK | Pin_dig2_A (IO) | 0x20 |
| LUT2 | and_B | PIN_Clock (IO) | MASK | Pin_dig2_B (IO) | 0x20 |
| LUT3 | mux_B | Pin_dig2_B (IO) | TCC0/WO[1] (TCC) | TCC0/WO[0] (TCC) | 0xCA |

---

## TC 32-Bit Counter Pairs

| | Channel A | Channel B |
|--|-----------|-----------|
| TC pair | TC0+TC1 (PCHCTRL[30]) | TC2+TC3 (PCHCTRL[31]) |
| Master | TC0 (COUNT32) | TC2 (COUNT32) |
| Slave | TC1 | TC3 |
| GCLK | GCLK1 = 24 MHz | GCLK1 = 24 MHz |
| Event input | EVSYS LUTOUT1 | EVSYS LUTOUT2 |
| EVACT | COUNT | COUNT |

TC count = number of PIN_Clock pulses (375 kHz) while Q was HIGH.
At TCC1 OVF: ISR reads TC0 and TC2, then resets them for the next window.

⚠ TC2+TC3 errata §35.6.2.4: avoid in new designs. Observed functional in COUNT32
mode on Rev F. Verify before committing to layout.

---

## EVSYS Channel Assignment

| CH | Generator | Gen ID | User | Path |
|----|-----------|--------|------|------|
| 0 | CCL LUTOUT1 | 83 | TC0 event (EVSYS_ID_USER_TC0_EVU) | ASYNC |
| 1 | CCL LUTOUT2 | 84 | TC2 event (EVSYS_ID_USER_TC2_EVU) | ASYNC |
| 2 | TCC0 OVF | — | TCC1 retrigger (optional, phase align) | ASYNC |
| 3 | TCC1 OVF | — | spare | — |

---

## Pin Allocation

| Pin | Function | Mux | Status |
|-----|----------|-----|--------|
| PA05 | AC COMP0 AIN1 / ADC0 AIN1 | B | Channel A analog in |
| PA06 | AC COMP1 AIN2 / ADC0 AIN2 | B | Channel B analog in |
| PA08 | TCC0/WO[0] slow-ref 1/8 | E | |
| PA09 | TCC0/WO[1] fast-ref 7/8 | E | |
| PA10 | TCC0/WO[2] PIN_Clock 1/2 | **F** | DFF clocks + CCL IO inputs |
| PA03 | VREF | — | 22Ω+22nF, leave at reset |
| PA04 | VREF | — | 22Ω+22nF, leave at reset |
| PA14 | XOSC XIN | — | crystal |
| PA15 | XOSC XOUT | — | crystal |
| PB30 | SERCOM5 TX | D | UART |
| PB31 | SERCOM5 RX | D | UART |
| PB22 | Bench button | GPIO | |
| PB23 | Bench LED | GPIO | |

### To verify from datasheet Ch.5/Ch.6/Ch.37

| Signal | Notes |
|--------|-------|
| CMP[0] output pin | Pin_dig1_A (DFF_A.D) — mux G or H, verify from Ch.5 |
| CMP[1] output pin | Pin_dig1_B (DFF_B.D) — mux G or H, verify from Ch.5 |
| LUT0/INSEL2 IO pin | Pin_dig2_A input for mux_A — verify from Ch.37 Table 37-4 |
| LUT1/INSEL2 IO pin | PIN_Clock input for and_A — verify from Ch.37 |
| LUT1/INSEL0 IO pin | Pin_dig2_A input for and_A — verify from Ch.37 |
| LUT2/INSEL2 IO pin | PIN_Clock input for and_B — verify from Ch.37 |
| LUT2/INSEL0 IO pin | Pin_dig2_B input for and_B — verify from Ch.37 |
| LUT3/INSEL2 IO pin | Pin_dig2_B input for mux_B — verify from Ch.37 |
| LUT0 OUT pin | Pin_dig3_A — verify from Ch.37 (mux I) |
| LUT1 OUT pin | Pin_dig4_A — verify from Ch.37 (mux I) |
| LUT2 OUT pin | Pin_dig4_B — verify from Ch.37 (mux I) |
| LUT3 OUT pin | Pin_dig3_B — verify from Ch.37 (mux I) |

---

## APB Clocks

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

1. **CMP[n] output pins**: verify physical pins for CMP[0] and CMP[1] from Ch.5
   signal descriptions (function G or H). These are Pin_dig1_A and Pin_dig1_B.
2. **CCL IO pin table**: read Ch.37 Table 37-4 to map each LUT/INSEL slot to a
   physical pin (function I). Needed for DFF Q routing and PIN_Clock routing.
3. **DFF package**: choose single or dual DFF package; confirm voltage/speed specs
   at 3.3 V, 375 kHz, with CML[n] drive capability.
4. **TC2+TC3 errata**: accept or find alternative (software capture on TC0+TC1).
5. **TCC1 alignment**: decide if TCC0 OVF→TCC1 retrigger is needed.
6. **AC MUXNEG**: choose VSCALE value or INTREF level per channel.
7. **SEQCTRL**: ensure SEQSEL0 and SEQSEL1 = 0x0 (disabled) — LUTs are purely
   combinational, no sequential pairing.
