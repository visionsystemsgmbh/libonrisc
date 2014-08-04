#include "vssys.h"

gpio* onrisc_gpio_init_sysfs(unsigned int gpio_id)
{
	gpio* test_gpio;
	test_gpio = libsoc_gpio_request(gpio_id, LS_SHARED);
	return test_gpio;
}

int timeval_subtract (result, x, y)
struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000)
	{
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

int onrisc_restore_leds(blink_led_t *blinker)
{
	int rc = EXIT_SUCCESS;

	switch (onrisc_system.model)
	{
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			if (ioctl(blinker->fd, GPIO_CMD_SET_LEDS, &blinker->leds_old) < 0)
			{
				perror("ioctl: GPIO_CMD_SET_LEDS");
				rc = EXIT_FAILURE;
			}

			close(blinker->fd);
			blinker->fd = -1;
			break;
		case  ALEKTO2:
		case  NETCON3:
		case  BALIOS_IR_5221:
			if (libsoc_gpio_set_direction(blinker->led, INPUT) == EXIT_FAILURE)
			{
				rc = EXIT_FAILURE;
			}
			if (libsoc_gpio_free(blinker->led) == EXIT_FAILURE)
			{
				rc = EXIT_FAILURE;
			}

			blinker->led = NULL;
			break;
	}

	return rc;
}

int onrisc_switch_led(blink_led_t *led, uint8_t state)
{
	unsigned long val;

	assert( init_flag == 1);

	if (onrisc_led_init(led) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	switch(onrisc_system.model)
	{
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			if (state)
			{
				/* HIGH phase */
				ioctl(led->fd, GPIO_CMD_GET_LEDS, &val);
				val |= led->led_type;
				ioctl(led->fd, GPIO_CMD_SET_LEDS, &val);
			}
			else
			{
				/* LOW phase */
				ioctl(led->fd, GPIO_CMD_GET_LEDS, &val);
				val &= (~led->led_type);
				ioctl(led->fd, GPIO_CMD_SET_LEDS, &val);
			}
			break;
		case ALEKTO2:
			if (state)
			{
				/* HIGH phase */
				libsoc_gpio_set_direction(led->led, INPUT);
			}
			else
			{
				/* LOW phase */
				libsoc_gpio_set_direction(led->led, OUTPUT);
				libsoc_gpio_set_level(led->led, LOW);
			}
			break;
		case BALIOS_IR_5221:
		case NETCON3:
			if (state)
			{
				/* HIGH phase */
				switch(led->led_type)
				{
					case LED_POWER:
						libsoc_gpio_set_direction(led->led, OUTPUT);
						libsoc_gpio_set_level(led->led, LOW);
						break;
					default:
						libsoc_gpio_set_direction(led->led, OUTPUT);
						libsoc_gpio_set_level(led->led, HIGH);
						break;
				}
			}
			else
			{
				/* LOW phase */
				switch(led->led_type)
				{
					case LED_POWER:
						libsoc_gpio_set_direction(led->led, OUTPUT);
						libsoc_gpio_set_level(led->led, HIGH);
						break;
					default:
						libsoc_gpio_set_direction(led->led, OUTPUT);
						libsoc_gpio_set_level(led->led, LOW);
						break;
				}
			}
			break;
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
	blink_led_t *led = (blink_led_t *)data;
	struct timeval low_phase_fixed, low_phase, high_phase;
	uint8_t run = 1;
	int32_t count = led->count;
	
	/* compute low phase duration */
	high_phase.tv_sec = led->high_phase.tv_sec;
	high_phase.tv_usec = led->high_phase.tv_usec;
	timeval_subtract(&low_phase_fixed, &led->interval, &high_phase);

	while(run)
	{
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

		if (count > 0)
		{
			count--;
		}

		if (count == 0)
		{
			run = 0;
		}
	}

	/* restore LED status and free GPIO,
	 * when counter is expired. If thread will be
	 * canceled, then cancelling task will restore
	 * LEDs */
	onrisc_restore_leds(led);
}

void onrisc_blink_create(blink_led_t *blinker)
{
	blinker->count = -1;
	blinker->thread_id = -1;
	blinker->led_type = LED_POWER;
	blinker->led = NULL;
	blinker->fd = -1;
}

void onrisc_blink_destroy(blink_led_t *blinker)
{
	blinker->count = -1;
	blinker->thread_id = -1;
	blinker->led_type = LED_POWER;
	if (blinker->led != NULL)
	{
		libsoc_gpio_free(blinker->led);
	}
	if (blinker->fd != -1)
	{
		close(blinker->fd);
	}
	blinker->led = NULL;
	blinker->fd = -1;
}

int onrisc_led_init(blink_led_t *blinker)
{
	int led_gpio = 0;
	gpio* pwr_gpio;

	switch(onrisc_system.model)
	{
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			/* check, if LED was already initialized */
			if (blinker->fd == -1)
			{
				break;
			}

			blinker->fd = open("/dev/gpio", O_RDWR);
			if (blinker->fd <= 0)
			{
				return EXIT_FAILURE;
			}
			if (ioctl(blinker->fd, GPIO_CMD_GET_LEDS, &blinker->leds_old) < 0)
			{
				perror("ioctl: GPIO_CMD_GET_LEDS");
			}
			break;
		case ALEKTO2:
			/* check, if LED was already initialized */
			if (blinker->led != NULL)
			{
				break;
			}

			if (blinker->led_type == LED_POWER)
			{
				int base;

				if (onrisc_get_tca6416_base(&base, 0x21) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}

				pwr_gpio = onrisc_gpio_init_sysfs(base + 8 + 1);
				if(pwr_gpio == NULL)
				{
					return EXIT_FAILURE;
				}

				blinker->led = pwr_gpio;
			}
			else
				return EXIT_FAILURE;
			break;
		case BALIOS_IR_5221:
			/* check, if LED was already initialized */
			if (blinker->led != NULL)
			{
				break;
			}

			switch (blinker->led_type)
			{
				case LED_POWER:
					led_gpio = 96;
					break;
				case LED_WLAN:
					led_gpio = 16;
					break;
				case LED_APP:
					led_gpio = 17;
					break;
				default:
					printf("Unknown LED\n");
					return EXIT_FAILURE;

			}
			pwr_gpio = onrisc_gpio_init_sysfs(led_gpio);
			if(pwr_gpio == NULL)
			{
				return EXIT_FAILURE;
			}

			blinker->led = pwr_gpio;
			break;
		case NETCON3:
			/* check, if LED was already initialized */
			if (blinker->led != NULL)
			{
				break;
			}

			switch (blinker->led_type)
			{
				case LED_POWER:
					led_gpio = 96;
					break;
				case LED_APP:
					led_gpio = 17;
					break;
				default:
					printf("Unknown LED\n");
					return EXIT_FAILURE;

			}
			pwr_gpio = onrisc_gpio_init_sysfs(led_gpio);
			if(pwr_gpio == NULL)
			{
				return EXIT_FAILURE;
			}

			blinker->led = pwr_gpio;
			break;
	}
	return EXIT_SUCCESS;
}

int onrisc_blink_led_start(blink_led_t *blinker)
{
	int rc;
	struct timeval tmp, tmp_res;

	assert( init_flag == 1);

	tmp.tv_sec = blinker->high_phase.tv_sec;
	tmp.tv_usec = blinker->high_phase.tv_usec;
	assert(timeval_subtract(&tmp_res, &blinker->interval, &tmp) == 0);

	if (onrisc_led_init(blinker) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	/* create blinking thread */
	rc = pthread_create(&blinker->thread_id,
			NULL,
			blink_thread,
			(void *)blinker);

	return rc?EXIT_FAILURE:EXIT_SUCCESS;
}


int onrisc_blink_led_stop(blink_led_t *blinker)
{
	assert( init_flag == 1);

	if(blinker->thread_id == -1)
	{
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
