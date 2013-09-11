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
	rv = read(fd, &hw_params, sizeof(struct _BSP_VS_HWPARAM));
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

/**
 * @brief blinking thread
 * @param data blink_led structure
 * @todo use switch/case structure for system identification
 */
void *blink_thread(void *data)
{
	unsigned long leds_old = 0, val;
	blink_led *led = (blink_led *)data;
	uint32_t high_phase, low_phase;
	
	high_phase = (led->interval * 1000) * led->high_phase / 100;
	low_phase = (led->interval * 1000) - high_phase;

	if ((onrisc_system.model == ALEKTO)
		|| (onrisc_system.model == ALENA)
		|| (onrisc_system.model == ALEKTO_LAN))
	{
		if (ioctl(led->fd, GPIO_CMD_GET_LEDS, &leds_old) < 0)
		{
			perror("ioctl: GPIO_CMD_GET_LEDS");
		}
		while(1)
		{
			/* HIGH phase */
			val = leds_old | LED_POWER;
			ioctl(led->fd, GPIO_CMD_SET_LEDS, &val);
			usleep(high_phase);

			/* LOW phase */
			val = leds_old & (~LED_POWER);
			ioctl(led->fd, GPIO_CMD_SET_LEDS, &val);
			usleep(low_phase);
		}

	}
	/* Alekto 2 */
	else
	{
		while(1)
		{
			/* HIGH phase */
			libsoc_gpio_set_direction(led->led, INPUT);
			usleep(high_phase);

			/* LOW phase */
			libsoc_gpio_set_direction(led->led, OUTPUT);
			libsoc_gpio_set_level(led->led, LOW);
			usleep(low_phase);
		}
	}
}

/**
 * @brief start blinking thread
 * @param blinker blink_led structure
 * @todo use switch/case structure for system identification
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_blink_pwr_led_start(blink_led *blinker)
{
	gpio* pwr_gpio;
	int rc;

	assert( init_flag == 1);

	if ((blinker->high_phase < 10) && (blinker->high_phase > 100))
	{
		printf("high_phase out of range\n");
	}

	if(onrisc_system.model == ALEKTO2)
	{
		pwr_gpio = onrisc_gpio_init_sysfs(217);
		if(pwr_gpio == NULL)
		{
			return EXIT_FAILURE;
		}

		blinker->led = pwr_gpio;
	}
	else
	{
		blinker->fd = open("/dev/gpio", O_RDWR);
		if (blinker->fd <= 0)
		{
			return EXIT_FAILURE;
		}
	}

	/* create blinking thread */
	rc = pthread_create(&blinker->thread_id,
			NULL,
			blink_thread,
			(void *)blinker);

	return EXIT_SUCCESS;
}

/**
 * @brief cancel blinking thread and free GPIO
 * @param blinker blink_led structure
 * @todo use switch/case structure for system identification
 */
int onrisc_blink_pwr_led_stop(blink_led *blinker)
{
	assert( init_flag == 1);

	/* cancel thread */
	pthread_cancel(blinker->thread_id);
	pthread_join(blinker->thread_id, NULL);

	/* restore LED status and free GPIO for Alekto 2 */
	if(onrisc_system.model == ALEKTO2)
	{
		libsoc_gpio_set_direction(blinker->led, INPUT);
		if (libsoc_gpio_free(blinker->led) == EXIT_FAILURE)
		{
			printf("Failed to free GPIO\n");
		}
	}
	else
	{
		unsigned long leds_old = 0;

		if (ioctl(blinker->fd, GPIO_CMD_GET_LEDS, &leds_old) < 0)
		{
			perror("ioctl: GPIO_CMD_GET_LEDS");
		}

		leds_old |= LED_POWER;
		if (ioctl(blinker->fd, GPIO_CMD_SET_LEDS, &leds_old) < 0)
		{
			perror("ioctl: GPIO_CMD_SET_LEDS");
		}

		close(blinker->fd);
	}

	return EXIT_SUCCESS;
}

/**
 * @brief get system, hardware parameters etc.
 * @param data pointer to the structure, where system data will be stored
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
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
			strncpy(onrisc_system.mac1, hw_nor.mac1, 6);
			strncpy(onrisc_system.mac2, hw_nor.mac2, 6);
			for (i = 0; i < 6; i++)
			{
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
			strncpy(onrisc_system.mac1, hw_eeprom.MAC1, 6);
			strncpy(onrisc_system.mac2, hw_eeprom.MAC2, 6);
			strncpy(onrisc_system.mac3, hw_eeprom.MAC3, 6);
			break;
	}

	if (data != NULL)
	{
		memcpy(data, &onrisc_system, sizeof(onrisc_system_t));
	}

	init_flag = 1;

	return EXIT_SUCCESS;
}
