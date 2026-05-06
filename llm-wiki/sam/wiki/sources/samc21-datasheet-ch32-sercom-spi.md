---
title: SAMC21 Datasheet Chapter 32 — SERCOM SPI
type: source
tags: [sercom, spi, firmware, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 32 — SERCOM SPI – Serial Peripheral Interface

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 32, pages 528–554.

## Overview

SPI is one of the operating modes of the SERCOM module. Master (MODE=0x3) and
slave (MODE=0x2) operation are supported. Full-duplex, 4-wire (MOSI, MISO, SCK,
SS̄). Character size 8 or 9 bits. All four SPI modes (CPOL/CPHA). LSB or MSB
first. Hardware SS̄ control in master mode (MSSEN). DMA capable.

## Clocks

- **CLK_SERCOMx_APB** (APBC): `MCLK->APBCMASK.reg |= MCLK_APBCMASK_SERCOMn`
- **GCLK_SERCOMx_CORE**: required for SPI; configure via `GCLK->PCHCTRL[SERCOMn_GCLK_ID_CORE]`

## Enable-Protected and Synchronized Registers

Enable-protected (must write while ENABLE=0):
- CTRLA (except ENABLE and SWRST)
- CTRLB (except RXEN)
- BAUD
- ADDR

Write-synchronized (poll SYNCBUSY after writing):
- CTRLA.SWRST → poll SYNCBUSY.SWRST
- CTRLA.ENABLE → poll SYNCBUSY.ENABLE
- CTRLB.RXEN → poll SYNCBUSY.CTRLB

## Register Map

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 0x00000000 | EP, WS: SWRST(0)/ENABLE(1)/MODE[4:2]/RUNSTDBY(7)/IBON(8)/DOPO[17:16]/DIPO[21:20]/FORM[27:24]/CPHA(28)/CPOL(29)/DORD(30) |
| 0x04 | CTRLB | 0x00000000 | EP, WS(RXEN): CHSIZE[2:0]/PLOADEN(6)/SSDE(9)/MSSEN(13)/AMODE[15:14]/RXEN(17) |
| 0x0C | BAUD | 0x00 | EP: BAUD[7:0] |
| 0x14 | INTENCLR | 0x00 | ERROR(7)/SSL(3)/RXC(2)/TXC(1)/DRE(0) |
| 0x16 | INTENSET | 0x00 | same |
| 0x18 | INTFLAG | 0x00 | ERROR(7,w1c)/SSL(3,w1c)/RXC(2,ro)/TXC(1,w1c)/DRE(0,ro) |
| 0x1A | STATUS | 0x0000 | BUFOVF(2, w1c) |
| 0x1C | SYNCBUSY | 0x00000000 | CTRLB(2)/ENABLE(1)/SWRST(0) |
| 0x24 | ADDR | 0x00000000 | EP: ADDRMASK[23:16]/ADDR[7:0] |
| 0x28 | DATA | 0x0000 | DATA[8:0] — write=TX, read=RX |
| 0x30 | DBGCTRL | 0x00 | DBGSTOP(0) |

## CTRLA Key Fields

### MODE[2:0] (bits 4:2)
- 0x2 = SPI slave
- 0x3 = SPI master

### DOPO[1:0] (bits 17:16) — Data Out Pinout

| DOPO | DO (MOSI/master, MISO/slave) | SCK | Slave SS̄ |
|------|------------------------------|-----|----------|
| 0x0 | PAD[0] | PAD[1] | PAD[2] |
| 0x1 | PAD[2] | PAD[3] | PAD[1] |
| 0x2 | PAD[3] | PAD[1] | PAD[2] |
| 0x3 | PAD[0] | PAD[3] | PAD[1] |

### DIPO[1:0] (bits 21:20) — Data In Pinout (MISO master / MOSI slave)
0x0=PAD[0], 0x1=PAD[1], 0x2=PAD[2], 0x3=PAD[3]

### FORM[3:0] (bits 27:24)
- 0x0 = SPI frame
- 0x2 = SPI frame with address (slave address matching, PLOADEN must be 0)

### SPI Transfer Modes (CPOL + CPHA)

| Mode | CPOL | CPHA | Leading edge | Trailing edge |
|------|------|------|-------------|---------------|
| 0 | 0 | 0 | Rising, sample | Falling, change |
| 1 | 0 | 1 | Rising, change | Falling, sample |
| 2 | 1 | 0 | Falling, sample | Rising, change |
| 3 | 1 | 1 | Falling, change | Rising, sample |

### DORD (bit 30): 0=MSB first, 1=LSB first

## CTRLB Key Fields

| Field | Bit(s) | Description |
|-------|--------|-------------|
| CHSIZE[2:0] | 2:0 | 0=8BIT, 1=9BIT |
| PLOADEN | 6 | Slave data preload enable |
| SSDE | 9 | Slave Select Low Detect Enable (sets INTFLAG.SSL on SS̄↓) |
| MSSEN | 13 | Master hardware SS̄ control |
| AMODE[1:0] | 15:14 | 0=MASK, 1=2_ADDRS, 2=RANGE |
| RXEN | 17 | Receiver Enable (not EP; write-sync via SYNCBUSY.CTRLB) |

## BAUD Register (0x0C, 8-bit, enable-protected)

```
f_SCK = GCLK_SERCOMx_CORE / (2 × (BAUD + 1))
BAUD = GCLK_SERCOMx_CORE / (2 × f_SCK) - 1
```

Examples with GCLK = 48 MHz:
- 1 MHz: BAUD = 23
- 4 MHz: BAUD = 5
- 8 MHz: BAUD = 2
- 12 MHz: BAUD = 1
- 24 MHz: BAUD = 0

## INTFLAG Flags

| Bit | Flag | Set condition | Clear condition |
|-----|------|---------------|-----------------|
| 0 | DRE | DATA empty, ready for TX write | Writing DATA (read-only to clear) |
| 1 | TXC | All data shifted out, no new DATA | Write 1 or write DATA |
| 2 | RXC | Received data in buffer | Reading DATA (read-only to clear) |
| 3 | SSL | SS̄ fell (slave mode, SSDE=1) | Write 1 |
| 7 | ERROR | Any error (BUFOVF etc.) | Write 1 |

## DMA Operation

- **TX DMA (TRIGACT=BEAT)**: trigger on DRE (SERCOMx_DMAC_ID_TX)
- **RX DMA (TRIGACT=BEAT)**: trigger on RXC (SERCOMx_DMAC_ID_RX)

## Slave Mode Notes

- First character after SS̄ asserted comes from shift register content, not DATA.
  Use PLOADEN=1 to preload shift register before transaction.
- CTRLB.RXEN not enable-protected — can set while enabled; sets SYNCBUSY.CTRLB.
- Writing CTRLB when SYNCBUSY.CTRLB=1 causes APB error.
- After DATA written: takes up to 3 SCK cycles before shift register is loaded.
  Must write next character with ≥3 SCK cycles remaining.

## Key Facts

- CTRLA, CTRLB (except RXEN), BAUD, ADDR are enable-protected.
- Three SYNCBUSY points: SWRST, ENABLE, CTRLB.RXEN.
- MSSEN=1 (master): hardware controls SS̄, one baud cycle before/after each frame.
- MSSEN=0 (master): SS̄ must be driven by software GPIO before/after transaction.
- IBON=1: BUFOVF set immediately on overflow; IBON=0: BUFOVF propagates through FIFO.
- DATA read-only clears RXC; DATA write-only clears DRE.
- DBGSTOP=0 (reset default): baud-rate generator continues during debug halt.

## See Also

- [[SERCOM SPI Configuration]] — firmware init patterns and transfer functions
- [[I/O Multiplexing]] — SERCOM PAD pin assignments for each SERCOM instance
- [[DMA Controller]] — SPI DMA transfer setup (TX/RX beat triggers)
- [[Clock System]] — GCLK_SERCOMx_CORE, APBC mask enable
