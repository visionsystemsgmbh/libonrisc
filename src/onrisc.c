#include "vssys.h"

int init_flag = 0;
onrisc_system_t onrisc_system;
onrisc_capabilities_t onrisc_capabilities;
onrisc_eeprom_t eeprom;

/* UART mode variables */
int serial_mode_first_pin = 200;

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

int onrisc_get_wlan_sw_state(gpio_level *state)
{
	int rc = EXIT_FAILURE;
	onrisc_wlan_sw_caps_t *wlan_sw = onrisc_capabilities.wlan_sw;

	if (NULL == wlan_sw) {
		fprintf(stderr, "Device has no WLAN switch\n");
		goto error;
	}

	if (!(wlan_sw->flags & WLAN_SW_IS_SETUP)) {
		/* request GPIO */
		wlan_sw->gpio = libsoc_gpio_request(6, LS_SHARED);
		if (NULL == wlan_sw->gpio) {
			fprintf(stderr, "failed to register WLAN switch GPIO\n");
			goto error;
		}

		/* set direction */
		if (libsoc_gpio_set_direction(wlan_sw->gpio, INPUT) == EXIT_FAILURE) {
			fprintf(stderr, "failed to set GPIO dir\n");
			goto error;
		}

		wlan_sw->flags |= WLAN_SW_IS_SETUP;
	}

	if ((*state = libsoc_gpio_get_level(wlan_sw->gpio)) == LEVEL_ERROR) {
		fprintf(stderr, "failed to set GPIO level\n");
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
	strcpy(ifr.ifr_name, "eth0"); //set to whatever your ethernet device is

	struct mii_ioctl_data* mii = (struct mii_ioctl_data*)(&ifr.ifr_data);
	mii->phy_id = phy_id; //set to your phy's ID
	mii->reg_num = reg; //the register you want to read
	mii->val_in = val;
	mii->val_out = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0)
	{
		fprintf(stderr, "failed to create MDIO write socket\n");
		goto error;
	}
	err = ioctl(fd, SIOCSMIIREG, &ifr);
	if (err)
	{
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
	strcpy(ifr.ifr_name, "eth0"); //set to whatever your ethernet device is

	struct mii_ioctl_data* mii = (struct mii_ioctl_data*)(&ifr.ifr_data);
	mii->phy_id = phy_id; //set to your phy's ID
	mii->reg_num = reg; //the register you want to read
	mii->val_in = 0;
	mii->val_out = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0)
	{
		fprintf(stderr, "failed to create MDIO read socket\n");
		goto error;
	}
	err = ioctl(fd, SIOCGMIIREG, &ifr);
	if (err)
	{
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

int onrisc_get_dips(uint32_t * dips)
{
	int rc = EXIT_SUCCESS;
	gpio *dip_gpios[4];
	gpio_level level;
	int i;
	onrisc_dip_caps_t *dip_caps = onrisc_capabilities.dips;

	assert(init_flag == 1);

	*dips = 0;

	if (NULL == dip_caps) {
		rc = EXIT_FAILURE;
		goto error;
	}

	for (i = 0; i < dip_caps->dip_switch[0].num; i++) {
		uint32_t pin = dip_caps->dip_switch[0].pin[i];

		/* export GPIO */
		dip_gpios[i] = libsoc_gpio_request(pin, LS_SHARED);
		if (dip_gpios[i] == NULL) {
			rc = EXIT_FAILURE;
			goto error;
		}

		/* set direction to input */
		if (libsoc_gpio_set_direction(dip_gpios[i], INPUT) ==
		    EXIT_FAILURE) {
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

		/* free GPIO */
		if (libsoc_gpio_free(dip_gpios[i]) == EXIT_FAILURE) {
			rc = EXIT_FAILURE;
			goto error;
		}
	}

 error:
	return rc;
}

/**
 * @brief read hardware parameter from EEPROM
 * @param hw_params structure to put hardware parameters to
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int onrisc_get_hw_params_eeprom(BSP_VS_HWPARAM * hw_params)
{
	int fd, rv, rc = EXIT_SUCCESS;

	fd = open(eeprom.path, O_RDONLY);
	if (fd <= 0) {
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
 * @return EXIT_SUCCES or EXIT_FAILURE
 */
int get_partition_size(char *name, ulong * size)
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
* @param hw_params hardware paramter structure
* @return EXIT_SUCCES or EXIT_FAILURE
*/
int onrisc_get_hw_params_nor(struct _param_hw *hw_params)
{
	ulong offset, size;
	int rv, ret = EXIT_SUCCESS, fd = 0;

	if (get_partition_size(PARTITION_REDBOOT, &size)) {
		ret = EXIT_FAILURE;
		goto error;
	}

	fd = open(PARTITION_REDBOOT, O_RDONLY);
	if (fd == -1) {
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

	/* get model from device tree */
	if (*model == ALEKTO2) {
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
	}

 error:
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
	int i, fd, rv, rc = EXIT_SUCCESS;
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
	if (fd <= 0) {
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

	if (onrisc_get_model(&model) == EXIT_FAILURE) {
		fprintf(stderr, "failed to get model\n");
		return EXIT_FAILURE;
	}

	/* find EEPROM */
	eeprom.flags = 0;
	eeprom.path = NULL;
	if (onrisc_get_eeprom(&eeprom) == EXIT_FAILURE && ALEKTO != model) {
		fprintf(stderr, "failed to find EEPROM\n");
		return EXIT_FAILURE;
	}

	if (ALEKTO == model) {
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
	} else {
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
	onrisc_init_caps();

	init_flag = 1;

	return EXIT_SUCCESS;
}
