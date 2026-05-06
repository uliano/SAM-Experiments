---
title: Clock System
type: concept
tags: [clock, oscctrl, gclk, mclk, osc48m, xosc, samc21]
sources: [firmware-core-library, application-bring-up, samc21-datasheet-ch16-gclk, samc21-datasheet-ch17-mclk]
created: 2026-05-05
updated: 2026-05-05
---

# Clock System

How clocks are configured from reset through the full bring-up sequence on the SAMC21J18A.

## Reset Defaults

After reset, OSC48M is enabled and **divided by 12** (OSC48MDIV=0xB) → GCLK_MAIN = **4 MHz**.
GCLK0 (Generator 0) sources OSC48M, no division. CPUDIV=1 → CPU = 4 MHz.
NVM has 0 wait states at reset (safe for 4 MHz).

## sys_init() — Minimal Setup

```cpp
// 2 wait states required at 48 MHz (per datasheet: Table 37-42)
NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_RWS(2) | NVMCTRL_CTRLB_MANW;

// Remove OSC48M reset prescaler /2 → full 48 MHz
OSCCTRL->OSC48MDIV.reg = OSCCTRL_OSC48MDIV_DIV(0);
while (OSCCTRL->OSC48MSYNCBUSY.reg);

// GCLK0 = OSC48M (CPU clock)
GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_OSC48M | GCLK_GENCTRL_GENEN;
while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL_GCLK0);
```

**Order matters**: set NVM wait states before raising CPU frequency.

## GCLK Generator Assignments

| Generator | Source | Frequency | When Set | Used By |
|-----------|--------|-----------|----------|---------|
| GCLK0 | OSC48M | 48 MHz | `sys_init()` | CPU, SERCOM5 UART |
| GCLK1 | XOSC | 24 MHz | `QuartzTest::init()` if crystal OK | Available for peripherals |
| GCLK2 | XOSC / 64 | 375 kHz | `QuartzTest::init()` if crystal OK | Slow peripheral clock |
| GCLK3 | XOSC32K | 32.768 kHz | `QuartzTest::init()` if enabled + OK | RTC, WDT, slow timing |

GCLK generators 4–8 are unused at bring-up.

## Peripheral Clock Connections (PCHCTRL)

Each peripheral has a GCLK Peripheral Channel (`GCLK->PCHCTRL[ID]`).
To connect GCLK0 to SERCOM5:

```cpp
GCLK->PCHCTRL[SERCOM5_GCLK_ID_CORE].reg =
    GCLK_PCHCTRL_GEN(0) | GCLK_PCHCTRL_CHEN;
while (0 == (GCLK->PCHCTRL[SERCOM5_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN));
```

PCHCTRL IDs for common peripherals (from `samc21j18a.h`):

| Peripheral | GCLK_ID_CORE define |
|------------|---------------------|
| SERCOM0 | `SERCOM0_GCLK_ID_CORE` |
| SERCOM5 | `SERCOM5_GCLK_ID_CORE` |
| TC0/TC1 | `TC0_GCLK_ID` |
| TC2/TC3 | `TC2_GCLK_ID` |
| ADC0 | `ADC0_GCLK_ID` |
| ADC1 | `ADC1_GCLK_ID` |

## APB Bus Clocks (MCLK)

Peripherals also need their APB bus clock enabled in MCLK:

| Register | Controls |
|----------|---------|
| `MCLK->APBAMASK` | PM, MCLK, RSTC, OSCCTRL, OSC32KCTRL, SUPC, GCLK, WDT, RTC, EIC, FREQM |
| `MCLK->APBBMASK` | PORT, DSU, NVMCTRL, HMATRIXHS |
| `MCLK->APBCMASK` | SERCOM0–5, TCC0–2, TC0–4, ADC0–1, AC, DAC, SDADC, PTC, FREQM, TSENS, CAN0–1 |
| `MCLK->AHBMASK` | DMAC, DSU, NVMCTRL, HPB0–3 |

`APBAMASK` and `APBBMASK` bits are enabled by default. `APBCMASK` and `AHBMASK` must be
enabled manually before accessing most peripherals.

## OSC48M Details

- Always-on internal 48 MHz oscillator, factory-calibrated.
- Reset prescaler: `/2` (24 MHz default). Remove with `OSC48MDIV.DIV = 0`.
- No external component required.
- Accuracy: ±0.5% at 25°C after calibration (good enough for UART up to 1 Mbaud).

## Crystal Failure Detection (CFD)

XOSC CFD is enabled via `OSCCTRL->XOSCCTRL.CFDEN`. If the crystal stops oscillating,
`OSCCTRL->STATUS.XOSCFAIL` and `OSCCTRL->INTFLAG.XOSCFAIL` are set.
The System Controller IRQ (`irq_handler_system`) fires if the interrupt is enabled.

See [[QuartzTest]] for the bring-up implementation.

## See Also

- [[OSCCTRL]] — oscillator control registers (XOSC, OSC48M, DPLL)
- [[SAMC21 Datasheet Ch. 16 — GCLK]] — full GENCTRLn/PCHCTRLm register detail and mapping table
- [[SAMC21 Datasheet Ch. 17 — MCLK]] — CPUDIV, AHBMASK, APBAMASK/B/C register details
- [[QuartzTest]] — XOSC/XOSC32K startup with timeout
- [[SERCOM UART Configuration]] — how GCLK is connected to SERCOM
- [[TC 32-Bit Paired Mode]] — how TC clocks are attached
- [[Application Bring-Up Source]] — full GCLK assignment table
