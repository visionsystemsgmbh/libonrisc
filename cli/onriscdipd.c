#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "onrisc.h"

onrisc_system_t onrisc_system;

#define ALL_PORTS -1

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

/*dip_entry dip_list[16] = { 
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
};*/

dip_entry dip_list[18] = { 
	{.action_type=UNSET}, // 0
	{.action_type=UNSET}, // 1
	{.action_type=UNSET}, // 2
	{.action_type=UNSET}, // 3
	{.action_type=UNSET}, // 4
	{.action_type=UNSET}, // 5
	{.action_type=UNSET}, // 6
	{.action_type=UNSET}, // 7
	{.action_type=UNSET}, // 8
	{.action_type=UNSET}, // 9
	{.action_type=UNSET}, //10
	{.action_type=UNSET}, //11
	{.action_type=UNSET}, //12
	{.action_type=UNSET}, //13
	{.action_type=UNSET}, //14
	{.action_type=UNSET}, //15
	{.action_type=UNSET}, //16 WLAN SW RISING EDGE
	{.action_type=UNSET}, //17 WLAN SW FALLING EDGE
};

bool rs232only = false;
bool hw_uart_dips = false;
bool wlan_sw = false;

int test_callback(onrisc_gpios_t dips, void* data)
{
	int port_nr = ALL_PORTS;
	char cmd[256];
	if(dips.mask == 0) {
		port_nr = *((int *) data);
	}
	if(dip_list[dips.value].action_type == SERIAL_MODE) {
		onrisc_set_uart_mode_raw(port_nr, dip_list[dips.value].action.mode);
	} else 	if(dip_list[dips.value].action_type == SHELL) {
		if(dips.mask != 0) {
			system(dip_list[dips.value].action.cmd);
		} else {
			sprintf(cmd, "%s%d", dip_list[dips.value].action.cmd, port_nr);
			system(cmd);
		}

	}
 
}	

int wlan_sw_callback(onrisc_gpios_t gpios, void* data)
{
	int port_nr = ALL_PORTS;
	
	if (gpios.value) {
		if(dip_list[16].action_type == SERIAL_MODE) {
			onrisc_set_uart_mode_raw(port_nr, dip_list[16].action.mode);
		} else 	if(dip_list[16].action_type == SHELL) {
			system(dip_list[16].action.cmd);
		}
	} else {
		if(dip_list[17].action_type == SERIAL_MODE) {
			onrisc_set_uart_mode_raw(port_nr, dip_list[17].action.mode);
		} else 	if(dip_list[17].action_type == SHELL) {
			system(dip_list[17].action.cmd);
		}
	}
}	

void print_usage()
{
	fprintf(stderr, "\nOnRISC DIP Daemon (Version %s)\n\n", LIBONRISC_VERSION);
	fprintf(stderr, "Usage: onriscdipd -d\n");
	fprintf(stderr, "       onriscdipd -e\n");
	fprintf(stderr,
		"Options: -c                 read configs from /etc/onriscdipd.conf\n");
	fprintf(stderr,
		"         -d                 start as DIP daemon\n");
	fprintf(stderr, "         -e                 execute callback at start\n");
}

int readconfig(char *filename)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int idx,i;
	uint8_t mode = 0;
	char type[20];
	char cmd[256];
	char *buf = NULL;

	fp = fopen("/etc/onriscdipd.conf", "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	while ((read = getline(&line, &len, fp)) != -1) {
		if (!read)
			continue;
		buf=strtok(line,":");
		sscanf(buf, "%02X", &idx);
		buf=strtok(NULL,":");
		sscanf(buf, "%s", type);
		if (!strcmp(type,"unset")){
			dip_list[idx].action_type = UNSET;
		} else if (!strcmp(type, "serial-mode")){
			if (rs232only || hw_uart_dips){
				dip_list[idx].action_type = UNSET;
				continue;
			}
			dip_list[idx].action_type = SERIAL_MODE;
			buf=strtok(NULL,"\n");
			sscanf(buf, "%s", cmd);

			if(strlen(cmd) > 5)
				exit(EXIT_FAILURE);

			mode = 0;
			for (i=0; i<strlen(cmd); ++i){
				if (cmd[i] == '1') {
					mode |= 1 << ((strlen(cmd) - 1) - i);
				} else if (cmd[i] == '0') {
				} else {
					exit(EXIT_FAILURE);
				}	
			}
			
			dip_list[idx].action.mode = mode;
		} else if (!strcmp(type, "shell")){
			dip_list[idx].action_type = SHELL;

			if(strlen(cmd) > 255)
				exit(EXIT_FAILURE);

			buf=strtok(NULL,"\n");
			strcpy(dip_list[idx].action.cmd, buf);
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int opt, i;
	int port = -1;
	int mode = TYPE_UNKNOWN;
	int termination = 0;
	int echo = 0;
	int dir_ctrl = DIR_ART;
	onrisc_gpios_t dips;
	uint32_t mask = 0, value = 0, dir_mask = 0, dir_value = 0;
	bool set_gpio = false, set_dir_gpio = false;
	gpio_level wlan_sw_state;

	if (onrisc_init(&onrisc_system) == EXIT_FAILURE) {
		printf("Failed to init\n");
		goto error;
	}

	onrisc_capabilities_t *caps = onrisc_get_dev_caps();
	
	if (argc == 1) {
		print_usage();
		return 1;
	}

	if (!caps->uarts) {
		printf("This device has no UARTS\n");
		goto error;
	}
	
	rs232only = !(caps->uarts->flags & UARTS_SWITCHABLE);	
	hw_uart_dips = !!(caps->uarts->flags & UARTS_DIPS_PHYSICAL);	
	wlan_sw = !!(caps->wlan_sw);	
	
	/* handle command line params */
	while ((opt = getopt(argc, argv, "cedh?")) != -1) {
		switch (opt) {

		case 'c':
			if (rs232only) {
				printf("This device only supports RS232\nAll mode switches are ignored\n");
			}

			if (hw_uart_dips) {
				printf("This device has UART-DIPs\nOnly scripts are applied to these DIPs\n");
			}
			readconfig(NULL);
			break;
		case 'd':
			if (hw_uart_dips) {
				printf("This device has UART-DIPs\n");
			} else {
				onrisc_dip_register_callback(0, test_callback, NULL, BOTH);
			}
			if (!wlan_sw) {
				printf("This device has no WLAN switch\n");
			} else {
				onrisc_wlan_sw_register_callback(wlan_sw_callback, NULL, BOTH);
			}
			pause();
			break;
		case 'e':
			if (hw_uart_dips) {
				for(i=1;i <= caps->uarts->num; ++i) {	
					uint32_t val;
					dips.mask=0;
					onrisc_get_uart_dips(i,&val);
					dips.value = val;
					test_callback(dips,&i);
				}
			} else {
				uint32_t val;
				dips.mask=0;
				i=ALL_PORTS;
				onrisc_get_dips(&val);
				dips.value = val;
				test_callback(dips,&i);
			}
			if (wlan_sw) {
				dips.mask=0;
				onrisc_get_wlan_sw_state(&wlan_sw_state);
				dips.value = (wlan_sw_state == HIGH) ? 1: 0;
				wlan_sw_callback(dips,NULL);
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

	return 0;
 error:
	return 1;
}
