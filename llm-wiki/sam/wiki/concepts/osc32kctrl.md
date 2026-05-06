---
title: OSC32KCTRL
type: concept
tags: [osc32kctrl, oscillator, clock, xosc32k, osc32k, osculp32k, rtc, samc21]
sources: [samc21-datasheet-ch21-osc32kctrl]
created: 2026-05-05
updated: 2026-05-05
---

# OSC32KCTRL — 32 kHz Oscillators Controller

OSC32KCTRL manages all three 32 kHz clock sources on the SAM C21:
- **XOSC32K** — external crystal oscillator, 32.768 kHz
- **OSC32K** — internal RC oscillator, ~32 kHz (factory-calibrated, ~1% accuracy)
- **OSCULP32K** — ultra-low-power internal RC, always-on after POR, ~32 kHz

Base address: **0x40001400** (APB-A, enabled by default in APBAMASK).

## Register Map

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | INTENCLR | 0x00 | Interrupt Enable Clear |
| 0x04 | INTENSET | 0x00 | Interrupt Enable Set |
| 0x08 | INTFLAG | 0x00 | Interrupt Flag Status and Clear |
| 0x0C | STATUS | 0x00 | Status |
| 0x10 | RTCCTRL | 0x00 | RTC Clock Selection |
| 0x14 | XOSC32K | 0x0080 | External Crystal Oscillator Control (16-bit) |
| 0x16 | CFDCTRL | 0x00 | Clock Failure Detection Control (8-bit) |
| 0x17 | EVCTRL | 0x00 | Event Control (8-bit) |
| 0x18 | OSC32K | 0x00000080 | Internal RC Oscillator Control (32-bit) |
| 0x1C | OSCULP32K | 0x00 | Ultra Low Power Oscillator (8-bit) |

## STATUS Register (offset 0x0C)

| Bit | Name | Description |
|-----|------|-------------|
| 3 | CLKSW | Clock switched to safe clock (CFD active) |
| 2 | CLKFAIL | XOSC32K clock failure detected |
| 1 | OSC32KRDY | OSC32K stable and ready |
| 0 | XOSC32KRDY | XOSC32K stable and ready |

## RTCCTRL Register (offset 0x10, 8-bit)

Selects the clock source fed to the RTC peripheral (bits 2:0 = RTCSEL).

| RTCSEL[2:0] | Source | Frequency |
|-------------|--------|-----------|
| 0x0 | OSCULP32K / 32 | 1.024 kHz |
| 0x1 | OSCULP32K | 32.768 kHz |
| 0x2 | OSC32K / 32 | 1.024 kHz |
| 0x3 | OSC32K | 32.768 kHz |
| 0x4 | XOSC32K / 32 | 1.024 kHz |
| 0x5 | XOSC32K | 32.768 kHz |

This register only routes the clock to RTC. The selected oscillator must also be enabled independently.

## XOSC32K Register (offset 0x14, 16-bit, reset=0x0080)

| Bits | Name | Description |
|------|------|-------------|
| 15 | WRTLOCK | Write Lock — once set, this register cannot be written until reset |
| 12 | ONDEMAND | Run only when requested by a peripheral (reset=1) |
| 11 | RUNSTDBY | Run in Standby sleep mode |
| 6 | EN1K | Enable 1.024 kHz output (divide-by-32 of 32.768 kHz, for RTC use) |
| 5 | EN32K | Enable 32.768 kHz output — **must set when used as GCLK source** |
| 4 | XTALEN | 1=crystal oscillator mode, 0=external clock input on XIN32 only |
| 3:1 | STARTUP[2:0] | Start-up time (see table) |
| 0 | ENABLE | Enable XOSC32K |

Reset 0x0080: ONDEMAND=1, all others 0 (disabled, no output enabled).

### STARTUP Table

| Value | Start-up Time |
|-------|--------------|
| 0x0 | 0.0625 s |
| 0x1 | 0.125 s |
| 0x2 | 0.25 s |
| 0x3 | 0.5 s |
| 0x4 | 1 s |
| 0x5 | 2 s |
| 0x6 | 4 s |
| 0x7 | 8 s |

32 kHz crystals oscillate slowly — startup times are 0.06–8 seconds, orders of magnitude longer than XOSC.

### Usage Example

```cpp
OSC32KCTRL->XOSC32K.reg =
    OSC32KCTRL_XOSC32K_ENABLE  |
    OSC32KCTRL_XOSC32K_XTALEN  |
    OSC32KCTRL_XOSC32K_EN32K   |    // required for GCLK use
    OSC32KCTRL_XOSC32K_STARTUP(4);  // ~1 s startup time

// Wait with timeout — crystal can fail silently
uint32_t start = millis();
while (!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
    if (millis() - start > 2000) { /* crystal failure */ break; }
}
```

## CFDCTRL Register (offset 0x16, 8-bit)

Clock Failure Detection for XOSC32K. When the crystal stops oscillating, GCLK generators sourced from XOSC32K automatically switch to a safe clock derived from OSCULP32K.

| Bit | Name | Description |
|-----|------|-------------|
| 2 | CFDPRESC | Safe clock prescaler: 0 = OSCULP32K, 1 = OSCULP32K/2 |
| 1 | SWBACK | Switch Back: automatically return to XOSC32K after recovery |
| 0 | CFDEN | Clock Failure Detection Enable |

Use CFDPRESC=0 (OSCULP32K ~32 kHz) as safe clock, matching the XOSC32K frequency.

## OSC32K Register (offset 0x18, 32-bit, reset=0x00000080)

| Bits | Name | Description |
|------|------|-------------|
| 31 | WRTLOCK | Write lock |
| 22:16 | CALIB[6:0] | Frequency calibration — **must load from NVM SW Cal area at startup** |
| 12 | ONDEMAND | Run only when requested (reset=1) |
| 11 | RUNSTDBY | Run in Standby |
| 6 | EN1K | Enable 1.024 kHz output |
| 5 | EN32K | Enable 32.768 kHz output |
| 3:1 | STARTUP[2:0] | Start-up time |
| 0 | ENABLE | Enable OSC32K |

Reset 0x80: ONDEMAND=1, CALIB=0 (uncalibrated until firmware loads from NVM).

### NVM Calibration Load

OSC32K CALIB[6:0] is at NVM Software Calibration Area (0x806020), word 0, bits 18:12.

```cpp
uint32_t cal_word = *(volatile uint32_t*)0x806020;
uint8_t osc32k_cal = (cal_word >> 12) & 0x7F;

OSC32KCTRL->OSC32K.reg =
    OSC32KCTRL_OSC32K_ENABLE            |
    OSC32KCTRL_OSC32K_EN32K             |
    OSC32KCTRL_OSC32K_CALIB(osc32k_cal);
while (!OSC32KCTRL->STATUS.bit.OSC32KRDY);
```

### STARTUP Table

| Value | Start-up Time |
|-------|--------------|
| 0x0 | 0.092 ms |
| 0x1 | 0.122 ms |
| 0x2 | 0.183 ms |
| 0x3 | 0.305 ms |
| 0x4 | 0.549 ms |
| 0x5 | 1.038 ms |
| 0x6 | 2.014 ms |
| 0x7 | 3.967 ms |

## OSCULP32K Register (offset 0x1C, 8-bit)

Ultra-low-power 32 kHz internal oscillator. **Always-on after POR — cannot be disabled.**

| Bits | Name | Description |
|------|------|-------------|
| 7 | WRTLOCK | Write lock |
| 4:0 | CALIB[4:0] | Frequency calibration (auto-loaded from factory flash at startup) |

No ENABLE bit. Firmware does not need to initialize OSCULP32K — it is ready immediately after POR. This is the always-available fallback clock for GCLK generators when other oscillators are not configured.

## INTFLAG / INTENSET / INTENCLR Bits

| Bit | Name | Event |
|-----|------|-------|
| 3 | CLKSW | Clock switched to safe clock (CFD) |
| 2 | CLKFAIL | XOSC32K failure detected |
| 1 | OSC32KRDY | OSC32K ready |
| 0 | XOSC32KRDY | XOSC32K ready |

OSC32KCTRL shares **NVIC line 0** with PM, MCLK, OSCCTRL, SUPC, and PAC. An ISR must read all INTFLAG registers to determine the source.

## Bring-Up Notes

- OSCULP32K is always available — good for safe-clock fallback and WDT at bring-up.
- OSC32K needs NVM calibration load or it runs un-calibrated (~±2%). Load CALIB early in `sys_init()`.
- XOSC32K needs long timeout handling — crystal can fail or simply be absent on boards without the 32K footprint populated. See [[QuartzTest]].
- On this board (`SAM_BOARD_TEST_XOSC32K=0`): XOSC32K is not tested at bring-up. Only OSCULP32K is available for GCLK3 / RTC.

## See Also

- [[Clock System]] — how XOSC32K / OSC32K feed GCLK generators (GCLK3 on this board)
- [[OSCCTRL]] — companion oscillator controller (XOSC, OSC48M, DPLL)
- [[Memory Map]] — NVM SW Cal area at 0x806020, OSC32K CALIB at bits 18:12
- [[NVIC Interrupt Map]] — line 0 shared interrupt; ISR must check all INTFLAG registers
- [[QuartzTest]] — firmware implementation for XOSC32K startup with CFD and timeout
- [[SAMC21 Datasheet Ch. 21 — OSC32KCTRL]] — register-level source reference
