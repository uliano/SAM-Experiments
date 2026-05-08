---
title: OSCCTRL
type: concept
tags: [oscctrl, oscillator, clock, xosc, osc48m, dpll, samc21]
sources: [samc21-datasheet-ch20-oscctrl]
created: 2026-05-05
updated: 2026-05-05
---

# OSCCTRL — Oscillators Controller

OSCCTRL manages three oscillators: XOSC (external crystal), OSC48M (internal 48 MHz), and FDPLL96M (PLL).
The 32K oscillators (XOSC32K, OSC32K, OSCULP32K) are managed by the separate OSC32KCTRL module.

Base address: 0x40001000 (APB-A, enabled by default in APBAMASK).

## Register Map

| Offset | Name | Description |
|--------|------|-------------|
| 0x00 | INTENCLR | Interrupt Enable Clear |
| 0x04 | INTENSET | Interrupt Enable Set |
| 0x08 | INTFLAG | Interrupt Flag Status and Clear |
| 0x0C | STATUS | Status |
| 0x10 | XOSCCTRL | XOSC Control (16-bit) |
| 0x12 | CFDPRESC | CFD Prescaler (8-bit) |
| 0x13 | EVCTRL | Event Control (8-bit) |
| 0x14 | OSC48MCTRL | OSC48M Control (8-bit) |
| 0x15 | OSC48MDIV | OSC48M Divider (8-bit) |
| 0x16 | OSC48MSTUP | OSC48M Startup (8-bit) |
| 0x18 | OSC48MSYNCBUSY | OSC48M Synchronization Busy (32-bit) |
| 0x1C | DPLLCTRLA | DPLL Control A (8-bit) |
| 0x20 | DPLLRATIO | DPLL Ratio (32-bit) |
| 0x24 | DPLLCTRLB | DPLL Control B (32-bit) |
| 0x28 | DPLLPRESC | DPLL Prescaler (8-bit) |
| 0x2C | DPLLSYNCBUSY | DPLL Synchronization Busy (8-bit) |
| 0x30 | DPLLSTATUS | DPLL Status (8-bit) |
| 0x38 | CAL48M | OSC48M Calibration (32-bit) |

## STATUS Register (offset 0x0C)

| Bit | Name | Description |
|-----|------|-------------|
| 11 | DPLLDRTO | DPLL Loop Divider Ratio Update Complete |
| 10 | DPLLLTO | DPLL Lock Timeout |
| 9 | DPLLCKF | DPLL Lock Fall |
| 8 | DPLLCKR | DPLL Lock Rise |
| 4 | OSC48MRDY | OSC48M stable and ready |
| 2 | CLKSW | XOSC switched to safe clock (CFD activated) |
| 1 | CLKFAIL | XOSC clock failure detected |
| 0 | XOSCRDY | XOSC ready |

## XOSC — External Crystal Oscillator

### XOSCCTRL (offset 0x10, 16-bit)

| Bits | Name | Description |
|------|------|-------------|
| 15:12 | STARTUP[3:0] | Start-up time in OSCULP32K cycles (see table) |
| 11 | AMPGC | Automatic Amplitude Gain Control |
| 10:8 | GAIN[2:0] | Crystal gain — must be set even when AMPGC=1 |
| 7 | ONDEMAND | Run only when requested by a peripheral |
| 6 | RUNSTDBY | Run in Standby sleep mode |
| 4 | SWBEN | Clock Switch Back Enable (return to XOSC after CFD recovery) |
| 3 | CFDEN | Clock Failure Detection Enable |
| 2 | XTALEN | 0=external clock input, 1=crystal connected to XIN/XOUT |
| 1 | ENABLE | Enable XOSC |

### STARTUP Table (XOSCCTRL bits 11:8)

| Value | Start-up Time |
|-------|--------------|
| 0x0 | 31 µs |
| 0x1 | 61 µs |
| 0x2 | 122 µs |
| 0x3 | 244 µs |
| 0x4 | 488 µs |
| 0x5 | 977 µs |
| 0x6 | 1.953 ms |
| 0x7 | 3.906 ms |
| 0x8 | 7.813 ms |
| 0x9 | 15.625 ms |
| 0xA | 31.25 ms |
| 0xB | 62.5 ms |
| 0xC | 125 ms |
| 0xD | 250 ms |
| 0xE | 500 ms |
| 0xF | 1000 ms |

### GAIN Table (XOSCCTRL bits 7:5)

| GAIN[2:0] | Crystal Frequency Range |
|-----------|------------------------|
| 0x0 | Up to 2 MHz |
| 0x1 | Up to 4 MHz |
| 0x2 | Up to 8 MHz |
| 0x3 | Up to 16 MHz |
| 0x4 | Up to 30 MHz — use this for 24 MHz crystal |

For 24 MHz crystal on this board (PA14/PA15):
- XTALEN=1
- GAIN[2:0]=0x4 (covers 24 MHz, up to 30 MHz range)
- AMPGC=1 recommended (automatic amplitude control saves power)
- STARTUP — use 0x8 (7.8 ms) or higher; set per crystal datasheet

### Ready Detection

Wait for STATUS.XOSCRDY=1 after enabling. Can also use interrupt (INTENSET.XOSCRDY=1).

```cpp
OSCCTRL->XOSCCTRL.reg =
    OSCCTRL_XOSCCTRL_ENABLE |
    OSCCTRL_XOSCCTRL_XTALEN |
    OSCCTRL_XOSCCTRL_AMPGC  |
    OSCCTRL_XOSCCTRL_GAIN(4) |   // GAIN=4: up to 30 MHz, required for 24 MHz
    OSCCTRL_XOSCCTRL_STARTUP(8); // ~16k cycles startup
while (!OSCCTRL->STATUS.bit.XOSCRDY);
```

## OSC48M — 48 MHz Internal Oscillator

### Reset State

At reset, OSC48M is enabled and **divided by 12** → outputs 4 MHz. GCLK_MAIN=4 MHz at boot.
The firmware init.hpp sets DIV=0 (no division) to run at full 48 MHz.

### OSC48MCTRL (offset 0x14, 8-bit)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | ONDEMAND | Run only when requested |
| 6 | RUNSTDBY | Run in Standby |
| 1 | ENABLE | Enable (default=1 at reset) |

### OSC48MDIV (offset 0x15, 8-bit, Write-Synchronized)

| Bits | Name | Description |
|------|------|-------------|
| 3:0 | DIV[3:0] | Division factor: f_out = 48 MHz / (DIV+1) |

| DIV | f_out | | DIV | f_out |
|-----|-------|-|-----|-------|
| 0x0 | 48 MHz | | 0x8 | 5.333 MHz |
| 0x1 | 24 MHz | | 0x9 | 4.8 MHz |
| 0x2 | 16 MHz | | 0xA | 4.364 MHz |
| 0x3 | 12 MHz | | **0xB** | **4 MHz ← reset** |
| 0x4 | 9.6 MHz | | 0xC | 3.692 MHz |
| 0x5 | 8 MHz | | 0xD | 3.429 MHz |
| 0x6 | 6.857 MHz | | 0xE | 3.2 MHz |
| 0x7 | 6 MHz | | 0xF | 3 MHz |

Default DIV=0xB (11) → 48/12 = 4 MHz.
To run at 48 MHz: write DIV=0, then wait OSC48MSYNCBUSY.OSC48MDIV=0.

```cpp
OSCCTRL->OSC48MDIV.reg = OSCCTRL_OSC48MDIV_DIV(0);
while (OSCCTRL->OSC48MSYNCBUSY.bit.OSC48MDIV);
```

### OSC48MSTUP (offset 0x16, 8-bit)

| Bits | Name | Description |
|------|------|-------------|
| 2:0 | STARTUP[2:0] | Start-up time after wake-up |

### CAL48M (offset 0x38, 32-bit)

Must be loaded from NVM Software Calibration Area (0x806020) at startup:

```cpp
uint64_t cal = *(volatile uint64_t*)0x806020;
// Select 5V or 3V3 calibration based on supply voltage
uint32_t cal48m;
if (VDD_RANGE_3V6_TO_5V5)
    cal48m = (cal >> 19) & 0x3FFFFF;  // bits 40:19 = CAL48M 5V
else
    cal48m = (cal >> 41) & 0x3FFFFF;  // bits 62:41 = CAL48M 3V3
OSCCTRL->CAL48M.reg = cal48m;
```

## Clock Failure Detection (CFD)

CFD monitors the XOSC. If XOSC fails, it switches GCLK generators using XOSC to a safe clock derived from OSC48M with a configurable prescaler.

### CFDPRESC (offset 0x12, 8-bit)

Bits 2:0 CFDPRESC[2:0]: safe clock = OSC48M / 2^P. Set so safe clock ≤ XOSC frequency.

For 24 MHz XOSC and OSC48M at 48 MHz: 48/2^1 = 24 MHz → CFDPRESC=1.

### CFD Status Bits

- STATUS.CLKFAIL=1: failure detected, safe clock is active
- STATUS.CLKSW=1: clock switch to safe clock has occurred
- INTFLAG.CLKFAIL=1: interrupt pending

### CFD Recovery

When XOSC recovers, write XOSCCTRL.SWBEN=1 to enable automatic switch back.

## FDPLL96M — Fractional Digital PLL

Output frequency: `f_CK = f_CKR × (LDR + 1 + LDRFRAC/16) × (1/2^PRESC)`

Reference clock sources (DPLLCTRLB.REFCLK[1:0]):
- 0x0 = XOSC32K (32.768 kHz)
- 0x1 = XOSC (24 MHz crystal for us)
- 0x2 = GCLK (from a GCLK peripheral channel)

### Typical Usage: 48 MHz from 32.768 kHz XOSC32K

LDR=1463, LDRFRAC=13: 32768 × (1464 + 13/16) = 32768 × 1464.8125 ≈ 48.0 MHz

```cpp
// Integer mode example: 32kHz × 1500 = 48 MHz
OSCCTRL->DPLLRATIO.reg =
    OSCCTRL_DPLLRATIO_LDR(1499) |    // LDR = ratio - 1
    OSCCTRL_DPLLRATIO_LDRFRAC(0);
while (OSCCTRL->DPLLSYNCBUSY.bit.DPLLRATIO);

OSCCTRL->DPLLCTRLB.reg =
    OSCCTRL_DPLLCTRLB_REFCLK_XOSC32K;

OSCCTRL->DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;
while (OSCCTRL->DPLLSYNCBUSY.bit.ENABLE);
while (!OSCCTRL->DPLLSTATUS.bit.LOCK &&
       !OSCCTRL->DPLLSTATUS.bit.CLKRDY);
```

### DPLLRATIO (offset 0x20, 32-bit, Write-Synchronized)

| Bits | Name |
|------|------|
| 19:16 | LDRFRAC[3:0] |
| 11:0 | LDR[11:0] |

### DPLLCTRLB (offset 0x24, 32-bit)

| Bits | Name | Description |
|------|------|-------------|
| 26:16 | DIV[10:0] | Reference clock divider (for XOSC reference) |
| 12 | LBYPASS | Lock Bypass (ignore lock, always output) |
| 10:8 | LTIME[2:0] | Lock timeout |
| 6 | WUF | Wake Up Fast (output before lock) |
| 5 | LPEN | Low Power mode (bypasses TDC) |
| 4:3 | FILTER[1:0] | Digital filter bandwidth |
| 1:0 | REFCLK[1:0] | 0=XOSC32K, 1=XOSC, 2=GCLK |

**DIV[10:0] for XOSC reference:** f_DIV = f_XOSC / (2 × (DIV+1)). Divide the crystal down to the PLL reference input range. Example: 24 MHz XOSC, DIV=3 → 24/(2×4) = 3 MHz.

**LTIME[2:0] — lock timeout:**

| Value | Timeout |
|-------|---------|
| 0x0 | No timeout (default) |
| 0x4 | 8 ms |
| 0x5 | 9 ms |
| 0x6 | 10 ms |
| 0x7 | 11 ms |

**FILTER[1:0] — digital loop filter:**

| Value | Mode |
|-------|------|
| 0x0 | Default filter |
| 0x1 | Low bandwidth (LBFILT) |
| 0x2 | High bandwidth (HBFILT) |
| 0x3 | High damping (HDFILT) |

### DPLLPRESC (offset 0x28, 8-bit, Write-Synchronized)

| Value | Divide By |
|-------|----------|
| 0 | 1 (output = CK) |
| 1 | 2 (output = CKDIV2) |
| 2 | 4 (output = CKDIV4) |

### DPLLSTATUS (offset 0x30, 8-bit)

| Bit | Name | Description |
|-----|------|-------------|
| 1 | CLKRDY | Output clock active and stable |
| 0 | LOCK | PLL locked to reference |

## INTFLAG / INTENSET / INTENCLR Bits

| Bit | Name | Event |
|-----|------|-------|
| 11 | DPLLDRTO | DPLL ratio update complete |
| 10 | DPLLLTO | DPLL lock timeout |
| 9 | DPLLCKF | DPLL lock fall |
| 8 | DPLLCKR | DPLL lock rise |
| 4 | OSC48MRDY | OSC48M ready |
| 1 | CLKFAIL | XOSC failure detected |
| 0 | XOSCRDY | XOSC ready |

All are synchronous wake-up sources. OSCCTRL shares NVIC line 0 with MCLK, OSC32KCTRL, SUPC, PAC. PM has no interrupt line.

## See Also

- [[Clock System]] — how OSCCTRL feeds the GCLK layer
- [[GCLK]] — generator configuration using OSC48M / XOSC / DPLL as sources
- [[QuartzTest]] — firmware that exercises XOSC and XOSC32K with CFD
- [[Memory Map]] — NVM calibration area at 0x806020 for CAL48M
