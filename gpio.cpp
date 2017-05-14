/* gpio.cpp
 *  
 */

#include <Wire.h>
#include "Arduino.h"

#include "gpio.h"
#include "gpioexp.h"

int gpio_read(unsigned int gpio)
{
  int value = -1;

  if (gpio_is_arduino(gpio)) {
  } else if (gpio_is_mcp(gpio)) {
  } else if (gpio_is_stm32(gpio)) {
    stm32_gpio_read(gpio - GPIO_BASE_STM32);
  }

  return value;
}


extern void gpio_set(unsigned int gpio, unsigned int state)
{
  if (gpio_is_arduino(gpio)) {
    digitalWrite(gpio, state);
  } else if (gpio_is_mcp(gpio)) {
    gpio_exp_setgpio(gpio - GPIO_BASE_MCP, state);
  } else if (gpio_is_stm32(gpio)) {
    stm32_gpio_set(gpio - GPIO_BASE_STM32, state);
  }
}

extern void gpio_set_dir(unsigned int gpio, unsigned int dir)
{
  if (gpio_is_arduino(gpio)) {
    pinMode(gpio, dir);
  } else if (gpio_is_mcp(gpio)) {
    // do we need this?
  } else if (gpio_is_stm32(gpio)) {
    stm32_gpio_setdir(gpio - GPIO_BASE_STM32, dir);
  }
}

