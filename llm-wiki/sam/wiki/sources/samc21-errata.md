---
title: SAMC20/C21 Errata (DS80000740S)
type: source
tags: [errata, silicon, samc21, samc20, workaround]
sources: [samc21-errata]
created: 2026-05-06
updated: 2026-05-18
---

# SAMC20/C21 Errata (DS80000740S)

Summary of `docs/SAM-C20-C21-Family-Silicon-Errata-and-Data-Sheet-Clarification-DS80000740.pdf`,
revision **DS80000740S**, April 2026.

Target hardware is ATSAMC21J18A-AU, E/G/J family table. The observed
`DSU.DID = 0x11010500` maps to `REVISION[3:0] = 0x5`, **Rev F**.

The previous wiki conclusion that "all errata are annulled on Rev F" was based
on an older imported document and is no longer correct. DS80000740S lists
multiple Rev-F-active errata for E/G/J devices.

## Immediate Rules For This Project

| Area | Current rule |
|---|---|
| AC output | Use `OUT_ASYNC` for the heartbeat/DFF path. `OUT_SYNC` was measured to add up to two `GCLK_AC` cycles of lag. |
| AC clock | Use the real AC generic clock channel, `AC_GCLK_ID = 40`, before using AC. The old `PCHCTRL[34]`/ADC1-clock workaround is not a Rev-F rule. |
| CCL init | Keep `CCL->CTRL.ENABLE=0` while writing all `SEQCTRLx` and `LUTCTRLn` values; include `LUTCTRLn.ENABLE` in those writes; enable `CCL->CTRL` last. |
| CCL reset | Avoid `CCL->CTRL.SWRST` in production paths because errata 1.7.4 reports a PAC protection error. |
| EVSYS init | Follow datasheet §29.6.2.2: configure `USERm` first, then `CHANNELn`. |
| EVSYS async | For async channels, do not set `EDGSEL`; async channels do not provide EVD/OVR interrupt flags. |
| TCC via EVSYS | Use `PATH_ASYNCHRONOUS` for TCC event users. Errata 1.21.9 says TCC is not compatible with SYNC/RESYNC EVSYS modes. |
| ADC events | Use async EVSYS path for ADC events. Errata 1.4.4 affects synchronized ADC events on Rev F. |

## Rev-F-Active Errata Relevant To Expansion

This table is the practical subset to re-check before extending the dual-channel
design. It is not a replacement for reading the PDF when using a new peripheral.

| # | Peripheral | Issue | Design impact |
|---|---|---|---|
| 1.4.4 | ADC | Synchronized Event | Use asynchronous EVSYS path for ADC event starts. |
| 1.4.5 | ADC | Software Trigger Sync Busy Status | After standby, do not block forever on `SYNCBUSY.SWTRIG`. |
| 1.4.6 | ADC | Reference Buffer Offset Compensation | Re-check analog calibration/reference assumptions before precision ADC work. |
| 1.4.9 | ADC | Sequence State | Re-check sequence-scan use before relying on it. |
| 1.4.10 | ADC | Syncbusy Enable | Enable ADC0 before ADC1 or do not rely on ADC0 `SYNCBUSY.ENABLE` in the affected mode. |
| 1.5.3 | AC | Analog Pins shared with PTC | Relevant only if PTC is used with AC-shared pins. |
| 1.5.6 | AC | Spurious COMP interrupt with `MUXNEG=INTREF` when enabling | Prefer `VSCALE` when possible, or clear/ignore the initial COMP interrupt after enabling with `INTREF`. |
| 1.7.2 | CCL | Sequential Logic | If disabling/re-enabling LUT0/LUT2 to clear sequential logic, write `CTRL.ENABLE` again after LUT enable. |
| 1.7.3 | CCL | Enable Protected Registers | `SEQCTRLx`/`LUTCTRLx` are effectively protected by global `CTRL.ENABLE`; configure them while CCL is globally disabled. |
| 1.7.4 | CCL | PAC Protection Error | Do not use CCL software reset as a normal init path. |
| 1.12.1 | EVSYS | Generic Clock | For synchronous EVSYS channels, set `CHANNEL.ONDEMAND=1` to avoid spurious overrun interrupts. |
| 1.12.3 | EVSYS | Software Event | Resynchronized software events need a delay before issuing another software event. |
| 1.12.4 | EVSYS | Event Channel Configuration | After configuring/enabling a channel, wait at least one `GCLK_EVSYS_CHANNELx` tick before the first trigger when that clock is relevant. |
| 1.20.3 | TC | SYNCBUSY Flag | Re-check buffered `PERBUF`/`CCBUFx` update logic if used. |
| 1.21.6 | TCC | SYNCBUSY | Re-check buffered status clearing/update logic if used. |
| 1.21.9 | TCC | TCC with EVSYS in SYNC/RESYNC Mode | Use EVSYS asynchronous path for TCC event input/output chains. |
| 1.21.10 | TCC | ALOCK Feature | Do not rely on ALOCK. |
| 1.21.11 | TCC | OVF | Re-check overflow handling if depending on the affected mode. |

## Older Notes That Are Not Current Rev-F Constraints

| Old note | Current status |
|---|---|
| "All documented errata are annulled on Rev F" | False for DS80000740S. |
| AC `GCLK_AC` non-functional, use ADC1 clock instead | Old early-silicon errata. Target Rev F uses `AC_GCLK_ID = 40`. |
| CCL TC input selection reversed | Old early-silicon errata. Use current datasheet mapping on target Rev F. |
| CCL `INSEL=TCC` reserved/unknown | Current datasheet and local tests support TCC input. For SAMC21J18A: LUT0=TCC0, LUT1=TCC1, LUT2=TCC2, LUT3=TCC0; slot selects WO[0]/WO[1]/WO[2]. |
| TC2+TC3 COUNT32 categorically prohibited by errata | Not a current DS80000740S Rev-F rule as written in older notes; still keep TC2+TC3 in board qualification because the design depends on it. |

## Other Rev-F-Active Areas To Check Before Use

The DS80000740S summary also identifies these Rev-F-active E/G/J issue groups
outside the current measurement path. Treat a new peripheral block as needing a
fresh PDF pass before design freeze.

| Module | Rev-F-active issue IDs to check |
|---|---|
| OSC48M | 1.2.2, 1.2.3 |
| FDPLL | 1.3.3, 1.3.4 |
| ADC | 1.4.4, 1.4.5, 1.4.6, 1.4.9, 1.4.10 |
| AC | 1.5.3, 1.5.6 |
| CAN | 1.6.13, 1.6.14, 1.6.15, 1.6.16, 1.6.17, 1.6.18 |
| CCL | 1.7.2, 1.7.3, 1.7.4 |
| Device | 1.8.7, 1.8.9, 1.8.10, 1.8.13, 1.8.14, 1.8.15 |
| DAC | 1.9.2 |
| DMAC | 1.10.4 |
| EIC | 1.11.6 |
| EVSYS | 1.12.1, 1.12.3, 1.12.4 |
| PORT | 1.13.2, 1.13.3, and verify 1.13.4 before using PORT user events on PA28 |
| RTC | 1.16.3 |
| SERCOM | 1.17.3 through 1.17.22, depending on USART/SPI/I2C mode |
| TSENS | 1.19.1 |
| TC | 1.20.3 |
| TCC | 1.21.5, 1.21.6, 1.21.7, 1.21.8, 1.21.9, 1.21.10, 1.21.11 |
| MCLK | 1.23.1 |
| FREQM | 1.24.1 |
| OSCCTRL | 1.25.1, 1.25.2 |

## Concepts Mentioned

- [[AC Configuration]]
- [[ADC Configuration]]
- [[CCL Configuration]]
- [[EVSYS]]
- [[TCC Configuration]]
- [[TC 32-Bit Paired Mode]]
- [[Clock System]]

## See Also

- [[SAMC21 Datasheet Ch.29 EVSYS]]
- [[SAMC21 Datasheet Ch.37 CCL]]
- [[SAMC21 Datasheet Ch.40 AC]]
- [[AC Configuration]]
- [[EVSYS]]
- [[CCL Configuration]]
