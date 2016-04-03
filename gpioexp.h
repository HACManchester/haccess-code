// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

#define MCP_IICADDR	(0x20)

#define MCP_IODIR   (0x00)
#define MCP_IPOL    (0x01)
#define MCP_GPINTEN (0x02)
#define MCP_DEFVAL  (0x03)
#define MCP_INTCON  (0x04)
#define MCP_IOCON   (0x05)
#define MCP_GPPU    (0x06)
#define MCP_INTF    (0x07)
#define MCP_INTCAP  (0x08)
#define MCP_GPIO    (0x09)
#define MCP_OLAT    (0x0A)

extern "C" {
extern void gpio_exp_setgpio(unsigned reg, bool to);
};

extern void gpio_exp_initout(uint8_t to);
extern void gpio_exp_wr(uint8_t reg, uint8_t val);
extern uint8_t gpio_exp_rd(uint8_t reg);

