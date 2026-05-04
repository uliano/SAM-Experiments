#ifndef _PIN_HPP_
#define _PIN_HPP_

#include <stdint.h>
#include "samc21.h"

namespace sam
{
namespace gpio
{

enum class Bank : uint8_t
{
  A = 0,
  B = 1,
  C = 2,
};

enum class Peripheral : uint8_t
{
  A = 0,
  B = 1,
  C = 2,
  D = 3,
  E = 4,
  F = 5,
  G = 6,
  H = 7,
  I = 8,
};

enum class Pull : uint8_t
{
  None = 0,
  Up,
  Down,
};

enum class DriveStrength : uint8_t
{
  Normal = 0,
  Strong,
};

template <Bank bank_index, uint8_t pin_index>
struct Pin
{
  static_assert(pin_index < 32, "SAMC21 pin index must be < 32");
  static_assert((uint8_t)bank_index < PORT_GROUPS, "Invalid SAMC21 port group");

  static constexpr uint32_t mask = (1u << pin_index);

  static inline void set(void)
  {
    group().OUTSET.reg = mask;
  }

  static inline void clear(void)
  {
    group().OUTCLR.reg = mask;
  }

  static inline void toggle(void)
  {
    group().OUTTGL.reg = mask;
  }

  static inline void write(bool value)
  {
    if (value)
      set();
    else
      clear();
  }

  static inline bool read(void)
  {
    return (group().IN.reg & mask) != 0;
  }

  static inline bool is_output(void)
  {
    return (group().DIR.reg & mask) != 0;
  }

  static inline void set_input_enable(bool enable = true)
  {
    if (enable)
      group().PINCFG[pin_index].reg |= PORT_PINCFG_INEN;
    else
      group().PINCFG[pin_index].reg &= ~PORT_PINCFG_INEN;
  }

  static inline void set_drive_strength(DriveStrength strength = DriveStrength::Normal)
  {
    if (strength == DriveStrength::Strong)
      group().PINCFG[pin_index].reg |= PORT_PINCFG_DRVSTR;
    else
      group().PINCFG[pin_index].reg &= ~PORT_PINCFG_DRVSTR;
  }

  static inline void set_pull(Pull pull = Pull::None)
  {
    if (pull == Pull::None)
    {
      group().PINCFG[pin_index].reg &= ~PORT_PINCFG_PULLEN;
      return;
    }

    if (pull == Pull::Up)
      set();
    else
      clear();

    group().PINCFG[pin_index].reg |= PORT_PINCFG_PULLEN;
  }

  static inline void as_input(Pull pull = Pull::None, bool input_enable = true)
  {
    group().DIRCLR.reg = mask;
    set_input_enable(input_enable);
    set_pull(pull);
  }

  static inline void as_output(
      bool initial_level = false,
      DriveStrength strength = DriveStrength::Normal,
      bool input_enable = true)
  {
    write(initial_level);
    group().DIRSET.reg = mask;
    set_input_enable(input_enable);
    set_drive_strength(strength);
  }

  static inline void set_peripheral(
      Peripheral peripheral,
      bool input_enable = true,
      Pull pull = Pull::None,
      DriveStrength strength = DriveStrength::Normal)
  {
    set_input_enable(input_enable);
    set_pull(pull);
    set_drive_strength(strength);

    if (pin_index & 1u)
      group().PMUX[pin_index >> 1].bit.PMUXO = (uint8_t)peripheral;
    else
      group().PMUX[pin_index >> 1].bit.PMUXE = (uint8_t)peripheral;

    group().PINCFG[pin_index].reg |= PORT_PINCFG_PMUXEN;
  }

  static inline void clear_peripheral(void)
  {
    group().PINCFG[pin_index].reg &= ~PORT_PINCFG_PMUXEN;
  }

  static inline void set_sampled_input(bool sampled)
  {
    if (sampled)
      group().CTRL.reg |= mask;
    else
      group().CTRL.reg &= ~mask;
  }

private:
  static inline PortGroup& group(void)
  {
    return PORT->Group[(uint8_t)bank_index];
  }
};

} // namespace gpio
} // namespace sam

#endif // _PIN_HPP_
