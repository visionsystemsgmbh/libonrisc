#include <stdlib.h>
#include <stdio.h>
#include <onrisc.h>

/*
 * This example shows how to control mPCIe slot.
 *
 * Compilation instructions:
 * 
 * gcc -o mpcie mpcie.c -l onrisc
 */
int main(int argc, char **argv)
{
	int i, rc = EXIT_FAILURE;
	gpio_level modem_state;

	/* 
	 * initialize libonrisc
	 * This routine must be invoked before any other libonrisc call
	 */
	if (onrisc_init(NULL) == EXIT_FAILURE) {
		perror("init libonrisc: ");
		goto error;
	}

	/* get current mPCIe slot state */
	if (onrisc_get_mpcie_sw_state(&modem_state)  == EXIT_FAILURE) {
		perror("get modem state: ");
		goto error;
	}
	
	/* if slot is disabled, enable it */
	if (LOW == modem_state) {
		printf("Turn modem on\n");
		if (onrisc_set_mpcie_sw_state(HIGH)  == EXIT_FAILURE) {
			perror("set modem state: ");
			goto error;
		}
	}

	/* perform required actions with GSM modem */
	printf("Modem will be turned off in ...\n");
	for (i = 40; i > 0; i--) {
		printf("%d\n", i);
		sleep(1);
	}

	/* 
	 * disable mPCIe slot
	 * don't turn the slot off before modem is completely
	 * initialized (ca. 40-50 seconds depending on modem type)
	 */
	printf("Turn modem off\n");
	if (onrisc_set_mpcie_sw_state(LOW)  == EXIT_FAILURE) {
		perror("set modem state: ");
		goto error;
	}

	rc = EXIT_SUCCESS;
 error:
	return rc;
}
