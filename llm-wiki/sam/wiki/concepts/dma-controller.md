---
title: DMA Controller
type: concept
tags: [dma, dmac, firmware, samc21]
sources: [firmware-core-library, samc21-datasheet-ch25-dmac]
created: 2026-05-05
updated: 2026-05-05
---

# DMA Controller

The SAMC21J18A has a 12-channel DMAC (base 0x41006000) with descriptor-based transfers, 4 priority levels, and a CRC engine. Descriptor arrays must reside in SRAM at 16-byte-aligned addresses.

This page covers firmware usage and register-level details. Source: [[SAMC21 Datasheet Ch.25 DMAC]].

## DMAC Initialization (dmac_init_once)

```cpp
MCLK->AHBMASK.reg |= MCLK_AHBMASK_DMAC;

DMAC->CTRL.reg = DMAC_CTRL_SWRST;
while (DMAC->CTRL.reg & DMAC_CTRL_SWRST);

DMAC->BASEADDR.reg = (uint32_t)&dma_descriptor_[0];
DMAC->WRBADDR.reg  = (uint32_t)&dma_writeback_[0];
DMAC->CTRL.reg     = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN0;

NVIC_ClearPendingIRQ(DMAC_IRQn);
NVIC_EnableIRQ(DMAC_IRQn);
```

Descriptor and writeback arrays must be **16-byte aligned** (`alignas(16)`) and allocated
for `DMAC_CH_NUM` channels (12 on SAMC21J18A).

## Descriptor Fields (DmacDescriptor)

| Field | Purpose |
|-------|---------|
| `BTCTRL` | Beat size, source/dest increment, valid bit |
| `BTCNT` | Number of beats (bytes for BEATSIZE_BYTE) |
| `SRCADDR` | Source address (**end** of source array, not start, when SRCINC=1) |
| `DSTADDR` | Destination address (**end** of dest array when DSTINC=1) |
| `DESCADDR` | Next descriptor (0 = stop, self = circular) |

**Address is end+1, not start** when increment is enabled — a common gotcha:

```cpp
// For src array of N bytes starting at buf:
descriptor.SRCADDR.reg = (uint32_t)(buf + N);  // points past end
descriptor.BTCNT.reg   = N;
```

## Channel Setup (dmac_init_channel)

```cpp
DMAC->CHID.reg = DMAC_CHID_ID(channel);        // select channel
DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;        // reset channel
while (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_SWRST);

DMAC->CHCTRLB.reg =
    DMAC_CHCTRLB_LVL(0)              |          // priority level 0
    DMAC_CHCTRLB_TRIGSRC(trigsrc)    |          // e.g. SERCOM5_DMAC_ID_RX
    DMAC_CHCTRLB_TRIGACT_BEAT;                   // one beat per trigger
```

`TRIGACT_BEAT`: one beat (byte) transferred per trigger. The SERCOM DATA register fires a
trigger each time a byte arrives (RX) or the data register is empty (TX).

## RX Channel (channel 0)

Circular descriptor: `DESCADDR` points to itself. BTCNT = 128. Source = `SERCOM->DATA`.
Destination = `rx_dma_buffer_[128]` (fixed buffer, no address increment on source).

When the 128-byte block fills, DMAC fires `TCMPL` interrupt → `rx_dma_block_count_++`.
The DMA immediately restarts from the beginning of `rx_dma_buffer_`.

Polling current RX position: briefly suspend the channel, read `writeback.BTCNT` (remaining
beats), resume. See `snapshot_rx_position()` in [[Uart DMA]].

## TX Channel (channel 1)

Not circular: `DESCADDR = 0` (stop after transfer). Source = `tx_dma_chunk_[count]`.
Destination = `SERCOM->DATA`. Each transfer sends up to 64 bytes.

On `TCMPL`: `dma_irq_handler()` checks if more bytes remain in `tx_` and starts a new
transfer if so.

## Channel Selection (CHID)

The SAMC21 DMAC uses a single-register channel select model — write the channel number to
`DMAC->CHID` before accessing `CHCTRLA`, `CHCTRLB`, `CHINTENCLR`, `CHINTFLAG`, etc.
This is not atomic — protect with [[IRQ Critical Section]] if needed.

## Register Reference

### Global Registers (base 0x41006000)

| Offset | Name | Key bits |
|--------|------|---------|
| 0x00 | CTRL | SWRST(0) / DMAENABLE(1) / CRCENABLE(2) / LVLENx[11:8] |
| 0x10 | SWTRIGCTRL | SWTRIGn — software-trigger any channel |
| 0x20 | INTPEND | PEND/BUSY/FERR/SUSP/TCMPL/TERR + ID[3:0] (lowest pending ch) |
| 0x24 | INTSTATUS | CHINTn[11:0] bitmask |
| 0x28 | BUSYCH | BUSYCHn[11:0] bitmask |
| 0x34 | BASEADDR | Descriptor base (16-byte aligned, enable-protected) |
| 0x38 | WRBADDR | Write-back base (16-byte aligned, enable-protected) |

### Per-Channel Registers (select channel with CHID first)

| Offset | Name | Key bits |
|--------|------|---------|
| 0x3F | CHID | ID[3:0] |
| 0x40 | CHCTRLA | RUNSTDBY(6) / ENABLE(1) / SWRST(0) |
| 0x44 | CHCTRLB | CMD[25:24] / TRIGACT[23:22] / TRIGSRC[13:8] / LVL[6:5] |
| 0x4E | CHINTFLAG | SUSP(2) / TCMPL(1) / TERR(0) — write-1-to-clear |
| 0x4F | CHSTATUS | FERR(2) / BUSY(1) / PEND(0) — read-only |

### CHCTRLB TRIGACT

| Value | Name | Use |
|-------|------|-----|
| 0x0 | BLOCK | One trigger per block — for memory-to-memory |
| 0x2 | BEAT | One trigger per byte — for peripheral streaming (UART, ADC) |
| 0x3 | TRANSACTION | One trigger per linked transaction |

### CHCTRLB TRIGSRC (firmware-relevant entries)

| Value | Peripheral |
|-------|-----------|
| 0x00 | DISABLE (software/event only) |
| 0x02/0x03 | SERCOM0 RX / TX |
| 0x04/0x05 | SERCOM1 RX / TX |
| 0x06/0x07 | SERCOM2 RX / TX |
| 0x08/0x09 | SERCOM3 RX / TX |
| 0x0A/0x0B | SERCOM4 RX / TX |
| 0x0C/0x0D | SERCOM5 RX / TX |
| 0x2A/0x2B | ADC0 / ADC1 RESRDY |
| 0x2D | DAC EMPTY |

### BTCTRL (Transfer Descriptor, in SRAM)

| Bits | Name | Description |
|------|------|-------------|
| 15:13 | STEPSIZE | Address step multiplier: X1–X128 |
| 12 | STEPSEL | 0=step on DST, 1=step on SRC |
| 11 | DSTINC | Destination address increment |
| 10 | SRCINC | Source address increment |
| 9:8 | BEATSIZE | 0=8-bit, 1=16-bit, 2=32-bit |
| 4:3 | BLOCKACT | 0=NOACT, 1=INT, 2=SUSPEND, 3=BOTH |
| 2:1 | EVOSEL | Event: 0=off, 1=block complete, 3=beat complete |
| 0 | VALID | Must be 1 for DMAC to execute descriptor |

## IRQ Handler

```cpp
void dma_irq_handler(void) {
    while (DMAC->INTSTATUS.reg) {         // any pending channel?
        uint8_t ch = DMAC->INTPEND.bit.ID; // lowest-priority pending channel
        dmac_select_channel(ch);
        uint8_t flags = DMAC->CHINTFLAG.reg;
        DMAC->CHINTFLAG.reg = flags;       // clear by writing 1s
        // dispatch on ch and flags
    }
}
```

## See Also

- [[Uart DMA]] — concrete DMAC usage for UART (RX circular + TX block)
- [[Startup SAMC21]] — `irq_handler_dmac` must be defined when using DMA
- [[Clock System]] — DMAC needs `MCLK->AHBMASK.DMAC` enabled
- [[SERCOM UART Configuration]] — DRE (TX) and RXC (RX) trigger source IDs
- [[SAMC21 Datasheet Ch.25 DMAC]] — full register reference, trigger source table
