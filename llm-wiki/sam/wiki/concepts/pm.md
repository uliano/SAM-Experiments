---
title: PM
type: concept
tags: [pm, power, sleep, standby, firmware, samc21]
sources: [samc21-datasheet-ch19-pm]
created: 2026-05-05
updated: 2026-05-05
---

# PM

Power Manager on the SAMC21. Selects between IDLE and STANDBY sleep modes
and controls RAM back-bias and regulator behavior in standby.

## Sleep Mode Selection

| SLEEPMODE | Name | What stays running |
|-----------|------|--------------------|
| 0x2 | IDLE | AHB/APB gated on demand; GCLK0/MCLK run; RAM normal; main VREG |
| 0x4 | STANDBY | AHB/APB stopped; GCLK stopped (unless RUNSTDBY); RAM back-biased; LP VREG |

All other SLEEPMODE values (0x0, 0x1, 0x3, 0x5–0x7) are reserved.

## Entering Sleep

```cpp
// Write sleep mode
PM->SLEEPCFG.reg = PM_SLEEPCFG_SLEEPMODE_STANDBY;
// Read back and verify — mandatory, register is write-synchronized
while (PM->SLEEPCFG.reg != PM_SLEEPCFG_SLEEPMODE_STANDBY);
// Enter sleep
__WFI();
// Execution resumes here after wake interrupt
```

The read-back loop is not optional. Executing WFI before the PM has latched
the new mode can enter the wrong sleep level.

## IDLE Mode

Use IDLE when peripherals must keep processing (e.g., DMAC transfer in
progress, SERCOM transmitting) while the CPU waits.

- AHB/APB clock gating: hardware only clocks peripherals with pending requests.
- All GCLK generators remain running.
- Wake latency is minimal (a few CPU cycles).
- Current savings: CPU + idle bus, not full clock tree.

## STANDBY Mode

Use STANDBY for deepest sleep between infrequent events (RTC alarm, EIC
edge-detect, WDT early warning).

- CPU and most clocks stopped.
- Peripherals with `RUNSTDBY=1` in their CTRLA remain active and can issue
  wake interrupts (RTC, WDT, EIC, SERCOM with async wakeup, TC with RUNSTDBY).
- RAM is back-biased (lower power, slightly degraded retention).
- After wake, GCLK generators restart; wait for oscillators to lock if needed.

## STDBYCFG

```cpp
PM->STDBYCFG.reg = PM_STDBYCFG_VREGSMOD_LP;   // LP regulator in standby
// BBIASHS defaults to 1 (back-bias enabled) — acceptable for most designs
```

| Bits | Name | Options |
|------|------|---------|
| 10 | BBIASHS | 1=back-bias enabled (reset default); 0=normal bias |
| 7:6 | VREGSMOD[1:0] | AUTO(0)/PERFORMANCE(1)/LP(2) |

## Wake Sources from STANDBY

- RTC: alarm or periodic event (needs `RUNSTDBY=1`)
- EIC: edge-detect on configured pin
- WDT: early warning interrupt
- SERCOM: async wakeup on activity (USART start-of-frame, I2C address match)
- Any peripheral with `RUNSTDBY=1` and an enabled interrupt

## Key Facts

- Read SLEEPCFG back after writing — write-synchronized register.
- SLEEPMODE=0x2 (IDLE) and 0x4 (STANDBY) are the only valid choices.
- Back-bias (BBIASHS=1) is the reset default and is appropriate for most designs.
- In STANDBY, GCLK generators do NOT restart automatically for peripherals
  without RUNSTDBY; re-lock oscillators on wake if they were stopped.

## See Also

- [[SAMC21 Datasheet Ch.19 PM]] — register reference
- [[SUPC]] — voltage regulator modes
- [[Clock System]] — GCLK RUNSTDBY, generator control
- [[WDT]] — RUNSTDBY watchdog in standby
