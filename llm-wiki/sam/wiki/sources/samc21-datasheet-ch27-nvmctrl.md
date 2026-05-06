---
title: SAMC21 Datasheet Chapter 27 — NVMCTRL
type: source
tags: [nvmctrl, flash, nvm, eeprom, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 27 — NVMCTRL – Nonvolatile Memory Controller

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 27, pages 410–434.

## Overview

The NVM Controller manages the 256 KB Flash on the SAMC21J18A. It provides
three address spaces: the main array (program + data), the RWWEE array (Read-
While-Write EEPROM emulation), and the Calibration/Auxiliary space (factory
calibration, NVM User Row). The AHB bus is used for reads and page-buffer
writes; the APB bus is used for control commands.

## Memory Organization (SAMC21J18A)

| Parameter | Value |
|-----------|-------|
| Main array size | 256 KB |
| Page size | 64 bytes (PSZ=0x3) |
| Pages per row | 4 |
| Row size | 256 bytes |
| Total rows | 256 |
| Regions | 16 × 16 KB |
| RWWEE base | NVM base + 0x00400000 |
| Calibration base | NVM base + 0x00800000 |
| NVM User Row | 0x00804000 |

- **Erase granularity**: row (256 bytes / 4 pages)
- **Write granularity**: page (64 bytes)
- Erase must precede every write (erased = all 0xFF)

## Wait States (Critical for Clock Setup)

| CPU Frequency | Wait States |
|--------------|-------------|
| 0–24 MHz | 0 |
| 24–48 MHz | 1 |

**Wait states must be set BEFORE increasing CPU frequency** — not after.
Set in `CTRLB.RWS` field.

## Register Map

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 0x0000 | CMD[6:0] / CMDEX[15:8] (key=0xA5) |
| 0x04 | CTRLB | 0x00000080 | MANW(7)/RWS[4:1]/SLEEPPRM[9:8]/CACHEDIS(18)/READMODE[17:16] |
| 0x08 | PARAM | (device) | NVMP[15:0]/PSZ[18:16]/RWWEEP[31:20] — read-only |
| 0x0C | INTENCLR | 0x00 | ERROR(1)/READY(0) |
| 0x10 | INTENSET | 0x00 | ERROR(1)/READY(0) |
| 0x14 | INTFLAG | 0x00 | ERROR(1, w1c)/READY(0, read-only) |
| 0x18 | STATUS | (device) | SB(8)/NVME(4)/LOCKE(3)/PROGE(2)/LOAD(1)/PRM(0) |
| 0x1C | ADDR | 0x00 | ADDR[20:0] — 16-bit word address |
| 0x20 | LOCK | (NVM UR) | LOCK[15:0] — region lock bits (1=unlocked, 0=locked) |
| 0x28 | PBLDATA0 | 0xFFFFFFFF | Page buffer data bits [31:0] — read-only |
| 0x2C | PBLDATA1 | 0xFFFFFFFF | Page buffer data bits [63:32] — read-only |

## CTRLB Key Fields

| Field | Bit(s) | Reset | Description |
|-------|--------|-------|-------------|
| MANW | 7 | 1 | Manual write: 1=must issue WP cmd; 0=auto on last buffer write |
| RWS[3:0] | 4:1 | 0 | Read wait states (0=0 ws, 1=1 ws, …15=15 ws) |
| SLEEPPRM[1:0] | 9:8 | 0 | Power reduction on sleep: WAKEUPACCESS(0)/WAKEUPINSTANT(1)/DISABLED(3) |
| CACHEDIS | 18 | 0 | Cache disable: 0=cache enabled |
| READMODE[1:0] | 17:16 | 0 | NO_MISS_PENALTY(0)/LOW_POWER(1)/DETERMINISTIC(2) |

## STATUS Key Bits

| Bit | Name | Description |
|-----|------|-------------|
| 8 | SB | Security bit active (read-only) |
| 4 | NVME | NVM error (w1c) |
| 3 | LOCKE | Write to locked region attempted (w1c) |
| 2 | PROGE | Bad command or key (w1c) |
| 1 | LOAD | Page buffer has been loaded with data |
| 0 | PRM | NVM in power reduction mode |

## Command Table (CTRLA.CMD + CMDEX=0xA5)

| CMD | Name | Action |
|-----|------|--------|
| 0x02 | ER | Erase Row at ADDR |
| 0x04 | WP | Write Page buffer to ADDR |
| 0x05 | EAR | Erase Auxiliary Row (User Row only) |
| 0x06 | WAP | Write Auxiliary Page (User Row only) |
| 0x1A | RWWEEER | RWWEE Erase Row |
| 0x1C | RWWEEWP | RWWEE Write Page |
| 0x40 | LR | Lock Region at ADDR |
| 0x41 | UR | Unlock Region at ADDR |
| 0x42 | SPRM | Set Power Reduction Mode |
| 0x43 | CPRM | Clear Power Reduction Mode |
| 0x44 | PBC | Page Buffer Clear |
| 0x45 | SSB | Set Security Bit (IRREVERSIBLE — debugger chip-erase only) |
| 0x46 | INVALL | Invalidate cache |

INTFLAG.READY must be 1 before issuing any command.
PROGE is set if CMDEX ≠ 0xA5 or CMD is invalid.

## NVM User Row (Auxiliary Space, 0x00804000)

Contains BOOTPROT[2:0], EEPROM[2:0], WDT defaults, BODVDD defaults, factory
calibration, and region lock default bits. Read via AHB (direct pointer read).
Write requires EAR + WAP commands (see [[Memory Map]] for layout).

## Key Facts

- Set CTRLB.RWS=1 **before** switching CPU to frequencies above 24 MHz.
- CTRLB.MANW resets to 1 — must issue WP command after loading page buffer.
- ADDR uses 16-bit word addressing: `NVMCTRL->ADDR.reg = byte_address >> 1`.
- Page buffer write crosses a 64-bit boundary: a write at an odd 64-bit offset
  overwrites the even word in the same 64-bit section with all-ones.
  Use sequential writes from start of page to avoid boundary corruption.
- STATUS.LOAD is set when page buffer has been partially written; cleared by
  WP or PBC commands.
- SSB (Set Security Bit) is effectively irreversible in normal operation.
- LOCK register bit=1 means region is UNlocked; bit=0 means locked.
- After chip erase, all regions are unlocked (LOCK=0xFFFF).

## See Also

- [[NVMCTRL]] — firmware read/write/erase patterns
- [[Memory Map]] — NVM address ranges, User Row layout, calibration values
- [[Clock System]] — wait state requirement before OSC48M switch to 48 MHz
