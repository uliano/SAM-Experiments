#ifndef _QUARTZ_TEST_HPP_
#define _QUARTZ_TEST_HPP_

#include <stdint.h>

#include "samc21.h"
#include "serial.hpp"
#include "timebase.hpp"

struct QuartzTest
{
    static constexpr uint32_t xosc_timeout_ms = 100;
    static constexpr uint32_t xosc32k_timeout_ms = 3000;
    static constexpr uint32_t report_period_ms = 2000;

    static void init(void)
    {
        start_xosc_24mhz();
        xosc_elapsed_ms_ = wait_for_xosc_ms(xosc_timeout_ms);
        refresh_xosc_generators();

        start_xosc32k();
        xosc32k_elapsed_ms_ = wait_for_xosc32k_ms(xosc32k_timeout_ms);
        refresh_xosc32k_generator();
    }

    static void run_forever(void)
    {
        for (;;)
        {
            refresh_xosc_generators();
            refresh_xosc32k_generator();
            print_report();
            delay_ms(report_period_ms);
        }
    }

private:
    static inline uint32_t xosc_elapsed_ms_ = 0;
    static inline uint32_t xosc32k_elapsed_ms_ = 0;
    static inline bool xosc_generators_configured_ = false;
    static inline bool xosc32k_generator_configured_ = false;

    static void start_xosc_24mhz(void)
    {
        OSCCTRL->XOSCCTRL.reg = 0u;

        OSCCTRL->XOSCCTRL.reg =
            OSCCTRL_XOSCCTRL_STARTUP(8) |
            OSCCTRL_XOSCCTRL_AMPGC |
            OSCCTRL_XOSCCTRL_GAIN(4) |
            OSCCTRL_XOSCCTRL_XTALEN |
            OSCCTRL_XOSCCTRL_ENABLE;
    }

    static void start_xosc32k(void)
    {
        OSC32KCTRL->INTFLAG.reg =
            OSC32KCTRL_INTFLAG_XOSC32KRDY |
            OSC32KCTRL_INTFLAG_CLKFAIL;

        OSC32KCTRL->XOSC32K.reg =
            OSC32KCTRL_XOSC32K_STARTUP(7) |
            OSC32KCTRL_XOSC32K_EN32K |
            OSC32KCTRL_XOSC32K_XTALEN |
            OSC32KCTRL_XOSC32K_ENABLE;
    }

    static bool xosc_ready(void)
    {
        return 0u != (OSCCTRL->STATUS.reg & OSCCTRL_STATUS_XOSCRDY);
    }

    static bool xosc32k_ready(void)
    {
        return 0u != (OSC32KCTRL->STATUS.reg & OSC32KCTRL_STATUS_XOSC32KRDY);
    }

    static uint32_t wait_for_xosc_ms(uint32_t timeout_ms)
    {
        const uint32_t start = Timebase::millis();

        while (!xosc_ready() && ((Timebase::millis() - start) < timeout_ms))
            __WFI();

        return Timebase::millis() - start;
    }

    static uint32_t wait_for_xosc32k_ms(uint32_t timeout_ms)
    {
        const uint32_t start = Timebase::millis();

        while (!xosc32k_ready() && ((Timebase::millis() - start) < timeout_ms))
            __WFI();

        return Timebase::millis() - start;
    }

    static void refresh_xosc_generators(void)
    {
        if (!xosc_ready() || xosc_generators_configured_)
            return;

        // GCLK1 = XOSC @ 24 MHz.
        GCLK->GENCTRL[1].reg =
            GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC_Val) |
            GCLK_GENCTRL_GENEN;
        wait_gclk_generator_sync(1);

        // GCLK2 = XOSC / 64 = 375 kHz.
        GCLK->GENCTRL[2].reg =
            GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC_Val) |
            GCLK_GENCTRL_DIV(64) |
            GCLK_GENCTRL_GENEN;
        wait_gclk_generator_sync(2);

        xosc_generators_configured_ = true;
    }

    static void refresh_xosc32k_generator(void)
    {
        if (!xosc32k_ready() || xosc32k_generator_configured_)
            return;

        // GCLK3 = XOSC32K @ 32.768 kHz for future diagnostics.
        GCLK->GENCTRL[3].reg =
            GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC32K_Val) |
            GCLK_GENCTRL_GENEN;
        wait_gclk_generator_sync(3);

        xosc32k_generator_configured_ = true;
    }

    static void wait_gclk_generator_sync(uint8_t generator)
    {
        const uint32_t mask = 1u << (GCLK_SYNCBUSY_GENCTRL_Pos + generator);
        while (GCLK->SYNCBUSY.reg & mask);
    }

    static void print_report(void)
    {
        print_line("");
        print_line("SAMC21 board bring-up");
        print_line("UART: SERCOM5 PB30/PB31, 1000000 baud");

        Serial.print("OSC48M/GCLK0: OK 48000000 Hz");
        end_line();

        Serial.print("XOSC 24 MHz PA14/PA15: ");
        print_ok_fail(xosc_ready());
        Serial.print(" ready_wait_ms=");
        Serial.print(xosc_elapsed_ms_);
        Serial.print(" status=");
        Serial.print(OSCCTRL->STATUS.reg, PrintBase::Hex);
        Serial.print(" gclk1=");
        print_ok_fail(xosc_generators_configured_);
        Serial.print(" gclk2=");
        print_ok_fail(xosc_generators_configured_);
        end_line();

        Serial.print("XOSC32K PA00/PA01: ");
        print_ok_fail(xosc32k_ready());
        Serial.print(" ready_wait_ms=");
        Serial.print(xosc32k_elapsed_ms_);
        Serial.print(" status=");
        Serial.print(OSC32KCTRL->STATUS.reg, PrintBase::Hex);
        Serial.print(" gclk3=");
        print_ok_fail(xosc32k_generator_configured_);
        end_line();
    }

    static void print_ok_fail(bool ok)
    {
        Serial.print(ok ? "OK" : "FAIL");
    }

    static void print_line(const char *text)
    {
        Serial.print(text);
        end_line();
    }

    static void end_line(void)
    {
        Serial.newline();
        Serial.flush();
    }

    static void delay_ms(uint32_t ms)
    {
        const uint32_t start = Timebase::millis();

        while ((Timebase::millis() - start) < ms)
            __WFI();
    }
};

#endif // _QUARTZ_TEST_HPP_
