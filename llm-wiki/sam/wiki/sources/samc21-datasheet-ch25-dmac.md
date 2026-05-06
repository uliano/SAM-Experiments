---
title: SAMC21 Datasheet Chapter 25 — DMAC
type: source
tags: [dma, dmac, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 25 — DMAC – Direct Memory Access Controller

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 25, pages 328–384.

## Overview

12-channel DMA controller with CRC engine. Transfers data between memory and peripherals with minimal CPU involvement. Uses SRAM-resident descriptor arrays. Supports beat, block, and transaction trigger modes; linked descriptors for circular/chained transfers; 4 priority levels with static or round-robin arbitration.

## Key Terms

- **Beat**: one bus access (8/16/32-bit). Size selected by BTCTRL.BEATSIZE.
- **Block**: a sequence of beats described by one descriptor (up to 65535 beats).
- **Transaction**: a chain of blocks linked via DESCADDR.
- **Descriptor section** (BASEADDR): SRAM array holding first descriptors for all channels, 16 bytes per channel.
- **Write-back section** (WRBADDR): SRAM array where DMAC stores ongoing descriptor state; same layout as descriptor section.

## Global Register Map (base 0x41006000)

| Offset | Name | Description |
|--------|------|-------------|
| 0x00 | CTRL | SWRST / DMAENABLE / CRCENABLE / LVLENx[3:0] — enable-protected |
| 0x02 | CRCCTRL | CRCSRC[5:0] / CRCPOLY[1:0] / CRCBEATSIZE[1:0] — enable-protected |
| 0x04 | CRCDATAIN | 32-bit CRC data input |
| 0x08 | CRCCHKSUM | 32-bit CRC result |
| 0x0C | CRCSTATUS | CRCZERO / CRCBUSY |
| 0x0D | DBGCTRL | DBGRUN — DMA continues when CPU halted by debugger |
| 0x0E | QOSCTRL | DQOS / FQOS / WRBQOS (data/fetch/writeback bus QoS) |
| 0x10 | SWTRIGCTRL | SWTRIGn[11:0] — software trigger per channel |
| 0x14 | PRICTRL0 | RRLVLENx / LVLPRIx — round-robin enable + last-served channel |
| 0x20 | INTPEND | PEND/BUSY/FERR/SUSP/TCMPL/TERR + ID[3:0] (lowest pending) |
| 0x24 | INTSTATUS | CHINTn[11:0] — channels with pending interrupts |
| 0x28 | BUSYCH | BUSYCHn[11:0] — channels currently transferring |
| 0x2C | PENDCH | PENDCHn[11:0] — channels with pending transfer |
| 0x30 | ACTIVE | BTCNT / ABUSY / ID[4:0] / LVLEXx[3:0] — active channel state |
| 0x34 | BASEADDR | Descriptor memory base — 128-bit aligned, enable-protected |
| 0x38 | WRBADDR | Write-back memory base — 128-bit aligned, enable-protected |

## Per-Channel Registers (select channel with CHID first)

| Offset | Name | Description |
|--------|------|-------------|
| 0x3F | CHID | ID[3:0] — select channel for all CH* registers |
| 0x40 | CHCTRLA | RUNSTDBY / ENABLE / SWRST |
| 0x44 | CHCTRLB | CMD[1:0] / TRIGACT[1:0] / TRIGSRC[5:0] / LVL[1:0] / EVOE / EVIE / EVACT[2:0] |
| 0x4C | CHINTENCLR | SUSP / TCMPL / TERR (write-1-to-clear enable) |
| 0x4D | CHINTENSET | SUSP / TCMPL / TERR (write-1-to-set enable) |
| 0x4E | CHINTFLAG | SUSP / TCMPL / TERR (write-1-to-clear flag) |
| 0x4F | CHSTATUS | FERR / BUSY / PEND (read-only) |

## CHCTRLB Key Fields

### TRIGACT
| Value | Name | Description |
|-------|------|-------------|
| 0x0 | BLOCK | One trigger per block transfer |
| 0x2 | BEAT | One trigger per beat — use for peripheral streaming (UART, ADC) |
| 0x3 | TRANSACTION | One trigger per transaction (chain of blocks) |

### TRIGSRC (Table 25-2, selected entries)
| Value | Name | Description |
|-------|------|-------------|
| 0x00 | DISABLE | Software / event triggers only |
| 0x02 | SERCOM0 RX | SERCOM0 RXC |
| 0x03 | SERCOM0 TX | SERCOM0 DRE |
| 0x04–0x0D | SERCOM1–5 RX/TX | Same pattern, +2 per SERCOM |
| 0x0C | SERCOM5 RX | SERCOM5 RXC trigger |
| 0x0D | SERCOM5 TX | SERCOM5 DRE trigger |
| 0x1B | TC0 OVF | TC0 overflow |
| 0x2A | ADC0 RESRDY | ADC0 result ready |
| 0x2B | ADC1 RESRDY | ADC1 result ready |
| 0x2D | DAC EMPTY | DAC empty |

### CMD
| Value | Description |
|-------|-------------|
| 0x0 | NOACT |
| 0x1 | SUSPEND channel |
| 0x2 | RESUME channel |

### LVL
0=LVL0 (lowest) … 3=LVL3 (highest priority)

## Transfer Descriptor — SRAM Layout

Offset relative to `BASEADDR + channel_number × 0x10` (and same in WRBADDR):

| Offset | Name | Description |
|--------|------|-------------|
| 0x00 | BTCTRL | 16-bit: STEPSIZE/STEPSEL/DSTINC/SRCINC/BEATSIZE/BLOCKACT/EVOSEL/VALID |
| 0x02 | BTCNT | 16-bit beat count (decremented by DMAC during transfer) |
| 0x04 | SRCADDR | 32-bit source address (END of source when SRCINC=1) |
| 0x08 | DSTADDR | 32-bit destination address (END of dest when DSTINC=1) |
| 0x0C | DESCADDR | 32-bit next descriptor (0 = end transaction; self = circular) |

## BTCTRL Bit Fields

| Bits | Name | Description |
|------|------|-------------|
| 15:13 | STEPSIZE[2:0] | Address step: X1/X2/X4/X8/X16/X32/X64/X128 × beat size |
| 12 | STEPSEL | 0=step applies to DSTADDR, 1=step applies to SRCADDR |
| 11 | DSTINC | Destination address increment enable |
| 10 | SRCINC | Source address increment enable |
| 9:8 | BEATSIZE[1:0] | 0=BYTE(8), 1=HWORD(16), 2=WORD(32) |
| 4:3 | BLOCKACT[1:0] | After block: 0=NOACT(disable), 1=INT(disable+irq), 2=SUSPEND, 3=BOTH |
| 2:1 | EVOSEL[1:0] | Event output: 0=disabled, 1=on block complete, 3=on beat complete |
| 0 | VALID | 1 = descriptor is valid; DMAC suspends if it fetches VALID=0 |

## Address End-Pointer Convention

When SRCINC or DSTINC is set, SRCADDR/DSTADDR must be written with the address **past the end** of the buffer (not the start):

```
SRCADDR = buffer_start + BTCNT × (BEATSIZE+1)
DSTADDR = buffer_start + BTCNT × (BEATSIZE+1)
```

For a fixed peripheral register (e.g., `SERCOM->DATA`), set INC=0 and write the register address directly.

## Memory Section Size

`Size = 128 bits × (highest_enabled_channel + 1)` = `16 bytes × (m + 1)`

Both sections (descriptor + write-back) must be this size. Using lowest channel numbers minimises SRAM usage.

## Interrupt Sources (per channel)

| Flag | Bit | Condition |
|------|-----|-----------|
| TERR | 0 | Bus error or invalid descriptor fetched |
| TCMPL | 1 | Block transfer complete (when BLOCKACT enables interrupt) |
| SUSP | 2 | Channel suspended |

Global ISR reads INTSTATUS to find which channels have pending interrupts, then reads INTPEND.ID for the lowest-priority pending channel.

## Key Facts

- CHID must be written before any CH* register access — no atomicity guarantee.
- BASEADDR and WRBADDR must be 128-bit (16-byte) aligned.
- CTRL.SWRST requires both DMAENABLE=0 and CRCENABLE=0.
- TRIGACT_BEAT: one trigger per byte — correct for streaming peripherals (UART/ADC).
- Disabling a running channel: ENABLE=0 is deferred until the current burst ends and WRBADDR is updated.
- Channel can run in standby if CHCTRLA.RUNSTDBY=1.
- CRC-32 result is bit-reversed and complemented when read from CRCCHKSUM (IEEE 802.3 format).

## See Also

- [[DMA Controller]] — firmware usage, init sequence, address gotcha, RX/TX channel patterns
- [[SERCOM UART Configuration]] — SERCOM5 DRE/RXC trigger IDs (0x0C/0x0D)
- [[NVIC Interrupt Map]] — DMAC IRQn
- [[Clock System]] — MCLK_AHBMASK_DMAC must be set before DMAC access
