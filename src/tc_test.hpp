#ifndef _TC_TEST_HPP_
#define _TC_TEST_HPP_

#include <stdint.h>
#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"
#include "bytestream.hpp"

namespace TcTest {

static inline void delay_ms(uint32_t ms)
{
  uint32_t start = Timebase::millis();
  while ((Timebase::millis() - start) < ms)
    ;
}

static inline void wait_tc_sync(Tc* tc, uint32_t mask)
{
  while (tc->COUNT32.SYNCBUSY.reg & mask)
    ;
}

static inline void enable_tc_clocks(void)
{
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC0 |
                        MCLK_APBCMASK_TC1 |
                        MCLK_APBCMASK_TC2 |
                        MCLK_APBCMASK_TC3;
}

static inline void attach_tc_clock(uint32_t gclk_id)
{
  GCLK->PCHCTRL[gclk_id].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[gclk_id].reg & GCLK_PCHCTRL_CHEN))
    ;
}

static inline void reset_tc(Tc* tc)
{
  tc->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
  while (tc->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
    ;
}

static inline void setup_tc32(Tc* tc)
{
  reset_tc(tc);
  tc->COUNT32.COUNT.reg = 0;
  wait_tc_sync(tc, TC_SYNCBUSY_COUNT);
  tc->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1024;
  wait_tc_sync(tc, TC_SYNCBUSY_COUNT);
  tc->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
  wait_tc_sync(tc, TC_SYNCBUSY_ENABLE);
}

static inline uint32_t read_tc32(Tc* tc)
{
  tc->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
  wait_tc_sync(tc, TC_SYNCBUSY_COUNT);
  return tc->COUNT32.COUNT.reg;
}

static inline void print_uint(const char* label, uint32_t value)
{
  Serial.print(label);
  Serial.print(value);
  Serial.newline();
}


static inline void run(void)
{
  Serial.print("TC 32-bit experiment");
  Serial.newline();
  Serial.print("GCLK0 -> TC0/TC1/TC2/TC3");
  Serial.newline();

  enable_tc_clocks();

  // Run TC0+TC1 test
  Serial.print("\r\n");
  Serial.print("TC0+TC1");
  Serial.print(" 32-bit test");
  Serial.newline();

  attach_tc_clock(TC0_GCLK_ID);
  attach_tc_clock(TC1_GCLK_ID);
  setup_tc32(TC0);
  setup_tc32(TC1);

  delay_ms(1600);  // ~1.6s to reach > 65535 counts (16-bit max)

  uint32_t tc01_master = read_tc32(TC0);
  uint32_t tc01_slave = read_tc32(TC1);

  Serial.print("TC0+TC1");
  Serial.print(" master count=");
  Serial.print(tc01_master);
  Serial.print(" (0x");
  Serial.print(tc01_master, PrintBase::Hex);
  Serial.print(") slave count=");
  Serial.print(tc01_slave);
  Serial.print(" (0x");
  Serial.print(tc01_slave, PrintBase::Hex);
  Serial.print(")");
  Serial.newline();

  // Run TC2+TC3 test
  Serial.print("\r\n");
  Serial.print("TC2+TC3");
  Serial.print(" 32-bit test");
  Serial.newline();

  attach_tc_clock(TC2_GCLK_ID);
  attach_tc_clock(TC3_GCLK_ID);
  setup_tc32(TC2);
  setup_tc32(TC3);

  delay_ms(1600);  // ~1.6s to reach > 65535 counts (16-bit max)

  uint32_t tc23_master = read_tc32(TC2);
  uint32_t tc23_slave = read_tc32(TC3);

  Serial.print("TC2+TC3");
  Serial.print(" master count=");
  Serial.print(tc23_master);
  Serial.print(" (0x");
  Serial.print(tc23_master, PrintBase::Hex);
  Serial.print(") slave count=");
  Serial.print(tc23_slave);
  Serial.print(" (0x");
  Serial.print(tc23_slave, PrintBase::Hex);
  Serial.print(")");
  Serial.newline();

  // Compare results - expect ~75,000 counts after 1.6s (well beyond 16-bit max of 65535)
  uint32_t expected_min = 65000;
  uint32_t expected_max = 90000;  // Extra margin for timing variations
  bool tc01_master_ok = (tc01_master >= expected_min && tc01_master <= expected_max);
  bool tc01_slave_ok = (tc01_slave >= expected_min && tc01_slave <= expected_max);
  bool tc23_master_ok = (tc23_master >= expected_min && tc23_master <= expected_max);
  bool tc23_slave_ok = (tc23_slave >= expected_min && tc23_slave <= expected_max);

  Serial.print("\r\nComparison:");
  Serial.newline();
  Serial.print("TC0+TC1: master=");
  Serial.print(tc01_master_ok ? "OK" : "FAIL");
  Serial.print(" slave=");
  Serial.print(tc01_slave_ok ? "OK" : "FAIL");
  Serial.newline();
  Serial.print("TC2+TC3: master=");
  Serial.print(tc23_master_ok ? "OK" : "FAIL");
  Serial.print(" slave=");
  Serial.print(tc23_slave_ok ? "OK" : "FAIL");
  Serial.newline();

  if (tc01_master_ok && tc01_slave_ok && tc23_master_ok && tc23_slave_ok) {
    Serial.print("RESULT: Both TC pairs support 32-bit mode!");
    Serial.newline();
  } else {
    Serial.print("RESULT: Some TC pairs failed - check hardware support");
    Serial.newline();
  }

  Serial.print("Experiment complete");
  Serial.newline();
}

} // namespace TcTest

#endif // _TC_TEST_HPP_
