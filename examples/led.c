#include <stdlib.h>
#include <stdio.h>
#include <onrisc.h>

/*
 * This example shows how to initialize and use LEDs.
 *
 * Compilation instructions:
 * 
 * gcc -o led led.c -l onrisc
 */
int main(int argc, char **argv)
{
	int rc = EXIT_FAILURE;
	blink_led_t led;

	/* 
	 * initialize libonrisc
	 * This routine must be invoked before any other libonrisc call
	 */
	if (onrisc_init(NULL) == EXIT_FAILURE) {
		perror("init libonrisc: ");
		goto error;
	}

	/* 
	 * init LED
	 * This call must be made before setting other LED settings as
	 * it sets default values for blink_led_t
	 */
	onrisc_blink_create(&led);

	/* set LED type to APP LED */
	led.led_type = LED_APP;

	/* turn LED on */
	if (onrisc_switch_led(&led, 1) == EXIT_FAILURE) {
		fprintf(stderr, "failed to switch LED\n");
	}

	sleep(2);

	/* turn LED off */
	if (onrisc_switch_led(&led, 0) == EXIT_FAILURE) {
		fprintf(stderr, "failed to switch LED\n");
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}
