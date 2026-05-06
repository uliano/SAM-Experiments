---
title: SAMC21 Datasheet Chapter 31 — SERCOM USART
type: source
tags: [sercom, usart, uart, dma, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 31 — SERCOM USART

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 31, pages 492–527.

## Overview

SERCOM USART implements a full-duplex asynchronous serial interface on any SERCOM instance. Supports 5–9 data bits, optional parity, 1 or 2 stop bits, hardware flow control (RTS/CTS), LIN master/slave, IrDA, RS-485, and Start-of-Frame detection. Two clocks required: APB (register access) and GCLK CORE (baud generation and shift clock).

## Features

- Full-duplex USART, synchronous USART, SPI master/slave
- 16× or 8× oversampling (arithmetic or fractional baud), 3× oversampling
- Hardware RTS/CTS flow control
- LIN master (LINCMD) and LIN slave (break detection)
- IrDA up to 115,200 baud
- RS-485 transmit-enable output
- Start-of-Frame (SoF) wakeup from standby
- DMA capable: DRE (TX empty), RXC (RX complete)
- Independent TX and RX enable

## Register Map (USART mode)

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| 0x00 | CTRLA | RW | Control A (mode, pads, oversampling, endianness) — enable-protected |
| 0x04 | CTRLB | RW | Control B (TX/RX enable, char size, parity, stop bits) — enable-protected |
| 0x08 | CTRLC | RW | Control C (RS-485, IrDA pulse width) |
| 0x0C | BAUD | RW | Baud rate register (16-bit) |
| 0x0E | RXPL | RW | Receive Pulse Length (IrDA) |
| 0x14 | INTENCLR | RW | Interrupt Enable Clear |
| 0x16 | INTENSET | RW | Interrupt Enable Set |
| 0x18 | INTFLAG | RW | Interrupt Flag Status and Clear |
| 0x1A | STATUS | RW | Status flags |
| 0x1C | SYNCBUSY | R | Synchronization Busy |
| 0x28 | DATA | RW | Receive/Transmit Data |
| 0x30 | DBGCTRL | RW | Debug Control |

## CTRLA Register (0x00) — Enable-Protected

| Bits | Name | Description |
|------|------|-------------|
| 30 | DORD | Data order: 0=MSB first, 1=LSB first (UART: always 1) |
| 29 | CPOL | Clock polarity (synchronous mode only) |
| 28 | CMODE | 0=asynchronous (UART), 1=synchronous |
| 27:24 | FORM[3:0] | Frame format: 0=USART no parity, 1=USART with parity, 2=LIN slave, 4=auto-baud no parity, 5=auto-baud with parity, 7=ISO 7816 |
| 23:22 | SAMPA[1:0] | Sample adjustment (which samples of 16/8 to use for majority vote) |
| 21:20 | RXPO[1:0] | RX PAD select: 0=PAD[0], 1=PAD[1], 2=PAD[2], 3=PAD[3] |
| 17:16 | TXPO[1:0] | TX PAD select (see table below) |
| 15:13 | SAMPR[2:0] | Sample rate and mode |
| 8 | IBON | Immediate Buffer Overflow Notification |
| 7 | RUNSTDBY | Run in Standby |
| 4:2 | MODE[2:0] | 0x0=USART external clock, 0x1=USART internal clock |
| 1 | ENABLE | Enable |
| 0 | SWRST | Software Reset |

### TXPO Table

| TXPO | TX | XCK/RTS/CTS | Notes |
|------|----|-------------|-------|
| 0x0 | PAD[0] | XCK=PAD[1] | Standard UART TX on PAD[0] |
| 0x1 | PAD[2] | XCK=PAD[3] | TX on PAD[2] |
| 0x2 | PAD[0] | RTS=PAD[2], CTS=PAD[3] | Hardware flow control |
| 0x3 | PAD[0] | XCK=PAD[1], RTS=PAD[2] | RS-485 (RS485 bit in CTRLC required) |

### RXPO Table

| RXPO | RX pad |
|------|--------|
| 0x0 | PAD[0] |
| 0x1 | PAD[1] |
| 0x2 | PAD[2] |
| 0x3 | PAD[3] |

### SAMPR Table

| SAMPR | Oversampling | Baud mode |
|-------|-------------|-----------|
| 0x0 | 16× | Arithmetic |
| 0x1 | 16× | Fractional |
| 0x2 | 8× | Arithmetic |
| 0x3 | 8× | Fractional |
| 0x4 | 3× | Arithmetic |

## CTRLB Register (0x04) — Enable-Protected

| Bits | Name | Description |
|------|------|-------------|
| 17 | RXEN | Receive Enable — must sync after write |
| 16 | TXEN | Transmit Enable — must sync after write |
| 13 | COLDEN | Collision Detection Enable |
| 12 | SFDE | Start-of-Frame Detection Enable |
| 11 | ENC | Encoding (IrDA) |
| 9:8 | LINCMD[1:0] | LIN Command |
| 6 | PMODE | Parity Mode: 0=even, 1=odd |
| 6 | SBMODE | Stop Bit Mode: 0=one stop bit, 1=two stop bits (note: shares position with PMODE in different sub-modes) |
| 2:0 | CHSIZE[2:0] | Character Size: 0=8 bits, 1=9 bits, 5=5 bits, 6=6 bits, 7=7 bits |

## BAUD Register (0x0C)

16-bit register. Value depends on SAMPR setting.

### Arithmetic Mode (SAMPR=0, 16× oversampling)

```
BAUD = 65536 × (1 − 16 × f_baud / f_ref)
```

### Fractional Mode (SAMPR=1, 16× oversampling)

```
BAUD[12:0] = integer part of f_ref / (16 × f_baud)
BAUD[15:13] = FP = fractional part (0–7 eighths)
```

### 8× Arithmetic (SAMPR=2)

```
BAUD = 65536 × (1 − 8 × f_baud / f_ref)
```

| f_ref | Baud | SAMPR | BAUD_reg |
|-------|------|-------|----------|
| 48 MHz | 1,000,000 | 0 (16× arith) | 0xAAAA (43691) |
| 48 MHz | 115,200 | 0 (16× arith) | 0xF5EB (63019) |
| 48 MHz | 9,600 | 0 (16× arith) | 0xFF93 (65427) |

## STATUS Register (0x1A)

| Bit | Name | Description |
|-----|------|-------------|
| 9 | TXE | Transmitter Empty (TX shift register empty) |
| 5 | COLL | Collision detected |
| 4 | ISF | Inconsistent Sync Field (auto-baud) |
| 3 | CTS | Clear To Send (inverted CTS pin) |
| 2 | BUFOVF | Buffer Overflow |
| 1 | FERR | Frame Error |
| 0 | PERR | Parity Error |

## SYNCBUSY Register (0x1C)

| Bit | Name | Wait condition |
|-----|------|---------------|
| 2 | CTRLB | After writing CTRLB |
| 1 | ENABLE | After setting/clearing ENABLE |
| 0 | SWRST | After writing SWRST |

## DATA Register (0x28)

9-bit register (bits 8:0). Read to receive; write to transmit. A read clears RXC flag; a write to full buffer sets BUFOVF.

## Interrupt Sources

| Flag | Bit | DMA trigger | Description |
|------|-----|-------------|-------------|
| DRE | 0 | Yes (TX) | Data Register Empty — TX buffer ready |
| TXC | 1 | No | Transmit Complete — shift register empty |
| RXC | 2 | Yes (RX) | Receive Complete — data in DATA |
| RXS | 3 | No | Receive Start (SoF) |
| CTSIC | 4 | No | CTS Input Change |
| RXBRK | 5 | No | Break received |
| ERROR | 7 | No | BUFOVF/FERR/PERR/ISF |

## Initialization Sequence

1. Enable APB clock via `MCLK->APBCMASK`
2. Connect GCLK to SERCOM CORE via `GCLK->PCHCTRL[]`
3. Write `CTRLA.SWRST = 1`, wait `SYNCBUSY.SWRST = 0` and `CTRLA.SWRST = 0`
4. Write `BAUD` register
5. Write `CTRLA` (MODE, TXPO, RXPO, SAMPR, DORD, FORM)
6. Write `CTRLB` (TXEN, RXEN, CHSIZE), wait `SYNCBUSY.CTRLB = 0`
7. Configure pin mux (PMUXEN, PMUXn, PINCFG.INEN for RX)
8. Set `CTRLA.ENABLE = 1`, wait `SYNCBUSY.ENABLE = 0`
9. If using interrupts: set `INTENSET`, enable NVIC IRQ
10. If using DMA: configure DMAC channels for DRE and RXC triggers

## Key Facts

- CTRLA and CTRLB are enable-protected — must be written while ENABLE=0 (or after SWRST).
- DRE fires when TX buffer empty (can accept next byte); TXC fires when TX shift register has completed.
- For DMA TX: use DRE trigger; for DMA RX: use RXC trigger.
- SYNCBUSY must be checked after SWRST, CTRLB write, and ENABLE write.
- TX pin should be driven high before enabling peripheral mux (prevents spurious start bit).
- BAUD formula assumes SAMPR=0 (16× arithmetic); use fractional mode for fewer errors at high baud rates.

## See Also

- [[SERCOM UART Configuration]] — firmware register values, trait system, init code
- [[I/O Multiplexing]] — SERCOM5 PAD assignments (PB30/PB31)
- [[DMA Controller]] — DRE/RXC DMA channel configuration
- [[Clock System]] — GCLK CORE assignment (PCHCTRL)
- [[NVIC Interrupt Map]] — SERCOM5 IRQn
