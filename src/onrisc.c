#include "vssys.h"

int init_flag = 0;
onrisc_system_t onrisc_system;

/* UART mode variables */
int init_uart_modes_flag = 0;
int serial_mode_first_pin = 200;
gpio *mode_gpios[8];

int onrisc_get_dips(uint32_t *dips) {
	int rc = EXIT_SUCCESS;
	gpio *dip_gpios[4];
	gpio_level level;
	int i;

	assert( init_flag == 1);

	*dips = 0;

	if (onrisc_system.model != NETCON3) {
		rc = EXIT_FAILURE;
		goto error;
	}

	for (i = 0; i < 4; i++) {

		/* export GPIO */
		dip_gpios[i] = libsoc_gpio_request(44 + i, LS_SHARED);
		if (dip_gpios[i] == NULL) {
			rc = EXIT_FAILURE;
			goto error;
		}

		/* set direction to input */
		if (libsoc_gpio_set_direction(dip_gpios[i], INPUT) == EXIT_FAILURE) {
			rc = EXIT_FAILURE;
			goto error;
		}

		/* get level */
		level = libsoc_gpio_get_level(dip_gpios[i]);
		if (level == LEVEL_ERROR) {
			rc = EXIT_FAILURE;
			goto error;
		}

		/* set DIP status variable. DIPs are low active */
		if (level == LOW) {
			*dips |= DIP_S1 << i;
		}
	}

error:
	return rc;
}

int onrisc_setup_uart_gpios(int dir) {
	int i, rc = EXIT_SUCCESS;

	if (init_uart_modes_flag) {
		return rc;
	}

	for (i = 0; i < 8; i++) {
		mode_gpios[i] = libsoc_gpio_request(serial_mode_first_pin + i, LS_SHARED);
		if (mode_gpios[i] == NULL) {
			rc = EXIT_FAILURE;
			goto error;
		}

		if (libsoc_gpio_set_direction(mode_gpios[i], dir) == EXIT_FAILURE) {
			rc = EXIT_FAILURE;
			goto error;
		}
	}

	init_uart_modes_flag = 1;

error:
	return rc;
}

int onrisc_proc_write(char *file, char *str)
{
	int rc = EXIT_SUCCESS;

	FILE *fp = fopen(file, "w");
	if (fp == NULL) {
		rc = EXIT_FAILURE;
		goto error;
	}

	fwrite(str, 1, strlen(str), fp);

	fclose(fp);
error:
	return rc;
}

int onrisc_set_uart_mode_ks8695(int port_nr, onrisc_uart_mode_t *mode)
{
	char file[64];

	sprintf(file, "/proc/vscom/epld_ttyS%d", port_nr);

	switch (mode->rs_mode) {
		case TYPE_LOOPBACK:
			onrisc_proc_write(file, "inactive");
			break;
		case TYPE_RS232:
			onrisc_proc_write(file, "rs232");
			break;
		case TYPE_RS422:
			onrisc_proc_write(file, "rs422");
			break;
		case TYPE_RS485_FD:
			if (mode->dir_ctrl == DIR_ART) {
				onrisc_proc_write(file, "rs485byart-4-wire");
			} else {
				onrisc_proc_write(file, "rs485byrts-4-wire");
			}
			break;
		case TYPE_RS485_HD:
			if (mode->dir_ctrl == DIR_ART) {
				if (mode->echo) {
					onrisc_proc_write(file, "rs485byart-2-wire-echo");
				} else {
					onrisc_proc_write(file, "rs485byart-2-wire-noecho");
				}
			} else {
				if (mode->echo) {
					onrisc_proc_write(file, "rs485byrts-2-wire-echo");
				} else {
					onrisc_proc_write(file, "rs485byrts-2-wire-noecho");
				}
			}
			break;
	}
}

int onrisc_set_uart_mode_omap3(int port_nr, onrisc_uart_mode_t * mode)
{
	/* special handling for TYPE_DIP */
	if (mode->rs_mode == TYPE_DIP) {
		return onrisc_setup_uart_gpios(INPUT);
	}

	/* handle RS-modes */
	if (onrisc_setup_uart_gpios(OUTPUT) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	switch (mode->rs_mode) {
		case TYPE_RS232:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			break;
		case TYPE_RS422:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], HIGH);
			break;
		case TYPE_RS485_FD:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			break;
		case TYPE_RS485_HD:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], HIGH);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			break;
		case TYPE_LOOPBACK:
			libsoc_gpio_set_level(mode_gpios[0 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[1 + 4 * (port_nr - 1)], LOW);
			libsoc_gpio_set_level(mode_gpios[2 + 4 * (port_nr - 1)], LOW);
			break;
		default:
			return EXIT_FAILURE;
	}

	/* handle termination */
	if (mode->termination) {
		libsoc_gpio_set_level(mode_gpios[3 + 4 * (port_nr - 1)], HIGH);
	} else {
		libsoc_gpio_set_level(mode_gpios[3 + 4 * (port_nr - 1)], LOW);
	}

	return EXIT_SUCCESS;
}

int onrisc_set_uart_mode(int port_nr, onrisc_uart_mode_t * mode)
{
	int rc = EXIT_SUCCESS;

	switch (onrisc_system.model) {
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			rc = onrisc_set_uart_mode_ks8695(port_nr, mode);
			break;
		case VS860:
		case ALEKTO2:
		case BALIOS_IR_5221:
			rc = onrisc_set_uart_mode_omap3(port_nr, mode);
			break;
	}

	return rc;
}

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
	else  if (model == ALEKTO2 || model == BALIOS_IR_5221 || model == NETCON3) {
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
		if (strstr(buf, "Alekto2") || strstr(buf, "am335xevm") || strstr(buf, "AM33X")) {
			/* Alekto2 */
			*model = ALEKTO2;
			serial_mode_first_pin = 208;
			break;
		} else if (strstr(buf, "VS-860")) {
			/* VS-860 */
			*model = VS860;
			serial_mode_first_pin = 200;
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

	/* get model from device tree */
	if (*model == ALEKTO2)
	{
		FILE *fp = fopen("/proc/device-tree/model", "r");
		/* Alekto2 doesn't have this entry, so leave type as is */
		if (fp == NULL)
			goto error;

		if (!fgets(buf, sizeof(buf), fp)) {
			perror("fgets");
			fclose(fp);
			*model = 0;
			goto error;
		}

		if (strstr(buf, "Balios iR 5221")) {
			*model = BALIOS_IR_5221;
			serial_mode_first_pin = 504;
		}

		if (strstr(buf, "NetCON 3")) {
			*model = NETCON3;
		}

		fclose(fp);
	}

error:
	return *model?EXIT_SUCCESS:EXIT_FAILURE;
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
		case BALIOS_IR_5221:
		case NETCON3:
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
