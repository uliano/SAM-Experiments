---
title: SAMC21 Datasheet Ch.13 DSU
type: source
tags: [dsu, device-service-unit, crc, chip-erase, mbist, debug, samc21, datasheet]
sources: [samc21-datasheet-ch13-dsu]
created: 2026-05-05
updated: 2026-05-05
---

# SAMC21 Datasheet Ch.13 DSU — Device Service Unit

Device Service Unit for the SAM C20/C21. Provides chip erase, CRC-32
calculation, memory built-in self-test (MBIST), debug communication channels,
and CoreSight ROM table.

## Abstract

The DSU is accessed over the AHB bus and provides device-level services used
primarily by debug tools and production test. The register space is split into
an internal range (0x0000–0x00FF) accessible without debug authentication, and
an external range (0x0100–0x01FF) that is restricted when the device is
protected (STATUSB.PROT=1).

## Key Facts

- Base address: 0x41002000 (APB-B Bridge).
- Internal range 0x0000–0x00FF: always accessible (CTRL, STATUSA/B, ADDR, LENGTH, DATA, DCC, DID).
- External range 0x0100–0x01FF: blocked when STATUSB.PROT=1 (CoreSight ROM table).
- CTRL is write-only and PAC Write-Protected (requires WRCTRL.KEY_CLEAR first).
- STATUSA/B are PAC Write-Protected.
- CRC-32: polynomial 0xEDB88320 (reversed/reflected). Write start address to ADDR, byte count to LENGTH, initial value (0xFFFFFFFF) to DATA, then write CTRL.CRC=1. Poll STATUSA.DONE=1. CRC result is in DATA.
- MBIST: March LR algorithm. Write CTRL.MBIST=1. STATUSA.DONE=1 when complete; STATUSA.FAIL=1 if failure detected.
- Chip Erase: CTRL.CE=1 (only from debugger; protected by SECCTRL). Erases Flash + User Row. STATUSA.DONE=1 on completion.
- Cold-Plugging: when debug adapter connects during reset, STATUSA.CRSTEXT=1 and CPU reset phase is extended until debugger writes STATUSA.CRSTEXT=1 to clear it.
- STATUSB.PROT: set at power-up when device is protected (BOOTPROT in NVM User Row). Never cleared except by a full Chip Erase.
- STATUSB.DBGPRES: set when a debug probe is detected. Never cleared.
- STATUSB.HPE=1: Hot-Plugging is enabled (SWCLK has not been repurposed).
- DID register: identifies exact device variant (PROCESSOR, FAMILY, SERIES, DIE, REVISION, DEVSEL).

## Register Summary

| Offset | Name | Key Fields |
|--------|------|-----------|
| 0x0000 | CTRL | CE(4), MBIST(3), CRC(2), SWRST(0) — write-only, PAC Write-Protected |
| 0x0001 | STATUSA | PERR(4), FAIL(3), BERR(2), CRSTEXT(1), DONE(0) — sticky, cleared by writing 1; PAC Write-Protected |
| 0x0002 | STATUSB | HPE(4), DCCD1(3), DCCD0(2), DBGPRES(1), PROT(0) — read-only; PAC Write-Protected |
| 0x0004 | ADDR | ADDR[31:2] word start address, AMOD[1:0] access mode (operation-dependent) — PAC Write-Protected |
| 0x0008 | LENGTH | LENGTH[31:2] length in words — PAC Write-Protected |
| 0x000C | DATA | DATA[31:0] — initial value for CRC or operation result — PAC Write-Protected |
| 0x0010 | DCC0 | DATA[31:0] — debug communication channel 0 (32-bit R/W) |
| 0x0014 | DCC1 | DATA[31:0] — debug communication channel 1 (32-bit R/W) |
| 0x0018 | DID | PROCESSOR[31:28], FAMILY[27:23], SERIES[21:16], DIE[15:12], REVISION[11:8], DEVSEL[7:0] — PAC Write-Protected |
| 0x1000 | ENTRY0 | ADDOFF[31:12], FMT(1)=1, EPRES(0) — CoreSight ROM table entry 0 |
| 0x1004 | ENTRY1 | ADDOFF[31:12], FMT(1)=1, EPRES(0) — CoreSight ROM table entry 1 |
| 0x1008 | END | END[31:0] — end marker for CoreSight ROM table |
| 0x1FCC | MEMTYPE | SMEMP(0) — system memory present (0 when protected) |
| 0x1FD0 | PID4 | FKBC[7:4], JEPCC[3:0] |
| 0x1FE0 | PID0 | PARTNBL[7:0] = 0xD0 (always, indicates DSU instance) |
| 0x1FE4 | PID1 | JEPIDCL[7:4]=0xF, PARTNBH[3:0]=0xC |
| 0x1FE8 | PID2 | REVISION[7:4], JEPU(3)=1, JEPIDCH[2:0]=0x1 |
| 0x1FEC | PID3 | REVAND[7:4]=0x0, CUSMOD[3:0]=0x0 |
| 0x1FF0 | CID0 | PREAMBLEB0[7:0] = 0x0D |
| 0x1FF4 | CID1 | CCLASS[7:4]=0x1 (ROM table), PREAMBLE[3:0]=0x0 |
| 0x1FF8 | CID2 | PREAMBLEB2[7:0] = 0x05 |
| 0x1FFC | CID3 | PREAMBLEB3[7:0] = 0xB1 |

## DID Register — Device Identification

Reading DSU->DID allows firmware to identify the exact device at runtime:

| Field | Bits | Description |
|-------|------|-------------|
| PROCESSOR | 31:28 | Processor type (4=Cortex-M0+) |
| FAMILY | 27:23 | Product family (SAM C) |
| SERIES | 21:16 | Product series (20/21) |
| DIE | 15:12 | Die family |
| REVISION | 11:8 | Die revision (0=rev.A, 1=rev.B, ...) |
| DEVSEL | 7:0 | Device selection (memory size, package variant) |

Note: device variant letter on the ordering code (e.g. ATSAMC21**J**18A-AU)
is **not** REVISION — it is encoded in DEVSEL. REVISION tracks silicon die revisions.

## Concepts Mentioned

- [[Memory Map]] — DSU base address in APB-B
- [[NVIC Interrupt Map]] — DSU has no dedicated IRQ; errors reported via PAC INTFLAGB.DSU

## See Also

- [[Memory Map]] — DSU base address
- [[SAMC21 Datasheet Ch.11 PAC]] — write-protection for DSU registers (INTFLAGB.DSU bit)
- [[SAMC21 Datasheet Ch.09 Memories]] — Flash protection bits in NVM User Row (BOOTPROT)
