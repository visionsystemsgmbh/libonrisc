#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <libudev.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Linux header */
#include <linux/sockios.h>
#include <linux/mii.h>
#include <linux/ethtool.h>

#include "onrisc.h"

/* driver−specific ioctls : */
#define TIOCGRS485 0x542E
#define TIOCSRS485 0x542F

#define TIOCGEPLD	0x5470
#define TIOCSEPLD	0x5471

//! @name EPLD modes
//@{
#define EPLD_PORTOFF					0x05  //!< turn the drivers off
#define CAP_EPLD_PORTOFF				0x0001  //!< turn the drivers off capability

#define EPLD_RS232					0x03  //!< RS232 mode
#define CAP_EPLD_RS232					0x0002  //!< RS232 mode capability

#define EPLD_RS422					0x01  //!< RS422 mode
#define CAP_EPLD_RS422					0x0004  //!< RS422 mode capability

#define EPLD_RS485_ART_4W				0x08  //!< RS485 4 wire direction control by ART
#define CAP_EPLD_RS485_ART_4W				0x0008  //!< RS485 4 wire direction control by ART capability

#define EPLD_RS485_ART_2W				0x0C  //!< RS485 2 wire direction control by ART
#define CAP_EPLD_RS485_ART_2W				0x0010  //!< RS485 2 wire direction control by ART capability

#define EPLD_RS485_ART_ECHO				0x04  //!< RS485 2 wire direction control by ART with echo
#define CAP_EPLD_RS485_ART_ECHO				0x0020  //!< RS485 2 wire direction control by ART with echo capability

#define EPLD_RS485_RTS_4W				0x0A  //!< RS485 4 wire direction control by RTS
#define CAP_EPLD_RS485_RTS_4W				0x0040  //!< RS485 4 wire direction control by RTS capability

#define EPLD_RS485_RTS_2W				0x0E  //!< RS485 2 wire direction control by RTS
#define CAP_EPLD_RS485_RTS_2W				0x0080  //!< RS485 2 wire direction control by RTS capability

#define EPLD_RS485_RTS_ECHO				0x06  //!< RS485 2 wire direction control by RTS with echo
#define CAP_EPLD_RS485_RTS_ECHO				0x0100  //!< RS485 2 wire direction control by RTS with echo capability

#define EPLD_CAN					0x09  //!< CAN mode
#define CAP_EPLD_CAN					0x0200  //!< CAN mode capability

#define CAP_EPLD_RS485					(CAP_EPLD_RS485_ART_4W|CAP_EPLD_RS485_ART_2W|CAP_EPLD_RS485_ART_ECHO|CAP_EPLD_RS485_RTS_4W|CAP_EPLD_RS485_RTS_2W|CAP_EPLD_RS485_RTS_ECHO) 						//!< All supported RS485 modes
#define CAP_EPLD_RS_ALL					(CAP_EPLD_RS232|CAP_EPLD_RS422|CAP_EPLD_RS485)	//!< All supported RS modes
//@}

//! @name GPIO bits macros
//@{
#define GPIO_BIT_0 0x01		//!< bit 0
#define GPIO_BIT_1 0x02		//!< bit 1
#define GPIO_BIT_2 0x04		//!< bit 2
#define GPIO_BIT_3 0x08		//!< bit 3
#define GPIO_BIT_4 0x10		//!< bit 4
#define GPIO_BIT_5 0x20		//!< bit 5
#define GPIO_BIT_6 0x40		//!< bit 6
#define GPIO_BIT_7 0x80		//!< bit 7
//@}

#define GPIO_VAL_DATA			0
#define GPIO_VAL_CTRL			1
#define GPIO_VAL_IRQMASK		2
#define GPIO_VAL_CHANGE			3
#define GPIO_VAL_CHANGES		4
#define GPIO_VAL_MAX			5

#define GPIO_CMD_GET_BTN_RST		1
#define GPIO_CMD_SET_BTN_RST		2
#define GPIO_CMD_GET_LEDS		3
#define GPIO_CMD_SET_LEDS		4
#define GPIO_CMD_SET_LED_POWER		5
#define GPIO_CMD_SET_LED_BLUE		6
#define GPIO_CMD_SET_LED_GREEN		7
#define GPIO_CMD_SET			8
#define GPIO_CMD_GET			9
#define GPIO_CMD_SET_CTRL		10
#define GPIO_CMD_GET_CTRL		11
#define GPIO_CMD_SET_IRQMASK		12
#define GPIO_CMD_GET_IRQMASK		13
#define GPIO_CMD_SET_CHANGE		14	//!< obsolete
#define GPIO_CMD_GET_CHANGE		15
#define GPIO_CMD_SET_CHANGES		16	//!< obsolete
#define GPIO_CMD_GET_CHANGES		17
#define GPIO_CMD_SET_BUZZER		18
#define GPIO_CMD_GET_BUZZER		19
#define GPIO_CMD_SET_BUZZER_FRQ		20
#define GPIO_CMD_GET_BUZZER_FRQ		21
#define GPIO_CMD_SET_LED_BTN_WLAN	22
#define GPIO_CMD_GET_BTN_WLAN		23
#define GPIO_CMD_MAX			GPIO_CMD_GET_BTN_WLAN

extern int init_flag;
extern onrisc_system_t onrisc_system;
extern onrisc_gpios_int_t onrisc_gpios;
extern int serial_mode_first_pin;
extern onrisc_capabilities_t onrisc_capabilities;
