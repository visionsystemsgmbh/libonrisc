#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "onrisc.h"

onrisc_system_t onrisc_system;
onrisc_capabilities_t * caps;

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

dip_entry gpio_list[16] = { 
	{.action_type=UNSET}, // IN 0 RISING EDGE
	{.action_type=UNSET}, // IN 1
	{.action_type=UNSET}, // IN 2
	{.action_type=UNSET}, // IN 3
	{.action_type=UNSET}, // IN 0 FALLING EDGE
	{.action_type=UNSET}, // IN 1
	{.action_type=UNSET}, // IN 2
	{.action_type=UNSET}, // IN 3
	{.action_type=UNSET}, // 
	{.action_type=UNSET}, //
	{.action_type=UNSET}, //
	{.action_type=UNSET}, //
	{.action_type=UNSET}, //
	{.action_type=UNSET}, //
	{.action_type=UNSET}, //
	{.action_type=UNSET}, //
};

bool rs232only = false;
bool hw_uart_dips = false;

int gpio_callback(onrisc_gpios_t gpios, void* data)
{
	int port_nr = ALL_PORTS;
	int i;
	
	if(gpios.mask == 0) {
		port_nr = *((int *) data);
	}
	for(i=0 ; i < caps->gpios->ngpio/2; i++) {
		if (gpios.mask & (1 << i)) {
			if (gpios.value & (1 << i)) {
				if(gpio_list[i].action_type == SERIAL_MODE) {
					onrisc_set_uart_mode_raw(port_nr, gpio_list[i].action.mode);
				} else 	if(gpio_list[i].action_type == SHELL) {
					system(gpio_list[i].action.cmd);
				}
			} else {
				if(gpio_list[i+caps->gpios->ngpio/2].action_type == SERIAL_MODE) {
					onrisc_set_uart_mode_raw(port_nr, gpio_list[i+caps->gpios->ngpio/2].action.mode);
				} else 	if(gpio_list[i+caps->gpios->ngpio/2].action_type == SHELL) {
					system(gpio_list[i+caps->gpios->ngpio/2].action.cmd);
				}
			}
		}
	}
}	

void print_usage()
{
	fprintf(stderr, "\nOnRISC GPIO Daemon (Version %s)\n\n", LIBONRISC_VERSION);
	fprintf(stderr, "Usage: onriscgpiod -d\n");
	fprintf(stderr, "       onriscgpiod -e\n");
	fprintf(stderr,
		"Options: -c                 read configs from /etc/onriscgpiod.conf\n");
	fprintf(stderr,
		"         -d                 start as GPIO daemon\n");
	fprintf(stderr, "         -e                 execute callback at start\n");
}

int readconfig_gpio(char *filename)
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

	fp = fopen("/etc/onriscgpiod.conf", "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	while ((read = getline(&line, &len, fp)) != -1) {
		if (!read)
			continue;
		buf=strtok(line,":");
		sscanf(buf, "%01X", &idx);
		buf=strtok(NULL,":");
		sscanf(buf, "%s", type);
		if (!strcmp(type,"unset")){
			gpio_list[idx].action_type = UNSET;
		} else if (!strcmp(type, "serial-mode")){
			if (rs232only || hw_uart_dips){
				gpio_list[idx].action_type = UNSET;
				continue;
			}
			gpio_list[idx].action_type = SERIAL_MODE;
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
			
			gpio_list[idx].action.mode = mode;
		} else if (!strcmp(type, "shell")){
			gpio_list[idx].action_type = SHELL;

			if(strlen(cmd) > 255)
				exit(EXIT_FAILURE);

			buf=strtok(NULL,"\n");
			strcpy(gpio_list[idx].action.cmd, buf);
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

	if (onrisc_init(&onrisc_system) == EXIT_FAILURE) {
		printf("Failed to init\n");
		goto error;
	}

	caps = onrisc_get_dev_caps();
	
	if (argc == 1) {
		print_usage();
		return 1;
	}

	rs232only = !(caps->uarts->flags & UARTS_SWITCHABLE);	
	hw_uart_dips = !!(caps->uarts->flags & UARTS_DIPS_PHYSICAL);	
	
	/* handle command line params */
	while ((opt = getopt(argc, argv, "cedh?")) != -1) {
		switch (opt) {
		case 'e':
			if (!caps->gpios) {
				printf("This device has no GPIOs\nNothing to do\n");
				break;
			}
			dips.mask=0xF;
			i=ALL_PORTS;
			onrisc_gpio_get_value(&dips);
			gpio_callback(dips,&i);
			break;
		case 'd':
			if (!caps->gpios) {
				printf("This device has no GPIOs\nNothing to do for daemon\n");
				break;
			}
			onrisc_gpios_t mask;
			mask.mask=0xF;
			onrisc_gpio_register_callback(mask, gpio_callback, NULL, BOTH);
			pause();
			break;
		case 'c':
			readconfig_gpio(NULL);
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
