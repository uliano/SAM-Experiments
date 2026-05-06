---
title: EIC
type: concept
tags: [eic, extint, interrupt, wakeup, firmware, samc21]
sources: [samc21-datasheet-ch26-eic]
created: 2026-05-05
updated: 2026-05-05
---

# EIC

External Interrupt Controller on the SAMC21. Configures external pins as
interrupt or event sources, with optional filtering and asynchronous wakeup
from STANDBY.

## Clock Requirements

| Mode | Clocks needed |
|------|--------------|
| Level detection, no filter | CLK_EIC_APB only |
| Async edge detection | CLK_EIC_APB only |
| Sync edge detection | CLK_EIC_APB + GCLK_EIC or CLK_ULP32K |
| Edge with filter | CLK_EIC_APB + GCLK_EIC or CLK_ULP32K |

CTRLA.CKSEL selects between GCLK_EIC (0, default) and CLK_ULP32K (1, lower power).

## Typical Init — Edge Interrupt on Pin

```cpp
// 1. APB clock
MCLK->APBBMASK.reg |= MCLK_APBBMASK_EIC;

// 2. Enable GCLK for EIC (needed for sync edge detection)
GCLK->PCHCTRL[EIC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
while (!(GCLK->PCHCTRL[EIC_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

// 3. Configure pin as input with EIC function
// PORT: PINCFG.INEN=1, PINCFG.PMUXEN=1, PMUX=PMUX_PMUXE_A (function A)

// 4. Sense configuration for EXTINT0 (enable-protected)
EIC->CONFIG[0].reg = EIC_CONFIG_SENSE0_RISE;  // rising edge, no filter

// 5. Enable interrupt for channel 0
EIC->INTENSET.reg = EIC_INTENSET_EXTINT(1 << 0);
NVIC_EnableIRQ(EIC_EXTINT_0_IRQn);

// 6. Enable EIC
EIC->CTRLA.reg = EIC_CTRLA_ENABLE;
while (EIC->SYNCBUSY.reg & EIC_SYNCBUSY_ENABLE);
```

## Async Wakeup from STANDBY

For wakeup with all clocks stopped (no GCLK running):

```cpp
// Configure channel 7 for async falling-edge wakeup
EIC->CONFIG[0].reg = (EIC->CONFIG[0].reg & ~EIC_CONFIG_SENSE7_Msk)
                   | EIC_CONFIG_SENSE7_FALL;
EIC->ASYNCH.reg |= EIC_ASYNCH_ASYNCH(1 << 7);  // async (enable-protected)
EIC->INTENSET.reg = EIC_INTENSET_EXTINT(1 << 7);
NVIC_EnableIRQ(EIC_EXTINT_7_IRQn);
EIC->CTRLA.reg = EIC_CTRLA_ENABLE;
while (EIC->SYNCBUSY.reg & EIC_SYNCBUSY_ENABLE);
// Now enter standby — EIC will wake CPU on falling edge without GCLK
```

**FILTENx must be 0 when ASYNCH[x]=1** — filter requires a clock.

## ISR Pattern

```cpp
void EIC_EXTINT_0_Handler(void) {
    if (EIC->INTFLAG.reg & EIC_INTFLAG_EXTINT(1 << 0)) {
        // handle
        EIC->INTFLAG.reg = EIC_INTFLAG_EXTINT(1 << 0);  // write-1-to-clear
    }
}
```

## SENSE Values

| Value | Name | Trigger |
|-------|------|---------|
| 0x0 | NONE | Disabled |
| 0x1 | RISE | Rising edge |
| 0x2 | FALL | Falling edge |
| 0x3 | BOTH | Both edges |
| 0x4 | HIGH | High level |
| 0x5 | LOW | Low level |

## Register Reference

| Offset | Name | Key bits |
|--------|------|----------|
| 0x00 | CTRLA | CKSEL(4, EP)/ENABLE(1, WS)/SWRST(0, WS) |
| 0x01 | NMICTRL | NMIASYNCH(4)/NMIFILTEN(3)/NMISENSE[2:0] |
| 0x02 | NMIFLAG | NMI(0) — w1c |
| 0x04 | SYNCBUSY | ENABLE(1)/SWRST(0) |
| 0x08 | EVCTRL | EXTINTEO[15:0] — event out enable (EP) |
| 0x0C | INTENCLR | EXTINT[15:0] |
| 0x10 | INTENSET | EXTINT[15:0] |
| 0x14 | INTFLAG | EXTINT[15:0] — w1c |
| 0x18 | ASYNCH | ASYNCH[15:0] (EP) |
| 0x1C | CONFIG0 | channels 0–7: FILTENx/SENSEx[2:0] per channel (EP) |
| 0x20 | CONFIG1 | channels 8–15 (EP) |

EP = Enable-Protected, WS = Write-Synchronized, w1c = write-1-to-clear

## Key Facts

- SAMC21J18A is not an "N" variant — DEBOUNCEN/DPRESCALER/PINSTATE not available.
- Enable-protected registers (EVCTRL, CONFIGn, ASYNCH, CTRLA.CKSEL) must be
  written before CTRLA.ENABLE=1 or after disabling.
- NMI is always enabled once NMISENSE ≠ NONE; does not need INTENSET.
- Each EXTINTx has its own NVIC IRQ line (EIC_EXTINT_0 through EIC_EXTINT_15).

## See Also

- [[SAMC21 Datasheet Ch.26 EIC]] — full register reference
- [[I/O Multiplexing]] — EXTINT pin assignments (function A)
- [[PORT]] — pin input enable (PINCFG.INEN)
- [[PM]] — standby wakeup via async mode
- [[NVIC Interrupt Map]] — EIC IRQn values
