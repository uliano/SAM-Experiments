---
title: SAMC21 Datasheet Chapter 34 — CAN
type: source
tags: [can, can-fd, firmware, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 34 — CAN – Control Area Network (SAM C21 Only)

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 34, pages 605–698.

## Overview

CAN peripheral present on SAM C21 only (not SAM C20). Conforms to
ISO 11898-1:2015 (Bosch CAN 2.0 A/B, ISO CAN FD). Up to two CAN instances
(CAN0/CAN1). CAN FD up to 64 data bytes, 10 Mb/s. Message RAM is external to
the CAN core, located in system SRAM, accessed via AHB.

## Features

- CAN 2.0 A/B and CAN-FD (ISO 11898-1:2015)
- Up to 64 dedicated Rx Buffers, 2 Rx FIFOs (up to 64 elements each)
- Up to 32 dedicated Tx Buffers; Tx FIFO or Tx Queue
- Tx Event FIFO (up to 32 elements)
- Acceptance filtering: up to 128 elements (standard + extended ID)
- Error logging, Bus-Off, Error Passive/Warning states
- Timestamp and timeout counters
- Loopback test modes; Bus Monitoring Mode
- Two interrupt lines (LINE0, LINE1) → NVIC

## Clocks

- **CLK_CAN_AHB**: `MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN0` (or CAN1)
- **GCLK_CAN**: `GCLK->PCHCTRL[CAN0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLKn | GCLK_PCHCTRL_CHEN`
- GCLK_CAN is asynchronous to CLK_CAN_AHB; only CCCR.INIT is write-synchronized.

## Power / Sleep

- Cannot operate in idle2 or standby sleep modes.
- Low-power sequence: set CCCR.CSR=1 → wait CCCR.CSA=1 → stop CLK_CAN_AHB and GCLK_CAN.
- Wake: restart clocks → clear CCCR.CSR → wait CCCR.CSA=0 → clear CCCR.INIT.

## Initialization Sequence

1. Set `CCCR.INIT = 1` (reset state; also auto-set on Bus-Off or Message RAM bit error)
2. Wait until `CCCR.INIT` reads back 1 (INIT is write-synchronized)
3. Set `CCCR.CCE = 1` (enables write access to configuration registers; only while INIT=1)
4. Configure NBTP, DBTP, RXESC, TXESC, TXBC, filter lists, RXF0C/RXF1C, etc.
5. Clear `CCCR.CCE = 0`
6. Clear `CCCR.INIT = 0` — CAN synchronizes to bus (waits 11 recessive bits) and enters Normal mode.

Configuration registers are only writable when both `CCCR.INIT=1` AND `CCCR.CCE=1`.

## Register Summary

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CREL | 0x32100000 | Core Release (read-only): REL/STEP/SUBSTEP version |
| 0x04 | ENDN | 0x87654321 | Endianness test value (read-only) |
| 0x08 | MRCFG | 0x00000002 | Message RAM Config: DQOS[1:0] (memory priority) |
| 0x0C | DBTP | 0x00000A33 | Data Bit Timing (WR): TDC/DBRP/DTSEG1/DTSEG2/DSJW |
| 0x10 | TEST | 0x00000000 | Test (WR): LBCK/TX/RX |
| 0x14 | RWD | 0x00000000 | RAM Watchdog (WR): WDC/WDV |
| 0x18 | CCCR | 0x00000001 | CC Control (WR): INIT/CCE/ASM/CSA/CSR/MON/DAR/TEST/FDOE/BRSE/PXHD/EFBI/TXP |
| 0x1C | NBTP | 0x06000A03 | Nominal Bit Timing (WR): NSJW/NBRP/NTSEG1/NTSEG2 |
| 0x20 | TSCC | 0x00000000 | Timestamp Counter Config (WR): TCP/TSS |
| 0x24 | TSCV | 0x00000000 | Timestamp Counter Value (RO): TSC[14:0] |
| 0x28 | TOCC | 0xFFFF0000 | Timeout Counter Config (WR): TOP/TOS/ETOC |
| 0x2C | TOCV | 0x0000FFFF | Timeout Counter Value (RO): TOC[15:0] |
| 0x40 | ECR | 0x00000000 | Error Counter (RO): TEC/REC/RP/CEL |
| 0x44 | PSR | — | Protocol Status (RO): LEC/ACT/EP/EW/BO/RFDF/RBRS/RESI/DLEC/TDCV/PXE |
| 0x48 | TDCR | — | Transmitter Delay Compensation: TDCF/TDCO |
| 0x50 | IR | 0x00000000 | Interrupt Register (w1c): all interrupt flags |
| 0x54 | IE | 0x00000000 | Interrupt Enable |
| 0x58 | ILS | 0x00000000 | Interrupt Line Select (0=LINE0, 1=LINE1) |
| 0x5C | ILE | 0x00000000 | Interrupt Line Enable: EINT0/EINT1 |
| 0x80 | GFC | 0x00000000 | Global Filter Config: ANFS/ANFE/RRFS/RRFE |
| 0x84 | SIDFC | 0x00000000 | Standard ID Filter Config: FLSSA/LSS |
| 0x88 | XIDFC | 0x00000000 | Extended ID Filter Config: FLESA/LSE |
| 0x90 | XIDAM | 0x1FFFFFFF | Extended ID AND Mask |
| 0x94 | HPMS | 0x00000000 | High Priority Message Status (RO) |
| 0x98 | NDAT1 | 0x00000000 | New Data 1 (Rx Buffers 0–31) |
| 0x9C | NDAT2 | 0x00000000 | New Data 2 (Rx Buffers 32–63) |
| 0xA0 | RXF0C | 0x00000000 | Rx FIFO 0 Config: F0SA/F0S/F0WM/F0OM |
| 0xA4 | RXF0S | 0x00000000 | Rx FIFO 0 Status (RO): F0GI/F0PI/RF0L/F0F/F0FL |
| 0xA8 | RXF0A | 0x00000000 | Rx FIFO 0 Acknowledge: F0AI |
| 0xAC | RXBC | 0x00000000 | Rx Buffer Config: RBSA (start address) |
| 0xB0 | RXF1C | 0x00000000 | Rx FIFO 1 Config: F1SA/F1S/F1WM/F1OM |
| 0xB4 | RXF1S | 0x00000000 | Rx FIFO 1 Status (RO): F1GI/F1PI/RF1L/F1F/F1FL/DMS |
| 0xB8 | RXF1A | 0x00000000 | Rx FIFO 1 Acknowledge: F1AI |
| 0xBC | RXESC | 0x00000000 | Rx Element Size Config: F1DS/F0DS/RBDS |
| 0xC0 | TXBC | 0x00000000 | Tx Buffer Config: TBSA/NDTB/TFQS/TFQM |
| 0xC4 | TXFQS | 0x00000000 | Tx FIFO/Queue Status (RO): TFFL/TFGI/TFQPI/TFQF |
| 0xC8 | TXESC | 0x00000000 | Tx Element Size Config: TBDS |
| 0xCC | TXBRP | 0x00000000 | Tx Buffer Request Pending (RO): TRPn |
| 0xD0 | TXBAR | 0x00000000 | Tx Buffer Add Request (writable while CCE=0): ARn |
| 0xD4 | TXBCR | 0x00000000 | Tx Buffer Cancellation Request (writable while CCE=0): CRn |
| 0xD8 | TXBTO | 0x00000000 | Tx Buffer Transmission Occurred (RO): TOn |
| 0xDC | TXBCF | 0x00000000 | Tx Buffer Cancellation Finished (RO): CFn |
| 0xE0 | TXBTIE | 0x00000000 | Tx Buffer Transmission Interrupt Enable |
| 0xE4 | TXBCIE | 0x00000000 | Tx Buffer Cancellation Interrupt Enable |
| 0xF0 | TXEFC | 0x00000000 | Tx Event FIFO Config: EFSA/EFS/EFWM |
| 0xF4 | TXEFS | 0x00000000 | Tx Event FIFO Status (RO): EFFL/EFGI/EFPI/TEFL/EFF |
| 0xF8 | TXEFA | 0x00000000 | Tx Event FIFO Acknowledge: EFAI |

## CCCR — CC Control (key bits)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | INIT | 1=Init mode (reset=1); write-synchronized |
| 1 | CCE | 1=Config Change Enable (writable while INIT=1) |
| 2 | ASM | 1=Restricted Operation Mode (writable while CCE+INIT=1) |
| 3 | CSA | Clock Stop Acknowledge (read-only) |
| 4 | CSR | Clock Stop Request |
| 5 | MON | Bus Monitoring Mode (writable while CCE+INIT=1) |
| 6 | DAR | Disable Automatic Retransmission (writable while CCE+INIT=1) |
| 7 | TEST | Test Mode Enable (writable while CCE+INIT=1) |
| 8 | FDOE | FD Operation Enable |
| 9 | BRSE | Bit Rate Switch Enable (requires FDOE=1) |
| 12 | PXHD | Protocol Exception Handling Disable |
| 13 | EFBI | Edge Filtering during Bus Integration |
| 14 | TXP | Transmit Pause (2 CAN bit times between TX frames) |

## Nominal Bit Timing (NBTP, write-restricted)

```
t_q = (NBRP + 1) / f_GCLK_CAN
Bit time = (1 + NTSEG1 + 1 + NTSEG2 + 1) × t_q = (NTSEG1 + NTSEG2 + 3) × t_q
f_CAN = f_GCLK_CAN / (NBRP+1) / (NTSEG1 + NTSEG2 + 3)
```

| f_GCLK_CAN | f_CAN | NBRP | NTSEG1 | NTSEG2 | NSJW | Sample point |
|-----------|-------|------|--------|--------|------|-------------|
| 8 MHz | 500 kbps | 0 | 10 | 3 | 6 | 75% (reset default) |
| 48 MHz | 500 kbps | 5 | 10 | 3 | 6 | 75% |
| 48 MHz | 1 Mbps | 2 | 10 | 4 | 4 | 73% |

Note: reset value NBTP=0x06000A03 gives 500 kbps at GCLK_CAN=8 MHz.

## Data Bit Timing (DBTP, write-restricted, CAN FD only)

```
t_q = (DBRP + 1) / f_GCLK_CAN
Bit time = (DTSEG1 + DTSEG2 + 3) × t_q
```

Reset value DBTP=0x00000A33 gives 500 kbps data rate at GCLK_CAN=8 MHz.

## Buffer/FIFO Element Size

| RBDS/FnDS/TBDS[2:0] | Data field [bytes] | RAM words |
|--------------------|--------------------|-----------|
| 000 | 8 | 4 |
| 001 | 12 | 5 |
| 010 | 16 | 6 |
| 011 | 20 | 7 |
| 100 | 24 | 8 |
| 101 | 32 | 10 |
| 110 | 48 | 14 |
| 111 | 64 | 18 |

## CAN FD DLC Extension

| DLC | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|-----|---|----|----|----|----|----|----|
| Data bytes | 12 | 16 | 20 | 24 | 32 | 48 | 64 |

## Rx FIFO Operation

- **RXF0C**: F0SA = start address (word address in Message RAM), F0S = size (0–64), F0WM = watermark, F0OM = overwrite mode
- **RXF0S**: F0FL = fill level, F0PI = put index, F0GI = get index, RF0L = message lost, F0F = full
- **RXF0A**: Write get index to F0AI to release element (advances get index)
- Rx FIFO element: R0 (ID+RTR+XTD+ESI+RXTS), R1 (DLC+FDF+BSR+FIDX+ANMF+RXTS), data words

## Tx Buffer Operation

- **TXBC**: TBSA = start address, NDTB = number of dedicated Tx buffers (0–32), TFQS = Tx FIFO/Queue size (0–32), TFQM = 0:FIFO, 1:Queue
- **TXBAR**: Write 1 to bit n to request transmission of Tx buffer n (writable while CCE=0)
- **TXBCR**: Write 1 to bit n to cancel pending transmission
- **TXBTO**: Read to see which buffers successfully transmitted
- Tx buffer element: T0 (ID+RTR+XTD+ESI+MM), T1 (DLC+FDF+BRS+EFC+MM), data words

## Acceptance Filtering

- **GFC**: ANFS/ANFE = action for non-matching standard/extended frames (0=accept in FIFO0, 1=accept in FIFO1, 2=reject); RRFS/RRFE = reject remote frames
- **SIDFC**: FLSSA = start address of standard filter list, LSS = number of elements
- **XIDFC**: FLESA = start address of extended filter list, LSE = number of elements
- Standard filter element: 1 word (SFEC[2:0] + SFID1[10:0] + SFT[1:0] + SFID2[10:0])
- Extended filter element: 2 words (EFEC[2:0] + EFID1[28:0] + EFT[1:0] + EFID2[28:0])

## Error Counter (ECR)

- **TEC[7:0]**: Transmit Error Counter (0–255); >127 → Error Passive
- **REC[6:0]**: Receive Error Counter (0–127)
- **RP**: Receive Error Passive (REC ≥ 128)
- **CEL[7:0]**: CAN Error Logging counter; incremented on each TEC/REC increment; read to reset; caps at 0xFF (sets IR.ELO)

## Protocol Status (PSR)

- **LEC[2:0]**: Last Error Code (0=NoErr, 1=Stuff, 2=Form, 3=Ack, 4=Bit1, 5=Bit0, 6=CRC, 7=NoChg)
- **ACT[1:0]**: 00=Synchronizing, 01=Idle, 10=Receiver, 11=Transmitter
- **EP**: Error Passive (TEC≥128 or REC≥128)
- **EW**: Error Warning (TEC≥96 or REC≥96)
- **BO**: Bus-Off (TEC≥256)
- **RFDF/RBRS/RESI**: Last frame was CAN FD / had bit rate switch / had ESI set

## Interrupt Register (IR, 0x50)

Key flags (w1c):
- **RF0N/RF1N**: Rx FIFO 0/1 New Message
- **RF0W/RF1W**: Rx FIFO 0/1 Watermark Reached
- **RF0F/RF1F**: Rx FIFO 0/1 Full
- **RF0L/RF1L**: Rx FIFO 0/1 Message Lost
- **DRX**: Message stored to Dedicated Rx Buffer
- **TC/TCF**: Transmission Completed / Cancelled
- **TFE**: Tx FIFO Empty
- **TEFN/TEFW/TEFF/TEFL**: Tx Event FIFO (New/Watermark/Full/Lost)
- **TSW**: Timestamp Counter Wraparound
- **MRAF**: Message RAM Access Failure
- **TOO**: Timeout Occurred
- **ELO**: Error Logging Overflow (CEL wrapped)
- **EP/EW/BO**: Error Passive / Warning / Bus-Off state changes
- **WDI**: Watchdog Interrupt (Message RAM timeout)
- **PEA/PED**: Protocol Exception Arbitration/Data phase
- **ARA**: Access to Reserved Address

## Key Facts

- CAN on SAM C21 only — not present on SAM C20.
- Message RAM is in system SRAM — must allocate and configure before enabling.
- CCCR.INIT=1 at reset; must clear INIT to start normal operation.
- Write access to configuration registers requires CCCR.INIT=1 AND CCCR.CCE=1.
- TXBAR and TXBCR are writable only while CCCR.CCE=0 (normal operation).
- Registers reset when CCE is set: HPMS, RXF0S, RXF1S, TXFQS, TXBRP, TXBTO, TXBCF, TXEFS.
- CAN FD: CCCR.FDOE=1; bit rate switching: CCCR.FDOE=1 + CCCR.BRSE=1 + Tx buffer FDF+BRS=1.
- CCCR.DAR=1: disable automatic retransmission (for time-triggered CAN).
- RxFIFO Overwrite Mode (F0OM/F1OM=1): oldest message overwritten when full.
- FIFO Acknowledge: write get index to RXF0A/RXF1A/TXEFA to advance the get index.
- CCCR.INIT is write-synchronized; read back INIT before setting to a new value.

## See Also

- [[CAN Configuration]] — firmware init, bit timing, Tx/Rx patterns
- [[I/O Multiplexing]] — CAN TX/RX pin assignments
- [[Clock System]] — GCLK_CAN / CLK_CAN_AHB setup
- [[DMA Controller]] — CAN debug message DMA support
