#include "vssys.h"

char *mtd_dev(const char *partition)
{
	FILE *f;
	char buf[128];
	char *par = NULL;
	char *dev;
	int i = 1;
	char *end;

	dev = malloc(20);
	if (dev == NULL)
		return NULL;

	sprintf(dev, "/dev/mtdblock000");

	f = fopen("/proc/mtd","r");
	if (!f) {
		goto error;
	}

	while (fgets(buf, sizeof(buf), f))
	{
		par = strchr(buf, '\"');
		if (par) {
			par++;
			end = strrchr(buf, '\"');
			if (end) {
				*end = '\0';
				if (!(strcmp(par, partition))) {
					*(strchr(buf, ':')) = '\0';
					strncpy(dev + 13, buf + 3, 3);
					goto out;
				}
			}
		}
		i++;
	}
error:
	free(dev);
	dev = NULL;
out:
	if (f)
		fclose(f);

	return dev;
}

int onrisc_sw_init(onrisc_sw_caps_t *sw)
{
	int rc = EXIT_FAILURE;
	gpio_direction dir;

	if (!(sw->flags & SW_IS_SETUP)) {
		/* request GPIO */
		sw->gpio = libsoc_gpio_request(sw->pin, LS_GPIO_SHARED);
		if (NULL == sw->gpio) {
			fprintf(stderr, "failed to register switch GPIO\n");
			goto error;
		}

		/* check GPIO direction settings */
		if ((dir = libsoc_gpio_get_direction(sw->gpio)) == DIRECTION_ERROR) {
			fprintf(stderr, "failed to get GPIO direction\n");
			return rc;
		}

		/* set direction */
		if (sw->flags & SW_IS_READ_ONLY) {
			if (INPUT != dir) {
				if (libsoc_gpio_set_direction(sw->gpio, INPUT) == EXIT_FAILURE) {
					fprintf(stderr, "failed to set GPIO dir\n");
					goto error;
				}
			}
		} else {
			if (OUTPUT != dir) {
				if (libsoc_gpio_set_direction(sw->gpio, OUTPUT) == EXIT_FAILURE) {
					fprintf(stderr, "failed to set GPIO dir\n");
					goto error;
				}
			}
		}

		sw->flags |= SW_IS_SETUP;
	}

	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_get_sw_state(onrisc_sw_caps_t *sw, gpio_level *state)
{
	int rc = EXIT_FAILURE;
	gpio_direction dir;

	if (!(sw->flags & SW_IS_SETUP)) {
		/* request GPIO */
		sw->gpio = libsoc_gpio_request(sw->pin, LS_GPIO_SHARED);
		if (NULL == sw->gpio) {
			fprintf(stderr, "failed to register switch GPIO\n");
			goto error;
		}

		/* check GPIO direction settings */
		if ((dir = libsoc_gpio_get_direction(sw->gpio)) == DIRECTION_ERROR) {
			fprintf(stderr, "failed to get GPIO direction\n");
			return rc;
		}

		/* set direction */
		if (sw->flags & SW_IS_READ_ONLY) {
			if (INPUT != dir) {
				if (libsoc_gpio_set_direction(sw->gpio, INPUT) == EXIT_FAILURE) {
					fprintf(stderr, "failed to set GPIO dir\n");
					goto error;
				}
			}
		} else {
			if (OUTPUT != dir) {
				if (libsoc_gpio_set_direction(sw->gpio, OUTPUT) == EXIT_FAILURE) {
					fprintf(stderr, "failed to set GPIO dir\n");
					goto error;
				}
			}
		}

		sw->flags |= SW_IS_SETUP;
	}

	if ((*state = libsoc_gpio_get_level(sw->gpio)) == LEVEL_ERROR) {
		fprintf(stderr, "failed to get GPIO level\n");
		goto error;
	}

	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_set_sw_state(onrisc_sw_caps_t *sw, gpio_level state)
{
	int rc = EXIT_FAILURE;
	gpio_direction dir;

	if (!(sw->flags & SW_IS_SETUP)) {
		/* request GPIO */
		sw->gpio = libsoc_gpio_request(sw->pin, LS_GPIO_SHARED);
		if (NULL == sw->gpio) {
			fprintf(stderr, "failed to register switch GPIO\n");
			goto error;
		}

		/* check GPIO direction settings */
		if ((dir = libsoc_gpio_get_direction(sw->gpio)) == DIRECTION_ERROR) {
			fprintf(stderr, "failed to get GPIO direction\n");
			return rc;
		}

		/* set direction */
		if (sw->flags & SW_IS_READ_ONLY) {
			if (INPUT != dir) {
				if (libsoc_gpio_set_direction(sw->gpio, INPUT) == EXIT_FAILURE) {
					fprintf(stderr, "failed to set GPIO dir\n");
					goto error;
				}
			}
		} else {
			if (OUTPUT != dir) {
				if (libsoc_gpio_set_direction(sw->gpio, OUTPUT) == EXIT_FAILURE) {
					fprintf(stderr, "failed to set GPIO dir\n");
					goto error;
				}
			}
		}

		sw->flags |= SW_IS_SETUP;
	}

	if (libsoc_gpio_set_level(sw->gpio, state) == LEVEL_ERROR) {
		fprintf(stderr, "failed to set GPIO level\n");
		goto error;
	}

	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_get_eeprom(onrisc_eeprom_t *eeprom)
{
	int rc = EXIT_FAILURE;
	DIR* dirp;
	struct dirent* direntp;
	struct stat buf;
	char path[256];

	if (eeprom->path != NULL) {
		return EXIT_SUCCESS;
	}

	dirp = opendir( "/sys/bus/i2c/devices" );
	if( dirp == NULL ) {
		fprintf(stderr, "can't find sysfs tree\n");
		goto error;
	} else {
		for(;;) {
			direntp = readdir( dirp );
			if( direntp == NULL ) break;
			sprintf(path, "/sys/bus/i2c/devices/%s/eeprom", direntp->d_name);
			if(!stat(path, &buf)) {
					eeprom->path = malloc(strlen(path));
					sprintf(eeprom->path, "%s", path);
					rc = EXIT_SUCCESS;
					break;
				}
			}

		closedir( dirp );

		if (NULL == eeprom->path) {
			fprintf(stderr, "failed to allocate memory for EEPROM path\n");
			goto error;
		}
	}

error:
	return rc;
}

int onrisc_get_i2c_address(const char *gpiochip)
{
	int addr = -1;
	char buf[512], path[128];
	
	sprintf(path, "/sys/class/gpio/%s", gpiochip);
	if (readlink(path, buf, sizeof(buf)) < 0) {
		perror("Read gpiochip symlink:");
		goto error;
	}

	if (strstr(buf, "-0020") != NULL)
		addr = 0x20;
	else if (strstr(buf, "-0021") != NULL)
		addr = 0x21;

error: 
	return addr;
}

int onrisc_find_ip175d(void)
{
	int rc = EXIT_FAILURE;
	FILE *fp;
	char buf[32];

	fp = fopen(ETH0_PHY, "r");
	if (fp == NULL) {
		fp = fopen(ETH0_PHY_FIXED, "r");
		if (fp == NULL)
			goto error;
	}

	while (fgets(buf, sizeof(buf), fp)) {
		buf[strcspn(buf, "\r\n")] = 0;
		if (strcmp(buf, "0x02430d80") && strcmp(buf, "0x00000000"))
			goto error;
	}

	rc = EXIT_SUCCESS;
 error:
	if (fp != NULL)
		fclose(fp);

	return rc;
}

int onrisc_get_tca6416_base(int *base, int addr)
{
	int rc = EXIT_FAILURE;
	DIR* dirp;
	struct dirent* direntp;
	char buf[256];
	char path[256];

	char tca_model[10] = "tca6416\n";

	*base = 0;

	if (onrisc_system.model == NETIO
	    || onrisc_system.model == NETIO_WLAN
	    || onrisc_system.model == NETCOM_PLUS_ECO_113A
	    || onrisc_system.model == NETCOM_PLUS_ECO_213A) {
		sprintf(tca_model, "tca6408\n");
	}

	dirp = opendir( "/sys/class/gpio" );
	if( dirp == NULL ) {
		fprintf(stderr, "can't find sysfs tree\n");
		goto error;
	} else {
		for(;;) {
			FILE* fp;
			direntp = readdir(dirp);
			if(direntp == NULL)
				break;
			if(strstr(direntp->d_name, "gpiochip") == NULL)
				continue;
			sprintf(path, "/sys/class/gpio/%s/device/name", direntp->d_name);
			if ((fp = fopen(path, "r")) > 0 ) {
				fgets(buf, 255, fp);
				fclose(fp);
				if (!strcmp(tca_model, buf)) {
					if (onrisc_get_i2c_address(direntp->d_name) == addr) {
						sprintf(path, "/sys/class/gpio/%s/base", direntp->d_name);
						if((fp = fopen(path, "r")) > 0 ) {
							fgets(buf, 255, fp);
							sscanf(buf, "%d", base);
							fclose(fp);
						} else {
							fprintf(stderr, "gpio base wasn't found\n");
							goto error;
						}
					}
				}
			}
		}
		closedir(dirp);
	}

	if (*base == 0) {
		fprintf(stderr, "tca6416 wasn't found\n");
		goto error;
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_get_gpio_number(char *name, int def_nr)
{
	int chip, offset, base;

	if (onrisc_get_gpiochip_and_offset(name, &chip, &offset, &base) != EXIT_SUCCESS) {
		return def_nr;
	}

	return base + offset;
}
