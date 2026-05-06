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

static inline void wait_sync(Tc* tc, uint32_t mask)
{
  while (tc->COUNT32.SYNCBUSY.reg & mask)
    ;
}

static inline void reset_tc(Tc* tc)
{
  tc->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
  while (tc->COUNT32.SYNCBUSY.reg & TC_SYNCBUSY_SWRST)
    ;
}

// Configure master TC only. Slave is controlled automatically by the hardware.
static inline void setup_tc32_master(Tc* tc)
{
  reset_tc(tc);
  // MODE and PRESCALER are enable-protected: write before setting ENABLE
  tc->COUNT32.CTRLA.reg = TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER_DIV1024;
  tc->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
  wait_sync(tc, TC_SYNCBUSY_ENABLE);
}

static inline uint32_t read_tc32(Tc* tc)
{
  tc->COUNT32.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
  wait_sync(tc, TC_SYNCBUSY_COUNT);
  return tc->COUNT32.COUNT.reg;
}

// TC0_GCLK_ID == TC1_GCLK_ID == 30; TC2_GCLK_ID == TC3_GCLK_ID == 31
// Only the master GCLK channel needs to be enabled.
static inline void test_pair(const char* name, Tc* master, Tc* slave, uint32_t gclk_id)
{
  Serial.print("\r\n");
  Serial.print(name);
  Serial.print(" 32-bit test");
  Serial.newline();

  GCLK->PCHCTRL[gclk_id].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[gclk_id].reg & GCLK_PCHCTRL_CHEN))
    ;

  setup_tc32_master(master);

  // STATUS.SLAVE is set by hardware on the slave when master enters COUNT32 mode
  bool slave_paired = (slave->COUNT32.STATUS.reg & TC_STATUS_SLAVE) != 0;
  Serial.print("slave STATUS.SLAVE=");
  Serial.print(slave_paired ? "1 (paired OK)" : "0 (NOT paired)");
  Serial.newline();

  // DIV1024 @ 48 MHz = 46875 Hz; after 1.6 s expect ~75000 counts (> 65535 = 16-bit max)
  delay_ms(1600);

  uint32_t count = read_tc32(master);
  Serial.print("master count=");
  Serial.print(count);
  Serial.print(" (0x");
  Serial.print(count, PrintBase::Hex);
  Serial.print(")");
  Serial.newline();

  bool ok = count > 65535u;
  Serial.print("RESULT: ");
  Serial.print(ok ? "OK - count exceeds 16-bit max, 32-bit mode confirmed"
                  : "FAIL - count within 16-bit range");
  Serial.newline();

  reset_tc(master);
}

static inline void run(void)
{
  Serial.print("TC 32-bit experiment");
  Serial.newline();
  Serial.print("Master-only config; slave pairs automatically");
  Serial.newline();

  // Both master and slave need APB clocks; only master needs GCLK
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC0 | MCLK_APBCMASK_TC1 |
                         MCLK_APBCMASK_TC2 | MCLK_APBCMASK_TC3;

  test_pair("TC0+TC1", TC0, TC1, TC0_GCLK_ID);
  test_pair("TC2+TC3", TC2, TC3, TC2_GCLK_ID);

  Serial.print("\r\nExperiment complete");
  Serial.newline();
}

} // namespace TcTest

#endif // _TC_TEST_HPP_
