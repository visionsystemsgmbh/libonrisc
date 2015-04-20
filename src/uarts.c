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

	fd = open(port, O_RDWR | O_NONBLOCK);
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

	/* check, if GPIOs are already initialized */
	if (ctrl->flags & RS_IS_SETUP) {
		/* check GPIO direction settings */
		if (libsoc_gpio_get_direction(ctrl->gpio[0]) == dir) {
			return rc;
		}
	}

	if (ctrl->flags & RS_NEEDS_I2C_ADDR) {
		if (onrisc_get_tca6416_base(&base, ctrl->i2c_id) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}

	for (i = 0; i < ctrl->num; i++) {
		/* create new GPIO object only, if not already done */
		if (!(ctrl->flags & RS_IS_SETUP)) {
			ctrl->gpio[i] = libsoc_gpio_request(base + ctrl->pin[i], LS_SHARED);
			if (NULL == ctrl->gpio[i]) {
				rc = EXIT_FAILURE;
				goto error;
			}
		}

		if (DIRECTION_ERROR != dir) {
			if (libsoc_gpio_set_direction(ctrl->gpio[i], dir) == EXIT_FAILURE) {
				rc = EXIT_FAILURE;
				goto error;
			}
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

int onrisc_get_uart_mode_omap3(int port_nr, onrisc_uart_mode_t * mode)
{
	int i,rc = EXIT_FAILURE;
	gpio_direction dir;
	onrisc_dip_switch_t *ctrl = &onrisc_capabilities.uarts->ctrl[port_nr - 1];
	gpio_level val[ctrl->num];

	memset(mode, 0, sizeof(onrisc_uart_mode_t));

	/* check, if GPIOs are already initialized */
	if (!(ctrl->flags & RS_IS_SETUP)) {
		if (onrisc_setup_uart_gpios(DIRECTION_ERROR, port_nr) == EXIT_FAILURE) {
			goto error;
		}
	}

	for (i = 0; i < ctrl->num; i++) {
		val[i] = libsoc_gpio_get_level(ctrl->gpio[i]);
		if (val[i] == LEVEL_ERROR) {
			goto error;
		}
	}

	/* get termination settings */
	if (val[3] == HIGH) {
		mode->termination = 1;
	}

	/* get RS settings */
	if (val[0] == HIGH && val[1] == LOW && val[2] == LOW) {
		mode->rs_mode = TYPE_RS232;
	} else if (val[0] == HIGH && val[1] == HIGH && val[2] == HIGH) {
		mode->rs_mode = TYPE_RS422;
	} else if (val[0] == HIGH && val[1] == HIGH && val[2] == LOW) {
		mode->rs_mode = TYPE_RS485_FD;
	} else if (val[0] == LOW && val[1] == HIGH && val[2] == LOW) {
		mode->rs_mode = TYPE_RS485_HD;
	} else if (val[0] == LOW && val[1] == LOW && val[2] == LOW) {
		mode->rs_mode = TYPE_LOOPBACK;
	} else {
		fprintf(stderr, "unknown RS mode\n");
		goto error;
	}

	dir = libsoc_gpio_get_direction(ctrl->gpio[0]);
	switch(dir) {
		case INPUT:
			mode->src = INPUT;
			break;
		case OUTPUT:
			mode->src = OUTPUT;
			break;
		case DIRECTION_ERROR:
			fprintf(stderr, "failed to get RS mode source\n");
			goto error;
	}
	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_get_uart_mode(int port_nr, onrisc_uart_mode_t * mode)
{
	int rc = EXIT_SUCCESS;

	if ((NULL == onrisc_capabilities.uarts) || !(UARTS_SWITCHABLE & onrisc_capabilities.uarts->flags)) {
		fprintf(stderr, "device has no switchable UARTs\n");
		rc = EXIT_FAILURE;
		goto error;
	}

	if (onrisc_capabilities.uarts->ctrl[port_nr - 1].flags & RS_IS_GPIO_BASED) {
		rc = onrisc_get_uart_mode_omap3(port_nr, mode);
	}
	else {
		rc = EXIT_SUCCESS;
	}

error:
	return rc;

}

int onrisc_get_uart_mode_raw(int port_nr, uint32_t * mode)
{
	int rc = EXIT_FAILURE;

	if ((NULL == onrisc_capabilities.uarts) || !(UARTS_SWITCHABLE & onrisc_capabilities.uarts->flags)) {
		fprintf(stderr, "device has no switchable UARTs\n");
		goto error;
	}

	if (onrisc_capabilities.uarts->ctrl[port_nr - 1].flags & RS_IS_GPIO_BASED) {
		int i;
		gpio_direction dir;
		onrisc_dip_switch_t *ctrl = &onrisc_capabilities.uarts->ctrl[port_nr - 1];
		gpio_level val;

		/* check, if GPIOs are already initialized */
		if (!(ctrl->flags & RS_IS_SETUP)) {
			if (onrisc_setup_uart_gpios(DIRECTION_ERROR, port_nr) == EXIT_FAILURE) {
				goto error;
			}
		}

		*mode=0;
		
		for (i = 0; i < ctrl->num; i++) {
			val = libsoc_gpio_get_level(ctrl->gpio[i]);
			if (val == LEVEL_ERROR) {
				goto error;
			} else if (val == HIGH){
				*mode |= 1 << i;
			}
		}

		*mode |= (!!libsoc_gpio_get_direction(ctrl->gpio[0])) << i;

		rc = EXIT_SUCCESS;
	} 

error:
	return rc;
}

int onrisc_set_uart_mode_raw(int port_nr, uint8_t mode)
{
	int i;
	if (port_nr == -1) {
		for (i=1; i <= onrisc_capabilities.uarts->num; ++i) {
			if(onrisc_set_uart_mode_raw(i, mode) == EXIT_FAILURE)
				return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}	 

	onrisc_dip_switch_t *ctrl = &onrisc_capabilities.uarts->ctrl[port_nr - 1];

	if ((NULL == onrisc_capabilities.uarts) || !(UARTS_SWITCHABLE & onrisc_capabilities.uarts->flags)) {
		fprintf(stderr, "device has no switchable UARTs\n");
		return EXIT_FAILURE;
	}

	/* handle RS-modes */
	if (onrisc_setup_uart_gpios(OUTPUT, port_nr) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	
	assert(ctrl->num == 4);

	libsoc_gpio_set_level(ctrl->gpio[0], (mode & DIP_S1)? HIGH:LOW);
	libsoc_gpio_set_level(ctrl->gpio[1], (mode & DIP_S2)? HIGH:LOW);
	libsoc_gpio_set_level(ctrl->gpio[2], (mode & DIP_S3)? HIGH:LOW);
	libsoc_gpio_set_level(ctrl->gpio[3], (mode & DIP_S4)? HIGH:LOW);

	uint8_t rs485 = (mode == DIP_S2) || (mode == (DIP_S2 | DIP_S1)) ||(mode == (DIP_S4 | DIP_S2)) || (mode == (DIP_S4 | DIP_S2 | DIP_S1));
	if (onrisc_set_sr485_ioctl(port_nr, rs485) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int onrisc_set_uart_mode(int port_nr, onrisc_uart_mode_t * mode)
{
	int rc = EXIT_SUCCESS;

	if ((NULL == onrisc_capabilities.uarts) || !(UARTS_SWITCHABLE & onrisc_capabilities.uarts->flags)) {
		fprintf(stderr, "device has no switchable UARTs\n");
		rc = EXIT_FAILURE;
		goto error;
	}

	if ((TYPE_DIP == mode->rs_mode) && !(UARTS_DIPS_PHYSICAL & onrisc_capabilities.uarts->flags)) {
		fprintf(stderr, "device has no UART-DIPs\n");
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
