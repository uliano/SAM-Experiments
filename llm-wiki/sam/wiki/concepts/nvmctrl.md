---
title: NVMCTRL
type: concept
tags: [nvmctrl, flash, nvm, eeprom, firmware, samc21]
sources: [samc21-datasheet-ch27-nvmctrl]
created: 2026-05-05
updated: 2026-05-05
---

# NVMCTRL

Nonvolatile Memory Controller on the SAMC21. Manages Flash read wait states,
page erase/write operations, RWWEE (EEPROM emulation), and the NVM User Row.

## Wait State Init (Mandatory Before 48 MHz)

```cpp
// Must be called BEFORE switching OSC48M to full speed
NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_RWS(1);  // 1 wait state for 24–48 MHz
```

This is the single most critical NVMCTRL operation for bringup. The existing
`init.hpp` already does this. Zero wait states are valid only up to 24 MHz.

## Reading Calibration / User Row

Calibration space is memory-mapped — read with a simple pointer:

```cpp
// NVM Software Calibration at 0x806020
uint32_t cal = *((volatile uint32_t *)0x00806020);

// OSC48M calibration for VDD >= 3.6V
uint8_t cal48m = (cal >> 2) & 0x3F;    // bits 7:2

// WDT/BOD defaults at NVM User Row base
uint32_t userrow = *((volatile uint32_t *)0x00804000);
```

No commands are needed to read calibration or user row data.

## Row Erase + Page Write (Manual Mode, MANW=1)

```cpp
static inline void nvm_wait_ready(void) {
    while (!NVMCTRL->INTFLAG.bit.READY);
}

static inline void nvm_issue_cmd(uint8_t cmd) {
    nvm_wait_ready();
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY
                       | NVMCTRL_CTRLA_CMD(cmd);
}

void nvm_erase_row(uint32_t byte_addr) {
    nvm_wait_ready();
    NVMCTRL->ADDR.reg = byte_addr >> 1;   // 16-bit word address
    nvm_issue_cmd(NVMCTRL_CTRLA_CMD_ER);  // 0x02
}

void nvm_write_page(uint32_t byte_addr, const uint32_t *data) {
    // 1. Clear page buffer
    nvm_issue_cmd(NVMCTRL_CTRLA_CMD_PBC);  // 0x44
    // 2. Write 16 words (64 bytes) sequentially to NVM address space
    volatile uint32_t *p = (volatile uint32_t *)byte_addr;
    for (int i = 0; i < 16; i++) p[i] = data[i];
    // 3. Issue write command
    NVMCTRL->ADDR.reg = byte_addr >> 1;
    nvm_issue_cmd(NVMCTRL_CTRLA_CMD_WP);   // 0x04
    nvm_wait_ready();
}

// Complete erase + write of one 256-byte row
void nvm_write_row(uint32_t row_byte_addr, const uint32_t *data_64words) {
    nvm_erase_row(row_byte_addr);
    nvm_wait_ready();
    for (int page = 0; page < 4; page++)
        nvm_write_page(row_byte_addr + page * 64, data_64words + page * 16);
}
```

Always erase the entire row before writing any page in it.

## Status Check

```cpp
uint16_t status = NVMCTRL->STATUS.reg;
if (status & NVMCTRL_STATUS_PROGE)  { /* bad command / key error */ }
if (status & NVMCTRL_STATUS_LOCKE)  { /* tried to write locked region */ }
if (status & NVMCTRL_STATUS_NVME)   { /* NVM error */ }
// Clear by writing 1:
NVMCTRL->STATUS.reg = NVMCTRL_STATUS_PROGE | NVMCTRL_STATUS_LOCKE | NVMCTRL_STATUS_NVME;
```

## Memory Layout (SAMC21J18A, 256 KB)

| Range | Contents |
|-------|----------|
| 0x00000000–0x0003FFFF | Main Flash array (256 KB) |
| 0x00400000–... | RWWEE array (EEPROM emulation) |
| 0x00804000–0x00804007 | NVM User Row (64 bytes) |
| 0x00806020 | SW calibration (OSC48M, ADC) |

## Key Facts

- Wait states must be set before clock frequency increases, not after.
- CTRLB.MANW=1 (reset default): must issue WP command to commit page buffer.
- ADDR register uses 16-bit word addressing: `ADDR = byte_addr >> 1`.
- Page buffer boundary: 64-bit aligned sections; a write crossing a 64-bit
  boundary overwrites the other word in the same section with 0xFF. Write
  pages sequentially from start to avoid silent data corruption.
- STATUS.LOAD indicates page buffer is dirty; issue PBC before writing a
  partial page to ensure the rest is 0xFF.
- LOCK bit=1 = unlocked, LOCK bit=0 = locked (inverted logic).
- SSB (security bit) is irreversible without debugger chip-erase.

## See Also

- [[SAMC21 Datasheet Ch.27 NVMCTRL]] — full command and register reference
- [[Memory Map]] — NVM address spaces and User Row layout
- [[Clock System]] — wait state requirement before OSC48M at 48 MHz
- [[OSCCTRL]] — NVM calibration readout for OSC48M.CAL48M
