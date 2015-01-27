#include "vssys.h"

gpio *mode_gpios[8];
int init_uart_modes_flag = 0;

int onrisc_set_sr485_ioctl(int port_nr, int on)
{
	int rc = EXIT_SUCCESS;
	int fd = 0;
	char port[16];
	struct serial_rs485 rs485conf ;

	if ((onrisc_system.model != BALIOS_IR_3220) && (onrisc_system.model != BALIOS_IR_5221)) {
		goto error;
	}

	sprintf(port, "/dev/ttyO%d", port_nr);

	fd = open(port, O_RDWR);
	if (fd <= 0) {
		fprintf(stderr, "failed to open /dev/ttyO%d\n", port_nr);
		rc = EXIT_FAILURE;
		goto error ;
	}
	/* get current RS485 settings */
	if (ioctl(fd , TIOCGRS485, &rs485conf ) < 0) {
		fprintf(stderr, "failed to invoke TIOCGRS485\n");
		rc = EXIT_FAILURE;
		goto error ;
	}

	if (on) {
		/* enable RS485 mode */
		rs485conf . flags |= SER_RS485_ENABLED;
		rs485conf . flags &= ~(SER_RS485_RTS_ON_SEND) ;
		rs485conf . flags |= SER_RS485_RTS_AFTER_SEND;
	}
	else {
		/* disable RS485 mode */
		rs485conf . flags &= ~(SER_RS485_ENABLED);
	}

	/* set new RS485 mode */
	if (ioctl(fd , TIOCSRS485, &rs485conf ) < 0) {
		fprintf(stderr, "failed to invoke TIOCSRS485\n");
		rc = EXIT_FAILURE;
		goto error ;
	}

error:
	if (fd > 0) {
		close(fd);
	}
	return rc;
}

int onrisc_setup_uart_gpios(int dir, int port_nr)
{
	int i, rc = EXIT_SUCCESS;
	int base;

	if (init_uart_modes_flag) {
		return rc;
	}

	switch(onrisc_system.model) {
		case ALEKTO2:
			if (onrisc_get_tca6416_base(&base, 0x21) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			serial_mode_first_pin = base;
			break;
		default:
			if (onrisc_get_tca6416_base(&base, 0x20) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			serial_mode_first_pin = base + 8;
			break;
	}

	for (i = 0; i < 4; i++) {
		int idx = i + 4 * (port_nr - 1);
		mode_gpios[idx] = libsoc_gpio_request(serial_mode_first_pin + idx, LS_SHARED);
		if (mode_gpios[idx] == NULL) {
			rc = EXIT_FAILURE;
			goto error;
		}

		if (libsoc_gpio_set_direction(mode_gpios[idx], dir) == EXIT_FAILURE) {
			rc = EXIT_FAILURE;
			goto error;
		}
	}

	init_uart_modes_flag = 1;

error:
	return rc;
}

int onrisc_proc_write(char *file, char *str)
{
	int rc = EXIT_SUCCESS;

	FILE *fp = fopen(file, "w");
	if (fp == NULL) {
		rc = EXIT_FAILURE;
		goto error;
	}

	fwrite(str, 1, strlen(str), fp);

	fclose(fp);
error:
	return rc;
}

int onrisc_set_uart_mode_ks8695(int port_nr, onrisc_uart_mode_t *mode)
{
	char file[64];

	sprintf(file, "/proc/vscom/epld_ttyS%d", port_nr);

	switch (mode->rs_mode) {
		case TYPE_LOOPBACK:
			onrisc_proc_write(file, "inactive");
			break;
		case TYPE_RS232:
			onrisc_proc_write(file, "rs232");
			break;
		case TYPE_RS422:
			onrisc_proc_write(file, "rs422");
			break;
		case TYPE_RS485_FD:
			if (mode->dir_ctrl == DIR_ART) {
				onrisc_proc_write(file, "rs485byart-4-wire");
			} else {
				onrisc_proc_write(file, "rs485byrts-4-wire");
			}
			break;
		case TYPE_RS485_HD:
			if (mode->dir_ctrl == DIR_ART) {
				if (mode->echo) {
					onrisc_proc_write(file, "rs485byart-2-wire-echo");
				} else {
					onrisc_proc_write(file, "rs485byart-2-wire-noecho");
				}
			} else {
				if (mode->echo) {
					onrisc_proc_write(file, "rs485byrts-2-wire-echo");
				} else {
					onrisc_proc_write(file, "rs485byrts-2-wire-noecho");
				}
			}
			break;
	}
}

int onrisc_set_uart_mode_omap3(int port_nr, onrisc_uart_mode_t * mode)
{
	/* special handling for TYPE_DIP */
	if (mode->rs_mode == TYPE_DIP) {
		return onrisc_setup_uart_gpios(INPUT, port_nr);
	}

	/* handle RS-modes */
	if (onrisc_setup_uart_gpios(OUTPUT, port_nr) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	switch (mode->rs_mode) {
		case TYPE_RS232:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 0) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_RS422:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], HIGH);
			if (onrisc_set_sr485_ioctl(port_nr, 0) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_RS485_FD:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 1) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_RS485_HD:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 1) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_LOOPBACK:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 0) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		default:
			return EXIT_FAILURE;
	}

	/* handle termination */
	if (mode->termination) {
		libsoc_gpio_set_level(mode_gpios[3 + 4 * (port_nr - 1)], HIGH);
	} else {
		libsoc_gpio_set_level(mode_gpios[3 + 4 * (port_nr - 1)], LOW);
	}

	return EXIT_SUCCESS;
}

int onrisc_set_uart_mode(int port_nr, onrisc_uart_mode_t * mode)
{
	int rc = EXIT_SUCCESS;

	switch (onrisc_system.model) {
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			rc = onrisc_set_uart_mode_ks8695(port_nr, mode);
			break;
		case VS860:
		case ALEKTO2:
		case BALIOS_IR_3220:
		case BALIOS_IR_5221:
			rc = onrisc_set_uart_mode_omap3(port_nr, mode);
			break;
		default:
			rc = EXIT_FAILURE;
			fprintf(stderr, "thin device doesn't support UART mode switching\n");
			break;
	}

	return rc;
}
