/* build configuration */

#define NFC_ELECHOUSE
#define GPIO_STM32_V2
//#define USE_PCD8544
//#define USE_MCP

#include "gpio.h"
#define GPIO_ESP(__x)     (__x)
#define GPIO_STM32(__g) ((__g) + GPIO_BASE_STM32)

#ifdef GPIO_STM32_V2
#define setup_gpioexp setup_gpioexp_stm32
#define wdt_pet stm32_pet_watchdog
#else
#define setup_gpioexp setup_gpioexp_mcp
#define wdt_pet no_watchdog_pet
#endif


#ifdef GPIO_MCP
#define GPIO_IN_R (GPIO_EXP(0))
#define GPIO_IN_G (GPIO_EXP(1))
#define GPIO_IN_USER  (GPIO_EXP(2))
#define GPIO_IN_IRQ (GPIO_EXP(3))
#define GPIO_OUT_RST  (GPIO_EXP(4))
#define GPIO_OUT_LCD_nCS (GPIO_EXP(5))
#define GPIO_OUT_LCD_BL (GPIP_EXP(6))
#define GPIO_OUT_OPTO (GPIO_EXP(7))
#endif

#ifdef GPIO_STM32_V2
#include "stm_protocol.h"
#define GPIO_RGB_LED      GPIO_ESP(15)
#define GPIO_IN_R         GPIO_STM32(STM_GPIO_SW_A)
#define GPIO_IN_G         GPIO_STM32(STM_GPIO_SW_B)
#define GPIO_IN_USER      GPIO_STM32(STM_GPIO_OPTO_IN)
#define GPIO_IN_IRQ       -1
#define GPIO_OUT_RST      -1
//#define GPIO_OUT_LCD_nCS
#define GPIO_OUT_BL       -1
#define GPIO_OUT_OPTO     GPIO_STM32(STM_GPIO_OPTO_OUT)
#endif

/* GPIO mappings for the GPIO expander */

#define GPIO_EXP(__g)  ((__g) + GPIO_BASE_MCP)

// gpio expander:
// gp0 = in_r
// gp1 = in_g
// gp2 = in_opto
// gp3 = 532_irq
// gp4 = out_reset_periph
// gp5 = out_lcd_dc
// gp6 = out_lcd_bl
// gp7 = out_opto

#ifndef GPIO_STM32_V2
#define GPIO_IN_R (GPIO_EXP(0))
#define GPIO_IN_G (GPIO_EXP(1))
#define GPIO_IN_USER  (GPIO_EXP(2))
#define GPIO_IN_IRQ (GPIO_EXP(3))
#define GPIO_OUT_RST  (GPIO_EXP(4))
#define GPIO_OUT_LCD_nCS (GPIO_EXP(5))
#define GPIO_OUT_OPTO (GPIO_EXP(7))
#endif
