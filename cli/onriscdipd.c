#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "onrisc.h"

onrisc_system_t onrisc_system;

typedef struct {
	enum {
		UNSET,
		SERIAL_MODE,
		SHELL,
	} action_type;
	union {
		uint8_t mode;
		char cmd[256];
	} action;
} dip_entry;

dip_entry dip_list[16] = { 
	{.action_type=UNSET},				 // 0
	{.action_type=SERIAL_MODE, .action.mode=0b0001}, // 1 RS232
	{.action_type=UNSET},				 // 2
	{.action_type=SERIAL_MODE, .action.mode=0b0010}, // 3 RS485-HD
	{.action_type=UNSET},				 // 4
	{.action_type=SERIAL_MODE, .action.mode=0b0111}, // 5 RS422
	{.action_type=UNSET},				 // 6
	{.action_type=SERIAL_MODE, .action.mode=0b0011}, // 7 RS485-FD
	{.action_type=UNSET},				 // 8
	{.action_type=UNSET},				 // 9
	{.action_type=UNSET},				 //10
	{.action_type=SERIAL_MODE, .action.mode=0b1010}, //11 RS485-HD + Term
	{.action_type=UNSET},				 //12
	{.action_type=SERIAL_MODE, .action.mode=0b1111}, //13 RS422 + Term
	{.action_type=UNSET},				 //14
	{.action_type=SERIAL_MODE, .action.mode=0b1011}, //15 RS485-FD + Term
};

int test_callback(onrisc_gpios_t dips, void* data)
{
	printf("CB:0x%08x\n", dips.mask);
	printf("CB:0x%08x\n", dips.value);
	if(dip_list[dips.value].action_type == SERIAL_MODE) {
		printf("MD:0x%08x\n", dip_list[dips.value].action.mode);
		onrisc_set_uart_mode_binary(1, dip_list[dips.value].action.mode);
		onrisc_set_uart_mode_binary(2, dip_list[dips.value].action.mode);
	} 
}	

void print_usage()
{
	fprintf(stderr, "\nOnRISC Tool (Version %s)\n\n", LIBONRISC_VERSION);
	fprintf(stderr, "Usage: onrisctool -s\n");
	fprintf(stderr, "       onrisctool -m\n");
	fprintf(stderr,
		"       onrisctool -p <num> -t <type> -r -d <dirctl> -e\n");
	fprintf(stderr,
		"Options: -s                 show hardware parameter\n");
	fprintf(stderr,
		"         -n                 show device capabilities\n");
	fprintf(stderr, "         -m                 set MAC addresses\n");
	fprintf(stderr,
		"         -p <num>           onboard serial port number\n");
	fprintf(stderr,
		"         -t <type>          RS232/422/485, DIP or loopback mode:\n");
	fprintf(stderr,
		"                            rs232, rs422, rs485-fd, rs485-hd, dip, loop\n");
	fprintf(stderr,
		"         -r                 enable termination for serial port\n");
	fprintf(stderr,
		"         -d <dirctl>        direction control for RS485: art or rts\n");
	fprintf(stderr, "         -e                 enable echo\n");
	fprintf(stderr,
		"         -j                 get configured RS modes\n");
	fprintf(stderr,
		"         -k                 set/get mPCIe switch state: 0 - off, 1 - on, 2 - get current state\n");
	fprintf(stderr,
		"         -l <name:[0|1|2]>  turn led [pwr, app, wln] on/off or blink: 0 - off, 1 - on. 2 - blink\n");
	fprintf(stderr,
		"         -S                 show DIP switch settings (Baltos 1080 only)\n");
	fprintf(stderr, "         -a                 GPIO data mask\n");
	fprintf(stderr, "         -c                 GPIO direction mask\n");
	fprintf(stderr, "         -b                 GPIO value\n");
	fprintf(stderr, "         -f                 GPIO direction value\n");
	fprintf(stderr, "         -g                 get GPIO values\n");
	fprintf(stderr,
		"         -i                 turn network switch off\n");
	fprintf(stderr, "         -w                 set wlan0 MAC\n");
	fprintf(stderr,
		"         -q                 query WLAN switch state\n");
	fprintf(stderr, "Examples:\n");
	fprintf(stderr,
		"onrisctool -p 1 -t rs232 (set first serial port into RS232 mode)\n");
	fprintf(stderr,
		"onrisctool -m (set MAC addresses for eth0 and eth1 stored in EEPROM)\n");
	fprintf(stderr, "onrisctool -l pwr:0 (turn power LED off)\n");
	fprintf(stderr,
		"onrisctool -c 0x0f -f 0x0f (set first 4 IOs to output)\n");
	fprintf(stderr,
		"onrisctool -a 0x07 -b 0x05 (turn pins 0 and 2 to high and clear pin 1)\n");
}

int main(int argc, char **argv)
{
	int opt;
	int port = -1;
	int mode = TYPE_UNKNOWN;
	int termination = 0;
	int echo = 0;
	int dir_ctrl = DIR_ART;
	uint32_t dips;
	onrisc_uart_mode_t onrisc_uart_mode;
	uint32_t mask = 0, value = 0, dir_mask = 0, dir_value = 0;
	onrisc_gpios_t gpios;
	bool set_gpio = false, set_dir_gpio = false;
	gpio_level wlan_sw_state;

	if (onrisc_init(&onrisc_system) == EXIT_FAILURE) {
		printf("Failed to init\n");
		goto error;
	}

	if (argc == 1) {
		onrisc_dip_register_callback(0, test_callback, NULL, BOTH);
		while(1);
	}
	
	/* handle command line params */
	while ((opt = getopt(argc, argv, "a:b:c:d:f:k:l:p:t:iegrhmnsSwqj?")) != -1) {
		switch (opt) {

		case 'a':
			mask = strtol(optarg, NULL, 16);
			set_gpio = true;
			break;
		case 'b':
			value = strtol(optarg, NULL, 16);
			set_gpio = true;
			break;
		case 'c':
			dir_mask = strtol(optarg, NULL, 16);
			set_dir_gpio = true;
			break;
		case 'd':
			if (!strcmp(optarg, "art")) {
				dir_ctrl = DIR_ART;
			} else if (!strcmp(optarg, "rts")) {
				dir_ctrl = DIR_RTS;
			} else {
				fprintf(stderr,
					"Unknown direction control mode (%s)\n",
					optarg);
				goto error;
			}
			break;
		case 'f':
			dir_value = strtol(optarg, NULL, 16);
			set_dir_gpio = true;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'g':
			{
				if (onrisc_gpio_get_value(&gpios) ==
				    EXIT_FAILURE) {
					fprintf(stderr,
						"Failed to get GPIO values\n");
					goto error;
				}

				printf("0x%08x\n", gpios.value);
			}
			break;
		case 'm':
			break;
		case 'n':
			break;
		case 'w':
			break;
		case 's':
			onrisc_print_hw_params();
			break;
		case 'S':
			if (onrisc_get_dips(&dips) == EXIT_FAILURE) {
				fprintf(stderr,
					"Failed to get DIP switch setting\n");
				goto error;
			}

			printf("DIP S1: %s\n", dips & DIP_S1 ? "on" : "off");
			printf("DIP S2: %s\n", dips & DIP_S2 ? "on" : "off");
			printf("DIP S3: %s\n", dips & DIP_S3 ? "on" : "off");
			printf("DIP S4: %s\n", dips & DIP_S4 ? "on" : "off");

			break;
		case 't':
			if (!strcmp(optarg, "rs232")) {
				mode = TYPE_RS232;
			} else if (!strcmp(optarg, "rs422")) {
				mode = TYPE_RS422;
			} else if (!strcmp(optarg, "rs485-fd")) {
				mode = TYPE_RS485_FD;
			} else if (!strcmp(optarg, "rs485-hd")) {
				mode = TYPE_RS485_HD;
			} else if (!strcmp(optarg, "dip")) {
				mode = TYPE_DIP;
			} else if (!strcmp(optarg, "loop")) {
				mode = TYPE_LOOPBACK;
			} else {
				fprintf(stderr, "Unknown UART mode (%s)\n",
					optarg);
				goto error;
			}
			break;
		case 'e':
			echo = 1;
			break;
		case 'i':
			{
				int i, nr_swithes = 4;

				/* perform only for devices with network switch chip */
				if (onrisc_system.model != BALTOS_IR_3220
				    && onrisc_system.model != BALTOS_IR_5221) {
					break;
				}

				if (onrisc_system.model == BALTOS_IR_3220) {
					nr_swithes = 2;
				}

				for (i = 1; i < nr_swithes; i++) {
					int ctrl_reg;

					if (onrisc_read_mdio_reg
					    (i, 0, &ctrl_reg)) {
						goto error;
					}

					ctrl_reg |= (1 << 11);

					if (onrisc_write_mdio_reg
					    (i, 0, ctrl_reg)) {
						goto error;
					}
				}

			}
			break;
		case 'l':
			break;
		case 'k':
			break;
		case 'r':
			termination = 1;
			break;
		case 'q':
			if (onrisc_get_wlan_sw_state(&wlan_sw_state) ==
			    EXIT_FAILURE) {
				printf("failed to get WLAN switch state\n");
			} else {
				printf("WLAN switch: %s\n",
				       wlan_sw_state ? "on" : "off");
			}
			break;
		case 'j':
			break;
		case '?':
		case 'h':
		default:
			print_usage();
			return 1;
			break;
		}
	}

	if ((port != -1) && (mode != TYPE_UNKNOWN)) {
		onrisc_uart_mode.rs_mode = mode;
		onrisc_uart_mode.termination = termination;
		onrisc_uart_mode.echo = echo;
		onrisc_uart_mode.dir_ctrl = dir_ctrl;;

		if (onrisc_set_uart_mode(port, &onrisc_uart_mode)) {
			fprintf(stderr, "Failed to set UART mode\n");
			goto error;
		}
	}

	if (set_dir_gpio) {
		gpios.mask = dir_mask;
		gpios.value = dir_value;

		if (onrisc_gpio_set_direction(&gpios) == EXIT_FAILURE) {
			fprintf(stderr, "Failed to set GPIO direction\n");
			goto error;
		}
	}

	if (set_gpio) {
		gpios.mask = mask;
		gpios.value = value;

		if (onrisc_gpio_set_value(&gpios) == EXIT_FAILURE) {
			fprintf(stderr, "Failed to set GPIOs\n");
			goto error;
		}

		if (onrisc_gpio_get_value(&gpios) == EXIT_FAILURE) {
			fprintf(stderr, "Failed to get GPIO values\n");
			goto error;
		}

		printf("0x%08x\n", gpios.value);

	}

	return 0;
 error:
	return 1;
}
