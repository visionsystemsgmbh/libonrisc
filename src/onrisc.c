#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "onrisc.h"

int init_flag = 0;
onrisc_system_t onrisc_system;

/**
 * @brief read hardware parameter from EEPROM
 * @param hw_params structure to put hardware parameters to
 * @param model OnRISC model
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_get_hw_params_eeprom(BSP_VS_HWPARAM *hw_params, int model)
{
	int fd, rv, rc = EXIT_SUCCESS;
	char *eeprom_file = NULL;

	if (model == VS860) {
		eeprom_file = VS860_EEPROM;
	}
	else  if (model == ALEKTO2) {
		eeprom_file = ALEKTO2_EEPROM;
	}

	fd = open(eeprom_file, O_RDONLY);
	if (fd <= 0)
	{
		rc = EXIT_FAILURE;
		goto error;
	}
	rv = read(fd, hw_params, sizeof(struct _BSP_VS_HWPARAM));
	if (rv != sizeof(struct _BSP_VS_HWPARAM))
	{
		rc = EXIT_FAILURE;
		goto error;
	}

error:
	if (fd > 0)
		close(fd);
	return rc;
}

/**
 * @brief get MTD partition size from /proc/mtd 
 * @param name the device name of the partition like /dev/mtdblockX
 * @param size partition size to return
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int get_partition_size(char *name, ulong *size)
{
	FILE *fp = NULL;
	char buf[64], final_buf[10], *tmp_ptr;
	int rc = EXIT_SUCCESS;

	/* derive MTD partition name from device name */
	tmp_ptr = strrchr(name, 'k');
	tmp_ptr++;
	strncpy(buf, tmp_ptr, name + strlen(name) - tmp_ptr);
	buf[name + strlen(name) - tmp_ptr] = '\0';
	sprintf(final_buf, "mtd%s", buf);

	/* open /proc/mtd to get the list of available partitions */
	fp = fopen("/proc/mtd", "r");
	if (!fp)
	{
		rc = EXIT_FAILURE;
		perror("fopen");
		goto error;
	}

	while (fgets(buf, sizeof buf, fp))
	{
		if((tmp_ptr = strstr(buf, final_buf)))
			break;
	}
	
	if(!tmp_ptr)
	{
		rc = EXIT_FAILURE;
		goto error;
	}

	/* get partition size */
	tmp_ptr = strchr(buf, ' ');
	tmp_ptr++;

	strncpy(final_buf, tmp_ptr, 8);
	final_buf[8] = '\0';

	*size = strtol(final_buf, NULL, 16);

 error:
	if (fp != NULL)
		fclose(fp);

	return rc;
}

/**
* @brief read hardware parameter from flash
* @param hw_params hardware paramter structure
* @return EXIT_SUCCES or EXIT_FAILURE
*/
int onrisc_get_hw_params_nor(struct _param_hw *hw_params)
{
	ulong offset, size;
	int rv, ret = EXIT_SUCCESS, fd = 0;

	if(get_partition_size(PARTITION_REDBOOT, &size))
	{
		ret = EXIT_FAILURE;
		goto error;
	}

	fd = open(PARTITION_REDBOOT, O_RDONLY);
	if (fd == -1)
	{
		ret = EXIT_FAILURE;
		goto error;
	}

	offset = size - sizeof(struct _param_hw);
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		ret = EXIT_FAILURE;
		goto error;
	}

	rv = read(fd, hw_params, sizeof(struct _param_hw));
	if (rv != sizeof(struct _param_hw))
	{
		ret = EXIT_FAILURE;
		goto error;
	}

	if (hw_params->magic != GLOBAL_MAGIC)
	{
		ret = EXIT_FAILURE;
		goto error;
	}

	close(fd);

	return ret;
 error:
	perror("param_read_hw_params");
	if (fd > 0)
		close(fd);
	return ret;
}

int onrisc_get_model(int *model)
{
	char buf[1024];

	*model = 0;

	FILE *fp = fopen("/proc/cpuinfo", "r");
	if (fp == NULL)
		return EXIT_FAILURE;

	while (fgets(buf, sizeof(buf), fp)) {
		if (strstr(buf, "Alekto2") || strstr(buf, "am335xevm")) {
			/* Alekto2 */
			*model = ALEKTO2;
			break;
		} else if (strstr(buf, "VS-860")) {
			/* VS-860 */
			*model = VS860;
			break;
		} else if (strstr(buf, "VScom OnRISC")) {
			/* Alekto */
			*model = ALEKTO;
			break;
		} else {
			/* TODO: default device */
		}
	}

	fclose(fp);

	return *model?EXIT_SUCCESS:EXIT_FAILURE;
}

gpio* onrisc_gpio_init_sysfs(unsigned int gpio_id)
{
	gpio* test_gpio;
	test_gpio = libsoc_gpio_request(gpio_id);	
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
	switch (onrisc_system.model)
	{
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			if (ioctl(blinker->fd, GPIO_CMD_SET_LEDS, &blinker->leds_old) < 0)
			{
				perror("ioctl: GPIO_CMD_SET_LEDS");
			}

			close(blinker->fd);
			break;
		case  ALEKTO2:
			libsoc_gpio_set_direction(blinker->led, INPUT);
			if (libsoc_gpio_free(blinker->led) == EXIT_FAILURE)
			{
				printf("Failed to free GPIO\n");
			}
			break;
	}
}

void onrisc_switch_led(blink_led_t *led, uint8_t state, unsigned long leds_old)
{
	unsigned long val;

	switch(onrisc_system.model)
	{
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			if (state)
			{
				/* HIGH phase */
				val = leds_old | led->led_type;
				ioctl(led->fd, GPIO_CMD_SET_LEDS, &val);
			}
			else
			{
				/* LOW phase */
				val = leds_old & (~led->led_type);
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
	}
}

/**
 * @brief blinking thread
 * @param data blink_led structure
 * @todo use switch/case structure for system identification
 */
void *blink_thread(void *data)
{
	unsigned long leds_old = 0;
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
		onrisc_switch_led(led, 1, leds_old);
		high_phase.tv_sec = led->high_phase.tv_sec;
		high_phase.tv_usec = led->high_phase.tv_usec;
		select(1, NULL, NULL, NULL, &high_phase);

		/* LOW phase */
		onrisc_switch_led(led, 0, leds_old);
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

	onrisc_restore_leds(led);
}

void onrisc_blink_create(blink_led_t *blinker)
{
	blinker->count = -1;
	blinker->thread_id = -1;
	blinker->led_type = LED_POWER;
}

int onrisc_blink_led_start(blink_led_t *blinker)
{
	gpio* pwr_gpio;
	int rc;
	struct timeval tmp, tmp_res;

	assert( init_flag == 1);

	tmp.tv_sec = blinker->high_phase.tv_sec;
	tmp.tv_usec = blinker->high_phase.tv_usec;
	assert(timeval_subtract(&tmp_res, &blinker->interval, &tmp) == 0);

	switch(onrisc_system.model)
	{
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
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
			pwr_gpio = onrisc_gpio_init_sysfs(217);
			if(pwr_gpio == NULL)
			{
				return EXIT_FAILURE;
			}

			blinker->led = pwr_gpio;
			break;
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

void onrisc_print_hw_params()
{
	int i;

	assert( init_flag == 1);

	printf("Hardware Parameters\n");
	printf("===================\n");
	printf("Model: %d\n", onrisc_system.model);
	printf("HW Revision: %d.%d\n", onrisc_system.hw_rev >> 16, onrisc_system.hw_rev & 0xff);
	printf("Serial Number: %d\n", onrisc_system.ser_nr);
	printf("Production date: %s\n", onrisc_system.prd_date);
	printf("MAC1: ");
	for(i = 0; i < 6; i++)
	{
		printf("%02x", onrisc_system.mac1[i]);
	}
	printf("\n");
	printf("MAC2: ");
	for(i = 0; i < 6; i++)
	{
		printf("%02x", onrisc_system.mac2[i]);
	}
	printf("\n");
	printf("MAC3: ");
	for(i = 0; i < 6; i++)
	{
		printf("%02x", onrisc_system.mac3[i]);
	}
	printf("\n");
}

int onrisc_init(onrisc_system_t *data)
{
	int model, i;
	struct _param_hw hw_nor;
	BSP_VS_HWPARAM hw_eeprom;

	if (onrisc_get_model(&model) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	switch (model)
	{
		case ALEKTO:
			if (onrisc_get_hw_params_nor(&hw_nor) == EXIT_FAILURE)
			{
				return EXIT_FAILURE;
			}
			onrisc_system.model = hw_nor.biosid;
			onrisc_system.hw_rev = hw_nor.hwrev;
			onrisc_system.ser_nr = hw_nor.serialnr;
			strncpy(onrisc_system.prd_date, hw_nor.prddate, 11);
			for (i = 0; i < 6; i++)
			{
				onrisc_system.mac1[i] = hw_nor.mac1[i];
				onrisc_system.mac2[i] = hw_nor.mac2[i];
				onrisc_system.mac3[i] = 0xff;
			}
			break;
		case ALEKTO2:
		case VS860:
			if (onrisc_get_hw_params_eeprom(&hw_eeprom, model) == EXIT_FAILURE)
			{
				return EXIT_FAILURE;
			}
			onrisc_system.model = hw_eeprom.SystemId;
			onrisc_system.hw_rev = hw_eeprom.HwRev;
			onrisc_system.ser_nr = hw_eeprom.SerialNumber;
			strncpy(onrisc_system.prd_date, hw_eeprom.PrdDate, 11);
			for (i = 0; i < 6; i++)
			{
				onrisc_system.mac1[i] = hw_eeprom.MAC1[i];
				onrisc_system.mac2[i] = hw_eeprom.MAC2[i];
				onrisc_system.mac3[i] = hw_eeprom.MAC3[i];
			}
			break;
	}

	if (data != NULL)
	{
		memcpy(data, &onrisc_system, sizeof(onrisc_system_t));
	}

	init_flag = 1;

	return EXIT_SUCCESS;
}
