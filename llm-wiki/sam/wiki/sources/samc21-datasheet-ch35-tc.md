---
title: SAMC21 Datasheet Chapter 35 — TC
type: source
tags: [tc, timer, counter, pwm, capture, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 35 — TC – Timer/Counter

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 35, pages 699–773.

## Overview

8 TC instances (TC0–TC7 on larger packages; SAMC21J18A has TC0–TC5). Each TC supports
three counting modes: 8-bit (COUNT8), 16-bit (COUNT16), and 32-bit (COUNT32). Two
compare/capture channels (CC0, CC1). Waveform output on WO[0]/WO[1]. Prescaler
DIV1–DIV1024. Optional double-buffering of CC and PER registers.

## Modes

| MODE[1:0] | Name | Count width | TOP register |
|-----------|------|-------------|-------------|
| 0x0 | COUNT16 | 16-bit | 0xFFFF (NFRQ/NPWM) or CC0 (MFRQ/MPWM) |
| 0x1 | COUNT8 | 8-bit | PER (NFRQ/NPWM) or CC0 (MFRQ/MPWM) |
| 0x2 | COUNT32 | 32-bit | 0xFFFFFFFF (NFRQ/NPWM) or CC0 (MFRQ/MPWM) |

## 32-Bit Pairing

Adjacent even+odd TC pairs share a single COUNT32 value controlled entirely by the
even (master) TC:

| Master (even) | Slave (odd) |
|--------------|------------|
| TC0 | TC1 |
| TC2 | TC3 |
| TC4 | TC5 |

Both master and slave require APB clocks enabled. Only the master needs a GCLK.
All COUNT32 registers are accessed through the master TC instance. STATUS.SLAVE is
set on the slave when its master is configured for COUNT32.

## Waveform Generation (WAVE.WAVEGEN)

| Value | Name | TOP | WO on match | WO on wraparound |
|-------|------|-----|------------|-----------------|
| 0x0 | NFRQ | PER (8-bit) / MAX (16/32-bit) | Toggle | No action |
| 0x1 | MFRQ | CC0 | Toggle | No action |
| 0x2 | NPWM | PER (8-bit) / MAX (16/32-bit) | Set | Clear |
| 0x3 | MPWM | CC0 | Set (WO[0]), CC1 controls WO[1] | Clear |

Frequency formula: `f = f_GCLK_TC / (prescaler × (TOP + 1))`

## Capture Modes

Set CTRLA.CAPTENx=1 to configure CCx as capture. CTRLA.COPENx=1 uses I/O pin as
trigger source; COPENx=0 uses Event System. CAPTMODE: DEFAULT (capture on event),
CAPTMIN (capture minimum value), CAPTMAX (capture maximum value).

EVACT modes for input capture: STAMP (timestamp to CCx), PPW (period in CC0, pulse
width in CC1), PWP (period in CC1, pulse width in CC0), PW (pulse width only).

## Key Synchronization Requirements

- COUNT must be read by issuing CTRLBSET.CMD=READSYNC (0x4); wait SYNCBUSY.COUNT.
- CTRLA (except ENABLE/SWRST) and WAVE/EVCTRL are enable-protected; configure while ENABLE=0.
- After SWRST wait SYNCBUSY.SWRST; after ENABLE write wait SYNCBUSY.ENABLE.
- CTRLB is write-synchronized — changes take effect on the next prescaled clock.

## Register Summary — All Modes (shared offsets)

| Offset | Name | Width | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 32-bit | Mode, prescaler, capture config — enable-protected |
| 0x04 | CTRLBCLR | 8-bit | Clear CTRLB bits (CMD/ONESHOT/LUPD/DIR) |
| 0x05 | CTRLBSET | 8-bit | Set CTRLB bits; CMD field for commands |
| 0x06 | EVCTRL | 16-bit | Event I/O control — enable-protected |
| 0x08 | INTENCLR | 8-bit | Interrupt enable clear |
| 0x09 | INTENSET | 8-bit | Interrupt enable set |
| 0x0A | INTFLAG | 8-bit | Interrupt flags — write-1-to-clear |
| 0x0B | STATUS | 8-bit | CCBUFV / PERBUFV / SLAVE / STOP — read-synchronized |
| 0x0C | WAVE | 8-bit | WAVEGEN — enable-protected |
| 0x0D | DRVCTRL | 8-bit | INVEN1/INVEN0 — enable-protected |
| 0x0F | DBGCTRL | 8-bit | DBGRUN |
| 0x10 | SYNCBUSY | 32-bit | CC1/CC0/COUNT/STATUS/CTRLB/ENABLE/SWRST |

## Mode-Specific Data Register Offsets

| Register | 8-bit | 16-bit | 32-bit |
|---------|-------|--------|--------|
| COUNT | 0x14 (8b) | 0x14 (16b) | 0x14 (32b) |
| PER / – | 0x1B (8b) | — | — |
| CC0 | 0x1C (8b) | 0x1C (16b) | 0x1C (32b) |
| CC1 | 0x1D (8b) | 0x1E (16b) | 0x20 (32b) |
| PERBUF | 0x2F (8b) | — | — |
| CCBUF0 | 0x30 (8b) | 0x30 (16b) | 0x30 (32b) |
| CCBUF1 | 0x31 (8b) | 0x32 (16b) | 0x34 (32b) |

## CTRLA Bit Fields

| Bits | Name | Description |
|------|------|-------------|
| 28:27 | CAPTMODE1[1:0] | Capture mode ch1: DEFAULT/CAPTMIN/CAPTMAX |
| 25:24 | CAPTMODE0[1:0] | Capture mode ch0: DEFAULT/CAPTMIN/CAPTMAX |
| 21 | COPEN1 | Capture on pin ch1 enable (0=event, 1=I/O pin) |
| 20 | COPEN0 | Capture on pin ch0 enable |
| 17 | CAPTEN1 | Capture enable ch1 (0=compare, 1=capture) |
| 16 | CAPTEN0 | Capture enable ch0 |
| 11 | ALOCK | Auto-lock: LUPD set on overflow/underflow/retrigger |
| 10:8 | PRESCALER[2:0] | Prescaler (see table below) |
| 7 | ONDEMAND | Stop clock when TC stopped (0=always keep clock) |
| 6 | RUNSTDBY | Keep TC running in standby |
| 5:4 | PRESCSYNC[1:0] | Prescaler sync: GCLK(0)/PRESC(1)/RESYNC(2) |
| 3:2 | MODE[1:0] | COUNT16(0x0)/COUNT8(0x1)/COUNT32(0x2) |
| 1 | ENABLE | Enable (write-synchronized) |
| 0 | SWRST | Software reset |

## PRESCALER Values

| Value | Name | Divisor |
|-------|------|---------|
| 0x0 | DIV1 | 1 |
| 0x1 | DIV2 | 2 |
| 0x2 | DIV4 | 4 |
| 0x3 | DIV8 | 8 |
| 0x4 | DIV16 | 16 |
| 0x5 | DIV64 | 64 |
| 0x6 | DIV256 | 256 |
| 0x7 | DIV1024 | 1024 |

## CTRLB CMD Values (written to CTRLBSET)

| Value | Name | Action |
|-------|------|--------|
| 0x0 | NONE | No action |
| 0x1 | RETRIGGER | Force start, restart, or retrigger |
| 0x2 | STOP | Force stop |
| 0x3 | UPDATE | Force update of double-buffered registers |
| 0x4 | READSYNC | Force read synchronization of COUNT |

CTRLB also contains: ONESHOT(2) — stop on overflow; LUPD(1) — lock update (prevent
buffer→CC copy); DIR(0) — count down.

## EVCTRL Bit Fields (16-bit, 0x06)

| Bits | Name | Description |
|------|------|-------------|
| 13 | MCEO1 | Match/capture event output enable ch1 |
| 12 | MCEO0 | Match/capture event output enable ch0 |
| 8 | OVFEO | Overflow/underflow event output enable |
| 5 | TCEI | TC event input enable |
| 4 | TCINV | Invert event input polarity |
| 2:0 | EVACT[2:0] | Event action (see below) |

EVACT: OFF(0x0)/RETRIGGER(0x1)/COUNT(0x2)/START(0x3)/STAMP(0x4)/PPW(0x5)/PWP(0x6)/PW(0x7)

## INTFLAG / INTENCLR / INTENSET Bits

| Bit | Name | Condition |
|-----|------|-----------|
| 5 | MC1 | Compare match or capture ch1 (auto-cleared when CCx read in capture) |
| 4 | MC0 | Compare match or capture ch0 |
| 1 | ERR | Capture overflow (new capture while MCx flag still set) |
| 0 | OVF | Overflow or underflow |

## STATUS Bits (0x0B, reset=0x01)

| Bit | Name | Description |
|-----|------|-------------|
| 5 | CCBUFV1 | CC1 buffer valid |
| 4 | CCBUFV0 | CC0 buffer valid |
| 3 | PERBUFV | PER buffer valid (8-bit mode only) |
| 1 | SLAVE | Set on slave TC when master is in COUNT32 mode |
| 0 | STOP | Reset=1; cleared when counter starts |

## SYNCBUSY Bits (0x10)

| Bit | Name | Wait after |
|-----|------|-----------|
| 7 | CC1 | CC1 write |
| 6 | CC0 | CC0 write |
| 4 | COUNT | COUNT write or READSYNC command |
| 3 | STATUS | STATUS read-sync |
| 2 | CTRLB | CTRLB write |
| 1 | ENABLE | ENABLE write |
| 0 | SWRST | SWRST write |

## Key Facts

- STATUS.STOP resets to 1 — TC starts stopped; CTRLBSET.CMD=RETRIGGER or ENABLE starts it.
- CTRLA.SWRST takes precedence over any concurrent write.
- DBGCTRL.DBGRUN=0: TC halts when CPU halted by debugger (default, good for debugging).
- Double-buffering: write CCBUFx; set CTRLBSET.CMD=UPDATE to transfer immediately, or
  it auto-transfers on the next wraparound.
- LUPD(1 in CTRLBSET): prevents the auto-transfer; use with ALOCK for glitch-free updates.

## See Also

- [[TC 32-Bit Paired Mode]] — firmware usage, init sequence, READSYNC pattern
- [[Clock System]] — GCLK and APBC clock setup for TC
- [[Memory Map]] — TC0–TC5 base addresses
- [[NVIC Interrupt Map]] — TC IRQn values
