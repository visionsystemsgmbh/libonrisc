#ifndef _ONRISC_H_
#define _ONRISC_H_

#include <pthread.h>
#include <stdint.h>
#include <libsoc_gpio.h>

#define ALEKTO2_EEPROM	"/sys/bus/i2c/devices/1-0050/eeprom"
#define VS860_EEPROM	"/sys/bus/i2c/devices/2-0054/eeprom"

//! @name Hardware parameter stuff
//@{
#define PARTITION_REDBOOT		"/dev/mtdblock1"	//!< RedBoot partition device
#define PARTITION_REDBOOT_SIZE		0x10000	//!< Flash size
#define GLOBAL_MAGIC			0xDEADBEEF	//!< Magic number
//@}

#define ALEKTO		1
#define ALENA		2
#define ALEKTO_LAN	3
#define VS860		100
#define ALEKTO2		200

typedef struct 
{
	uint16_t model;	
	uint32_t hw_rev;
	uint32_t ser_nr;
	char prd_date[11];
	uint8_t mac1[6];
	uint8_t mac2[6];
	uint8_t mac3[6];
} __attribute__ ((packed)) onrisc_system_t; 

//! @brief Hardware parameter stored in flash
//!
//! This struct lies at the end of the prebootloaders
//! partition. It is build from top (highest addr)
//! to down -> magic lies at the last address of the
//! partition.
struct _param_hw		// v1.0
{
	uint8_t pad[1];
	uint16_t szram;		//!< direct in MB
	uint16_t szflash2;
	uint16_t szflash1;	//!< direct in MB
	uint8_t mac2[6];		//!< MAC address for the second interface
	uint8_t mac1[6];		//!< MAC address for the first interface
	uint16_t biosid;		//!< BIOS ID to distinguish between various OnRISC products
	char prddate[11];	//!< as a string ie. "01.01.2006"
	uint32_t serialnr;		//!< Serial number
	uint32_t hwrev;		//!< Hardware revision
	uint32_t magic;
} __attribute__ ((packed));

typedef struct _BSP_VS_HWPARAM    // v1.0
{
	uint32_t Magic;
	uint32_t HwRev;
	uint32_t SerialNumber;
	char PrdDate[11];    // as a string ie. "01.01.2006"
	uint16_t SystemId;
	uint8_t MAC1[6];        // internal EMAC
	uint8_t MAC2[6];        // SMSC9514
	uint8_t MAC3[6];        // WL1271 WLAN
} __attribute__ ((packed)) BSP_VS_HWPARAM; 

/* Definition for KS8695 based devices */

#define LED_POWER				0x01	//!< power LED red
#define LED_BLUE				0x02	//!< WLAN LED blue
#define LED_GREEN				0x04	//!< LED green
#define LED_BTN_WLAN				0x08	//!< LED button wlan (Arete)

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

struct epld_struct
{
	unsigned long port;
	unsigned long reg_shift;
	unsigned char value;
};

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

struct gpio_struct
{
	unsigned long mask;
	unsigned long value;
};

typedef struct
{
	uint32_t interval;
	uint8_t high_phase;
	pthread_t thread_id;
	gpio *led;
	int fd;
} blink_led;

/* prototypes */
void onrisc_print_hw_params();
int onrisc_blink_pwr_led_start(blink_led *blinker);
int onrisc_blink_pwr_led_stop(blink_led *blinker);

#endif	/*_ONRISC_H_ */
