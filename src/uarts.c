#include "vssys.h"

gpio *mode_gpios[8];

int onrisc_set_sr485_ioctl(int port_nr, int on)
{
	int rc = EXIT_SUCCESS;
	int fd = 0;
	char port[16];
	struct serial_rs485 rs485conf ;

	if (!(onrisc_capabilities.uarts->ctrl[port_nr - 1].flags & RS_HAS_485_SW)) {
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
		rs485conf.flags |= SER_RS485_ENABLED;
		rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND) ;
		rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;
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
	int base = 0;
	onrisc_dip_switch_t *ctrl = &onrisc_capabilities.uarts->ctrl[port_nr - 1];

	if (ctrl->flags & RS_IS_SETUP) {
		return rc;
	}

	if (ctrl->flags & RS_NEEDS_I2C_ADDR) {
		if (onrisc_get_tca6416_base(&base, ctrl->i2c_id) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}

	for (i = 0; i < ctrl->num; i++) {
		ctrl->gpio[i] = libsoc_gpio_request(base + ctrl->pin[i], LS_SHARED);
		if (NULL == ctrl->gpio[i]) {
			rc = EXIT_FAILURE;
			goto error;
		}

		if (libsoc_gpio_set_direction(ctrl->gpio[i], dir) == EXIT_FAILURE) {
			rc = EXIT_FAILURE;
			goto error;
		}
	}

	ctrl->flags |= RS_IS_SETUP;

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
	onrisc_dip_switch_t *ctrl = &onrisc_capabilities.uarts->ctrl[port_nr - 1];

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
			libsoc_gpio_set_level(ctrl->gpio[0], HIGH);
			libsoc_gpio_set_level(ctrl->gpio[1], LOW);
			libsoc_gpio_set_level(ctrl->gpio[2], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 0) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_RS422:
			libsoc_gpio_set_level(ctrl->gpio[0], HIGH);
			libsoc_gpio_set_level(ctrl->gpio[1], HIGH);
			libsoc_gpio_set_level(ctrl->gpio[2], HIGH);
			if (onrisc_set_sr485_ioctl(port_nr, 0) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_RS485_FD:
			libsoc_gpio_set_level(ctrl->gpio[0], HIGH);
			libsoc_gpio_set_level(ctrl->gpio[1], HIGH);
			libsoc_gpio_set_level(ctrl->gpio[2], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 1) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_RS485_HD:
			libsoc_gpio_set_level(ctrl->gpio[0], LOW);
			libsoc_gpio_set_level(ctrl->gpio[1], HIGH);
			libsoc_gpio_set_level(ctrl->gpio[2], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 1) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		case TYPE_LOOPBACK:
			libsoc_gpio_set_level(ctrl->gpio[0], LOW);
			libsoc_gpio_set_level(ctrl->gpio[1], LOW);
			libsoc_gpio_set_level(ctrl->gpio[2], LOW);
			if (onrisc_set_sr485_ioctl(port_nr, 0) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			break;
		default:
			return EXIT_FAILURE;
	}

	/* handle termination */
	if (mode->termination) {
		libsoc_gpio_set_level(ctrl->gpio[3], HIGH);
	} else {
		libsoc_gpio_set_level(ctrl->gpio[3], LOW);
	}

	return EXIT_SUCCESS;
}

int onrisc_set_uart_mode(int port_nr, onrisc_uart_mode_t * mode)
{
	int rc = EXIT_SUCCESS;

	if (NULL == onrisc_capabilities.uarts) {
		fprintf(stderr, "device has no switchable UARTs\n");
		rc = EXIT_FAILURE;
		goto error;
	}

	if (onrisc_capabilities.uarts->ctrl[port_nr - 1].flags & RS_IS_GPIO_BASED) {
		rc = onrisc_set_uart_mode_omap3(port_nr, mode);
	}
	else {
		rc = onrisc_set_uart_mode_ks8695(port_nr, mode);
	}

error:
	return rc;
}
