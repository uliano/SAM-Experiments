---
title: SAMC21 Datasheet Ch.41 DAC
type: source
tags: [dac, analog, samc21, datasheet]
sources: [samc21-datasheet-ch41-dac]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.41 DAC

Digital-to-Analog Converter. SAM C21 only (not SAM C20). One channel, 10-bit
resolution, up to 350 ksps. Hardware dithering extends effective resolution to
14 bits. Output can be routed externally (VOUT pin) or internally to AC, ADC,
or SDADC.

## Abstract

The DAC converts a 10-bit digital value in the DATA register to a voltage on
VOUT. A two-stage FIFO (DATABUF → DATA) combined with a START event allows
DMA-fed continuous waveform generation. Reference selectable from internal
bandgap (INTREF), VDDANA, or external pin (VREFA). Voltage pump auto-managed
for operation below 2.5 V.

## Key Facts

- SAM C21 only (not SAM C20). One instance.
- Resolution: 10-bit. Dithering mode extends to 14-bit effective.
- Maximum rate: 350 ksps.
- Output formula: `V_OUT = DATA / 0x3FF × VREF`.
- Enable-protected: CTRLB, EVCTRL — configure before CTRLA.ENABLE=1.
- Write-synchronized (poll SYNCBUSY): CTRLA.SWRST, CTRLA.ENABLE, DATA, DATABUF.
  If written while SYNCBUSY set, write is discarded and bus error generated.
- No bits require read synchronization.
- CTRLB.EOEN=1: enables high-drive output buffer on VOUT pin.
- CTRLB.IOEN=1: enables internal DAC output to AC/ADC/SDADC. Output buffer must
  be enabled (EOEN=1) when used by ADC.
- Both EOEN and IOEN can be enabled simultaneously.
- CTRLB.REFSEL[1:0]: INTREF(0x0)/VDDANA(0x1)/VREFA(0x2). Configure before enable.
- CTRLB.LEFTADJ: 0=right-adjusted DATA[9:0], 1=left-adjusted DATA[15:6].
- CTRLB.DITHER: 0=10-bit mode, 1=14-bit dithering mode (see DATA format table).
- CTRLB.VPD: 0=voltage pump auto (required at <2.5V), 1=pump force-disabled.
  Setting VPD=1 saves power when VDDANA > 2.5V. Pump uses async GCLK_DAC.
- STATUS.READY: 1 when DAC startup time has elapsed, ready for conversion.
- CTRLA.RUNSTDBY: DAC output buffer keeps value in STANDBY when set to 1.

## Two-Stage FIFO / Data Buffer

DATABUF and DATA form a two-stage FIFO:
1. CPU/DMA writes value to DATABUF.
2. On START event: DATABUF → DATA, conversion begins.
3. DATABUF becomes empty; INTFLAG.EMPTY fires (and DMA request generated).
4. If START event occurs while DATABUF is empty: INTFLAG.UNDERRUN fires.

INTFLAG.EMPTY cleared by writing 1, or by writing new data to DATABUF.

## Reference Voltage

| REFSEL | Name | Description |
|--------|------|-------------|
| 0x0 | INTREF | Internal bandgap (level set by SUPC.VREF.SEL) |
| 0x1 | VDDANA | Analog supply voltage |
| 0x2 | VREFA | External reference pin |
| 0x3 | — | Reserved |

## DATA Register Format

| CTRLB.DITHER | CTRLB.LEFTADJ | DATA bits used | Description |
|--------------|---------------|----------------|-------------|
| 0 | 0 | DATA[9:0] | Right-adjusted, 10-bit |
| 0 | 1 | DATA[15:6] | Left-adjusted, 10-bit |
| 1 | 0 | DATA[13:4] + DATA[3:0] | Right-adjusted, 14-bit dither |
| 1 | 1 | DATA[15:6] + DATA[5:2] | Left-adjusted, 14-bit dither |

In dithering mode: 16 sub-conversions per DATA[13:4] value; STARTEI must generate
16 events per DATABUF write. DMA is typically used to feed DATABUF continuously.

## DMA

DMA trigger: EMPTY (data buffer empty). Request cleared when DATABUF is written or
INTFLAG.EMPTY is cleared.

Typical DMA waveform generation: configure DMA to write DATABUF from a sample buffer
on each EMPTY trigger; configure EVSYS to generate STARTEI periodically from a timer.

## Interrupts

- EMPTY: DATABUF is empty (ready for new data).
- UNDERRUN: START event occurred while DATABUF was empty (data underrun).

## Events

- Output: EMPTYEO — generated when DATABUF becomes empty (EVCTRL.EMPTYEO=1).
- Input: STARTEI — loads DATABUF into DATA and starts conversion (EVCTRL.STARTEI=1).
- INVEI: invert STARTEI input edge (0=rising, 1=falling).

## Sleep

- CTRLA.RUNSTDBY=1: DAC output buffer keeps last value in STANDBY.
- RUNSTDBY=0: output buffer disabled in STANDBY.
- DAC continues operation in IDLE sleep if GCLK_DAC is running.
- DAC interrupts can wake the CPU from sleep modes.

## Synchronization

Write-synchronized (poll SYNCBUSY):
- SWRST, ENABLE: poll SYNCBUSY.SWRST / SYNCBUSY.ENABLE.
- DATA: poll SYNCBUSY.DATA.
- DATABUF: poll SYNCBUSY.DATABUF.

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | CTRLA | RUNSTDBY, ENABLE, SWRST (write-sync: ENABLE, SWRST) |
| 0x01 | CTRLB | REFSEL[1:0], DITHER, VPD, LEFTADJ, IOEN, EOEN (enable-protected) |
| 0x02 | EVCTRL | INVEI, EMPTYEO, STARTEI (enable-protected) |
| 0x03 | Reserved | — |
| 0x04 | INTENCLR | EMPTY, UNDERRUN |
| 0x05 | INTENSET | EMPTY, UNDERRUN |
| 0x06 | INTFLAG | EMPTY, UNDERRUN (write 1 to clear) |
| 0x07 | STATUS | READY (read-only) |
| 0x08 | DATA | DATA[15:0] (16-bit, write-only, write-sync) |
| 0x0C | DATABUF | DATABUF[15:0] (16-bit, write-only, write-sync) |
| 0x10 | SYNCBUSY | DATABUF, DATA, ENABLE, SWRST (32-bit, read-only) |
| 0x18 | DBGCTRL | DBGRUN |

## See Also

- [[SUPC]] — VREF configuration for INTREF voltage level
- [[Clock System]] — GCLK_DAC setup
- [[EVSYS]] — periodic START events from timer for waveform generation
- [[DMA Controller]] — DMA feed via EMPTY trigger
- [[ADC Configuration]] — DAC as ADC reference (MUXPOS=DAC)
- [[AC Configuration]] — DAC as comparator negative reference (MUXNEG=DAC)
- [[SDADC Configuration]] — DAC as SDADC reference (REFSEL=DAC)
