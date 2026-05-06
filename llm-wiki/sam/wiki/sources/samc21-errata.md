---
title: SAMC21 Errata (DS80000748)
type: source
tags: [errata, silicon, samc21, samc20, workaround]
sources: [samc21-errata]
created: 2026-05-06
updated: 2026-05-06
---

# SAMC21 Errata (DS80000748)

Summary of the SAMC20/C21 silicon errata document (DS80000748), 40 pages,
covering rev A through D silicon for E/G/J and N package variants.

## Abstract

This document lists silicon errata for the SAMC20 and SAMC21 families across
all silicon revisions (A–D) and package variants (E/G/J and N).

**Verified silicon revision**: DSU.DID = 0x11010500 → REVISION = 0x5 = **Rev F**
(DEVSEL=0x00 confirms ATSAMC21J18A, confirmed by Table 2 of the errata doc:
ATSAMC21J18A = DID 0x1101xx00, revision A=0x0 ... E=0x4, our chip REVISION=0x5=F).

**All documented errata are ANNULLED on Rev F.** The document covers revisions
A–E. Examining every single errata row: **no errata has an X in the Rev E
column for E/G/J devices.** All errata affecting E/G/J were fixed by Rev E at
the latest. Since our chip is Rev F (one step beyond the last documented revision),
all documented errata are resolved. The only open risk is undocumented Rev F
silicon bugs — not covered by any errata document at this time.

Note: errata 1.8.12 (OSC48M accuracy) mentions Rev D and E in the workaround
text but shows no X in the Rev E table column — the CAL48M NVM calibration
load at startup is standard practice regardless of silicon revision.

## Errata Relevant to CCL PWM Design — ALL ANNULLED on Rev F

---

### 1.4.4 ADC — Synchronized Event (Rev A–D, fixed in Rev E)

**Problem**: The ADC synchronous event path does not work correctly.

**Workaround**: Use only the **asynchronous** event channel (PATH=0x2) to
trigger ADC conversions. Never configure ADC as a synchronous event user.

See also: [[EVSYS]], [[ADC Configuration]].

---

### 1.7.1 CCL — RS Latch Reset Not Functional (Rev A, E/G/J)

**Problem**: The RS latch sequential circuit reset function (SEQCTRL.SEQSEL=0x4)
does not reset correctly. The latch cannot be cleared via the reset input.

**Workaround**: To clear the RS latch, **disable the LUT** (LUTCTRLn.ENABLE=0),
then re-enable it.

See also: [[CCL Configuration]], [[SAMC21 Datasheet Ch.37 CCL]].

---

### 1.8.2 AC — Clock Configuration (Rev A, E/G/J)

**Problem**: The dedicated AC GCLK peripheral channel (`GCLK_AC`, PCHCTRL[34]
on E/G/J) is non-functional. Enabling it has no effect.

**Workaround**: Clock the AC using **GCLK_ADC1** (PCHCTRL[36] on E/G/J) instead.
The AC will pick up the ADC1 GCLK as its clock source.

> This means AC and ADC1 share a GCLK generator. Choose a generator frequency
> suitable for both, or use two separate generators and pick the one for ADC1.

See also: [[AC Configuration]], [[ADC Configuration]], [[Clock System]].

---

### 1.8.3 CCL — TC Input Selection (Rev A, E/G/J)

**Problem**: The TC input selection for CCL LUT inputs is swapped in hardware
compared to the datasheet specification.

**Actual hardware mapping (INSEL=TC)**:

| LUT | Datasheet says | Actual silicon |
|-----|---------------|----------------|
| LUT0 | TC0 | TC4 |
| LUT1 | TC1 | TC0 |
| LUT2 | TC2 | TC1 |
| LUT3 | TC3 | TC2 |

**Actual hardware mapping (INSEL=ALTTC)**:

| LUT | Datasheet says | Actual silicon |
|-----|---------------|----------------|
| LUT0 | TC1 | TC0 |
| LUT1 | TC2 | TC1 |
| LUT2 | TC3 | TC2 |
| LUT3 | TC4 | TC3 |

**Workaround**: Use INSEL=ALTTC to get the TC0/TC1/TC2/TC3 mapping that the
datasheet assigns to INSEL=TC. Adjust TC selection accordingly.

> On SAMC21J18A (J-variant), TC4 is standalone (no TC5 to pair). Avoid using
> TC4 as CCL input via INSEL=TC (would need LUT0 per errata mapping).

See also: [[CCL Configuration]], [[TC 32-Bit Paired Mode]].

---

### 1.12.1 EVSYS — Spurious Overrun (Rev A–D, fixed in Rev E)

**Problem**: Using a synchronous EVSYS channel with an always-on GCLK generator
(GCLK_GENCTRL.GENEN=1, GCLK_GENCTRL.OE=1) causes spurious overrun events.

**Workaround**: Set **GCLK_GENCTRL.ONDEMAND=1** for any GCLK generator used
as the clock for a synchronous EVSYS channel. The on-demand flag prevents the
spurious overrun condition.

See also: [[EVSYS]], [[Clock System]].

---

### 1.20.2 TC — Capture on I/O Pins (Rev A, E/G/J)

**Problem**: TC capture triggered from I/O pins (TC WO input capture via PMux)
does not work correctly.

**Workaround**: Use the **Event System** with EIC or CCL to route the capture
trigger signal to TC instead of using TC I/O pin capture directly.

See also: [[TC 32-Bit Paired Mode]], [[EVSYS]], [[EIC]], [[CCL Configuration]].

---

## Additional Errata (Lower Priority for Current Design)

| # | Peripheral | Summary | Revisions |
|---|-----------|---------|-----------|
| 1.1.1 | CPU | Cortex-M0+ erratum — see ARM ref | All |
| 1.2.1 | PM | Incorrect STATUS.SLEEPACK clearing | Rev A, E/G/J |
| 1.3.1 | SUPC | BOD33 level incorrect in standby | Rev A, E/G/J |
| 1.4.1 | ADC | GAIN correction offset error | Rev A, E/G/J |
| 1.4.2 | ADC | Sequence scan lockup on overrun | Rev A–D, E/G/J |
| 1.4.3 | ADC | First result invalid after enable | Rev A–D, E/G/J |
| 1.5.1 | DAC | OUTPUT glitch at startup | Rev A, E/G/J |
| 1.6.1 | SDADC | Incorrect result after SEQCTRL enable | Rev A, E/G/J |
| 1.9.1 | SERCOM | I2C clock stretching issue | Rev A, E/G/J |
| 1.10.1 | WDT | Window open period too short | Rev A, E/G/J |
| 1.11.1 | RTC | Count value incorrect after read-sync | Rev A, E/G/J |
| 1.13.1 | NVMCTRL | Flash corruption on partial page write | Rev A, E/G/J |
| 1.14.1 | DMAC | Channel priority inversion | Rev A, E/G/J |
| 1.15.1 | CAN | Tx FIFO full flag race | Rev A, E/G/J |
| 1.16.1 | DSU | CRC peripheral lockup | Rev A, E/G/J |
| 1.17.1 | FREQM | First measurement may be invalid | Rev A, E/G/J |
| 1.18.1 | EIC | Spurious interrupt on enable | Rev A, E/G/J |
| 1.19.1 | PORT | Drive strength not settable in WRCONFIG | Rev A, E/G/J |
| 1.21.1 | TCC | Fault input not cleared by FAULTA/B | Rev A, E/G/J |

## Entities Mentioned

- ATSAMC21J18A-AU (target hardware)

## Concepts Mentioned

- [[CCL Configuration]]
- [[AC Configuration]]
- [[ADC Configuration]]
- [[EVSYS]]
- [[TC 32-Bit Paired Mode]]
- [[Clock System]]
- [[EIC]]

## See Also

- [[CCL Configuration]]
- [[SAMC21 Datasheet Ch.37 CCL]]
- [[AC Configuration]]
- [[EVSYS]]
- [[TC 32-Bit Paired Mode]]
