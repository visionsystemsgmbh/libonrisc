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

dip_entry dip_list[16] = { 
	{.action_type=UNSET},				 // 0
	{.action_type=SERIAL_MODE, .action.mode=0b0001}, // 1 RS232
	{.action_type=UNSET},				 // 2
	{.action_type=SERIAL_MODE, .action.mode=0b0010}, // 3 RS485-HD
	{.action_type=SHELL, .action.cmd="echo Hello World!"}, // 4
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
	if(dip_list[dips.value].action_type == SERIAL_MODE) {
		onrisc_set_uart_mode_raw(ALL_PORTS, dip_list[dips.value].action.mode);
		uint8_t mode;
		onrisc_get_uart_mode_raw(2, &mode);
	} else 	if(dip_list[dips.value].action_type == SHELL) {
		system(dip_list[dips.value].action.cmd);
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
		"         -d                 start as daemon\n");
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
		sscanf(buf, "%01X", &idx);
		buf=strtok(NULL,":");
		sscanf(buf, "%s", type);
		if (!strcmp(type,"unset")){
			dip_list[idx].action_type = UNSET;
		} else if (!strcmp(type, "serial-mode")){
			dip_list[idx].action_type = SERIAL_MODE;
			buf=strtok(NULL,"\n");
			sscanf(buf, "%s", cmd);

			if(strlen(cmd) > 4)
				exit(EXIT_FAILURE);

			for (i=0; i<4; ++i){
				if (cmd[i] == '1') {
					mode |= 1 << (3 - i);
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
	int opt;
	int port = -1;
	int mode = TYPE_UNKNOWN;
	int termination = 0;
	int echo = 0;
	int dir_ctrl = DIR_ART;
	onrisc_gpios_t dips;
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
		print_usage();
		return 1;
	}
	
	/* handle command line params */
	while ((opt = getopt(argc, argv, "cedh?")) != -1) {
		switch (opt) {

		case 'c':
			readconfig(NULL);
			break;
		case 'd':
			onrisc_dip_register_callback(0, test_callback, NULL, BOTH);
			while(1);
			break;
		case 'e':
			dips.mask=0;
			onrisc_get_dips(&dips.value);
			test_callback(dips,NULL);
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
