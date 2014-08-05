#include "vssys.h"

onrisc_gpios_int_t onrisc_gpios;

int onrisc_gpio_init_alekto2()
{
	int i, rc = EXIT_FAILURE;
	int base = onrisc_gpios.base;
	gpio_direction cur_dir;

	for (i = 0; i < onrisc_gpios.ngpio / 2; i++) {
		onrisc_gpios.gpios[i].direction = INPUT;
		onrisc_gpios.gpios[i].dir_fixed = 0;
	}

	/* init driver control pins */

	/* group 0 */
	onrisc_gpios.gpios_ctrl[0].pin = libsoc_gpio_request(base, LS_SHARED);
	if (onrisc_gpios.gpios_ctrl[0].pin == NULL) {
		goto error;
	}
	onrisc_gpios.gpios_ctrl[1].pin =
	    libsoc_gpio_request(base + 1, LS_SHARED);
	if (onrisc_gpios.gpios_ctrl[1].pin == NULL) {
		goto error;
	}

	/* group 1 */
	onrisc_gpios.gpios_ctrl[2].pin =
	    libsoc_gpio_request(base + 4, LS_SHARED);
	if (onrisc_gpios.gpios_ctrl[2].pin == NULL) {
		goto error;
	}
	onrisc_gpios.gpios_ctrl[3].pin =
	    libsoc_gpio_request(base + 5, LS_SHARED);
	if (onrisc_gpios.gpios_ctrl[3].pin == NULL) {
		goto error;
	}

	/* group 2 */
	onrisc_gpios.gpios_ctrl[4].pin =
	    libsoc_gpio_request(base + 6, LS_SHARED);
	if (onrisc_gpios.gpios_ctrl[4].pin == NULL) {
		goto error;
	}
	onrisc_gpios.gpios_ctrl[5].pin =
	    libsoc_gpio_request(base + 7, LS_SHARED);
	if (onrisc_gpios.gpios_ctrl[5].pin == NULL) {
		goto error;
	}

	for (i = 0; i < 3; i++) {
		cur_dir =
		    libsoc_gpio_get_direction(onrisc_gpios.gpios[2 * i].pin);
		if (cur_dir != OUTPUT) {
			if (libsoc_gpio_set_direction
			    (onrisc_gpios.gpios_ctrl[2 * i].pin, OUTPUT) == EXIT_FAILURE) {
				goto error;
			}
		}
		cur_dir =
		    libsoc_gpio_get_direction(onrisc_gpios.gpios[2 * i + 1].pin);
		if (cur_dir != OUTPUT) {
			if (libsoc_gpio_set_direction
			    (onrisc_gpios.gpios_ctrl[2 * i + 1].pin,
			     OUTPUT) == EXIT_FAILURE) {
				goto error;
			}
		}

		if (libsoc_gpio_set_level
		    (onrisc_gpios.gpios_ctrl[2 * i].pin, LOW) == EXIT_FAILURE) {
			goto error;
		}
		if (libsoc_gpio_set_level
		    (onrisc_gpios.gpios_ctrl[2 * i + 1].pin,
		     HIGH) == EXIT_FAILURE) {
			goto error;
		}
	}

	rc = EXIT_SUCCESS;

 error:
	return rc;
}

int onrisc_gpio_init_balios()
{
	int i, rc = EXIT_FAILURE;

	for (i = 0; i < onrisc_gpios.ngpio / 2; i++) {
		gpio_direction cur_dir;
		int offset = i + onrisc_gpios.ngpio / 2;
		int base = onrisc_gpios.base;

		onrisc_gpios.gpios[i].direction = INPUT;
		onrisc_gpios.gpios[i].dir_fixed = 1;
		onrisc_gpios.gpios[offset].direction = OUTPUT;
		onrisc_gpios.gpios[offset].dir_fixed = 1;

		/* init inputs */
		onrisc_gpios.gpios[i].pin =
		    libsoc_gpio_request(i + base, LS_SHARED);
		if (onrisc_gpios.gpios[i].pin == NULL) {
			goto error;
		}

		/* get current direction and set desired one */
		cur_dir = libsoc_gpio_get_direction(onrisc_gpios.gpios[i].pin);
		if (cur_dir != onrisc_gpios.gpios[i].direction) {
			/* set direction */
			if (libsoc_gpio_set_direction
			    (onrisc_gpios.gpios[i].pin,
			     onrisc_gpios.gpios[i].direction) == EXIT_FAILURE) {
				goto error;
			}
		}

		/* init outputs */
		onrisc_gpios.gpios[offset].pin =
		    libsoc_gpio_request(offset + base, LS_SHARED);
		if (onrisc_gpios.gpios[offset].pin == NULL) {
			goto error;
		}

		/* get current direction and set desired one */
		cur_dir =
		    libsoc_gpio_get_direction(onrisc_gpios.gpios[offset].pin);
		if (cur_dir != onrisc_gpios.gpios[offset].direction) {
			/* set direction */
			if (libsoc_gpio_set_direction
			    (onrisc_gpios.gpios[offset].pin,
			     onrisc_gpios.gpios[offset].direction) ==
			    EXIT_FAILURE) {
				goto error;
			}
		}
	}

	rc = EXIT_SUCCESS;

 error:
	return rc;
}

int onrisc_gpio_init()
{
	int i, rc = EXIT_FAILURE;

	assert(init_flag == 1);

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
		onrisc_gpios.ngpio = 8;

		for (i = 0; i < onrisc_gpios.ngpio; i++) {
			onrisc_gpios.gpios[i].direction = INPUT;
			onrisc_gpios.gpios[i].dir_fixed = 0;
		}
		break;
	case ALENA:
		onrisc_gpios.ngpio = 8;

		/* inputs */
		onrisc_gpios.gpios[0].direction = INPUT;
		onrisc_gpios.gpios[0].dir_fixed = 1;
		onrisc_gpios.gpios[1].direction = INPUT;
		onrisc_gpios.gpios[1].dir_fixed = 1;

		/* changeable direction */
		for (i = 2; i < 6; i++) {
			onrisc_gpios.gpios[i].direction = INPUT;
			onrisc_gpios.gpios[i].dir_fixed = 0;
		}

		/* outputs */
		onrisc_gpios.gpios[6].direction = INPUT;
		onrisc_gpios.gpios[6].dir_fixed = 1;
		onrisc_gpios.gpios[7].direction = INPUT;
		onrisc_gpios.gpios[7].dir_fixed = 1;

		break;
	case ALEKTO2:
		onrisc_gpios.ngpio = 8;

		/*TODO implement initialization for Alekto 2 */
		return EXIT_SUCCESS;

		if (onrisc_gpio_init_alekto2() == EXIT_FAILURE) {
			goto error;
		}

		break;
	case BALIOS_IR_5221:
		onrisc_gpios.ngpio = 8;
		if (onrisc_gpio_init_balios() == EXIT_FAILURE) {
			goto error;
		}

		break;
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_set_direction_sysfs(onrisc_gpio_t * gpio)
{
	int rc = EXIT_FAILURE;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
	case ALEKTO2:
		break;
	case BALIOS_IR_5221:
		if (libsoc_gpio_set_direction(gpio->pin, gpio->direction) ==
		    EXIT_FAILURE) {
			goto error;
		}
		break;
	}

	rc = EXIT_SUCCESS;

 error:
	return rc;
}

int onrisc_gpio_set_value_sysfs(onrisc_gpio_t * gpio)
{
	int rc = EXIT_FAILURE;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
		break;
	case ALEKTO2:
		break;
	case BALIOS_IR_5221:
		if (libsoc_gpio_set_level(gpio->pin, gpio->value) ==
		    EXIT_FAILURE) {
			goto error;
		}
		break;
	}

	rc = EXIT_SUCCESS;

 error:
	return rc;
}

int onrisc_gpio_set_direction(onrisc_gpios_t * gpio_dir)
{
	int i, rc = EXIT_FAILURE;

	assert(init_flag == 1);

	for (i = 0; i < onrisc_gpios.ngpio; i++) {
		if (gpio_dir->mask & (1 << i)) {
			/* check, if it is a fixed GPIO */
			if (onrisc_gpios.gpios[i].dir_fixed) {
				printf
				    ("gpio %d is fixed and cannot change it's direction",
				     i);
				goto error;
			}

			/* get direction */
			onrisc_gpios.gpios[i].direction =
			    gpio_dir->value & (1 << i);

			/* set GPIO direction */
			if (onrisc_gpio_set_direction_sysfs
			    (&onrisc_gpios.gpios[i]) == EXIT_FAILURE) {
				goto error;
			}
		}
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_set_value(onrisc_gpios_t * gpio_val)
{
	int i, rc = EXIT_FAILURE;

	assert(init_flag == 1);

	for (i = 0; i < onrisc_gpios.ngpio; i++) {
		if (gpio_val->mask & (1 << i)) {
			/* check, if it is a fixed GPIO */
			if (onrisc_gpios.gpios[i].direction == INPUT) {
				continue;
			}

			/* get direction */
			onrisc_gpios.gpios[i].value =
			    gpio_val->value & (1 << i) ? HIGH : LOW;

			/* set GPIO direction */
			if (onrisc_gpio_set_value_sysfs
			    (&onrisc_gpios.gpios[i]) == EXIT_FAILURE) {
				goto error;
			}
		}
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_get_value(onrisc_gpios_t * gpio_val)
{
	int i, rc = EXIT_FAILURE;
	gpio_level level;

	assert(init_flag == 1);

	gpio_val->value = 0;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
		break;
	case ALEKTO2:
	case BALIOS_IR_5221:
		for (i = 0; i < onrisc_gpios.ngpio; i++) {
			if ((level =
			     libsoc_gpio_get_level(onrisc_gpios.gpios[i].
						   pin)) == LEVEL_ERROR) {
				goto error;
			}

			if (level == LOW) {
				gpio_val->value &= ~(1 << i);
			} else {
				gpio_val->value |= 1 << i;
			}
		}
		break;
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}
