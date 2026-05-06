---
title: SAMC21 Datasheet Chapter 33 — SERCOM I2C
type: source
tags: [sercom, i2c, twi, firmware, samc21, datasheet]
date: 2026-05-05
---

# SAMC21 Datasheet Ch. 33 — SERCOM I2C – Inter-Integrated Circuit

Source: SAM C20/C21 Data Sheet DS60001479D, chapter 33, pages 555–604.

## Overview

I2C is one of the operating modes of the SERCOM module. Both master (MODE=0x5)
and slave (MODE=0x4) operation are supported. 7-bit and 10-bit addressing.
Standard-mode (Sm, 100 kHz), Fast-mode (Fm, 400 kHz), Fast-mode Plus (Fm+,
1 MHz), and High-speed mode (Hs, 3.4 MHz). PAD[0]=SDA, PAD[1]=SCL (fixed).

## Clocks

- **CLK_SERCOMx_APB** (APBC): `MCLK->APBCMASK.reg |= MCLK_APBCMASK_SERCOMn`
- **GCLK_SERCOMx_CORE**: required for all I2C operation
- **GCLK_SERCOM_SLOW**: required only for SMBus timeouts (INACTOUT/SEXTTOEN/LOWTOUT)

## Enable-Protected and Synchronized Registers

Enable-protected (write while ENABLE=0):
- CTRLA (except ENABLE and SWRST)
- CTRLB (except ACKACT and CMD — slave; except ACKACT and CMD — master)
- BAUD (master only)
- ADDR (slave only)

Write-synchronized (poll SYNCBUSY after writing):
- CTRLA.SWRST → poll SYNCBUSY.SWRST
- CTRLA.ENABLE → poll SYNCBUSY.ENABLE
- CTRLB.CMD (master) → poll SYNCBUSY.SYSOP
- STATUS.BUSSTATE (master) → poll SYNCBUSY.SYSOP
- ADDR.ADDR (master) → poll SYNCBUSY.SYSOP
- DATA (master) → poll SYNCBUSY.SYSOP

## Register Map — I2C Slave

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 0x00000000 | EP,WS: SWRST(0)/ENABLE(1)/MODE[4:2]/RUNSTDBY(7)/SDAHOLD[21:20]/PINOUT(16)/SCLSM(27)/SPEED[25:24]/SEXTTOEN(23)/LOWTOUT(30) |
| 0x04 | CTRLB | 0x00000000 | EP(excl.ACKACT,CMD): SMEN(8)/GCMD(9)/AACKEN(10)/AMODE[15:14]/CMD[17:16]/ACKACT(18) |
| 0x14 | INTENCLR | 0x00 | ERROR(7)/DRDY(2)/AMATCH(1)/PREC(0) |
| 0x16 | INTENSET | 0x00 | same |
| 0x18 | INTFLAG | 0x00 | ERROR(7)/DRDY(2)/AMATCH(1)/PREC(0) |
| 0x1A | STATUS | 0x0000 | BUSERR(0)/COLL(1)/RXNACK(2)/DIR(3)/SR(4)/LOWTOUT(6)/CLKHOLD(7,ro)/SEXTTOUT(9)/HS(10) |
| 0x1C | SYNCBUSY | 0x00000000 | ENABLE(1)/SWRST(0) |
| 0x24 | ADDR | 0x00000000 | EP: GENCEN(0)/ADDR[10:1]/TENBITEN(15)/ADDRMASK[26:17] |
| 0x28 | DATA | 0x0000 | WS,RS: DATA[7:0] |

## Register Map — I2C Master

| Offset | Name | Reset | Description |
|--------|------|-------|-------------|
| 0x00 | CTRLA | 0x00000000 | EP,WS: SWRST(0)/ENABLE(1)/MODE[4:2]/RUNSTDBY(7)/SDAHOLD[21:20]/PINOUT(16)/SCLSM(27)/SPEED[25:24]/SEXTTOEN(23)/MEXTTOEN(22)/INACTOUT[29:28]/LOWTOUT(30) |
| 0x04 | CTRLB | 0x00000000 | EP(excl.ACKACT,CMD): SMEN(8)/QCEN(9)/CMD[17:16]/ACKACT(18) |
| 0x0C | BAUD | 0x00000000 | EP: BAUD[7:0]/BAUDLOW[15:8]/HSBAUD[23:16]/HSBAUDLOW[31:24] |
| 0x14 | INTENCLR | 0x00 | ERROR(7)/SB(1)/MB(0) |
| 0x16 | INTENSET | 0x00 | same |
| 0x18 | INTFLAG | 0x00 | ERROR(7)/SB(1)/MB(0) |
| 0x1A | STATUS | 0x0000 | WS: BUSERR(0)/ARBLOST(1)/RXNACK(2)/CLKHOLD(7,ro)/LOWTOUT(6)/BUSSTATE[5:4]/MEXTTOUT(8)/SEXTTOUT(9)/LENERR(10) |
| 0x1C | SYNCBUSY | 0x00000000 | SWRST(0)/ENABLE(1)/SYSOP(2) |
| 0x24 | ADDR | 0x0000 | WS: ADDR[10:0]/LENEN(13)/HS(14)/TENBITEN(15)/LEN[23:16] |
| 0x28 | DATA | 0x0000 | DATA[7:0] |
| 0x30 | DBGCTRL | 0x00 | DBGSTOP(0) |

## CTRLA Key Fields (Both Modes)

### MODE[2:0] (bits 4:2)
- 0x4 = I2C slave
- 0x5 = I2C master

### SPEED[1:0] (bits 25:24)
| Value | Mode | Max f_SCL |
|-------|------|-----------|
| 0x0 | Sm/Fm | 400 kHz |
| 0x1 | Fm+ | 1 MHz |
| 0x2 | Hs-mode | 3.4 MHz |

### SDAHOLD[1:0] (bits 21:20) — SDA Hold Time after SCL negative edge
| Value | Name | Hold time |
|-------|------|-----------|
| 0x0 | DIS | Disabled |
| 0x1 | 75NS | 50–100 ns |
| 0x2 | 450NS | 300–600 ns |
| 0x3 | 600NS | 400–800 ns |

### SCLSM (bit 27) — SCL Clock Stretch Mode
- 0: SCL stretched between ACK and next data transfer
- 1: SCL stretched only after ACK bit (reduces latency in fast systems)

### LOWTOUT (bit 30)
- Slave: releases SCL hold if SCL held low 25–35 ms (bus recovery)
- Master: releases SCL hold if SCL held low 25–35 ms, sends STOP

### INACTOUT[1:0] (bits 29:28, master only) — Inactive Bus Time-Out
| Value | Name | Time-out |
|-------|------|---------|
| 0x0 | DIS | Disabled |
| 0x1 | 55US | 5–6 SCL cycles (50–60 μs) |
| 0x2 | 105US | 10–11 SCL cycles (100–110 μs) |
| 0x3 | 205US | 20–21 SCL cycles (200–210 μs) |

### MEXTTOEN (bit 22, master) / SEXTTOEN (bit 23)
- MEXTTOEN=1: master releases SCL if held >10 ms (START-to-ACK, ACK-to-ACK, ACK-to-STOP)
- SEXTTOEN=1: master/slave releases SCL if held cumulatively >25 ms from START to STOP

## CTRLB Key Fields

### ACKACT (bit 18) — Acknowledge Action (not enable-protected)
- 0 = Send ACK
- 1 = Send NACK
- Slave: executed when CMD is written, or when DATA is read in smart mode
- Master: executed when CMD is written, or when DATA is read in smart mode

### CMD[1:0] (bits 17:16) — Command (strobe, always reads 0)

**Slave CMD table:**
| CMD | DIR | Action |
|-----|-----|--------|
| 0x0 | X | No action |
| 0x2 | 0 (master write) | Execute ACK action + wait for any START |
| 0x2 | 1 (master read) | Wait for any START |
| 0x3 | 0 (master write) | Execute ACK action + receive next byte |
| 0x3 | 1 (master read) | Execute byte read + send ACK/NACK |

**Master CMD table:**
| CMD | Action |
|-----|--------|
| 0x0 | No action |
| 0x1 | Execute ACK action + issue repeated START (retransmit current ADDR) |
| 0x2 (write) | No operation |
| 0x2 (read) | Execute ACK action + byte read |
| 0x3 | Execute ACK action + issue STOP |

### SMEN (bit 8) — Smart Mode Enable
- Slave: ACK/NACK sent automatically when DATA.DATA is read
- Master: ACK action sent automatically when DATA.DATA is read

### AMODE[1:0] (bits 15:14, slave) — Address Mode
- 0x0: MASK — respond to ADDR.ADDR masked by ADDR.ADDRMASK
- 0x1: 2_ADDRS — respond to two addresses in ADDR.ADDR and ADDR.ADDRMASK
- 0x2: RANGE — respond to range ADDR.ADDR to ADDR.ADDRMASK

### QCEN (bit 9, master) — Quick Command Enable
SMBus Quick Command: START + address + R/W + STOP without data.

## BAUD Register (master, 0x0C, enable-protected)

```
f_SCL = f_GCLK / (10 + 2×BAUD + f_GCLK×T_RISE)   [BAUDLOW=0]
BAUD  = (f_GCLK/f_SCL - 10 - f_GCLK×T_RISE) / 2

T_HIGH = (BAUD + 5)   / f_GCLK
T_LOW  = (BAUDLOW + 5) / f_GCLK   [if BAUDLOW=0 uses BAUD]
```

Examples at f_GCLK=48 MHz, BAUDLOW=0:
- 100 kHz (T_RISE≈100 ns): BAUD ≈ 232
- 400 kHz (T_RISE≈80 ns): BAUD ≈ 53
- 1 MHz (T_RISE≈60 ns): BAUD ≈ 19

High-speed BAUD: `HSBAUD = f_GCLK × T_HIGH − 1`

## INTFLAG Flags

### Slave
| Bit | Flag | Set condition | Clear condition |
|-----|------|---------------|-----------------|
| 7 | ERROR | Any error | Write 1 |
| 2 | DRDY | Byte TX/RX complete | Write DATA, read DATA (smart), write CMD |
| 1 | AMATCH | Valid address received | Write CMD |
| 0 | PREC | Stop detected on addressed transaction | Cleared on next AMATCH+CMD |

### Master
| Bit | Flag | Set condition | Clear condition |
|-----|------|---------------|-----------------|
| 7 | ERROR | Any error (see STATUS) | Write 1 |
| 1 | SB | Byte received in master read mode (no ARB lost) | Write ADDR, DATA, read DATA (smart), write CMD |
| 0 | MB | Byte transmitted in master write mode | Write ADDR, DATA, read DATA (smart), write CMD |

## STATUS Key Bits

### Slave
- **CLKHOLD** (7, read-only): slave holds SCL low; set when DRDY or AMATCH pending
- **DIR** (3): 0=master write in progress, 1=master read in progress
- **SR** (4): 1=repeated start on last AMATCH (valid only while INTFLAG.AMATCH=1)
- **RXNACK** (2): master responded with NACK to last byte sent by slave
- **COLL** (1): transmit collision (slave could not drive SDA high)
- **BUSERR** (0): illegal bus condition

### Master
- **BUSSTATE[1:0]** (5:4): 0x0=UNKNOWN, 0x1=IDLE, 0x2=OWNER, 0x3=BUSY
  - Force IDLE from UNKNOWN: write 0x1 to BUSSTATE (sets SYNCBUSY.SYSOP)
- **CLKHOLD** (7, read-only): master holds SCL low; set when SB or MB pending
- **RXNACK** (2): slave responded with NACK to last address or data sent
- **ARBLOST** (1): arbitration lost; INTFLAG.MB is set simultaneously
- **LENERR** (10): slave NACKed before ADDR.LEN bytes written (DMA mode)
- **BUSERR** (0): illegal bus condition; auto-cleared when ADDR is written

## SYNCBUSY

### Slave: ENABLE(1), SWRST(0)

### Master: SYSOP(2), ENABLE(1), SWRST(0)
- SYSOP is set when writing CTRLB.CMD, STATUS.BUSSTATE, ADDR.ADDR, or DATA

## ADDR Register Key Fields

### Slave ADDR (enable-protected)
- ADDR[10:1] = 7-bit address shifted left by 1 (or 10-bit address in bits 10:1)
- TENBITEN (15) = 1 for 10-bit addressing
- GENCEN (0) = 1 to respond to general call address (0x00)
- ADDRMASK[26:17] = address mask / second address / range limit per AMODE

### Master ADDR (write-synchronized → SYNCBUSY.SYSOP)
- ADDR[10:0]: 7-bit (bits 7:1 = address, bit 0 = R/W direction: 0=write, 1=read) or 10-bit
- TENBITEN (15): 1 for 10-bit addressing
- HS (14): enables high-speed for current transfer (repeated START to STOP)
- LENEN (13): enables automatic transfer length (must be 1 for DMA use)
- LEN[23:16]: transaction length in bytes for DMA (0–255)

Writing ADDR.ADDR triggers the actual I2C transaction start:
- UNKNOWN: sets MB+BUSERR
- IDLE: issues START condition
- BUSY: waits for bus to become IDLE
- OWNER: issues repeated START

## DMA Operation

Master with automatic length:
```c
// Set LEN and LENEN in ADDR before writing address
SERCOMn->I2CM.ADDR.reg =
    SERCOM_I2CM_ADDR_ADDR(slave_addr << 1)   // bit 0 = 0 for write
    | SERCOM_I2CM_ADDR_LENEN
    | SERCOM_I2CM_ADDR_LEN(byte_count);
// DMA writes DATA register byte_count times (TRIGACT=BEAT, TRIGSRC=SERCOMn_DMAC_ID_TX)
// Hardware auto-NACKs and issues STOP after LEN bytes
```

## Key Facts

- PAD[0]=SDA, PAD[1]=SCL — fixed, unlike SPI DOPO/DIPO.
- GCLK_SERCOM_SLOW shared across all SERCOM instances — one generator feeds all.
- Master SYNCBUSY.SYSOP: poll after any CTRLB.CMD, STATUS.BUSSTATE, ADDR, or DATA write.
- Slave SYNCBUSY: only ENABLE and SWRST — slave DATA is not write-synchronized.
- BUSSTATE=UNKNOWN after reset; must write 0x1 to force IDLE before first transaction.
- ACKACT=1 sends NACK; set before last byte in master read to terminate properly.
- SMEN=1 (smart mode): saves code in ISR — ACK is issued automatically on DATA read.
- ADDR bit 0 is the R/W flag: 0=write, 1=read (unlike 7-bit address convention).
- Writing ADDR clears STATUS.BUSERR, ARBLOST, INTFLAG.MB, INTFLAG.SB.
- LENERR (STATUS.10): slave NACKed before ADDR.LEN bytes transferred — check in DMA ISR.
- DBGSTOP=0 (reset default): baud-rate generator continues during debug halt.

## See Also

- [[SERCOM I2C Configuration]] — firmware init patterns and transfer functions
- [[I/O Multiplexing]] — SERCOM PAD to physical pin mapping
- [[DMA Controller]] — I2C master DMA transfer (LENEN, LEN, TRIGSRC)
- [[Clock System]] — GCLK_SERCOMx_CORE and GCLK_SERCOM_SLOW setup
