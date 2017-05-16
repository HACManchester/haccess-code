/* stm32 gpio/general support
 *  (c) 2017 Ben Dooks
 */
#include <Wire.h>
#include "Arduino.h"

#include "gpio.h"
#include "stm_protocol.h"
#include "stm32_support.h"

#define CMD(__cmd, __subcmd) (((__cmd) << 4) | (__subcmd))

static unsigned int i2c_fails = 0;

// issue a command to stm32
static int stm32_cmd(unsigned cmd, uint8_t *wr, unsigned int wr_size, uint8_t *rd, unsigned read_size)
{
  unsigned int ptr;
  int ret;

  if (i2c_fails > 5)
    return -1;
  
  Wire.beginTransmission(STM_I2C_ADDR);
  Wire.write(cmd);
  
  if (wr_size) {
    for (ptr = 0; ptr < wr_size; ptr++)
       Wire.write(wr[ptr]);
  }
  
  ret = Wire.endTransmission();
  if (ret != 0) {
    Serial.print("Failed talking to STM during read (write command), ret=");
    Serial.println(ret);
    i2c_fails++;
    return -1;   
  }

  delay(1);
  
  if (read_size) {
    Wire.requestFrom(STM_I2C_ADDR, read_size);

    for (ptr = 0 ; ptr < read_size; ptr++)
      rd[ptr] = Wire.read();

    ret = Wire.endTransmission();
    if (ret != 0) {
      Serial.print("Failed talking to STM during read (end data), ret=");
      Serial.println(ret);
      i2c_fails++;
      return -1;
    }
  }

  delay(1);
  return 0;
}

static unsigned long last_read = -1;
static unsigned char gpio_last_get[4];
static const bool dbg_gpio_read = false;

extern int stm32_gpio_read(unsigned int gpio)
{
  unsigned int byt = gpio >> 3;
  unsigned int off = gpio & 7;
  int ret;

  if (last_read != millis()) {
    ret = stm32_cmd(CMD(STM_CMD_READ, 0), NULL, 0, gpio_last_get, 4);
    if (ret < 0)
      return ret;
    last_read = millis();
    if (dbg_gpio_read) {
      Serial.print("GPIO-RD: ");
      Serial.print(gpio_last_get[0]);
      Serial.print(gpio_last_get[1]);
      Serial.print(gpio_last_get[2]);
      Serial.print(gpio_last_get[3]);
      Serial.println(".");
    }
  }

  return (gpio_last_get[byt] >> off) & 1;
}

extern int stm32_gpio_set(unsigned int gpio, unsigned int state)
{
  uint8_t data;
  int ret;
  
  data = (gpio << 3) | (state ? 1 : 0);
  ret = stm32_cmd(CMD(STM_CMD_SET, 0), &data, 1, NULL, 0);
  return ret;
}

int stm32_gpio_setdir(unsigned int gpio, unsigned int dir)
{
}

void setup_gpioexp_stm32(void)
{
  uint8_t data[16];
  int sz;
  int ret;

  Serial.println("starting stm32 gpio code");

  ret = stm32_cmd(CMD(0, 0), NULL, 0, data, 3);
  if (ret < 0) {
    Serial.println("Failed to find stm32 info");
  } else {
    sz = data[2];
    if (sz > sizeof(data)) {
       Serial.println("truncating info buffer size");
       sz = sizeof(data);
    }
      
    ret = stm32_cmd(CMD(0,0), NULL, 0, data, sz);
    if (ret) {
      Serial.println("failed to read all stm32 info");
    } else {
      Serial.println("Found stm32 info:");
      Serial.print("Major version "); Serial.println(data[0]);
      Serial.print("Minor version "); Serial.println(data[1]);
    }
  }
}

void stm32_watchdog_pet(void)
{
  unsigned char data = STM_CMD_WATCHDOG_MAGIC;
  stm32_cmd(CMD(STM_CMD_WATCHDOG, STM_WDOG_CMD_PET), &data, 1, NULL, 0);
}


