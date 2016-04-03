// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

extern bool fs_opened;



/* GPIO mappings for the GPIO expander */

#define GPIO_EXP(__g)	((__g) + 32)

// gpio expander:
// gp0 = in_r
// gp1 = in_g
// gp2 = in_opto
// gp3 = 532_irq
// gp4 = out_reset_periph
// gp5 = out_lcd_dc
// gp6 = out_lcd_bl
// gp7 = out_opto

#define GPIO_IN_R	(GPIO_EXP(0))
#define GPIO_IN_G	(GPIO_EXP(1))
#define GPIO_IN_USER	(GPIO_EXP(2))
#define GPIO_IN_IRQ	(GPIO_EXP(3))
#define GPIO_OUT_RST	(GPIO_EXP(4))
#define GPIO_OUT_LCD_nCS (GPIO_EXP(5))
#define GPIO_OUT_OPTO	(GPIO_EXP(7))
