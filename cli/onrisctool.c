#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "onrisc.h"

#define ALL_UARTS 0

onrisc_system_t onrisc_system;

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
		"         -p <num>           onboard serial port number starting with '1', '0' affects all ports\n");
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
		"         -l <name:[0|1|2|3]>  turn led [pwr, app, wln] on/off or blink: 0 - off, 1 - on. 2 - blink. 3 - blink continuously\n");
	fprintf(stderr,
		"         -S                 show DIP switch settings\n");
	fprintf(stderr, "         -a                 GPIO data mask\n");
	fprintf(stderr, "         -c                 GPIO direction mask\n");
	fprintf(stderr, "         -b                 GPIO value\n");
	fprintf(stderr, "         -f                 GPIO direction value\n");
	fprintf(stderr, "         -g                 get GPIO values\n");
	fprintf(stderr,
		"         -i                 turn network switch off\n");
	fprintf(stderr,
		"         -I                 initalize device hardware\n");
	fprintf(stderr, "         -w                 set wlan0 MAC\n");
	fprintf(stderr,
		"         -q                 query WLAN switch state\n");
	fprintf(stderr, "Examples:\n");
	fprintf(stderr,
		"onrisctool -p 1 -t rs232 (set first serial port into RS232 mode)\n");
	fprintf(stderr,
		"onrisctool -p 0 -t rs232 (set all serial port into RS232 mode)\n");
	fprintf(stderr,
		"onrisctool -m (set MAC addresses for eth0 and eth1 stored in EEPROM)\n");
	fprintf(stderr, "onrisctool -l pwr:0 (turn power LED off)\n");
	fprintf(stderr,
		"onrisctool -c 0x0f -f 0x0f (set first 4 IOs to output)\n");
	fprintf(stderr,
		"onrisctool -a 0x70 -b 0x50 (turn pins 0 and 2 to high and clear pin 1)\n");
}

int show_rs_modes()
{
	int i, rc = EXIT_FAILURE;
	onrisc_capabilities_t *caps = onrisc_get_dev_caps();
	if ((NULL == caps->uarts) || !(UARTS_SWITCHABLE & caps->uarts->flags)) {
		fprintf(stderr, "device has no RS mode switchable UARTs\n");
		goto error;
	}

	for (i = 0; i < caps->uarts->num; i++) {
		onrisc_uart_mode_t mode;
		uint32_t dips;
		if (onrisc_get_uart_mode(i + 1, &mode)) {
			fprintf(stderr, "failed to get UART mode\n");
			goto error;
		}

		switch (mode.rs_mode) {
		case TYPE_RS232:
			printf
			    ("Port %d: mode: rs232 termination: %s source: %s\n",
			     i + 1, mode.termination ? "on" : "off",
			     mode.src == INPUT ? "DIP" : "software");
			break;
		case TYPE_RS422:
			printf
			    ("Port %d: mode: rs422 termination: %s source: %s\n",
			     i + 1, mode.termination ? "on" : "off",
			     mode.src == INPUT ? "DIP" : "software");
			break;
		case TYPE_RS485_HD:
			printf
			    ("Port %d: mode: rs485-hd termination: %s source: %s\n",
			     i + 1, mode.termination ? "on" : "off",
			     mode.src == INPUT ? "DIP" : "software");
			break;
		case TYPE_RS485_FD:
			printf
			    ("Port %d: mode: rs485-fd termination: %s source: %s\n",
			     i + 1, mode.termination ? "on" : "off",
			     mode.src == INPUT ? "DIP" : "software");
			break;
		case TYPE_LOOPBACK:
			printf
			    ("Port %d: mode: loopback termination: %s source: %s\n",
			     i + 1, mode.termination ? "on" : "off",
			     mode.src == INPUT ? "DIP" : "software");
			break;
		case TYPE_UNKNOWN:
			if (onrisc_get_uart_dips(i + 1, &dips)) {
				fprintf(stderr, "failed to get UART mode\n");
				goto error;
			}
			printf
			    ("Port %d: mode: unknown vector: %s%s%s%s\n",
			     i + 1, dips & DIP_S1 ? "1" : "0",
				dips & DIP_S2 ? "1" : "0",
				dips & DIP_S3 ? "1" : "0",
				dips & DIP_S4 ? "1" : "0");

			break;
		}

	}
	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int handle_mpcie_switch(char *str)
{
	uint8_t sw_state = 0;

	if (sscanf(str, "%1hhu",&sw_state) != 1) {
		fprintf(stderr, "error parsing switch\n");
		return EXIT_FAILURE;
	}

	switch(sw_state) {
		case 0:
			if (onrisc_set_mpcie_sw_state(LOW) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case 1:
			if (onrisc_set_mpcie_sw_state(HIGH) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		default:
			{
				gpio_level sw_level;

				if (onrisc_get_mpcie_sw_state(&sw_level) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}

				printf("mPCIe switch: %s\n", sw_level == HIGH ? "on" : "off");
			}
	}

	return EXIT_SUCCESS;
}

int handle_leds(char *str)
{
	char name[16];
	blink_led_t led;
	uint8_t led_state = 0;

	if (sscanf(str, "%3[^:]:%1hhu", name, &led_state) != 2) {
		fprintf(stderr, "error parsing LEDs\n");
		return EXIT_FAILURE;
	}

	/* init LED */
	onrisc_blink_create(&led);

	/* parse LED name */
	if (!strcmp(name, "pwr")) {
		led.led_type = LED_POWER;
	} else if (!strcmp(name, "app")) {
		led.led_type = LED_APP;
	} else if (!strcmp(name, "wln")) {
		led.led_type = LED_WLAN;
	} else if (!strcmp(name, "cer")) {
		led.led_type = LED_CAN_ERROR;
	} else {
		fprintf(stderr, "unknown LED: %s\n", name);
		return EXIT_FAILURE;
	}

	/* check on/off state */
	switch (led_state) {
	case 0:
	case 1:
		if (onrisc_switch_led(&led, led_state) == EXIT_FAILURE) {
			fprintf(stderr, "failed to switch %s LED\n", name);
		}

		break;
	case 2:
		led.count = -1;	/* blinking continuously */
		led.interval.tv_sec = 1;
		led.interval.tv_usec = 0;
		led.high_phase.tv_sec = 0;
		led.high_phase.tv_usec = 500000;

		if (onrisc_blink_led_start(&led) == EXIT_FAILURE) {
			fprintf(stderr, "failed to start %s LED\n", name);
			return EXIT_FAILURE;
		}

		sleep(5);

		if (onrisc_blink_led_stop(&led) == EXIT_FAILURE) {
			fprintf(stderr, "failed to stop %s LED\n", name);
			return EXIT_FAILURE;
		}

		/* blink with another frequency */
		led.count = -1;	/* blinking continuously */
		led.interval.tv_sec = 2;
		led.interval.tv_usec = 0;
		led.high_phase.tv_sec = 1;
		led.high_phase.tv_usec = 0;

		if (onrisc_blink_led_start(&led) == EXIT_FAILURE) {
			fprintf(stderr, "failed to start %s LED\n", name);
			return EXIT_FAILURE;
		}

		sleep(5);

		if (onrisc_blink_led_stop(&led) == EXIT_FAILURE) {
			fprintf(stderr, "failed to stop %s LED\n", name);
			return EXIT_FAILURE;
		}
		break;
	case 3:
		led.count = -1;	/* blinking continuously */
		led.interval.tv_sec = 3;
		led.interval.tv_usec = 0;
		led.high_phase.tv_sec = 2;
		led.high_phase.tv_usec = 0;

		if (onrisc_blink_led_start(&led) == EXIT_FAILURE) {
			fprintf(stderr, "failed to start %s LED\n", name);
			return EXIT_FAILURE;
		}

		while(true);

		if (onrisc_blink_led_stop(&led) == EXIT_FAILURE) {
			fprintf(stderr, "failed to stop %s LED\n", name);
			return EXIT_FAILURE;
		}
		break;
	default:
		if (onrisc_get_led_state(&led, &led_state) == EXIT_FAILURE) {
			fprintf(stderr, "failed to get LED state\n");
			return EXIT_FAILURE;
		}
		printf("%s: %s\n", name, led_state ? "on" : "off");
	}

	return EXIT_SUCCESS;
}

int set_wlan_mac()
{
	char cmd[128];

	sprintf(cmd, "ifconfig wlan0 hw ether %02x:%02x:%02x:%02x:%02x:%02x",
		onrisc_system.mac3[0],
		onrisc_system.mac3[1],
		onrisc_system.mac3[2],
		onrisc_system.mac3[3],
		onrisc_system.mac3[4], onrisc_system.mac3[5]
	    );

	system(cmd);

	return 0;
}

int set_macs()
{
	char cmd[128];

	sprintf(cmd, "ifconfig eth0 hw ether %02x:%02x:%02x:%02x:%02x:%02x",
		onrisc_system.mac1[0],
		onrisc_system.mac1[1],
		onrisc_system.mac1[2],
		onrisc_system.mac1[3],
		onrisc_system.mac1[4], onrisc_system.mac1[5]
	    );

	system(cmd);

	if (VS860 == onrisc_system.model ||
	    NETCOM_PLUS_ECO_111 == onrisc_system.model ||
	    NETCOM_PLUS_ECO_113 == onrisc_system.model) {
		return 0;
	}

	sprintf(cmd, "ifconfig eth1 hw ether %02x:%02x:%02x:%02x:%02x:%02x",
		onrisc_system.mac2[0],
		onrisc_system.mac2[1],
		onrisc_system.mac2[2],
		onrisc_system.mac2[3],
		onrisc_system.mac2[4], onrisc_system.mac2[5]
	    );

	system(cmd);

	return 0;
}

int print_caps() {
	onrisc_capabilities_t * caps = onrisc_get_dev_caps();

	printf(" Device Capabilities\n---------------------------\n");
	if (caps->gpios) {
		printf("GPIOS: %d\n", caps->gpios->ngpio);
	}
	if (caps->uarts) {
		printf("UARTS: %d ", caps->uarts->num);
		if (caps->uarts->flags & UARTS_SWITCHABLE) {
			printf(" (modes switchable by software");
			if (caps->uarts->flags & UARTS_DIPS_PHYSICAL) {
				printf(" & DIPs");
			}
			printf(")");
		}
		printf("\n");
	}
	if (caps->dips) {
		printf("DIPS: %d\n", caps->dips->num);
	}
	if (caps->leds) {
		printf("LEDS: %d\n", caps->leds->num);
	}
	if (caps->wlan_sw) {
		printf("WLAN switch present\n");
	}
	if (caps->mpcie_sw) {
		printf("miniPCIe switch present\n");
	}
	if (caps->eths) {
		int idx;
		printf("ETHS: %d\n", caps->eths->num);
		for(idx=0; idx < caps->eths->num; idx++) {
			printf("\t%s: %d ports speed: %s%s%s\n", caps->eths->eth[idx].if_name, caps->eths->eth[idx].num,
				(caps->eths->eth[idx].flags & ETH_RMII_100M) ? "100Mbit ":"",
				(caps->eths->eth[idx].flags & ETH_RGMII_1G) ? "1Gbit ":"",
				(caps->eths->eth[idx].flags & ETH_PHYSICAL) ? "":"INTERNAL ONLY");
		}
		printf("\n");
	}
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

	if (argc == 1) {
		print_usage();
		return 1;
	}

	if (onrisc_init(&onrisc_system) == EXIT_FAILURE) {
		printf("Failed to init\n");
		goto error;
	}

	/* handle command line params */
	while ((opt = getopt(argc, argv, "a:b:c:d:f:k:l:p:t:iIegGrhmnsSwqj?")) != -1) {
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
		case 'G':
			{
				if (onrisc_gpio_get_direction(&gpios) ==
				    EXIT_FAILURE) {
					fprintf(stderr,
						"Failed to get GPIO values\n");
					goto error;
				}

				printf("0x%08x\n", gpios.mask);
				printf("0x%08x\n", gpios.value);
			}
			break;
		case 'm':
			set_macs();
			break;
		case 'n':
			print_caps();
			break;
		case 'w':
			set_wlan_mac();
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
			printf("DIP S4: %s\n\n", dips & DIP_S4 ? "on" : "off");
			printf("Vector: %s%s%s%s\n", dips & DIP_S1 ? "1" : "0",
				dips & DIP_S2 ? "1" : "0",
				dips & DIP_S3 ? "1" : "0",
				dips & DIP_S4 ? "1" : "0");


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
				if (onrisc_find_ip175d()) {
					/* warn, if switch wasn't found on devices, that must have it */
					if (onrisc_system.model == BALTOS_IR_3220
					    || onrisc_system.model == BALTOS_IR_5221) {
						fprintf(stderr, "Failed to find IP175D chip, though it must be present\n");
						goto error;
					}

					break;
				}

				printf("IP175D switch chip found\n");

				for (i = 1; i < nr_swithes; i++) {
					int ctrl_reg;

					if (onrisc_read_mdio_reg
					    (i, 0, &ctrl_reg)) {
						fprintf(stderr, "Failed to read MDIO register\n");
						goto error;
					}

					ctrl_reg |= (1 << 11);

					if (onrisc_write_mdio_reg
					    (i, 0, ctrl_reg)) {
						fprintf(stderr, "Failed to write MDIO register\n");
						goto error;
					}

					if (onrisc_read_mdio_reg
					    (i, 0, &ctrl_reg)) {
						fprintf(stderr, "Failed to read MDIO register\n");
						goto error;
					}

					if (!(ctrl_reg & (1 << 11))) {
						fprintf(stderr, "Failed to turn port %d off\n", i);
						goto error;
					}
				}

			}
			break;
		case 'I':
			if (onrisc_uart_init()) {
				goto error;
			}
			break;
		case 'l':
			if (handle_leds(optarg) == EXIT_FAILURE) {
				goto error;
			}
			break;
		case 'k':
			if (handle_mpcie_switch(optarg) == EXIT_FAILURE) {
				goto error;
			}
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
			if (show_rs_modes() == EXIT_FAILURE) {
				exit(1);
			}
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

		if (port == ALL_UARTS) {
			int i;
			onrisc_capabilities_t *caps = onrisc_get_dev_caps();
			for (i = 0; i < caps->uarts->num; i++) {
				if (onrisc_set_uart_mode(i + 1, &onrisc_uart_mode)) {
					fprintf(stderr, "Failed to set UART mode\n");
					goto error;
				}
			}
		} else {
			if (onrisc_set_uart_mode(port, &onrisc_uart_mode)) {
				fprintf(stderr, "Failed to set UART mode\n");
				goto error;
			}
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
