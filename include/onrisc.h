#ifndef _ONRISC_H_
#define _ONRISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <libsoc_gpio.h>

#define ALEKTO2_EEPROM	"/sys/bus/i2c/devices/1-0050/eeprom"
#define VS860_EEPROM	"/sys/bus/i2c/devices/2-0054/eeprom"

/** @name Hardware parameter stuff */
/*@{*/
#define PARTITION_REDBOOT	"/dev/mtdblock1"	//!< RedBoot partition device
#define PARTITION_REDBOOT_SIZE	0x10000			//!< Flash size
#define GLOBAL_MAGIC		0xDEADBEEF		//!< Magic number
/*@}*/

#define ALEKTO		1
#define ALENA		2
#define ALEKTO_LAN	3
#define VS860		100
#define ALEKTO2		200
#define BALIOS_IR_5221	210
#define NETCON3		215

enum rs_mode {TYPE_UNKNOWN, TYPE_RS232, TYPE_RS422, TYPE_RS485_FD, TYPE_RS485_HD, TYPE_LOOPBACK, TYPE_DIP, TYPE_CAN};

enum dir_ctrl {DIR_ART, DIR_RTS};

typedef struct {
	uint32_t rs_mode;
	uint32_t termination;
	uint32_t dir_ctrl;
	uint32_t echo;
} __attribute__ ((packed)) onrisc_uart_mode_t;

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
#define LED_WLAN				0x02	//!< WLAN LED blue
#define LED_APP					0x04	//!< LED green
#define LED_BTN_WLAN				0x08	//!< LED button wlan (Arete)

#define DIP_S1					0x01
#define DIP_S2					0x02
#define DIP_S3					0x04
#define DIP_S4					0x08

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
	struct timeval interval;	/**< time for complete cycles (high and low phase) */
	struct timeval high_phase;	/**< high phase duration */
	uint32_t led_type;		/**< LED type: power led, user led etc. */
	int32_t count;			/**< number of cycles or -1 for infinite */

	/* internal fields */
	pthread_t thread_id;
	gpio *led;
	int fd;
	unsigned long leds_old;
} blink_led_t;

#define ONRISC_MAX_GPIOS        32

typedef struct {
	uint8_t direction;
	uint8_t value;
	uint8_t dir_fixed;
	gpio *pin;
} __attribute__ ((packed)) onrisc_gpio_t;

typedef struct {
	int ngpio;
	int base;
	onrisc_gpio_t gpios[ONRISC_MAX_GPIOS];
	onrisc_gpio_t gpios_ctrl[6];
} __attribute__ ((packed)) onrisc_gpios_int_t;

typedef struct  {
	uint32_t mask;
	uint32_t value;
} __attribute__ ((packed)) onrisc_gpios_t;

/* prototypes */

/**
 * @brief get system, hardware parameters etc.
 * @param data pointer to the structure, where system data will be stored
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_init(onrisc_system_t *data);

/**
 * @brief show content of the internal onrisc_system_t structure
 */
void onrisc_print_hw_params();

/**
 * @brief init blinker structure
 * @param blinker blink_led structure
 */
void onrisc_blink_create(blink_led_t *blinker);

/**
 * @brief free blinker's internal elements
 * @param blinker blink_led structure
 */
void onrisc_blink_destroy(blink_led_t *blinker);

/**
 * @brief start blinking thread
 * @param blinker blink_led structure
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_blink_led_start(blink_led_t *blinker);

/**
 * @brief cancel blinking thread and free GPIO
 * @param blinker blink_led structure
 * @todo send signal to the blinking thread and make cleanup only there
 */
int onrisc_blink_led_stop(blink_led_t *blinker);

/**
 * @brief turn LED on/off
 * @param led blink_led structure
 * @param state 0 - off, 1 - on
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_switch_led(blink_led_t *led, uint8_t state);

/**
 * @brief set UART's mode like RS232/RS422/RS485 and termination
 * @param port_nr port number, i.e. 1 - for /dev/ttyS1 or /dev/sertest0
 * @param mode pointer to onrisc_uart_mode_t describing UART's mode: RS-modes
 * termination, echo and direction control
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_set_uart_mode(int port_nr, onrisc_uart_mode_t *mode);

/**
 * @brief get DIP switches values
 * @param dips variable to hold the DIP switches value
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_get_dips(uint32_t *dips);

/**
 * @brief change GPIO direction
 * @param gpio_dir pointer to a structure holding mask and direction values for all GPIOs
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_gpio_set_direction(onrisc_gpios_t * gpio_dir);

/**
 * @brief change GPIO output value
 * @param gpio_dir pointer to a structure holding mask and values for all outputs
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_gpio_set_value(onrisc_gpios_t * gpio_val);

/**
 * @brief get GPIO value for both inputs and outputs
 * @param gpio_dir pointer to a structure, that will return GPIO value in the value field
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_gpio_get_value(onrisc_gpios_t * gpio_val);

#ifdef __cplusplus
}
#endif

#endif	/*_ONRISC_H_ */
