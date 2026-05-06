---
title: PAC Configuration
type: concept
tags: [pac, peripheral-access-controller, write-protection, firmware, samc21]
sources: [samc21-datasheet-ch11-pac]
created: 2026-05-05
updated: 2026-05-05
---

# PAC Configuration

Peripheral Access Controller on the SAM C20/C21. Provides optional register
write protection for each peripheral, detectable by interrupt or event.

## WRCTRL — the only register you normally write

```cpp
// PERID formula: 32 × BridgeNumber + N
// BridgeNumber: A=0, B=1, C=2, D=3
// N: peripheral index from Ch.12 "PAC, Index" column

// Unprotect a peripheral (KEY=CLEAR=0x1)
PAC->WRCTRL.reg = PAC_WRCTRL_KEY_CLEAR | PAC_WRCTRL_PERID(perid);

// Re-protect (KEY=SET=0x2)
PAC->WRCTRL.reg = PAC_WRCTRL_KEY_SET | PAC_WRCTRL_PERID(perid);

// Lock permanently until next hardware reset (KEY=LOCK=0x3)
PAC->WRCTRL.reg = PAC_WRCTRL_KEY_LOCK | PAC_WRCTRL_PERID(perid);
```

## PERID Quick Reference (most commonly used peripherals)

| Bridge | Peripheral | BridgeNum | N | PERID |
|--------|-----------|-----------|---|-------|
| A | PAC | 0 | 0 | 0 |
| A | PM | 0 | 1 | 1 |
| A | MCLK | 0 | 2 | 2 |
| A | RSTC | 0 | 3 | 3 |
| A | OSCCTRL | 0 | 4 | 4 |
| A | OSC32KCTRL | 0 | 5 | 5 |
| A | SUPC | 0 | 6 | 6 |
| A | GCLK | 0 | 7 | 7 |
| A | WDT | 0 | 8 | 8 |
| A | RTC | 0 | 9 | 9 |
| A | EIC | 0 | 10 | 10 |
| A | FREQM | 0 | 11 | 11 |
| A | TSENS | 0 | 12 | 12 |
| B | PORT | 1 | 0 | 32 |
| B | DSU | 1 | 1 | 33 |
| B | NVMCTRL | 1 | 2 | 34 |
| B | DMAC | 1 | 3 | 35 |
| B | MTB | 1 | 4 | 36 |
| C | EVSYS | 2 | 0 | 64 |
| C | SERCOM0 | 2 | 1 | 65 |
| C | SERCOM1 | 2 | 2 | 66 |
| C | SERCOM2 | 2 | 3 | 67 |
| C | SERCOM3 | 2 | 4 | 68 |
| C | SERCOM4 | 2 | 5 | 69 |
| C | SERCOM5 | 2 | 6 | 70 |
| C | TCC0 | 2 | 9 | 73 |
| C | TCC1 | 2 | 10 | 74 |
| C | TCC2 | 2 | 11 | 75 |
| C | TC0 | 2 | 12 | 76 |
| C | TC1 | 2 | 13 | 77 |
| C | TC2 | 2 | 14 | 78 |
| C | TC3 | 2 | 15 | 79 |
| C | TC4 | 2 | 16 | 80 |
| C | ADC0 | 2 | 17 | 81 |
| C | ADC1 | 2 | 18 | 82 |
| C | SDADC | 2 | 19 | 83 |
| C | AC | 2 | 20 | 84 |
| C | DAC | 2 | 21 | 85 |
| C | PTC | 2 | 22 | 86 |
| C | CCL | 2 | 23 | 87 |
| C | CAN0 | 2 | 7 | 71 |
| C | CAN1 | 2 | 8 | 72 |
| D | SERCOM6 | 3 | 0 | 96 |
| D | SERCOM7 | 3 | 1 | 97 |
| D | TC5 | 3 | 2 | 98 |
| D | TC6 | 3 | 3 | 99 |
| D | TC7 | 3 | 4 | 100 |

*Verify N values against the Ch.12 "PAC, Index" column for the exact variant.*

## Typical Unprotect / Write / Re-Protect Sequence

Many peripherals are write-protected after a hardware reset. When configuring
clock-related peripherals (OSCCTRL, GCLK, etc.), you must unprotect first:

```cpp
// Example: configure GCLK (PERID = 7, Bridge A)
PAC->WRCTRL.reg = PAC_WRCTRL_KEY_CLEAR | PAC_WRCTRL_PERID(7);
// ... configure GCLK registers ...
PAC->WRCTRL.reg = PAC_WRCTRL_KEY_SET | PAC_WRCTRL_PERID(7);
```

In practice, many drivers skip the re-protect step during bring-up. The LOCK
key is useful for production code where no further changes should be allowed.

## Key Facts

- PAC write-protection state is not reset by user SWRST — only hardware reset clears it.
- At reset, all peripherals listed in STATUSA–D are **not** write-protected (STATUS bits = 0).
  Write protection is only active after firmware explicitly sets it with KEY=SET.
- Exception: PAC itself (PERID=0) — PAC registers INTENCLR, INTENSET, and EVCTRL are PAC Write-Protected by default. WRCTRL is not.
- Each peripheral's individual registers may also carry "PAC Write-Protection" independently — the register description lists "Property: PAC Write-Protection" for each affected register.
- EVCTRL.ERREO=1: generates a system event on any PAC error. Useful for production error monitoring.
- INTENCLR/SET.ERR=1: enables interrupt on PAC error (shares NVIC line 0 with PM, MCLK, OSC32KCTRL, SUPC, OSCCTRL).

## See Also

- [[SAMC21 Datasheet Ch.11 PAC]] — full register reference
- [[SAMC21 Datasheet Ch.12 Peripherals Configuration Summary]] — PAC Index column for exact PERID values
- [[NVIC Interrupt Map]] — PAC error on NVIC line 0 (shared)
