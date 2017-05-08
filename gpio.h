/* GPIO handling */

/* uninified STM32/Arduino/EXP handling */

#define GPIO_BASE_STM32     (128)
#define GPIO_BASE_MCP       (64)

extern int gpio_read(unsigned int gpio);
extern void gpio_set(unsigned int gpio, unsigned int state);

static inline bool gpio_is_stm32(unsigned int gpio)
{
  return gpio >= GPIO_BASE_STM32;
}

static inline bool gpio_is_arduino(unsigned int gpio)
{
  return gpio < GPIO_BASE_MCP;
}

static inline bool gpio_is_mcp(unsigned int gpio)
{
  return gpio >= GPIO_BASE_MCP && gpio <= (GPIO_BASE_MCP + 7);
}

extern int stm32_gpio_read(unsigned int gpio);
extern int stm32_gpio_set(unsigned int gpio, unsigned int state);
extern int stm32_gpio_setdir(unsigned int gpio, unsigned int dir);
