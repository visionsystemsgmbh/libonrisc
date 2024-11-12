#include "vssys.h"

int init_flag = 0;
onrisc_system_t onrisc_system;
onrisc_capabilities_t onrisc_capabilities;
onrisc_sw_caps_t * onrisc_wlsw;
onrisc_eeprom_t eeprom;

/* UART mode variables */
int serial_mode_first_pin = 200;

int onrisc_get_gpiochip_and_offset(const char *name, int *chip, int *offset, int *base)
{
	DIR *dp;
	struct dirent *item;
	int rc = EXIT_FAILURE;

	*chip = -1;
	*offset = -1;

	dp = opendir("/dev");
	if (dp == NULL) {
		fprintf(stderr, "Failed to open /dev\n");
		goto error;
	}

	while((item = readdir(dp))) {
		if (strstr(item->d_name, "gpiochip")) {
			int fd, ret;
			unsigned int i;
			char gpiochip_name[16];
			struct gpiochip_info info;
			sprintf(gpiochip_name, "/dev/%s", item->d_name);
			fd = open(gpiochip_name, O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "Unabled to open gpiochip device\n");
				goto error;
			}
			ret = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
			if (ret < 0) {
				fprintf(stderr, "Failed to get gpiochip info\n");
				close(fd);
				goto error;
			}

			for (i = 0; i < info.lines; i++) {
				struct gpioline_info line_info;
				line_info.line_offset = i;
				ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line_info);
				if (ret < 0) {
					 fprintf(stderr, "Failed to get gpioline info\n");
					 close(fd);
					 goto error;
				}
				if (!strcmp(name, line_info.name)) {
					int chip_nr, chip_base;
					*offset = i;
					if (sscanf(gpiochip_name, "/dev/gpiochip%d", &chip_nr) != 1) {
						perror("sscanf gpiochip: ");
						close(fd);
						goto error;
					}
					*chip = chip_nr;
					if (sscanf(info.label, "gpio-%d", &chip_base) != 1) {
						perror("sscanf gpiochip label: ");
						close(fd);
						goto error;
					}
					*base = chip_base;
					break;
				}
			}

			close(fd);

			if (*offset != -1 && *chip != -1)
				break;
		}

	}

	if (*offset != -1 && *chip != -1)
		rc = EXIT_SUCCESS;

error:
	if (dp != NULL)
		closedir(dp);

	return rc;
}

char *onrisc_get_eeprom_path(void)
{
	assert(init_flag == 1);

	return eeprom.path;
}

onrisc_capabilities_t *onrisc_get_dev_caps(void)
{
	assert(init_flag == 1);

	return &onrisc_capabilities;
}

int onrisc_get_mpcie_sw_state(gpio_level *state)
{
	int rc = EXIT_FAILURE;
	onrisc_sw_caps_t *sw = onrisc_capabilities.mpcie_sw;

	if (NULL == sw) {
		fprintf(stderr, "Device has no mPCIe switch\n");
		goto error;
	}

	if (onrisc_get_sw_state(sw, state) == EXIT_FAILURE) {
		goto error;
	}

	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_set_mpcie_sw_state(gpio_level state)
{
	int rc = EXIT_FAILURE;
	onrisc_sw_caps_t *sw = onrisc_capabilities.mpcie_sw;

	if (NULL == sw) {
		fprintf(stderr, "Device has no mPCIe switch\n");
		goto error;
	}

	if (onrisc_set_sw_state(sw, state) == EXIT_FAILURE) {
		goto error;
	}

	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_get_wlan_sw_state(gpio_level *state)
{
	int rc = EXIT_FAILURE;
	onrisc_sw_caps_t *sw = onrisc_capabilities.wlan_sw;

	if (NULL == sw) {
		fprintf(stderr, "Device has no WLAN switch\n");
		goto error;
	}

	if (onrisc_get_sw_state(sw, state) == EXIT_FAILURE) {
		goto error;
	}

	rc = EXIT_SUCCESS;
error:
	return rc;
}

int onrisc_write_mdio_reg(int phy_id, int reg, int val)
{
	int rc = EXIT_FAILURE;
	int fd;
	int err;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, "eth1");

	struct mii_ioctl_data* mii = (struct mii_ioctl_data*)(&ifr.ifr_data);
	mii->phy_id = phy_id;
	mii->reg_num = reg;
	mii->val_in = val;
	mii->val_out = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0) {
		fprintf(stderr, "failed to create MDIO write socket\n");
		goto error;
	}
	err = ioctl(fd, SIOCSMIIREG, &ifr);
	if (err) {
		fprintf(stderr, "failed to perform SIOCSMIIREG\n");
		goto error;
	}

	rc = EXIT_SUCCESS;
error:
	if (fd > 0) {
		close(fd);
	}

	return rc;
}

int onrisc_read_mdio_reg(int phy_id, int reg, int *val)
{
	int rc = EXIT_FAILURE;
	int fd;
	int err;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, "eth1");

	struct mii_ioctl_data* mii = (struct mii_ioctl_data*)(&ifr.ifr_data);
	mii->phy_id = phy_id;
	mii->reg_num = reg;
	mii->val_in = 0;
	mii->val_out = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0) {
		fprintf(stderr, "failed to create MDIO read socket\n");
		goto error;
	}
	err = ioctl(fd, SIOCGMIIREG, &ifr);
	if (err) {
		fprintf(stderr, "failed to perform SIOCGMIIREG\n");
		goto error;
	}

	*val = mii->val_out;

	rc = EXIT_SUCCESS;
error:
	if (fd > 0) {
		close(fd);
	}

	return rc;
}

/**
 * @brief read hardware parameter from EEPROM
 * @param hw_params structure to put hardware parameters to
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int onrisc_get_hw_params_eeprom(BSP_VS_HWPARAM * hw_params)
{
	int fd, rv, rc = EXIT_SUCCESS;

	fd = open(eeprom.path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open EEPROM (%s)\n", eeprom.path);
		rc = EXIT_FAILURE;
		goto error;
	}
	rv = read(fd, hw_params, sizeof(struct _BSP_VS_HWPARAM));
	if (rv != sizeof(struct _BSP_VS_HWPARAM)) {
		fprintf(stderr, "failed to read EEPROM\n");
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
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int get_partition_size(char *name, uint32_t * size)
{
	FILE *fp;
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
	if (!fp) {
		rc = EXIT_FAILURE;
		perror("fopen");
		goto error;
	}

	while (fgets(buf, sizeof buf, fp)) {
		if ((tmp_ptr = strstr(buf, final_buf)))
			break;
	}

	if (!tmp_ptr) {
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
* @param hw_params hardware parameter structure
* @return EXIT_SUCCESS or EXIT_FAILURE
*/
int onrisc_get_hw_params_nor(struct _param_hw *hw_params)
{
	uint32_t offset, size;
	int rv, ret = EXIT_SUCCESS, fd = 0;

	if (get_partition_size(PARTITION_REDBOOT, &size)) {
		ret = EXIT_FAILURE;
		goto error;
	}

	fd = open(PARTITION_REDBOOT, O_RDONLY);
	if (fd < 0) {
		ret = EXIT_FAILURE;
		goto error;
	}

	offset = size - sizeof(struct _param_hw);
	if (lseek(fd, offset, SEEK_SET) != offset) {
		ret = EXIT_FAILURE;
		goto error;
	}

	rv = read(fd, hw_params, sizeof(struct _param_hw));
	if (rv != sizeof(struct _param_hw)) {
		ret = EXIT_FAILURE;
		goto error;
	}

	if (hw_params->magic != GLOBAL_MAGIC) {
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
		if (strstr(buf, "Alekto2") || strstr(buf, "am335xevm")
		    || strstr(buf, "AM33X")) {
			/* Alekto2 */
			*model = ALEKTO2;
			break;
		} else if (strstr(buf, "VS-860") || strstr(buf, "AM3517")) {
			/* VS-860 */
			*model = VS860;
			break;
		} else if (strstr(buf, "VScom OnRISC")) {
			/* Alekto */
			*model = ALEKTO;
			break;
		} else if (strstr(buf, "Atheros")) {
			/* Atheros */
			eeprom.path = mtd_dev("HW");
			if (eeprom.path == NULL) {
				goto error;
			}

			*model = NETCOM_PLUS_ECO_111;
			break;
		} else if (strstr(buf, "GenuineIntel") || strstr(buf, "AuthenticAMD")) {
			*model = GENERIC_X86_HOST;
		} else {
			/* TODO: default device */
		}
	}

	fclose(fp);
	fp = NULL;

	/* get model from device tree */
	if (*model == ALEKTO2) {
		fp = fopen("/proc/device-tree/model", "r");
		/* Alekto2 doesn't have this entry, so leave type as is */
		if (fp == NULL)
			goto error;

		if (!fgets(buf, sizeof(buf), fp)) {
			perror("fgets");
			fclose(fp);
			*model = 0;
			goto error;
		}

		if (strstr(buf, "Alekto 2")) {
			*model = ALEKTO2;
		}

		if (strstr(buf, "Balios iR 5221") || strstr(buf, "Baltos iR 5221")) {
			*model = BALTOS_IR_5221;
		}

		if (strstr(buf, "Balios iR 3220") || strstr(buf, "Baltos iR 3220")) {
			*model = BALTOS_IR_3220;
		}

		if (strstr(buf, "Balios DIO 1080") || strstr(buf, "Baltos DIO 1080")) {
			*model = BALTOS_DIO_1080;
		}

		if (strstr(buf, "NetCON 3")) {
			*model = NETCON3;
		}

		if (strstr(buf, "NetCom Plus")) {
			*model = NETCOM_PLUS;
		}

		fclose(fp);
		fp = NULL;
	}

 error:
	if (fp)
		fclose(fp);

	return *model ? EXIT_SUCCESS : EXIT_FAILURE;
}

int onrisc_read_hw_params(onrisc_system_t * data)
{
	int i, rc = EXIT_SUCCESS;
	BSP_VS_HWPARAM hw_eeprom;

	if (NULL == eeprom.path) {
		fprintf(stderr, "no EEPROM found\n");
		rc = EXIT_FAILURE;
		goto error;
	}

	if (onrisc_get_hw_params_eeprom(&hw_eeprom) == EXIT_FAILURE) {
		rc = EXIT_FAILURE;
		goto error;
	}

	data->model = hw_eeprom.SystemId;
	data->hw_rev = hw_eeprom.HwRev;
	data->ser_nr = hw_eeprom.SerialNumber;
	strncpy(data->prd_date, hw_eeprom.PrdDate, 11);
	for (i = 0; i < 6; i++) {
		data->mac1[i] = hw_eeprom.MAC1[i];
		data->mac2[i] = hw_eeprom.MAC2[i];
		data->mac3[i] = hw_eeprom.MAC3[i];
	}

error:
	return rc;

}

int onrisc_write_hw_params(onrisc_system_t * data)
{
	int i, rv, rc = EXIT_SUCCESS;
	int fd = -1;
	BSP_VS_HWPARAM hw_eeprom, tmp_eeprom;

	if (NULL == eeprom.path) {
		fprintf(stderr, "no EEPROM found\n");
		rc = EXIT_FAILURE;
		goto error;
	}

	if (data->model == 0xffff) {
		memset(&hw_eeprom, 0xff, sizeof(hw_eeprom));
	} else {
		hw_eeprom.Magic = 0xDEADBEEF;
		hw_eeprom.SystemId = data->model;
		hw_eeprom.HwRev = data->hw_rev;
		hw_eeprom.SerialNumber = data->ser_nr;
		strncpy(hw_eeprom.PrdDate, data->prd_date, 11);
		for (i = 0; i < 6; i++) {
			hw_eeprom.MAC1[i] = data->mac1[i];
			hw_eeprom.MAC2[i] = data->mac2[i];
			hw_eeprom.MAC3[i] = data->mac3[i];
		}
	}

	fd = open(eeprom.path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open EEPROM (%s)\n", eeprom.path);
		rc = EXIT_FAILURE;
		goto error;
	}
	rv = write(fd, &hw_eeprom, sizeof(struct _BSP_VS_HWPARAM));
	if (rv != sizeof(struct _BSP_VS_HWPARAM)) {
		fprintf(stderr, "failed to write EEPROM\n");
		rc = EXIT_FAILURE;
		goto error;
	}

	if (onrisc_get_hw_params_eeprom(&tmp_eeprom) == EXIT_FAILURE) {
		rc = EXIT_FAILURE;
		goto error;
	}

	if (memcmp(&hw_eeprom, &tmp_eeprom, sizeof(struct _BSP_VS_HWPARAM))) {
		fprintf(stderr, "wrong EEPROM data saved\n");
		rc = EXIT_FAILURE;
		goto error;
	}

error:
	if (fd > 0)
		close(fd);
	return rc;
}

void onrisc_print_hw_params()
{
	int i;

	assert(init_flag == 1);

	printf("Hardware Parameters\n");
	printf("===================\n");
	printf("Model: %d\n", onrisc_system.model);
	printf("HW Revision: %d.%d\n", onrisc_system.hw_rev >> 16,
	       onrisc_system.hw_rev & 0xff);
	printf("Serial Number: %d\n", onrisc_system.ser_nr);
	printf("Production date: %s\n", onrisc_system.prd_date);
	printf("MAC1: ");
	for (i = 0; i < 6; i++) {
		printf("%02x", onrisc_system.mac1[i]);
	}
	printf("\n");
	printf("MAC2: ");
	for (i = 0; i < 6; i++) {
		printf("%02x", onrisc_system.mac2[i]);
	}
	printf("\n");
	printf("MAC3: ");
	for (i = 0; i < 6; i++) {
		printf("%02x", onrisc_system.mac3[i]);
	}
	printf("\n");
}


int onrisc_init(onrisc_system_t * data)
{
	int model, i;
	struct _param_hw hw_nor;
	BSP_VS_HWPARAM hw_eeprom;
	eeprom.flags = 0;
	eeprom.path = NULL;

	if (onrisc_get_model(&model) == EXIT_FAILURE) {
		fprintf(stderr, "failed to get model\n");
		return EXIT_FAILURE;
	}

	/* find EEPROM */
	if (model != ALEKTO && model != GENERIC_X86_HOST) {
		if (onrisc_get_eeprom(&eeprom) == EXIT_FAILURE) {
			fprintf(stderr, "failed to find EEPROM\n");
			return EXIT_FAILURE;
		}
	}

	switch (model) {
		case ALEKTO:
			if (onrisc_get_hw_params_nor(&hw_nor) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			onrisc_system.model = hw_nor.biosid;
			onrisc_system.hw_rev = hw_nor.hwrev;
			onrisc_system.ser_nr = hw_nor.serialnr;
			strncpy(onrisc_system.prd_date, hw_nor.prddate, 11);
			for (i = 0; i < 6; i++) {
				onrisc_system.mac1[i] = hw_nor.mac1[i];
				onrisc_system.mac2[i] = hw_nor.mac2[i];
				onrisc_system.mac3[i] = 0xff;
			}
			break;
		case GENERIC_X86_HOST:
			onrisc_system.model = GENERIC_X86_HOST;
			onrisc_system.hw_rev = 0x0101;
			onrisc_system.ser_nr = 12345678;
			strncpy(onrisc_system.prd_date, "01.01.2023", 11);
			for (i = 0; i < 6; i++) {
				onrisc_system.mac1[i] = 0x01;
				onrisc_system.mac2[i] = 0x02;
				onrisc_system.mac3[i] = 0x03;
			}
			break;
		default:
			if (onrisc_get_hw_params_eeprom(&hw_eeprom) ==
			    EXIT_FAILURE) {
				return EXIT_FAILURE;
			}

			onrisc_system.model = hw_eeprom.SystemId;
			onrisc_system.hw_rev = hw_eeprom.HwRev;
			onrisc_system.ser_nr = hw_eeprom.SerialNumber;
			strncpy(onrisc_system.prd_date, hw_eeprom.PrdDate, 11);
			for (i = 0; i < 6; i++) {
				onrisc_system.mac1[i] = hw_eeprom.MAC1[i];
				onrisc_system.mac2[i] = hw_eeprom.MAC2[i];
				onrisc_system.mac3[i] = hw_eeprom.MAC3[i];
			}
	}

	/* copy onrisc_system_t to user space */
	if (data != NULL) {
		memcpy(data, &onrisc_system, sizeof(onrisc_system_t));
	}

	/* initialize devices specific capabilities */
	if (!init_flag) {
		onrisc_init_caps();
	}

	init_flag = 1;

	return EXIT_SUCCESS;
}

int generic_wlan_sw_callback(void * arg)
{
	callback_int_arg_t * params = (callback_int_arg_t *) arg;
	onrisc_gpios_t val;
	val.mask = 1;
	gpio_level level;

	onrisc_wlsw = onrisc_capabilities.wlan_sw;

	val.value = 0;

	level = libsoc_gpio_get_level(onrisc_wlsw->gpio);

	if (level == HIGH) {
		val.value = 1;
	} else {
		val.value = 0;
	}

	return params->callback_fn(val, params->args);
}

int onrisc_wlan_sw_register_callback(int (*callback_fn) (onrisc_gpios_t, void *), void *arg, gpio_edge edge)
{
	int rc = EXIT_FAILURE;
	callback_int_arg_t * params;

	if (!onrisc_capabilities.wlan_sw){
		goto error;
	}

	onrisc_wlsw = onrisc_capabilities.wlan_sw;

	if (onrisc_sw_init(onrisc_wlsw) == EXIT_FAILURE) {
		goto error;
	}

	/* set trigger edge */
	libsoc_gpio_set_edge(onrisc_wlsw->gpio, edge);
	params = malloc(sizeof(callback_int_arg_t));
	if (NULL == params) {
		goto error;
	}
	memset(params, 0, sizeof(callback_int_arg_t));

	params->callback_fn = callback_fn;
	params->index = 0;
	params->args = arg;

	/* register ISR */
	libsoc_gpio_callback_interrupt(onrisc_wlsw->gpio, generic_wlan_sw_callback, (void *) params);

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_wlan_sw_cancel_callback()
{
	int rc = EXIT_FAILURE;

	if (!onrisc_capabilities.wlan_sw){
		goto error;
	}

	onrisc_wlsw = onrisc_capabilities.wlan_sw;

	/* register ISR */
	libsoc_gpio_callback_interrupt_cancel(onrisc_wlsw->gpio);

	rc = EXIT_SUCCESS;
 error:
	return rc;
}

