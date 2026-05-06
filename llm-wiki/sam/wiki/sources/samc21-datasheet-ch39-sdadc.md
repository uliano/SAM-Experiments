---
title: SAMC21 Datasheet Ch.39 SDADC
type: source
tags: [sdadc, sigma-delta, analog, samc21, datasheet]
sources: [samc21-datasheet-ch39-sdadc]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.39 SDADC

Sigma-Delta Analog-to-Digital Converter. Present on SAM C21 only (not SAM C20).
One instance. 16-bit resolution, three differential input channels, output rate
up to 1.5 Msps / OSR (maximum 23.4 ksps at OSR64).

## Abstract

The SDADC converts differential analog signals to 16-bit signed values using a
sigma-delta modulator with a 3rd-order SINC CIC decimation filter. Three external
differential pairs (AINP0/AINN0, AINP1/AINN1, AINP2/AINN2) plus internal
references (INTREF, VREFB, DAC, VDDANA) are available. Gain, offset, and shift
post-processing are applied per-source. Chopper mode reduces offset errors.
Automatic channel sequencing, window monitoring, and DMA/event integration
are included.

## Key Facts

- SAM C21 only (not SAM C20). One instance.
- Resolution: 16-bit, signed result (differential: range −V_REF to +V_REF).
- Up to 3 external differential channels: (AINP0,AINN0), (AINP1,AINN1), (AINP2,AINN2).
- Clock chain:
  - `f_CLK_SDADC = f_GCLK_SDADC / (2 × (PRESCALER + 1))`
  - `f_CLK_SDADC_FS = f_CLK_SDADC / 4` (sigma-delta sampling clock)
  - `f_output = f_CLK_SDADC_FS / OSR = f_CLK_SDADC / (4 × OSR)`
- OSR values (CTRLB.OSR): 64, 128, 256, 512, 1024.
- Maximum output rate at OSR64: 1.5 Msps / 64 ≈ 23.4 ksps.
- OSR must not be changed while SDADC is running; reset the SDADC first.
- PRESCALER[7:0]: 8-bit value; CLK_SDADC range = GCLK/2 (PRESCALER=0) to GCLK/512 (PRESCALER=255).
- SKPCNT[3:0]: skip count — how many samples to discard at startup before first valid result.
  First valid sample always starts from 3rd sample onward (sigma-delta settling).
- Enable-protected: CTRLA.ONDEMAND/RUNSTDBY bits, CTRLB, CTRLC, EVCTRL, ANACTRL.
- Write-synchronized (poll SYNCBUSY): INPUTCTRL, REFCTRL, CTRLC, WINLT, WINUT,
  OFFSETCORR, GAINCORR, SHIFTCORR, SWTRIG, ANACTRL.
- Result is always in the 16 LSBs of the 32-bit RESULT register (signed).

## Reference Voltage (REFCTRL)

| REFSEL[1:0] | Name | Description |
|-------------|------|-------------|
| 0x0 | INTREF | Internal bandgap (SUPC.VREF.SEL) |
| 0x1 | VREFB | External reference pin |
| 0x2 | DAC | DAC internal output |
| 0x3 | VDDANA | Supply voltage 2.7–5.5 V |

REFRANGE[1:0]: Reference voltage range (must match actual V_REF):
- 0x0: V_REF < 1.4 V
- 0x1: 1.4 V < V_REF < 2.4 V
- 0x2: 2.4 V < V_REF < 3.6 V
- 0x3: V_REF > 3.6 V

ONREFBUF=1: enables internal reference buffer (reduces load current from 5 µA to 0.10 µA).
Required when using INTREF or DAC as reference.

## Over Sampling Ratio (CTRLB.OSR)

| OSR value | Name | Output Rate (at f_CLK_SDADC=1 MHz) |
|-----------|------|-------------------------------------|
| 0x0 | OSR64 | ~3906 sps |
| 0x1 | OSR128 | ~1953 sps |
| 0x2 | OSR256 | ~977 sps |
| 0x3 | OSR512 | ~488 sps |
| 0x4 | OSR1024 | ~244 sps |

## Data Correction (Post-Processing)

Applied per channel source after decimation:

```
Result = (Data0 + OFFSETCORR) × GAINCORR / 2^SHIFTCORR
```

- `OFFSETCORR`: 24-bit signed integer (−V_REF offset compensation).
- `GAINCORR`: 14-bit unsigned integer (1.0 = 0x2000).
- `SHIFTCORR`: 4-bit unsigned right-shift (0–15).
- Result is saturated to [0, 2^16 − 1] then lower 16 bits stored in RESULT.

## Chopper Mode (ANACTRL.ONCHOP)

Enables analog chopper to reduce offset error at the sigma-delta modulator input.
Configure before enabling the SDADC (enable-protected via ANACTRL).

## Window Monitor

WINCTRL.WINMODE[2:0]: same 5 modes as ADC (disabled, above, below, inside, outside).
Thresholds: WINLT and WINUT are 32-bit (full RESULT range).

## Automatic Sequence

SEQCTRL.SEQENn[2:0]: enable automatic sequence over channels 0–2.
Sequence order: channel 0 → channel 1 → channel 2 (ascending).
SEQSTATUS.SEQSTATE[3:0]: last completed channel index.
SEQSTATUS.SEQBUSY: sequence in progress.

## DMA

DMA request RESRDY: set when result is available; cleared when RESULT is read.

## Interrupts

- RESRDY: result ready.
- OVERRUN: result not read before next conversion (free-running only).
- WINMON: window monitor condition matched.

INTFLAG.RESRDY and INTFLAG.WINMON also cleared by reading RESULT register.

## Events

- Output: RESRDYEO (result ready), WINMONEO (window match).
- Input: STARTEI (start conversion), FLUSHEI (flush).
- Asynchronous event paths only; STARTINV/FLUSHINV for falling-edge trigger.
- FLUSH has priority over START.

## Synchronization

Write-synchronized (poll SYNCBUSY):
INPUTCTRL, REFCTRL, CTRLC, WINLT, WINUT, OFFSETCORR, GAINCORR, SHIFTCORR, SWTRIG, ANACTRL.
CTRLA.SWRST and CTRLA.ENABLE: poll SYNCBUSY.SWRST/ENABLE.
Note: if a write to a synchronized register occurs while its SYNCBUSY bit is set, the
write is discarded and a bus error is generated.

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x00 | CTRLA | SWRST, ENABLE, RUNSTDBY, ONDEMAND |
| 0x01 | REFCTRL | REFSEL[1:0], REFRANGE[1:0], ONREFBUF (enable-protected) |
| 0x02 | CTRLB | PRESCALER[7:0], OSR[2:0], SKPCNT[3:0] (enable-protected) |
| 0x04 | EVCTRL | FLUSHEI, STARTEI, FLUSHINV, STARTINV, RESRDYEO, WINMONEO (enable-protected) |
| 0x05 | INTENCLR | WINMON, OVERRUN, RESRDY |
| 0x06 | INTENSET | (same) |
| 0x07 | INTFLAG | (same — write 1 or read RESULT to clear RESRDY/WINMON) |
| 0x08 | SEQSTATUS | SEQBUSY, SEQSTATE[3:0] (read-only) |
| 0x09 | INPUTCTRL | MUXSEL[3:0] (write-sync) |
| 0x0A | CTRLC | FREERUN (enable-protected) |
| 0x0B | WINCTRL | WINMODE[2:0] |
| 0x0C | WINLT | WINLT[31:0] (32-bit, write-sync) |
| 0x10 | WINUT | WINUT[31:0] (32-bit, write-sync) |
| 0x14 | OFFSETCORR | OFFSETCORR[23:0] signed (write-sync) |
| 0x18 | GAINCORR | GAINCORR[13:0] unsigned (write-sync) |
| 0x1A | SHIFTCORR | SHIFTCORR[3:0] (write-sync) |
| 0x1C | SWTRIG | START, FLUSH (write-sync) |
| 0x20 | SYNCBUSY | SWRST, ENABLE, CTRLC, MUXCTRL, WINCTRL, WINLT, WINUT, OFFSETCORR, GAINCORR, ANACTRL, SWTRIG, SHIFTCORR |
| 0x24 | RESULT | RESULT[15:0] signed 16-bit (lower 16 bits of 32-bit register) |
| 0x28 | SEQCTRL | SEQEN[2:0] (one bit per channel) |
| 0x2C | ANACTRL | BUFTEST, ONCHOP, CTLSDADC[4:0] (enable-protected, write-sync) |
| 0x2E | DBGCTRL | DBGRUN |

## See Also

- [[SAMC21 Datasheet Ch.38 ADC]] — SAR ADC (vs. sigma-delta SDADC)
- [[SUPC]] — VREF configuration for INTREF
- [[Clock System]] — GCLK_SDADC setup
- [[EVSYS]] — event routing for SDADC trigger
- [[DMA Controller]] — SDADC result DMA transfer
