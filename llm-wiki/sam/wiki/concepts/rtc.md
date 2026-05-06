---
title: RTC
type: concept
tags: [rtc, timer, calendar, alarm, firmware, samc21]
sources: [samc21-datasheet-ch24-rtc]
created: 2026-05-05
updated: 2026-05-05
---

# RTC

Real-Time Counter on the SAMC21. Three modes: COUNT32 (32-bit free counter),
COUNT16 (16-bit counter with period), and CLOCK/Calendar (packed date+time).
Clocked from OSCULP32K or XOSC32K, independent of CPU clock. Survives all
resets except POR and SWRST.

## Clock and APB Setup

```cpp
// APB clock (APBA, not APBB)
MCLK->APBAMASK.reg |= MCLK_APBAMASK_RTC;

// Clock source: XOSC1K (1024 Hz from XOSC32K) for accurate 1 Hz calendar
// Set in OSC32KCTRL (must also enable XOSC32K with EN1K=1)
OSC32KCTRL->RTCCTRL.reg = OSC32KCTRL_RTCCTRL_RTCSEL_XOSC1K;
// For lower power (less accurate): use RTCSEL_ULP1K (OSCULP32K/1024)
```

## Mode 2 (Calendar) Init — 1 Hz Tick

```cpp
void rtc_calendar_init(void) {
    // 1. Software reset
    RTC->MODE2.CTRLA.reg = RTC_MODE2_CTRLA_SWRST;
    while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_SWRST);

    // 2. Configure: MODE=2, PRESCALER=DIV1024, 24h, no read-sync yet
    RTC->MODE2.CTRLA.reg = RTC_MODE2_CTRLA_MODE(2)
                          | RTC_MODE2_CTRLA_PRESCALER_DIV1024
                          | RTC_MODE2_CTRLA_CLOCKSYNC;  // enable read sync
    while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_CLOCKSYNC);

    // 3. Set initial date/time (YEAR is offset from reference year)
    // Example: 2026-01-01 00:00:00 with reference year = 2000 → YEAR=26
    RTC->MODE2.CLOCK.reg = RTC_MODE2_CLOCK_YEAR(26)
                          | RTC_MODE2_CLOCK_MONTH(1)
                          | RTC_MODE2_CLOCK_DAY(1)
                          | RTC_MODE2_CLOCK_HOUR(0)
                          | RTC_MODE2_CLOCK_MINUTE(0)
                          | RTC_MODE2_CLOCK_SECOND(0);
    while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_CLOCK);

    // 4. Enable
    RTC->MODE2.CTRLA.reg |= RTC_MODE2_CTRLA_ENABLE;
    while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_ENABLE);
}
```

## Reading the Calendar

CLOCKSYNC=1 must be set before reading CLOCK. Each read takes ~2 RTC clock
cycles of latency (wait SYNCBUSY.CLOCK):

```cpp
RTC_MODE2_CLOCK_Type read_rtc_clock(void) {
    while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_CLOCK);
    return RTC->MODE2.CLOCK;
}

// Unpack fields:
// clock.bit.YEAR   (0–63, add reference year for real year)
// clock.bit.MONTH  (1–12)
// clock.bit.DAY    (1–31)
// clock.bit.HOUR   (0–23 in 24h mode)
// clock.bit.MINUTE (0–59)
// clock.bit.SECOND (0–59)
```

## Mode 2 Alarm

```cpp
void rtc_set_alarm_hourly(void) {
    // Fire every hour at :00:00
    RTC->MODE2.ALARM[0].reg = RTC_MODE2_ALARM_MINUTE(0)
                            | RTC_MODE2_ALARM_SECOND(0);
    while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_ALARM0);

    // Match seconds + minutes + hours (fires at exact H:00:00 each day)
    // Use MMSS (0x2) to fire at :00:00 of every hour
    RTC->MODE2.MASK[0].reg = RTC_MODE2_MASK_SEL(0x2);  // MMSS
    while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_MASK0);

    RTC->MODE2.INTENSET.reg = RTC_MODE2_INTENSET_ALARM0;
    NVIC_EnableIRQ(RTC_IRQn);
}

void RTC_Handler(void) {
    if (RTC->MODE2.INTFLAG.reg & RTC_MODE2_INTFLAG_ALARM0) {
        RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_ALARM0;  // w1c
        // handle alarm
    }
}
```

## MASK.SEL Values (Mode 2)

| SEL | Name | Matches |
|-----|------|---------|
| 0x0 | OFF | Alarm disabled |
| 0x1 | SS | Seconds only |
| 0x2 | MMSS | Seconds + minutes |
| 0x3 | HHMMSS | Seconds + minutes + hours |
| 0x4 | DDHHMMSS | + Day |
| 0x5 | MMDDHHMMSS | + Month |
| 0x6 | YYMMDDHHMMSS | + Year (full exact match) |

## Mode 0 (COUNT32) Init — 1 Hz Tick

```cpp
void rtc_count32_init(void) {
    RTC->MODE0.CTRLA.reg = RTC_MODE0_CTRLA_SWRST;
    while (RTC->MODE0.SYNCBUSY.reg & RTC_MODE0_SYNCBUSY_SWRST);

    RTC->MODE0.CTRLA.reg = RTC_MODE0_CTRLA_MODE(0)
                          | RTC_MODE0_CTRLA_PRESCALER_DIV1024
                          | RTC_MODE0_CTRLA_COUNTSYNC;
    while (RTC->MODE0.SYNCBUSY.reg & RTC_MODE0_SYNCBUSY_COUNTSYNC);

    RTC->MODE0.CTRLA.reg |= RTC_MODE0_CTRLA_ENABLE;
    while (RTC->MODE0.SYNCBUSY.reg & RTC_MODE0_SYNCBUSY_ENABLE);
}

uint32_t rtc_read_count(void) {
    while (RTC->MODE0.SYNCBUSY.reg & RTC_MODE0_SYNCBUSY_COUNT);
    return RTC->MODE0.COUNT.reg;
}
```

## Mode 1 (COUNT16) Init — Periodic Wakeup at 8 Hz

```cpp
void rtc_count16_init(void) {
    RTC->MODE1.CTRLA.reg = RTC_MODE1_CTRLA_SWRST;
    while (RTC->MODE1.SYNCBUSY.reg & RTC_MODE1_SYNCBUSY_SWRST);

    // 1024 Hz source, DIV1 prescaler → 1024 Hz counter
    RTC->MODE1.CTRLA.reg = RTC_MODE1_CTRLA_MODE(1)
                          | RTC_MODE1_CTRLA_PRESCALER_DIV1;
    // Period = 128 counts → wraps at 128/1024 Hz = 8 Hz
    RTC->MODE1.PER.reg = 128 - 1;
    while (RTC->MODE1.SYNCBUSY.reg & RTC_MODE1_SYNCBUSY_PER);

    RTC->MODE1.INTENSET.reg = RTC_MODE1_INTENSET_OVF;
    NVIC_EnableIRQ(RTC_IRQn);

    RTC->MODE1.CTRLA.reg |= RTC_MODE1_CTRLA_ENABLE;
    while (RTC->MODE1.SYNCBUSY.reg & RTC_MODE1_SYNCBUSY_ENABLE);
}
```

## Periodic Interval Interrupts (PERn)

Available in all modes. PERn fires on the 0-to-1 transition of prescaler bit [n+2],
giving a rate of CLK_RTC / 2^(n+2):

| Interrupt | Prescaler bit | Rate at 1024 Hz input |
|-----------|--------------|----------------------|
| PER0 | bit 2 | 256 Hz (3.9 ms) |
| PER1 | bit 3 | 128 Hz (7.8 ms) |
| PER2 | bit 4 | 64 Hz (15.6 ms) |
| PER3 | bit 5 | 32 Hz (31.3 ms) |
| PER4 | bit 6 | 16 Hz (62.5 ms) |
| PER5 | bit 7 | 8 Hz (125 ms) |
| PER6 | bit 8 | 4 Hz (250 ms) |
| PER7 | bit 9 | 2 Hz (500 ms) |

## FREQCORR — Frequency Correction

```cpp
// Slow RTC by 10 steps (SIGN=0 → decrease frequency)
RTC->MODE2.FREQCORR.reg = RTC_FREQCORR_VALUE(10);  // SIGN=0
// Speed up RTC by 5 steps
RTC->MODE2.FREQCORR.reg = RTC_FREQCORR_SIGN | RTC_FREQCORR_VALUE(5);
while (RTC->MODE2.SYNCBUSY.reg & RTC_MODE2_SYNCBUSY_FREQCORR);
```

## Key Facts

- APB bus: APBA — enable with `MCLK->APBAMASK.reg |= MCLK_APBAMASK_RTC`.
- RTC persists through WDT/EXT/SYST resets; cleared only by POR or SWRST.
- CLOCKSYNC=1 (Mode 2) / COUNTSYNC=1 (Mode 0/1): required for read sync; adds ~2 RTC clocks of latency.
- All write-synchronized registers: poll SYNCBUSY after every write before next access.
- YEAR[1:0]=0 is treated as a leap year — base your reference year accordingly (e.g. 2000).
- DBGRUN=0 (reset default): RTC halts when debugger halts CPU; set DBGRUN=1 to let it free-run.
- DBGRUN is NOT cleared by SWRST — survives soft resets.
- FREQCORR.SIGN=0 → slows RTC (positive correction offset); SIGN=1 → speeds up.
- MATCHCLR=1 in Mode 2 resets CLOCK to zero on alarm match — not useful for calendar.

## See Also

- [[SAMC21 Datasheet Ch.24 RTC]] — full register reference for all three modes
- [[OSC32KCTRL]] — RTCCTRL.RTCSEL clock source configuration, XOSC32K EN1K
- [[Clock System]] — GCLK_RTC source routing if using RTCSEL=GCLK_RTC
- [[NVIC Interrupt Map]] — RTC IRQn
- [[PM]] — RTC as wakeup source from STANDBY
