#include "vssys.h"

int gpio_init_flag = 0;
onrisc_gpios_int_t * onrisc_gpios;
FILE * vdir;

int invert_level(int val)
{
	if (val != LEVEL_ERROR)
		val = (val == HIGH) ? LOW : HIGH;
	return val;
}

int onrisc_gpio_init_netio()
{
	int i, rc = EXIT_FAILURE;
	int base;
	gpio_direction cur_dir;

	onrisc_gpios = onrisc_capabilities.gpios;

	if (onrisc_get_tca6416_base(&onrisc_gpios->base, 0x20) == EXIT_FAILURE) {
		goto error;
	}

	base = onrisc_gpios->base;

	for (i = 0; i < onrisc_gpios->ngpio; i++) {
		/* init input pins */
		onrisc_gpios->gpios[i].dir_fixed = 0;
		onrisc_gpios->gpios[i].pin =
			libsoc_gpio_request(i + base, LS_GPIO_SHARED);
		if (onrisc_gpios->gpios[i].pin == NULL) {
			goto error;
		}

		/* init control pins */
		onrisc_gpios->gpios_ctrl[i].pin =
			libsoc_gpio_request(i + 4 + base, LS_GPIO_SHARED);
		if (onrisc_gpios->gpios_ctrl[i].pin == NULL) {
			goto error;
		}
		onrisc_gpios->gpios[i].direction =
		    libsoc_gpio_get_direction(onrisc_gpios->gpios_ctrl[i].pin);
	}

	rc = EXIT_SUCCESS;

error:
	return rc;
}

int onrisc_gpio_init_alekto2()
{
	int i, rc = EXIT_FAILURE;
	int base;
	gpio_direction cur_dir;

	onrisc_gpios = onrisc_capabilities.gpios;

	if (onrisc_get_tca6416_base(&onrisc_gpios->base, 0x20) == EXIT_FAILURE) {
		goto error;
	}

	base = onrisc_gpios->base;

	for (i = 0; i < onrisc_gpios->ngpio / 2; i++) {
		onrisc_gpios->gpios[i].direction = INPUT;
		onrisc_gpios->gpios[i].dir_fixed = 0;
	}

	/* init driver control pins */

	/* group 0 */
	onrisc_gpios->gpios_ctrl[0].pin = libsoc_gpio_request(base, LS_GPIO_SHARED);
	if (onrisc_gpios->gpios_ctrl[0].pin == NULL) {
		goto error;
	}
	onrisc_gpios->gpios_ctrl[1].pin =
	    libsoc_gpio_request(base + 1, LS_GPIO_SHARED);
	if (onrisc_gpios->gpios_ctrl[1].pin == NULL) {
		goto error;
	}

	/* group 1 */
	onrisc_gpios->gpios_ctrl[2].pin =
	    libsoc_gpio_request(base + 4, LS_GPIO_SHARED);
	if (onrisc_gpios->gpios_ctrl[2].pin == NULL) {
		goto error;
	}
	onrisc_gpios->gpios_ctrl[3].pin =
	    libsoc_gpio_request(base + 5, LS_GPIO_SHARED);
	if (onrisc_gpios->gpios_ctrl[3].pin == NULL) {
		goto error;
	}

	/* group 2 */
	onrisc_gpios->gpios_ctrl[4].pin =
	    libsoc_gpio_request(base + 6, LS_GPIO_SHARED);
	if (onrisc_gpios->gpios_ctrl[4].pin == NULL) {
		goto error;
	}
	onrisc_gpios->gpios_ctrl[5].pin =
	    libsoc_gpio_request(base + 7, LS_GPIO_SHARED);
	if (onrisc_gpios->gpios_ctrl[5].pin == NULL) {
		goto error;
	}

	for (i = 0; i < 3; i++) {
		gpio_level ctrl_in, ctrl_out;

		/* change control pins to OUTPUT, if not already done via
		 * previous invocation */
		cur_dir =
		    libsoc_gpio_get_direction(onrisc_gpios->gpios_ctrl[2 * i].pin);
		if (cur_dir != OUTPUT) {
			if (libsoc_gpio_set_direction
			    (onrisc_gpios->gpios_ctrl[2 * i].pin, OUTPUT) == EXIT_FAILURE) {
				goto error;
			}
		}
		cur_dir =
		    libsoc_gpio_get_direction(onrisc_gpios->gpios_ctrl[2 * i + 1].pin);
		if (cur_dir != OUTPUT) {
			if (libsoc_gpio_set_direction
			    (onrisc_gpios->gpios_ctrl[2 * i + 1].pin,
			     OUTPUT) == EXIT_FAILURE) {
				goto error;
			}
		}

		/* get current control signal levels */
		if ((ctrl_in = libsoc_gpio_get_level
		    (onrisc_gpios->gpios_ctrl[2 * i].pin)) == LEVEL_ERROR) {
			goto error;
		}

		if ((ctrl_out = libsoc_gpio_get_level
		    (onrisc_gpios->gpios_ctrl[2 * i + 1].pin)) == LEVEL_ERROR) {
			goto error;
		}

		/* if both drivers are on or off, configure them to
		 * default: input - on, output - off */
		if (ctrl_in == ctrl_out) {
			ctrl_in = LOW;
			ctrl_out = HIGH;

			if (libsoc_gpio_set_level
			    (onrisc_gpios->gpios_ctrl[2 * i].pin, ctrl_in) == EXIT_FAILURE) {
				goto error;
			}
			if (libsoc_gpio_set_level
			    (onrisc_gpios->gpios_ctrl[2 * i + 1].pin,
			     ctrl_out) == EXIT_FAILURE) {
				goto error;
			}
		} else {
			if (ctrl_out == LOW) {
				switch(i) {
					case 0:
						onrisc_gpios->gpios[0].direction = OUTPUT;
						onrisc_gpios->gpios[1].direction = OUTPUT;
						onrisc_gpios->gpios[2].direction = OUTPUT;
						onrisc_gpios->gpios[3].direction = OUTPUT;
						break;
					case 1:
						onrisc_gpios->gpios[4].direction = OUTPUT;
						onrisc_gpios->gpios[5].direction = OUTPUT;
						break;
					case 2:
						onrisc_gpios->gpios[6].direction = OUTPUT;
						onrisc_gpios->gpios[7].direction = OUTPUT;
						break;
				}
			}
		}
	}

	/* init data pins */
	for(i = 0; i < onrisc_gpios->ngpio; i++) {
		onrisc_gpios->gpios[i].pin =
		    libsoc_gpio_request(base + 8 + i, LS_GPIO_SHARED);
		if (onrisc_gpios->gpios[i].pin == NULL) {
			goto error;
		}

		cur_dir =
		    libsoc_gpio_get_direction(onrisc_gpios->gpios[i].pin);
		if (cur_dir != onrisc_gpios->gpios[i].direction) {
			/* set direction */
			if (libsoc_gpio_set_direction
			    (onrisc_gpios->gpios[i].pin,
			     onrisc_gpios->gpios[i].direction) ==
			    EXIT_FAILURE) {
				goto error;
			}
		}
	}

	rc = EXIT_SUCCESS;

 error:
	return rc;
}

int onrisc_gpio_init_baltos()
{
	int i, rc = EXIT_FAILURE;

	onrisc_gpios = onrisc_capabilities.gpios;

	if (onrisc_get_tca6416_base(&onrisc_gpios->base, 0x20) == EXIT_FAILURE) {
		goto error;
	}

	for (i = 0; i < onrisc_gpios->ngpio / 2; i++) {
		gpio_direction cur_dir;
		int offset = i + onrisc_gpios->ngpio / 2;
		int base = onrisc_gpios->base;

		/* init inputs */
		onrisc_gpios->gpios[i].pin =
		    libsoc_gpio_request(i + base, LS_GPIO_SHARED);
		if (onrisc_gpios->gpios[i].pin == NULL) {
			goto error;
		}

		/* get current direction and set desired one */
		cur_dir = libsoc_gpio_get_direction(onrisc_gpios->gpios[i].pin);
		if (cur_dir != onrisc_gpios->gpios[i].direction) {
			/* set direction */
			if (libsoc_gpio_set_direction
			    (onrisc_gpios->gpios[i].pin,
			     onrisc_gpios->gpios[i].direction) == EXIT_FAILURE) {
				goto error;
			}
		}

		/* init outputs */
		onrisc_gpios->gpios[offset].pin =
		    libsoc_gpio_request(offset + base, LS_GPIO_SHARED);
		if (onrisc_gpios->gpios[offset].pin == NULL) {
			goto error;
		}

		/* get current direction and set desired one */
		cur_dir =
		    libsoc_gpio_get_direction(onrisc_gpios->gpios[offset].pin);
		if (cur_dir != onrisc_gpios->gpios[offset].direction) {
			/* set direction */
			if (libsoc_gpio_set_direction
			    (onrisc_gpios->gpios[offset].pin,
			     onrisc_gpios->gpios[offset].direction) ==
			    EXIT_FAILURE) {
				goto error;
			}
		}
	}

	if (0 < onrisc_gpios->nvgpio) {
		if ((vdir = fopen("/tmp/vdir", "r+")) == NULL) {
			if ((vdir = fopen("/tmp/vdir", "w+")) == NULL) {
				goto error;
			}
		}

		int vdir_data = 0;
		int res = 0;
		if(0 >= (res = fscanf(vdir, "%1X", &vdir_data))) {
			vdir_data = 0;
			fprintf(vdir, "%X", vdir_data);
		}

		fclose(vdir);

		for (i = 0; i < onrisc_gpios->nvgpio; i++) {
			int offset = i + onrisc_gpios->ngpio;

			/* set direction */
			onrisc_gpios->gpios[offset].direction = (vdir_data & (1 << i)) ? OUTPUT : INPUT;
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

	onrisc_gpios = onrisc_capabilities.gpios;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
		onrisc_gpios->ngpio = 8;

		for (i = 0; i < onrisc_gpios->ngpio; i++) {
			onrisc_gpios->gpios[i].direction = INPUT;
			onrisc_gpios->gpios[i].dir_fixed = 0;
		}
		break;
	case ALENA:
		onrisc_gpios->ngpio = 8;

		/* inputs */
		onrisc_gpios->gpios[0].direction = INPUT;
		onrisc_gpios->gpios[0].dir_fixed = 1;
		onrisc_gpios->gpios[1].direction = INPUT;
		onrisc_gpios->gpios[1].dir_fixed = 1;

		/* changeable direction */
		for (i = 2; i < 6; i++) {
			onrisc_gpios->gpios[i].direction = INPUT;
			onrisc_gpios->gpios[i].dir_fixed = 0;
		}

		/* outputs */
		onrisc_gpios->gpios[6].direction = INPUT;
		onrisc_gpios->gpios[6].dir_fixed = 1;
		onrisc_gpios->gpios[7].direction = INPUT;
		onrisc_gpios->gpios[7].dir_fixed = 1;

		break;
	case ALEKTO2:
		onrisc_gpios->ngpio = 8;

		if (onrisc_gpio_init_alekto2() == EXIT_FAILURE) {
			fprintf(stderr, "failed to init gpios\n");
			goto error;
		}

		break;
	case BALTOS_IR_3220:
	case BALTOS_IR_5221:
	case BALTOS_DIO_1080:
	case NETCON3:
		if (onrisc_gpio_init_baltos() == EXIT_FAILURE) {
			fprintf(stderr, "failed to init gpios\n");
			goto error;
		}

		break;
	case NETIO:
	case NETIO_WLAN:
		if (onrisc_gpio_init_netio() == EXIT_FAILURE) {
			fprintf(stderr, "failed to init gpios\n");
			goto error;
		}

		break;
	}

	/* GPIO subsystem initialized */
	gpio_init_flag = 1;

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_set_value_sysfs(int idx, int val)
{
	int rc = EXIT_FAILURE;

	onrisc_gpios = onrisc_capabilities.gpios;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
		break;
	case BALTOS_IR_3220:
	case BALTOS_IR_5221:
		if (onrisc_gpios->gpios[idx].flags & GPIO_IS_VIRTUAL) {
			if (onrisc_gpios->gpios[idx].direction == OUTPUT) {
				if (libsoc_gpio_set_level(onrisc_gpios->gpios[idx - 4].pin, val) ==
				    EXIT_FAILURE) {
					goto error;
				}
			}
		} else {
			if (libsoc_gpio_set_level(onrisc_gpios->gpios[idx].pin, val) ==
			    EXIT_FAILURE) {
				goto error;
			}
		}
		break;
	case ALEKTO2:
	case BALTOS_DIO_1080:
	case NETCON3:
	case NETIO:
	case NETIO_WLAN:
		if (libsoc_gpio_set_level(onrisc_gpios->gpios[idx].pin, val) ==
		    EXIT_FAILURE) {
			goto error;
		}
		break;
	}

	rc = EXIT_SUCCESS;

 error:
	return rc;
}

int onrisc_gpio_get_value_sysfs(int idx)
{
	onrisc_gpios = onrisc_capabilities.gpios;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
		break;
	case BALTOS_IR_3220:
	case BALTOS_IR_5221:
		if (onrisc_gpios->gpios[idx].flags && GPIO_IS_VIRTUAL) {
			return invert_level(libsoc_gpio_get_level(onrisc_gpios->gpios[idx - 8].pin));
		} else {
			return libsoc_gpio_get_level(onrisc_gpios->gpios[idx].pin);
		}
		break;
	case ALEKTO2:
	case BALTOS_DIO_1080:
	case NETCON3:
	case NETIO:
	case NETIO_WLAN:
		return libsoc_gpio_get_level(onrisc_gpios->gpios[idx].pin);
		break;
	}

	return LEVEL_ERROR;
}

int onrisc_gpio_set_direction_alekto2(onrisc_gpio_t * gpio, int idx)
{
	int rc = EXIT_FAILURE;
	int ctrl_in_pin, ctrl_out_pin;

	onrisc_gpios = onrisc_capabilities.gpios;

	switch(idx) {
		case 0:
		case 1:
		case 2:
		case 3:
			ctrl_in_pin = 0;
			ctrl_out_pin = 1;
			break;
		case 4:
		case 5:
			ctrl_in_pin = 2;
			ctrl_out_pin = 3;
			break;
		case 6:
		case 7:
			ctrl_in_pin = 4;
			ctrl_out_pin = 5;
			break;
	}

	/* disable both drivers */
	if (libsoc_gpio_set_level
	    (onrisc_gpios->gpios_ctrl[ctrl_in_pin].pin, HIGH) == EXIT_FAILURE) {
		goto error;
	}
	if (libsoc_gpio_set_level
	    (onrisc_gpios->gpios_ctrl[ctrl_out_pin].pin, HIGH) == EXIT_FAILURE) {
		goto error;
	}

	/* enable required driver */
	if (gpio->direction == INPUT) {
		if (libsoc_gpio_set_level
		    (onrisc_gpios->gpios_ctrl[ctrl_in_pin].pin, LOW) == EXIT_FAILURE) {
			goto error;
		}
	} else {
		if (libsoc_gpio_set_level
		    (onrisc_gpios->gpios_ctrl[ctrl_out_pin].pin, LOW) == EXIT_FAILURE) {
			goto error;
		}
	}

	rc = EXIT_SUCCESS;

error:
	return rc;

}

int onrisc_gpio_get_direction(onrisc_gpios_t * gpio_dir)
{
	int i, rc = EXIT_FAILURE;

	if (!onrisc_capabilities.gpios){
		goto error;
	}

	onrisc_gpios = onrisc_capabilities.gpios;

	if (!gpio_init_flag) {
		if (onrisc_gpio_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	gpio_dir->mask = 0;
	gpio_dir->value = 0;

	for (i = 0; i < (onrisc_gpios->ngpio + onrisc_gpios->nvgpio); i++) {
		if (onrisc_gpios->gpios[i].dir_fixed) {
			gpio_dir->mask |= (1 << i);
		}
		gpio_dir->value |= (onrisc_gpios->gpios[i].direction) ? (1 << i) : 0;
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_set_direction(onrisc_gpios_t * gpio_dir)
{
	int i, rc = EXIT_FAILURE;

	if (!onrisc_capabilities.gpios){
		goto error;
	}

	onrisc_gpios = onrisc_capabilities.gpios;

	if (!gpio_init_flag) {
		if (onrisc_gpio_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	for (i = 0; i < (onrisc_gpios->ngpio + onrisc_gpios->nvgpio); i++) {
		if (gpio_dir->mask & (1 << i)) {
			/* check, if it is a fixed GPIO */
			if (onrisc_gpios->gpios[i].dir_fixed) {
				fprintf
				    (stderr, "gpio %d is fixed and cannot change it's direction",
				     i);
				goto error;
			}


			/* get direction */
			onrisc_gpios->gpios[i].direction =
			    !!(gpio_dir->value & (1 << i));

			switch (onrisc_system.model) {
			case ALEKTO:
			case ALEKTO_LAN:
			case ALENA:
				break;
			case ALEKTO2:
				if (onrisc_gpio_set_direction_alekto2(&onrisc_gpios->gpios[i], i) == EXIT_FAILURE) {
					goto error;
				}
				break;
			case BALTOS_IR_3220:
			case BALTOS_IR_5221:
				if (onrisc_gpios->gpios[i].flags & GPIO_IS_VIRTUAL) {
					if (onrisc_gpios->gpios[i].direction == INPUT) {
						if (libsoc_gpio_set_level(onrisc_gpios->gpios[i - 4].pin, LOW) ==
						    EXIT_FAILURE) {
							goto error;
						}
					}
				}
				break;
			case NETIO:
			case NETIO_WLAN:
				if (libsoc_gpio_set_direction(
				     onrisc_gpios->gpios_ctrl[i].pin,
					onrisc_gpios->gpios[i].direction) == EXIT_FAILURE) {
					goto error;
				}
				break;
			}
		}
	}

	if (0 < onrisc_gpios->nvgpio) {
		if ((vdir = fopen("/tmp/vdir", "w")) == NULL) 
				goto error;

		fprintf(vdir, "%X", gpio_dir->value >> onrisc_gpios->ngpio );

		fclose(vdir);
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_set_value(onrisc_gpios_t * gpio_val)
{
	int i, rc = EXIT_FAILURE;

	if (!onrisc_capabilities.gpios){
		goto error;
	}

	onrisc_gpios = onrisc_capabilities.gpios;

	if (!gpio_init_flag) {
		if (onrisc_gpio_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	for (i = 0; i < (onrisc_gpios->ngpio + onrisc_gpios->nvgpio); i++) {
		if (gpio_val->mask & (1 << i)) {
			/* check, if it is OUTPUT */
			if (onrisc_gpios->gpios[i].direction == INPUT) {
				continue;
			}

			/* set value */
			if (onrisc_gpio_set_value_sysfs(i, gpio_val->value & (1 << i) ? HIGH : LOW) == EXIT_FAILURE) {
				goto error;
			}
		}
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_get_value_netio(uint32_t * value)
{
	int i, rc = EXIT_FAILURE;
	gpio_level level;
	onrisc_gpios = onrisc_capabilities.gpios;

	for (i = 0; i < onrisc_gpios->ngpio; i++) {
		if (onrisc_gpios->gpios[i].direction == INPUT) {
			if ((level =
			     libsoc_gpio_get_level(onrisc_gpios->gpios[i].
						   pin)) == LEVEL_ERROR) {
				fprintf(stderr, "failed to get GPIO %d\n", i);
				goto error;
			}
			if (level == LOW) {
				*value &= ~(1 << i);
			} else {
				*value |= 1 << i;
			}
		} else {
			if ((level =
			     libsoc_gpio_get_level(onrisc_gpios->gpios_ctrl[i].
						   pin)) == LEVEL_ERROR) {
				fprintf(stderr, "failed to get GPIO %d\n", i);
				goto error;
			}
			if (level == LOW) {
				*value &= ~(1 << i);
			} else {
				*value |= 1 << i;
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

	if (!onrisc_capabilities.gpios){
		goto error;
	}

	onrisc_gpios = onrisc_capabilities.gpios;

	if (!gpio_init_flag) {
		if (onrisc_gpio_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	gpio_val->value = 0;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
		break;
	case ALEKTO2:
	case BALTOS_IR_3220:
	case BALTOS_IR_5221:
	case BALTOS_DIO_1080:
	case NETCON3:
		for (i = 0; i < (onrisc_gpios->ngpio + onrisc_gpios->nvgpio); i++) {
			if ((level =
			     onrisc_gpio_get_value_sysfs(i)) == LEVEL_ERROR) {
				fprintf(stderr, "failed to get GPIO %d\n", i);
				goto error;
			}

			if (level == LOW) {
				gpio_val->value &= ~(1 << i);
			} else {
				gpio_val->value |= 1 << i;
			}
		}
		break;
	case NETIO:
	case NETIO_WLAN:
		if (onrisc_gpio_get_value_netio(&gpio_val->value) == EXIT_FAILURE) {
			goto error;
		}
		break;
	}
	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int generic_gpio_callback(void * arg)
{
	callback_int_arg_t * params = (callback_int_arg_t *) arg;
	onrisc_gpios_t val;
	val.mask = 1 << (params->index);
	gpio_level level;
	int i;

	val.value = 0;

	for (i = 0; i < onrisc_gpios->ngpio; i++) {
		level = libsoc_gpio_get_level(onrisc_gpios->gpios[i].
					   pin);

		if (level == LOW) {
			val.value &= ~(1 << i);
		} else {
			val.value |= 1 << i;
		}
	}

	return params->callback_fn(val, params->args);
}

int onrisc_gpio_register_callback(onrisc_gpios_t mask, int (*callback_fn) (onrisc_gpios_t, void *), void *arg, gpio_edge edge)
{
	int i, rc = EXIT_FAILURE;
	callback_int_arg_t * params;

	if (!onrisc_capabilities.gpios){
		goto error;
	}

	onrisc_gpios = onrisc_capabilities.gpios;

	if (!gpio_init_flag) {
		if (onrisc_gpio_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	mask.value = 0;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
		break;
	case ALEKTO2:
	case BALTOS_IR_3220:
	case BALTOS_IR_5221:
	case BALTOS_DIO_1080:
	case NETCON3:
		for (i = 0; i < onrisc_gpios->ngpio; i++) {
			if (mask.mask & (1 << i)) {
				/* check, if it is INPUT */
				if (onrisc_gpios->gpios[i].direction == OUTPUT) {
					continue;
				}

				/* set trigger edge */
				libsoc_gpio_set_edge(onrisc_gpios->gpios[i].pin, edge);	

				params = malloc(sizeof(callback_int_arg_t));
				if (NULL == params) {
					goto error;
				}
				memset(params, 0, sizeof(callback_int_arg_t));
 				
				params->callback_fn = callback_fn;				
				params->index = i;				
				params->args = arg;				
	
				/* register ISR */
				libsoc_gpio_callback_interrupt(onrisc_gpios->gpios[i].pin, generic_gpio_callback, (void *) params);
			}
		}
		break;
	}
	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_gpio_cancel_callback(onrisc_gpios_t mask)
{
	int i, rc = EXIT_FAILURE;

	if (!onrisc_capabilities.gpios){
		goto error;
	}

	onrisc_gpios = onrisc_capabilities.gpios;

	if (!gpio_init_flag) {
		if (onrisc_gpio_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	mask.value = 0;

	switch (onrisc_system.model) {
	case ALEKTO:
	case ALEKTO_LAN:
	case ALENA:
		break;
	case ALEKTO2:
	case BALTOS_IR_3220:
	case BALTOS_IR_5221:
	case BALTOS_DIO_1080:
	case NETCON3:
		for (i = 0; i < onrisc_gpios->ngpio; i++) {
			if (mask.mask & (1 << i)) {
				/* check, if it is INPUT */
				if (onrisc_gpios->gpios[i].direction == OUTPUT) {
					continue;
				}

 				/* register ISR */
				libsoc_gpio_callback_interrupt_cancel(onrisc_gpios->gpios[i].pin);
			}
		}
		break;
	}
	rc = EXIT_SUCCESS;
 error:
	return rc;
}


int onrisc_gpio_get_number()
{
	onrisc_gpios = onrisc_capabilities.gpios;

	if (!gpio_init_flag) {
		if (onrisc_gpio_init() == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}

	return onrisc_gpios->ngpio;
}
