---
title: SAMC21 Datasheet Ch.38 ADC
type: source
tags: [adc, analog, samc21, datasheet]
sources: [samc21-datasheet-ch38-adc]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.38 ADC

Analog-to-Digital Converter. Two independent instances (ADC0 and ADC1) on SAM
C20/C21. 12-bit resolution (or 8/10-bit), up to 1 MSPS, single-ended and
differential, with averaging, oversampling, window monitor, and sequence scan.

## Abstract

The ADC converts analog signals to 8-, 10-, or 12-bit digital values at up to
1 MSPS. Both ADC0 and ADC1 include a dedicated sample-and-hold circuit and can
operate simultaneously in master/slave mode. Multiple internal signal inputs
(bandgap INTREF, scaled supply voltages, DAC, OPAMP) and up to 12 external
analog pins per ADC are selectable. Accumulation and decimation extend effective
resolution up to 16 bits. A windowing monitor, automatic channel sequencing,
offset/gain correction, and DMA/event integration are included.

## Key Facts

- Two instances: ADC0 and ADC1. ADC0 is master; ADC1 can be slaved to ADC0.
- Resolution: 12-bit (default), 10-bit, or 8-bit (CTRLC.RESSEL).
- Maximum sampling rate: 1 MSPS at 12-bit with 4-cycle sampling time.
  For 1 MSPS: need f_CLK_ADC = 16 MHz → GCLK_ADC ≥ 32 MHz (with DIV2 prescaler).
- Input range: V_REF = [2.0 V to VDD_ANA].
- Prescaler: minimum division is DIV2 (no divide-by-1 option).
  f_CLK_ADC = f_GCLK_ADC / 2^(1 + CTRLB.PRESCALER)
- Sampling time: 1 cycle (fixed) + SAMPCTRL.SAMPLEN additional cycles.
- Propagation delay: (1 + Resolution bits) / f_CLK_ADC.
- Enable-protected registers: CTRLB, REFCTRL, EVCTRL, CALIB.
- Double-buffered (write takes effect after current conversion):
  INPUTCTRL, CTRLC, AVGCTRL, SAMPCTRL, WINLT, WINUT, GAINCORR, OFFSETCORR.
- Write-synchronized: same as double-buffered, plus SWTRIG.
  Poll SYNCBUSY after each write to these registers.
- CALIB (BIASCOMP, BIASREFBUF): must be loaded from NVM Software Calibration Area.
- Rail-to-rail operation (CTRLC.R2R=1): requires offset compensation (OFFCOMP=1).
- Offset compensation (SAMPCTRL.OFFCOMP=1): sampling time fixed to 4 CLK_ADC cycles.
- Correction formula: Result = (ConversionValue+ − OFFSETCORR) × GAINCORR.
- Correction latency: 13 CLK_ADC cycles (only on first conversion in free-running mode).

## Reference Voltage Selection (REFCTRL.REFSEL)

| Value | Name | Description |
|-------|------|-------------|
| 0x0 | INTREF | Internal bandgap reference (set voltage via SUPC.VREF.SEL) |
| 0x1 | INTVCC0 | 1/1.6 × VDDANA |
| 0x2 | INTVCC1 | 1/2 × VDDANA (requires VDDANA > 4.0 V) |
| 0x3 | VREFA | External reference pin |
| 0x4 | DAC | DAC internal output |
| 0x5 | INTVCC2 | VDDANA |

REFCOMP=1: enables reference buffer offset compensation (reduces gain error, increases start-up time).

## Input Mux (INPUTCTRL)

- MUXPOS[4:0]: positive input selection.
  - 0x00–0x0B: AIN0–AIN11 (external analog pins)
  - 0x18: INTREF/Bandgap
  - 0x19: INTVCC0 (scaled core supply)
  - 0x1A: INTVCC1 (scaled I/O supply)
  - 0x1B: DAC
  - 0x1C: OPAMP (ADC0 only)
- MUXNEG[4:0]: negative input (differential mode only).
  - 0x00–0x06: AIN0–AIN6 (limited set for negative)
  - 0x18: GND (for single-ended = differential with GND)

## Prescaler Table (CTRLB.PRESCALER)

| Value | Name | Division |
|-------|------|---------|
| 0x0 | DIV2 | /2 |
| 0x1 | DIV4 | /4 |
| 0x2 | DIV8 | /8 |
| 0x3 | DIV16 | /16 |
| 0x4 | DIV32 | /32 |
| 0x5 | DIV64 | /64 |
| 0x6 | DIV128 | /128 |
| 0x7 | DIV256 | /256 |

## Accumulation / Averaging (AVGCTRL)

SAMPLENUM: number of samples accumulated.
ADJRES: additional right shifts after automatic shifts.

| Samples | SAMPLENUM | Auto Right Shifts | ADJRES for averaging | Final Precision |
|---------|-----------|-------------------|---------------------|----------------|
| 1 | 0x0 | 0 | 0x0 | 12 bits |
| 2 | 0x1 | 0 | 0x1 | 12 bits |
| 4 | 0x2 | 0 | 0x2 | 12 bits |
| 8 | 0x3 | 0 | 0x3 | 12 bits |
| 16 | 0x4 | 0 | 0x4 | 12 bits |
| 32 | 0x5 | 1 | 0x4 | 12 bits |
| 64 | 0x6 | 2 | 0x4 | 12 bits |
| 128 | 0x7 | 3 | 0x4 | 12 bits |
| 256 | 0x8 | 4 | 0x4 | 12 bits |
| 512 | 0x9 | 5 | 0x4 | 12 bits |
| 1024 | 0xA | 6 | 0x4 | 12 bits |

## Oversampling and Decimation (no ADJRES division)

For higher effective resolution (CTRLC.RESSEL must be set for accumulation ≥ 2):

| Target Resolution | SAMPLENUM | ADJRES |
|------------------|-----------|--------|
| 13 bits | 0x2 (4 samples) | 0x1 |
| 14 bits | 0x4 (16 samples) | 0x2 |
| 15 bits | 0x6 (64 samples) | 0x1 |
| 16 bits | 0x8 (256 samples) | 0x0 |

## Window Monitor (CTRLC.WINMODE)

| WINMODE | Condition for WINMON flag |
|---------|--------------------------|
| 0x0 | Disabled |
| 0x1 | Result > WINLT |
| 0x2 | Result < WINUT |
| 0x3 | WINLT < Result < WINUT (inside window) |
| 0x4 | Result < WINLT or Result > WINUT (outside window) |

## Automatic Sequence (SEQCTRL)

- 32-bit register; each bit n corresponds to MUXPOS=n.
- When any bits are set, ADC automatically steps through all enabled inputs
  after each start trigger.
- Sequence order: ascending MUXPOS from AIN0 upward.
- SEQSTATUS.SEQSTATE[4:0]: input number of last completed conversion.
- SEQSTATUS.SEQBUSY: sequence in progress.

## Master/Slave Operation

- ADC1.CTRLA.SLAVEEN=1: ADC1 is slaved to ADC0.
- When slaved: ADC0 GCLK and ADC0 controls route internally to ADC1.
- ADC0.CTRLC.DUALSEL: selects simultaneous or interleaved trigger mode.
- In master/slave, event configuration must be done in ADC0 (master).
- ADC1 ONDEMAND and RUNSTDBY ignored; ADC0's bits control both.

## DMA Operation

- Single DMA request: RESRDY; set when conversion result is available.
- Request cleared when RESULT register is read.
- With averaging: DMA request set when averaged result is complete.

## Interrupts

- RESRDY: result conversion ready.
- WINMON: window monitor threshold match.
- OVERRUN: result not read before next conversion completed (RESRDY still set).
  Note: OVERRUN is NOT a sleep wakeup source.

## Events

- Output: RESRDY (result ready), WINMON (window match). Enable via EVCTRL.RESRDYEO/WINMONEO.
- Input: START (trigger conversion), FLUSH (flush current conversion). Enable via EVCTRL.STARTEI/FLUSHEI.
- ADC uses asynchronous event channel paths only.
- Edge: rising by default; EVCTRL.STARTINV=1 / FLUSHINV=1 for falling edge.
- FLUSH takes priority over START when both are available simultaneously.

## Synchronization

Write-synchronized (poll SYNCBUSY after write):
INPUTCTRL, CTRLC, AVGCTRL, SAMPCTRL, WINLT, WINUT, GAINCORR, OFFSETCORR, SWTRIG.
CTRLA.SWRST and CTRLA.ENABLE: poll SYNCBUSY.SWRST and SYNCBUSY.ENABLE.

## Sleep Behavior

| RUNSTDBY | ONDEMAND | Behavior |
|----------|----------|---------|
| 0 | x | Disabled in standby |
| 0 | 0 | Run in all sleep modes except standby |
| 0 | 1 | Run in all sleep modes on request, except standby |
| 1 | 0 | Run in all sleep modes |
| 1 | 1 | Run in all sleep modes on request |

## Calibration

BIASCOMP[2:0] and BIASREFBUF[2:0] in CALIB register must be loaded from NVM
Software Calibration Area at 0x00800080 before enabling the ADC.

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | CTRLA | SWRST, ENABLE, SLAVEEN, RUNSTDBY, ONDEMAND |
| 0x01 | CTRLB | PRESCALER[2:0] (enable-protected) |
| 0x02 | REFCTRL | REFSEL[3:0], REFCOMP (enable-protected) |
| 0x03 | EVCTRL | FLUSHEI, STARTEI, FLUSHINV, STARTINV, RESRDYEO, WINMONEO (enable-protected) |
| 0x04 | INTENCLR | WINMON, OVERRUN, RESRDY |
| 0x05 | INTENSET | (same) |
| 0x06 | INTFLAG | (same — write 1 to clear) |
| 0x07 | SEQSTATUS | SEQBUSY, SEQSTATE[4:0] (read-only) |
| 0x08 | INPUTCTRL | MUXPOS[4:0], MUXNEG[4:0] (double-buffered, write-sync) |
| 0x0A | CTRLC | DIFFMODE, LEFTADJ, FREERUN, CORREN, RESSEL[1:0], R2R, WINMODE[2:0], DUALSEL[1:0] (double-buffered) |
| 0x0C | AVGCTRL | SAMPLENUM[3:0], ADJRES[2:0] (double-buffered) |
| 0x0D | SAMPCTRL | SAMPLEN[5:0], OFFCOMP (double-buffered) |
| 0x0E | WINLT | WINLT[15:0] (double-buffered, write-sync) |
| 0x10 | WINUT | WINUT[15:0] (double-buffered, write-sync) |
| 0x12 | GAINCORR | GAINCORR[11:0] (double-buffered, write-sync) |
| 0x14 | OFFSETCORR | OFFSETCORR[11:0] signed (double-buffered, write-sync) |
| 0x18 | SWTRIG | FLUSH, START (write-sync) |
| 0x1C | DBGCTRL | DBGRUN |
| 0x20 | SYNCBUSY | SWRST, ENABLE, INPUTCTRL, CTRLC, AVGCTRL, SAMPCTRL, WINLT, WINUT, GAINCORR, OFFSETCORR, SWTRIG |
| 0x24 | RESULT | RESULT[15:0] (16-bit, read-only) |
| 0x28 | SEQCTRL | SEQENn (1 bit per positive input, 32-bit) |
| 0x2C | CALIB | BIASCOMP[2:0], BIASREFBUF[2:0] (enable-protected) |

## See Also

- [[SAMC21 Datasheet Ch.38 ADC]] — full register reference
- [[SUPC]] — VREF configuration (INTREF voltage level)
- [[Clock System]] — GCLK_ADC setup, APBCMASK
- [[NVMCTRL]] — NVM calibration area for BIASCOMP/BIASREFBUF
- [[EVSYS]] — ADC event trigger routing
- [[DMA Controller]] — ADC DMA result transfer
