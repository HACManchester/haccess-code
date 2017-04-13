/* esp<>stm protocol code
 *
 * Copyright 2017 Ben Dooks <ben@fluff.org>
 */

/* I2C protcol:
 *
 * read returns data from last written command
 * write is:
 * byte[0] = command
 *	bits 7..4 = major command number
 *	bits 3..0 = minor command info (depending on major command)
 *
 * bytes[1..n] = command data (if required)
 */

#define STM_I2C_ADDR	(0x10)	/**< IIC bus address for STM comms */

/* 16 main commands sent to the STM32 */

#define STM_CMD_INFO	(0x0)	/**< Returns info on STM32 */
#define STM_CMD_SET	(0x1)	/**< Set one or more GPIO lines */
#define STM_CMD_READ	(0x2)	/**< Read GPIO lines */
#define STM_CMD_WATCHDOG (0x3)	/**< Send watchdog heartbeat */
#define STM_CMD_DISPLAY	(0x4)	/**< Write data to attached display */
#define STM_CMD_TEST	(0xf)	/**< Test command, returns data sent */

/* sub-command info */


/* STM_CMD_INFO returns information on the system
 *
 * returns the following:
 * byte[0] = major revision
 * byte[1] = minor revision
 * byte[2] = length of information struct (incl revision)
 * byte[3] = board type
*/

struct stm_cmd_info {
	uint8_t		version_major;
	uint8_t		version_minor;
	uint8_t		info_size;
	uint8_t		board_type;
};

/* STM_CMD_SET takes an array of gpio commands, 1 byte per cmd as follows:
 *
 * byte[1..x] = gpio set information
 *	bits 0..2: STM_CMD_GPIO_xxx value (set0/set1/tristate)
 *	bits 7..3: GPIO number
 */
#define STM_CMD_GPIO_SET0	(0x0)	/**< Set GPIO value to output 0 */
#define STM_CMD_GPIO_SET1	(0x1)	/**< Set GPIO value to output 1 */
#define STM_CMD_GPIO_TRI	(0x2)	/**< Set GPIO value to trisate/in */

/* GPIO numbers
 */

#define STM_GPIO_SW_A		(0x00)	/**< GPIO for switch a */
#define STM_GPIO_SW_B		(0x01)	/**< GPIO for switch a */
#define STM_GPIO_OPTO_IN	(0x02)  /**< GPIO for opto-in/ext-in */
#define STM_GPIO_OPTO_OUT	(0x03)
#define STM_GPIO_LED_A		(0x04)
#define STM_GPIO_LED_B		(0x05)
#define STM_GPIO_LED_C		(0x06)

#define STM_NR_GPIO		(8)	/* max bits for gpios */


/* STM_CMD_WATCHDOG
 *
 * STM_WDOG_CMD_PET:
 * to stm:
 *	byte[1] = 0x39 (watchdog magic number)
 * from stm:
 *	result byte
 *
 * STM_WDOG_CMD_INFO
 * to stm:
 *	noting
 * from stm:
 *	struct stm_wdog_info
 */

#define STM_WDOG_CMD_INFO	(0x0)	/**< Get watchdog information */
#define STM_WDOG_CMD_PET	(0x1)	/**< Get watchdog information */
#define STM_WDOG_CMD_FIRE	(0x2)	/**< Fire `immediately` */

#define STM_CMD_WATCHDOG_MAGIC	(0x39)	/**< Watchdog magic number */

struct stm_wdog_state {
	uint8_t		active;
	uint8_t		unused;
	uint16_t	pets;		/**< Number of pets since start */
	uint16_t	timeout;	/**< Time to watchdog timeout */
	uint16_t	fired;		/**< Count of no of times fired */
	uint16_t	fire_time;	/**< Countdown during watchdog */
};

/* commands for display
 *
 */

#define STM_DISP_CMD_MOVE	(0x0)	/**< Move to x,y co-ordinate */
#define STM_DISP_CMD_WRITE	(0x1)	/**< Write data to display */
#define STM_DISP_CMD_CLEAR	(0x2)	/**< Clear the display */
#define STM_DISP_CMD_RAWWRITE	(0x3)	/**< Write raw data to the LCD */

/* MOVE:
 * in: buff[1] = x, buff[2] = y;
 */


/* return codes from commands that don't return any other state */

#define STM_RET_OK		(0x0)
#define STM_RET_ERR_LEN		(0x1)
#define STM_RET_ERR_ARG		(0x2)

/* board types */

#define BOARD_TYPE_PROTO_V2_STRIP	(1)
#define BOARD_TYPE_PROTO_V2_R1		(2)
