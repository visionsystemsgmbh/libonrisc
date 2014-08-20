#include "vssys.h"

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
