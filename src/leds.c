#include "vssys.h"

int onrisc_led_init(blink_led_t * blinker);

gpio *onrisc_gpio_init_sysfs(unsigned int gpio_id)
{
	gpio *test_gpio;
	test_gpio = libsoc_gpio_request(gpio_id, LS_GPIO_SHARED);
	return test_gpio;
}

int timeval_subtract(result, x, y)
struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 *           tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

int timeval_multiply(result, x, y)
struct timeval *result, *x;
int32_t y;
{
	result->tv_usec = y * x->tv_usec;
	result->tv_sec = y * x->tv_sec;

	if (x->tv_usec > 1000000) {
		result->tv_sec += result->tv_usec / 1000000;
		result->tv_usec %= 1000000;
	}
	/* Return 1 if result is negative. */
	return result->tv_sec < 0;
}

int onrisc_restore_leds(blink_led_t * blinker)
{
	int rc = EXIT_SUCCESS;
	uint8_t led_flags =
	    onrisc_capabilities.leds->led[blinker->led_type].flags;

	if (led_flags & LED_IS_GPIO_BASED) {
		if (libsoc_gpio_set_direction(blinker->led, INPUT) ==
		    EXIT_FAILURE) {
			rc = EXIT_FAILURE;
		}
		if (libsoc_gpio_free(blinker->led) == EXIT_FAILURE) {
			rc = EXIT_FAILURE;
		}

		blinker->led = NULL;
	} else {
		if (ioctl(blinker->fd, GPIO_CMD_SET_LEDS, &blinker->leds_old) <
		    0) {
			perror("ioctl: GPIO_CMD_SET_LEDS");
			rc = EXIT_FAILURE;
		}

		close(blinker->fd);
		blinker->fd = -1;
	}

	return rc;
}

int onrisc_get_led_state(blink_led_t * led, uint8_t * state)
{
	int rc = EXIT_FAILURE;
	unsigned long val;
	uint8_t led_flags = onrisc_capabilities.leds->led[led->led_type].flags;
	onrisc_led_t *led_cap = &onrisc_capabilities.leds->led[led->led_type];

	assert(init_flag == 1);

	if (onrisc_led_init(led) == EXIT_FAILURE) {
		goto error;
	}

	if (led_cap->flags & LED_IS_LED_CLASS_BASED) {
		char path[256];
		sprintf(path,"/sys/class/leds/%s/brightness",led_cap->name);
		FILE *fp = fopen(path, "r");
		if (fp == NULL)
			return EXIT_FAILURE;
		fscanf(fp,"%d", state);
		fclose(fp);

		return EXIT_SUCCESS;
	}

	if (led_flags & LED_IS_GPIO_BASED) {
		val = libsoc_gpio_get_level(led->led);
		switch (val) {
			case HIGH:
				*state = led_flags & LED_IS_HIGH_ACTIVE ? HIGH : LOW;
				break;
			case LOW:
				*state = led_flags & LED_IS_HIGH_ACTIVE ? LOW : HIGH;
				break;
			case LEVEL_ERROR:
				*state = val;
				fprintf(stderr, "Failed to get LED state\n");
				goto error;
		}
	}

	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_switch_led(blink_led_t * led, uint8_t state)
{
	unsigned long val;
	uint8_t led_flags = onrisc_capabilities.leds->led[led->led_type].flags;
	onrisc_led_t *led_cap = &onrisc_capabilities.leds->led[led->led_type];

	assert(init_flag == 1);

	if (onrisc_led_init(led) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (led_cap->flags & LED_IS_LED_CLASS_BASED) {
		char path[256];
		sprintf(path,"/sys/class/leds/%s/trigger",led_cap->name);
		FILE *fp = fopen(path, "w");
		if (fp == NULL)
			return EXIT_FAILURE;
		fprintf(fp,"none");
		fclose(fp);

		sprintf(path,"/sys/class/leds/%s/brightness",led_cap->name);
		fp = fopen(path, "w");
		if (fp == NULL)
			return EXIT_FAILURE;
		fprintf(fp,state ? "1" : "0");
		fclose(fp);

		return EXIT_SUCCESS;
	}

	if (led_flags & LED_IS_GPIO_BASED) {
		libsoc_gpio_set_direction(led->led, OUTPUT);
		if (state) {
			/* HIGH phase */
			if (led_flags & LED_IS_HIGH_ACTIVE) {
				if (led_flags & LED_IS_INPUT_ACTIVE) {
					libsoc_gpio_set_direction(led->led,
								  INPUT);
				} else {
					libsoc_gpio_set_level(led->led, HIGH);
				}
			} else {
				libsoc_gpio_set_level(led->led, LOW);
			}
		} else {
			/* LOW phase */
			if (led_flags & LED_IS_HIGH_ACTIVE) {
				libsoc_gpio_set_level(led->led, LOW);
			} else {
				libsoc_gpio_set_level(led->led, HIGH);
			}
		}

	} else {
		if (state) {
			/* HIGH phase */
			ioctl(led->fd, GPIO_CMD_GET_LEDS, &val);
			val |= led->led_type;
			ioctl(led->fd, GPIO_CMD_SET_LEDS, &val);
		} else {
			/* LOW phase */
			ioctl(led->fd, GPIO_CMD_GET_LEDS, &val);
			val &= (~led->led_type);
			ioctl(led->fd, GPIO_CMD_SET_LEDS, &val);
		}
	}

	return EXIT_SUCCESS;
}

/**
 * @brief blinking thread
 * @param data blink_led structure
 * @todo use switch/case structure for system identification
 */
void *blink_thread(void *data)
{
	blink_led_t *led = (blink_led_t *) data;
	struct timeval low_phase_fixed, low_phase, high_phase;
	uint8_t run = 1;
	int32_t count = led->count;

	/* compute low phase duration */
	high_phase.tv_sec = led->high_phase.tv_sec;
	high_phase.tv_usec = led->high_phase.tv_usec;
	timeval_subtract(&low_phase_fixed, &led->interval, &high_phase);

	while (run) {
		/* HIGH phase */
		onrisc_switch_led(led, 1);
		high_phase.tv_sec = led->high_phase.tv_sec;
		high_phase.tv_usec = led->high_phase.tv_usec;
		select(1, NULL, NULL, NULL, &high_phase);

		/* LOW phase */
		onrisc_switch_led(led, 0);
		low_phase.tv_sec = low_phase_fixed.tv_sec;
		low_phase.tv_usec = low_phase_fixed.tv_usec;
		select(1, NULL, NULL, NULL, &low_phase);

		if (count > 0) {
			count--;
		}

		if (count == 0) {
			run = 0;
		}
	}
}

/**
 * @brief blinking thread
 * @param data blink_led structure
 * @todo use switch/case structure for system identification
 */
void *blink_class_thread(void *data)
{
	blink_led_t *led = (blink_led_t *) data;
	struct timeval stop_after;
	uint8_t run = 1;
	int32_t count = led->count;
	onrisc_led_t *led_cap = &onrisc_capabilities.leds->led[led->led_type];

	timeval_multiply(&stop_after, &led->interval, count);

	select(1, NULL, NULL, NULL, &stop_after);

	char path[256];
	sprintf(path,"/sys/class/leds/%s/trigger",led_cap->name);
	FILE *fp = fopen(path, "w");
	if (fp == NULL)
		return NULL;
	fprintf(fp,"none");
	fclose(fp);
}

void onrisc_blink_create(blink_led_t * blinker)
{
	blinker->count = -1;
	blinker->thread_id = -1;
	blinker->led_type = LED_POWER;
	blinker->led = NULL;
	blinker->fd = -1;
}

void onrisc_blink_destroy(blink_led_t * blinker)
{
	blinker->count = -1;
	blinker->thread_id = -1;
	blinker->led_type = LED_POWER;
	if (blinker->led != NULL) {
		libsoc_gpio_free(blinker->led);
	}
	if (blinker->fd != -1) {
		close(blinker->fd);
	}
	blinker->led = NULL;
	blinker->fd = -1;
}

int onrisc_led_init(blink_led_t * blinker)
{
	gpio *libsoc_gpio;
	uint8_t led_flags;
	uint32_t pin;
	char *name; 
	struct stat buf;

	assert(blinker->led_type < ONRISC_MAX_LEDS);

	led_flags = onrisc_capabilities.leds->led[blinker->led_type].flags;
	pin = onrisc_capabilities.leds->led[blinker->led_type].pin;
	name = onrisc_capabilities.leds->led[blinker->led_type].name;

	if (!(led_flags & LED_IS_AVAILABLE)) {
		fprintf(stderr, "LED (%d) is not available on this device\n",
			blinker->led_type);
		return EXIT_FAILURE;
	}

	if (led_flags & LED_IS_LED_CLASS_BASED) {
		/* check, if LED (leds-gpio) was already initialized */
		return EXIT_SUCCESS;
	}

	if(!stat("/sys/class/leds/onrisc:red:power", &buf)) {
		onrisc_capabilities.leds->led[blinker->led_type].flags |= LED_IS_LED_CLASS_BASED;
		led_flags = onrisc_capabilities.leds->led[blinker->led_type].flags;
		return EXIT_SUCCESS;
	}

	if (led_flags & LED_IS_GPIO_BASED) {
		/* check, if LED was already initialized */
		if (blinker->led != NULL) {
			return EXIT_SUCCESS;
		}

		/* handle I2C GPIO expander based LEDs */
		if (led_flags & LED_NEEDS_I2C_ADDR) {
			int base;
			uint8_t i2c_id =
			    onrisc_capabilities.leds->led[blinker->led_type].
			    i2c_id;

			if (onrisc_get_tca6416_base(&base, i2c_id) ==
			    EXIT_FAILURE) {
				return EXIT_FAILURE;
			}

			pin += base;
		}

		/* initialize libsoc gpio structure with required pin */
		libsoc_gpio = onrisc_gpio_init_sysfs(pin);
		if (libsoc_gpio == NULL) {
			return EXIT_FAILURE;
		}

		blinker->led = libsoc_gpio;
	} else {
		/* check, if LED was already initialized */
		if (blinker->fd == -1) {
			return EXIT_SUCCESS;
		}

		blinker->fd = open("/dev/gpio", O_RDWR);
		if (blinker->fd < 0) {
			return EXIT_FAILURE;
		}
		if (ioctl(blinker->fd, GPIO_CMD_GET_LEDS, &blinker->leds_old) <
		    0) {
			perror("ioctl: GPIO_CMD_GET_LEDS");
		}
	}

	return EXIT_SUCCESS;
}

int onrisc_blink_led_start(blink_led_t * blinker)
{
	int rc;
	struct timeval tmp, tmp_res, off;
	onrisc_led_t *led_cap = &onrisc_capabilities.leds->led[blinker->led_type];


	assert(init_flag == 1);

	tmp.tv_sec = blinker->high_phase.tv_sec;
	tmp.tv_usec = blinker->high_phase.tv_usec;
	timeval_subtract(&tmp_res, &blinker->interval, &tmp);

	if (onrisc_led_init(blinker) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (led_cap->flags & LED_IS_LED_CLASS_BASED) {
		char path[256];
		sprintf(path,"/sys/class/leds/%s/trigger",led_cap->name);
		FILE *fp = fopen(path, "w");
		if (fp == NULL)
			return EXIT_FAILURE;
		fprintf(fp,"timer");
		fclose(fp);

		sprintf(path,"/sys/class/leds/%s/delay_on",led_cap->name);
		fp = fopen(path, "w");
		if (fp == NULL)
			return EXIT_FAILURE;
		fprintf(fp,"%d", (tmp.tv_sec * 1000) + (tmp.tv_usec / 1000));
		fclose(fp);

		sprintf(path,"/sys/class/leds/%s/delay_off",led_cap->name);
		fp = fopen(path, "w");
		if (fp == NULL)
			return EXIT_FAILURE;
		fprintf(fp,"%d", (tmp_res.tv_sec * 1000) + (tmp_res.tv_usec / 1000));
		fclose(fp);

		if(blinker->count > 0) {
			/* create blinking thread */
			rc = pthread_create(&blinker->thread_id,
						NULL, blink_class_thread, (void *)blinker);

			return rc ? EXIT_FAILURE : EXIT_SUCCESS;
		}

		return EXIT_SUCCESS;
	}

	/* create blinking thread */
	rc = pthread_create(&blinker->thread_id,
			    NULL, blink_thread, (void *)blinker);

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}

int onrisc_blink_led_stop(blink_led_t * blinker)
{
	assert(init_flag == 1);

	onrisc_led_t *led_cap = &onrisc_capabilities.leds->led[blinker->led_type];

	if (led_cap->flags & LED_IS_LED_CLASS_BASED) {
		char path[256];
		sprintf(path,"/sys/class/leds/%s/trigger",led_cap->name);
		FILE *fp = fopen(path, "w");
		if (fp == NULL)
			return EXIT_FAILURE;
		fprintf(fp,"none");
		fclose(fp);

		if(blinker->count > 0) {
			/* cancel thread */
			pthread_cancel(blinker->thread_id);
			pthread_join(blinker->thread_id, NULL);
		}

		return EXIT_SUCCESS;
	}

	if (blinker->thread_id == -1) {
		return EXIT_SUCCESS;
	}

	/* cancel thread */
	pthread_cancel(blinker->thread_id);
	pthread_join(blinker->thread_id, NULL);

	/* restore LED status and free GPIO for Alekto 2 */
	onrisc_restore_leds(blinker);

	blinker->thread_id = -1;

	return EXIT_SUCCESS;
}
