#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "onrisc.h"

onrisc_system_t onrisc_system;

void print_usage(char *prg)
{
	fprintf(stderr, "\n%s: OnRISC Tool\n\n", prg);
}

int set_macs()
{
	char cmd[128];

	sprintf(cmd, "ifconfig eth0 hw ether %02x:%02x:%02x:%02x:%02x:%02x",
			onrisc_system.mac1[0],
			onrisc_system.mac1[1],
			onrisc_system.mac1[2],
			onrisc_system.mac1[3],
			onrisc_system.mac1[4],
			onrisc_system.mac1[5]
			);

	system(cmd);
	sprintf(cmd, "ifconfig eth1 hw ether %02x:%02x:%02x:%02x:%02x:%02x",
			onrisc_system.mac2[0],
			onrisc_system.mac2[1],
			onrisc_system.mac2[2],
			onrisc_system.mac2[3],
			onrisc_system.mac2[4],
			onrisc_system.mac2[5]
			);
	system(cmd);

	return 0;
}

int main(int argc, char **argv)
{
	int opt;
	int port = -1;
	int mode = TYPE_UNKNOWN;
	int termination = 0;
	int echo = 0;
	int dir_ctrl = DIR_ART;
	onrisc_uart_mode_t onrisc_uart_mode;

	if (onrisc_init(&onrisc_system) == EXIT_FAILURE) {
		printf("Failed to init\n");
		goto error;
	}

	/* handle command line params */
	while ((opt = getopt(argc, argv, "d:p:t:erhms?")) != -1) {
		switch (opt) {

		case 'd':
			if (!strcmp(optarg, "art")) {
				dir_ctrl = DIR_ART;
			} else if (!strcmp(optarg, "rts")) {
				dir_ctrl = DIR_RTS;
			} else {
				fprintf(stderr, "Unknown direction control mode (%s)\n", optarg);
				goto error;
			}
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'm':
			set_macs();
			break;
		case 's':
			onrisc_print_hw_params();
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
				fprintf(stderr, "Unknown UART mode (%s)\n", optarg);
				goto error;
			}
			break;
		case 'e':
			echo = 1;
			break;
		case 'r':
			termination = 1;
			break;
		case '?':
		case 'h':
		default:
			print_usage(basename(argv[0]));
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

	return 0;
error:
	return 1;
}
