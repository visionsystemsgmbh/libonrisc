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

int onrisc_get_sw_state(onrisc_sw_caps_t *sw, gpio_level *state)
{
	int rc = EXIT_FAILURE;
	gpio_direction dir;

	if (!(sw->flags & SW_IS_SETUP)) {
		/* request GPIO */
		sw->gpio = libsoc_gpio_request(sw->pin, LS_SHARED);
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
		sw->gpio = libsoc_gpio_request(sw->pin, LS_SHARED);
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
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;

	if (eeprom->path != NULL) {
		return EXIT_SUCCESS;
	}

	/* create the udev object */
	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "can't create udev object\n");
		goto error;
	}

	/* create a list of the devices in the 'i2c' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "i2c");
	udev_enumerate_add_match_sysattr(enumerate, "eeprom", NULL);
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;

		/* get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		eeprom->path = malloc(strlen(path) + 10);
		if (NULL == eeprom->path) {
			fprintf(stderr, "failed to allocate memory for EEPROM path\n");
			goto error;
		}
		sprintf(eeprom->path, "%s/eeprom", path);

		rc = EXIT_SUCCESS;
		break;
	}

	/* clean up */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
error:
	return rc;
}

int onrisc_get_i2c_address(const char *path)
{
	int addr = -1;
	char *ptr, *ptr2;
	
	ptr = strstr(path, "i2c");
	if (ptr == NULL) {
		goto error;
	}
	
	ptr2 = strchr(ptr + 10, '-');
	if (ptr2 == NULL) {
		goto error;
	}

	if (sscanf(ptr2, "-%4x/", &addr) != 1) {
		fprintf(stderr, "i2c addres not found\n");
		goto error;
	}

	return addr;
error: 
	return -1;
}

int onrisc_find_ip175d(void)
{
	int rc = EXIT_FAILURE, ret;
	FILE *fp;
	char buf[32];

	fp = fopen(ETH0_PHY, "r");
	if (fp == NULL) {
		goto error;
	}

	while (fgets(buf, sizeof(buf), fp)) {
		if (strcmp(buf, "0x02430d80")) {
			break;
		}
	}

	rc = EXIT_SUCCESS;
 error:
	if (fp != NULL) {
		fclose(fp);
	}
	return rc;
}

int onrisc_get_tca6416_base(int *base, int addr)
{
	int rc = EXIT_FAILURE;
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;

	*base = 0;

	/* create the udev object */
	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "can't create udev object\n");
		goto error;
	}

	/* create a list of the devices in the 'gpio' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "gpio");
	udev_enumerate_add_match_sysattr(enumerate, "label", "tca6416");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path, *value;
		int addr_tmp;

		/* get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);

		/* extract i2c address from /sys path */
		addr_tmp = onrisc_get_i2c_address(path);
		if (addr_tmp == -1) {
			fprintf(stderr, "failed to get i2c address for tca6416\n");
			goto error;
		}

		if (addr != addr_tmp) {
			continue;
		}

		dev = udev_device_new_from_syspath(udev, path);
		if (dev == NULL) {
			fprintf(stderr, "failed to create udev device\n");
			goto error;
		}

		value = udev_device_get_sysattr_value(dev, "base");
		if (value == NULL) {
			fprintf(stderr, "failed to get tca6416 GPIO base\n");
			udev_device_unref(dev);
			goto error;
		}

		*base = atoi(value);

		udev_device_unref(dev);
	}

	/* clean up */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	if (*base == 0) {
		fprintf(stderr, "tca6416 wasn't found\n");
		goto error;
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}
