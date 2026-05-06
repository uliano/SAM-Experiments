---
title: SAMC21 Datasheet Chapter 21 — OSC32KCTRL
type: source
tags: [osc32kctrl, oscillator, clock, xosc32k, osc32k, osculp32k, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 21 — OSC32KCTRL

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 21, pages 228–248.

## Overview

OSC32KCTRL manages all 32 kHz oscillators: XOSC32K (external crystal), OSC32K (internal RC), and OSCULP32K (ultra-low-power, always-on). Provides RTC clock selection and Clock Failure Detection for XOSC32K.

Base address: 0x40001400 (APB-A, always enabled in reset).

## Register Summary

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | INTENCLR | 0x00 | Interrupt Enable Clear |
| 0x04 | INTENSET | 0x00 | Interrupt Enable Set |
| 0x08 | INTFLAG | 0x00 | Interrupt Flags (XOSC32KRDY, OSC32KRDY, CLKFAIL, CLKSW) |
| 0x0C | STATUS | 0x00 | Status (same bits as INTFLAG, read-only) |
| 0x10 | RTCCTRL | 0x00 | RTC clock selection: RTCSEL[2:0] (6 options from OSCULP to XOSC32K) |
| 0x14 | XOSC32K | 0x0080 | ENABLE, XTALEN, EN32K, EN1K, STARTUP[2:0] (0x0=62ms…0x7=8s), ONDEMAND, WRTLOCK |
| 0x16 | CFDCTRL | 0x00 | CFDEN, SWBACK, CFDPRESC (0=OSCULP32K, 1=OSCULP32K/2) |
| 0x17 | EVCTRL | 0x00 | CFDEO event output enable |
| 0x18 | OSC32K | 0x00000080 | ENABLE, EN32K, EN1K, STARTUP[2:0] (0x0=0.09ms…0x7=4ms), CALIB[6:0], ONDEMAND, WRTLOCK |
| 0x1C | OSCULP32K | — | CALIB[4:0] (auto-loaded), WRTLOCK; always-on, no ENABLE bit |

## Key Facts

- XOSC32K startup range: 0.0625 s (0x0) to 8 s (0x7) — much longer than XOSC due to 32 kHz crystal physics.
- OSC32K CALIB[6:0] must be loaded from NVM SW Cal area at 0x806020 bits 18:12.
- OSCULP32K CALIB[4:0] is auto-loaded from factory flash — firmware does nothing.
- EN32K must be set in XOSC32K to drive any GCLK generator. EN1K is only for RTC 1 kHz path.
- CFD safe clock is OSCULP32K (or /2), not OSC48M — unlike OSCCTRL's CFD which uses OSC48M.

## See Also

- [[OSC32KCTRL]] — concept page with full register field tables and code examples
- [[Clock System]] — how XOSC32K is assigned to GCLK3 in this bring-up
- [[OSCCTRL]] — chapter 20, companion oscillator controller for XOSC/OSC48M/DPLL
- [[Memory Map]] — NVM SW Cal layout; OSC32K CALIB at word 0 bits 18:12
