// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

#include <Wire.h>

#include "gpioexp.h"

static uint8_t olat_state;

void gpio_exp_wr(uint8_t reg, uint8_t val)
{
  Wire.beginTransmission(MCP_IICADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void gpio_exp_setgpio(unsigned gpio, bool to)
{
  uint8_t old_state = olat_state;

  if (gpio >= 8)
    return;

  if (to)
    olat_state |= 1 << gpio;
  else
    olat_state &= ~(1 << gpio);
  
  if (olat_state != old_state)
    gpio_exp_wr(MCP_OLAT, olat_state);
}

void gpio_exp_initout(uint8_t to)
{
  olat_state = to;
  gpio_exp_wr(MCP_OLAT, olat_state);
}

uint8_t gpio_exp_rd(uint8_t reg)
{
  uint8_t result;
  
  Wire.beginTransmission(MCP_IICADDR);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(MCP_IICADDR, 1);

  while (!Wire.available()) { }
  return Wire.read();
}

