#include "vssys.h"

void onrisc_config_switch(onrisc_dip_switch_t *sw, uint32_t flags, uint8_t num, uint8_t pin_offset, uint8_t i2c_id)
{
	int i;

	sw->num = num;
	sw->i2c_id = i2c_id;
	sw->flags = flags;

	for (i = 0; i < num; i++) {
		sw->pin[i] = pin_offset + i;
	}
}

int onrisc_init_caps()
{
	int i, rc = EXIT_FAILURE;
	int maj, min;
	onrisc_gpios_int_t *gpios = NULL;
	onrisc_led_caps_t *leds = NULL;
	onrisc_dip_caps_t *dips = NULL;
	onrisc_uart_caps_t *uarts = NULL;
	onrisc_sw_caps_t *wlan_sw = NULL;
	onrisc_sw_caps_t *mpcie_sw = NULL;
	onrisc_eth_caps_t *eths = NULL;

	maj = onrisc_system.hw_rev >> 16;
	min = onrisc_system.hw_rev & 0xffff;

	/* LEDS */
	/* initialize LED caps */
	leds = malloc(sizeof(onrisc_led_caps_t));
	if (NULL == leds) {
		goto error;
	}
	memset(leds, 0, sizeof(onrisc_led_caps_t));

	switch(onrisc_system.model) {
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE);
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE);
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE);

			break;
		case BALTOS_IR_5221:
		case BALTOS_IR_3220:
		case BALTOS_IR_2110:
		case NETCOM_PLUS_111:
		case NETCOM_PLUS_113:
		case NETCOM_PLUS_211:
		case NETCOM_PLUS_213:
		case NETCAN:
			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
			leds->led[LED_POWER].pin = 96;
			strcpy(leds->led[LED_POWER].name,"onrisc:red:power");
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 16;
			strcpy(leds->led[LED_WLAN].name,"onrisc:blue:wlan");
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 17;
			strcpy(leds->led[LED_APP].name,"onrisc:green:app");

			if (NETCAN == onrisc_system.model) {
				leds->num = 4;
				leds->led[LED_CAN_ERROR].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
				leds->led[LED_CAN_ERROR].pin = 15;
				break;
			}

			break;
		case ALEKTO2:
			leds->num = 1;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE | LED_NEEDS_I2C_ADDR | LED_IS_INPUT_ACTIVE);
			leds->led[LED_POWER].pin = 9;
			leds->led[LED_POWER].i2c_id = 0x21;

			break;
		/*TODO: detect hw rev */
		case NETCOM_PLUS:
		case NETCOM_PLUS_811:
		case NETCOM_PLUS_413:
		case NETCOM_PLUS_813:
			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
			leds->led[LED_POWER].pin = 96;
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 17;
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 16;

			if (NETCOM_PLUS_811 == onrisc_system.model || NETCOM_PLUS == onrisc_system.model) {
				if (1 == maj && 2 == min) {
					leds->led[LED_WLAN].pin = 16;
					leds->led[LED_APP].pin = 17;
				}
			}
			break;
		case BALTOS_DIO_1080:
		case NETCON3:
			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
			leds->led[LED_POWER].pin = 96;
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 16;
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 17;
			break;
		case NETCOM_PLUS_ECO_111:
		case NETCOM_PLUS_ECO_113:
		case NETCOM_PLUS_ECO_111_WLAN:
		case NETCOM_PLUS_ECO_113_WLAN:
		case NETCOM_PLUS_ECO_411:
		case NETCOM_PLUS_ECO_111A:
		case NETCOM_PLUS_ECO_113A:
		case NETCOM_PLUS_ECO_211A:
		case NETCOM_PLUS_ECO_213A:
		case NETCAN_PLUS_ECO:
		case NETCAN_PLUS_ECO_WLAN:
		case NETCAN_PLUS_ECO_110A:
		case NETCAN_PLUS_ECO_210A:
		case NETIO:
		case NETIO_WLAN:
		case MICROROUTER:
		case NETUSB:
			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE | LED_IS_INPUT_ACTIVE);
			leds->led[LED_POWER].pin = 27;
			strcpy(leds->led[LED_POWER].name,"onrisc:red:power");
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 0;
			strcpy(leds->led[LED_WLAN].name,"onrisc:blue:wlan");
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 15;
			strcpy(leds->led[LED_APP].name,"onrisc:green:app");
			if (NETUSB == onrisc_system.model) {
				leds->num = 5;
				leds->led[LED_USB1].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
				leds->led[LED_USB1].pin = 18;
				strcpy(leds->led[LED_USB1].name,"onrisc:green:usb1");
				leds->led[LED_USB2].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
				leds->led[LED_USB2].pin = 19;
				strcpy(leds->led[LED_USB2].name,"onrisc:green:usb2");
			}
			break;
	}

	/* UARTS */
	/* initialize UART caps */
	uarts = malloc(sizeof(onrisc_uart_caps_t));
	if (NULL == uarts) {
		goto error;
	}
	memset(uarts, 0, sizeof(onrisc_uart_caps_t));

	switch(onrisc_system.model) {
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			uarts->num = 2;
			uarts->flags = UARTS_SWITCHABLE;
			for (i = 0; i < uarts->num; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					0,
					0,
					0,
					0);
			}
			break;
		case BALTOS_IR_5221:
		case BALTOS_IR_3220:
			uarts->num = 2;
			uarts->flags = UARTS_SWITCHABLE | UARTS_DIPS_PHYSICAL;
			for (i = 0; i < uarts->num; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					(RS_HAS_485_SW | RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					8 + 4 * i,
					0x20);
			}
			break;
		case ALEKTO2:
			uarts->num = 2;
			uarts->flags = UARTS_SWITCHABLE | UARTS_DIPS_PHYSICAL;
			for (i = 0; i < uarts->num; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					4 * i,
					0x21);
			}
			break;
		case VS860:
			uarts->num = 2;
			uarts->flags = UARTS_SWITCHABLE | UARTS_DIPS_PHYSICAL;
			for (i = 0; i < uarts->num; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					8 + 4 * i,
					0x20);
			}
			break;
		/*TODO: detect hw rev */
		case BALTOS_DIO_1080:
		case NETCON3:
			uarts->num = 8;
			uarts->flags = 0;
			break;

		case NETCOM_PLUS:
		case NETCOM_PLUS_413:
		case NETCOM_PLUS_813:
		case NETCOM_PLUS_811:
			if (NETCOM_PLUS_413 == onrisc_system.model || NETCOM_PLUS == onrisc_system.model) {
				uarts->num = 4;
			} else {
				uarts->num = 8;
			}

			if (NETCOM_PLUS_811 == onrisc_system.model || NETCOM_PLUS == onrisc_system.model) {
				if (1 == maj && 2 == min) {
					uarts->flags = 0;
					break;
				}
			}

			uarts->flags = UARTS_SWITCHABLE;

			for (i = 0; i < 4; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					4 * i,
					0x20);
			}

			if (NETCOM_PLUS_813 == onrisc_system.model || NETCOM_PLUS_811 == onrisc_system.model) {
				for (i = 0; i < 4; i++) {
					onrisc_config_switch(&uarts->ctrl[i + 4],
						(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
						4,
						4 * i,
						0x21);
				}
			}
			break;

		case NETCOM_PLUS_111:
		case NETCOM_PLUS_113:
		case NETCOM_PLUS_211:
		case NETCOM_PLUS_213:
		case BALTOS_IR_2110:
			if (NETCOM_PLUS_111 == onrisc_system.model || NETCOM_PLUS_113 == onrisc_system.model || BALTOS_IR_2110 == onrisc_system.model) {
				uarts->num = 1;
			} else {
				uarts->num = 2;
			}

			if (NETCOM_PLUS_111 == onrisc_system.model || NETCOM_PLUS_211 == onrisc_system.model) {
				uarts->flags = 0;
			} else {
				uarts->flags = UARTS_SWITCHABLE;
			}

			onrisc_config_switch(&uarts->ctrl[0],
				(RS_HAS_485_SW | RS_HAS_TERMINATION | RS_IS_GPIO_BASED),
				4,
				32 * 3 + 14,
				0);

			if (NETCOM_PLUS_211 == onrisc_system.model || NETCOM_PLUS_213 == onrisc_system.model) {
				onrisc_config_switch(&uarts->ctrl[1],
					(RS_HAS_485_SW | RS_HAS_TERMINATION | RS_IS_GPIO_BASED),
					4,
					32 * 3 + 18,
					0);
			}

			break;

		case NETCOM_PLUS_ECO_111:
		case NETCOM_PLUS_ECO_113:
		case NETCOM_PLUS_ECO_111_WLAN:
		case NETCOM_PLUS_ECO_113_WLAN:
		case MICROROUTER:
			uarts->num = 1;
			uarts->flags = UARTS_SWITCHABLE;
			if (NETCOM_PLUS_ECO_111 == onrisc_system.model ||
			    NETCOM_PLUS_ECO_111_WLAN == onrisc_system.model) {
				uarts->flags = 0;
			}
			uarts->ctrl[0].num = 4;
			uarts->ctrl[0].flags = RS_HAS_TERMINATION | RS_IS_GPIO_BASED;
			uarts->ctrl[0].pin[0] = 18;
			uarts->ctrl[0].pin[1] = 19;
			uarts->ctrl[0].pin[2] = 16;
			uarts->ctrl[0].pin[3] = 14;
			break;

		case NETCOM_PLUS_ECO_411:
			uarts->num = 4;
			uarts->flags = 0;
			break;
		case NETCOM_PLUS_ECO_111A:
		case NETCOM_PLUS_ECO_113A:
		case NETCOM_PLUS_ECO_211A:
		case NETCOM_PLUS_ECO_213A:
			if (NETCOM_PLUS_ECO_111A == onrisc_system.model
			    || NETCOM_PLUS_ECO_113A == onrisc_system.model) {
				uarts->num = 1;
			} else {
				uarts->num = 2;
			}

			if (NETCOM_PLUS_ECO_111A == onrisc_system.model
			    || NETCOM_PLUS_ECO_211A == onrisc_system.model) {
				uarts->flags = 0;
				break;
			} else {
				uarts->flags = UARTS_SWITCHABLE;
			}

			onrisc_config_switch(&uarts->ctrl[0],
				(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
				4,
				0,
				0x20);
			if (NETCOM_PLUS_ECO_213A == onrisc_system.model) {
				onrisc_config_switch(&uarts->ctrl[1],
					(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					4,
					0x20);
			}
			break;
	}

	/* GPIOS */
	switch(onrisc_system.model) {
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			gpios = malloc(sizeof(onrisc_gpios_int_t));
			if (NULL == gpios) {
				goto error;
			}
			memset(gpios, 0, sizeof(onrisc_gpios_int_t));

			/* TODO: fill with standard config */
			gpios->ngpio = 8;

			break;
		case ALEKTO2:
			gpios = malloc(sizeof(onrisc_gpios_int_t));
			if (NULL == gpios) {
				goto error;
			}
			memset(gpios, 0, sizeof(onrisc_gpios_int_t));

			/* TODO: fill with standard config */
			gpios->ngpio = 8;

			break;
		case BALTOS_IR_5221:
		case BALTOS_IR_3220:
			gpios = malloc(sizeof(onrisc_gpios_int_t));
			if (NULL == gpios) {
				goto error;
			}
			memset(gpios, 0, sizeof(onrisc_gpios_int_t));

			gpios->ngpio = 8;

			for (i = 0; i < gpios->ngpio / 2; i++) {
				int offset = i + gpios->ngpio / 2;

				gpios->gpios[i].direction = INPUT;
				gpios->gpios[i].dir_fixed = 1;
				gpios->gpios[offset].direction = OUTPUT;
				gpios->gpios[offset].dir_fixed = 1;
			}

			for (i = 0; i < gpios->ngpio / 2; i++) {
				int offset = i + gpios->ngpio;

				gpios->gpios[offset].flags = GPIO_IS_VIRTUAL;
				gpios->gpios[offset].direction = INPUT;
				gpios->gpios[offset].dir_fixed = 0;
			}

			gpios->nvgpio = 4;

			break;
		case BALTOS_DIO_1080:
		case NETCON3:
			gpios = malloc(sizeof(onrisc_gpios_int_t));
			if (NULL == gpios) {
				goto error;
			}
			memset(gpios, 0, sizeof(onrisc_gpios_int_t));

			gpios->ngpio = 16;

			for (i = 0; i < gpios->ngpio / 2; i++) {
				int offset = i + gpios->ngpio / 2;

				gpios->gpios[i].direction = INPUT;
				gpios->gpios[i].dir_fixed = 1;
				gpios->gpios[offset].direction = OUTPUT;
				gpios->gpios[offset].dir_fixed = 1;
			}
			break;
		case NETIO:
		case NETIO_WLAN:
			gpios = malloc(sizeof(onrisc_gpios_int_t));
			if (NULL == gpios) {
				goto error;
			}
			memset(gpios, 0, sizeof(onrisc_gpios_int_t));

			gpios->ngpio = 4;
			break;
	}

	/* DIPS */
	switch(onrisc_system.model) {
		case BALTOS_DIO_1080:
		case NETCON3:
			/* initialize DIP caps */
			dips = malloc(sizeof(onrisc_dip_caps_t));
			if (NULL == dips) {
				goto error;
			}
			memset(dips, 0, sizeof(onrisc_dip_caps_t));

			dips->num = 1;
			dips->dip_switch[0].num = 4;
			dips->dip_switch[0].pin[0] = 44;
			dips->dip_switch[0].pin[1] = 45;
			dips->dip_switch[0].pin[2] = 46;
			dips->dip_switch[0].pin[3] = 47;

			break;
		case NETCOM_PLUS:
		case NETCOM_PLUS_811:
			/* initialize DIP caps */
			dips = malloc(sizeof(onrisc_dip_caps_t));
			if (NULL == dips) {
				goto error;
			}
			memset(dips, 0, sizeof(onrisc_dip_caps_t));

			if (1 == maj && 2 == min) {
				dips->num = 1;
				dips->dip_switch[0].num = 4;
				dips->dip_switch[0].pin[0] = 44;
				dips->dip_switch[0].pin[1] = 45;
				dips->dip_switch[0].pin[2] = 46;
				dips->dip_switch[0].pin[3] = 47;
			} else {
				dips->num = 1;
				dips->dip_switch[0].num = 4;
				dips->dip_switch[0].pin[0] = 32 * 2 + 18;
				dips->dip_switch[0].pin[1] = 32 * 2 + 19;
				dips->dip_switch[0].pin[2] = 32 * 3 + 9;
				dips->dip_switch[0].pin[3] = 32 * 3 + 10;
			}
			break;
		case NETCOM_PLUS_413:
		case NETCOM_PLUS_813:
		case NETCOM_PLUS_111:
		case NETCOM_PLUS_113:
		case NETCOM_PLUS_211:
		case NETCOM_PLUS_213:
		case NETCAN:
		case BALTOS_IR_2110:
			/* initialize DIP caps */
			dips = malloc(sizeof(onrisc_dip_caps_t));
			if (NULL == dips) {
				goto error;
			}
			memset(dips, 0, sizeof(onrisc_dip_caps_t));

			dips->num = 1;
			dips->dip_switch[0].num = 4;
			dips->dip_switch[0].pin[0] = 32 * 2 + 18;
			dips->dip_switch[0].pin[1] = 32 * 2 + 19;
			dips->dip_switch[0].pin[2] = 32 * 3 + 9;
			dips->dip_switch[0].pin[3] = 32 * 3 + 10;
			break;
		case NETCOM_PLUS_ECO_111:
		case NETCOM_PLUS_ECO_113:
		case NETCOM_PLUS_ECO_111_WLAN:
		case NETCOM_PLUS_ECO_113_WLAN:
		case NETCOM_PLUS_ECO_411:
		case NETCOM_PLUS_ECO_111A:
		case NETCOM_PLUS_ECO_113A:
		case NETCOM_PLUS_ECO_211A:
		case NETCOM_PLUS_ECO_213A:
		case NETCAN_PLUS_ECO:
		case NETCAN_PLUS_ECO_WLAN:
		case NETCAN_PLUS_ECO_110A:
		case NETCAN_PLUS_ECO_210A:
		case MICROROUTER:
		case NETUSB:
			/* initialize DIP caps */
			dips = malloc(sizeof(onrisc_dip_caps_t));
			if (NULL == dips) {
				goto error;
			}
			memset(dips, 0, sizeof(onrisc_dip_caps_t));

			dips->num = 1;
			dips->dip_switch[0].num = 4;
			dips->dip_switch[0].pin[0] = 20;
			dips->dip_switch[0].pin[1] = 21;
			dips->dip_switch[0].pin[2] = 22;
			dips->dip_switch[0].pin[3] = 23;
			break;
		case NETIO:
		case NETIO_WLAN:
			/* initialize DIP caps */
			dips = malloc(sizeof(onrisc_dip_caps_t));
			if (NULL == dips) {
				goto error;
			}
			memset(dips, 0, sizeof(onrisc_dip_caps_t));

			dips->num = 1;
			dips->dip_switch[0].num = 3;
			dips->dip_switch[0].pin[0] = 20;
			dips->dip_switch[0].pin[1] = 21;
			dips->dip_switch[0].pin[2] = 22;
			break;
	}

	/* SWITCHES */
	switch(onrisc_system.model) {
		case BALTOS_IR_5221:
		case BALTOS_IR_3220:
			/* initialize WLAN switch caps */
			wlan_sw = malloc(sizeof(onrisc_sw_caps_t));
			if (NULL == wlan_sw) {
				goto error;
			}
			memset(wlan_sw, 0, sizeof(onrisc_sw_caps_t));

			wlan_sw->pin = 6;
			wlan_sw->flags = SW_IS_READ_ONLY;

			/* initialize mPCIe switch caps */
			mpcie_sw = malloc(sizeof(onrisc_sw_caps_t));
			if (NULL == mpcie_sw) {
				goto error;
			}
			memset(mpcie_sw, 0, sizeof(onrisc_sw_caps_t));

			mpcie_sw->pin = 100;

			break;
		/*TODO: detect hw rev */
		case NETCOM_PLUS:
		case NETCOM_PLUS_811:
			/* TODO only for new 811/411 */
			/* initialize mPCIe switch caps */
			mpcie_sw = malloc(sizeof(onrisc_sw_caps_t));
			if (NULL == mpcie_sw) {
				goto error;
			}
			memset(mpcie_sw, 0, sizeof(onrisc_sw_caps_t));

			mpcie_sw->pin = 100;

			break;
		/*TODO: detect hw rev */
		case NETCOM_PLUS_413:
		case NETCOM_PLUS_813:
			/* initialize mPCIe switch caps */
			mpcie_sw = malloc(sizeof(onrisc_sw_caps_t));
			if (NULL == mpcie_sw) {
				goto error;
			}
			memset(mpcie_sw, 0, sizeof(onrisc_sw_caps_t));

			mpcie_sw->pin = 100;

			break;
	}

	/* ETHS */
	eths = malloc(sizeof(onrisc_eth_caps_t));
	if (NULL == eths) {
		goto error;
	}
	memset(eths, 0, sizeof(onrisc_eth_caps_t));

	eths->num = 2;
	
	eths->eth[0].num = 1;
	eths->eth[0].flags = (ETH_RGMII_1G | ETH_PHYSICAL);
	eths->eth[0].phy_id = 7;
	strcpy(eths->eth[0].if_name, "eth1");

	switch(onrisc_system.model) {
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
		case ALEKTO2:
			eths->eth[0].num = 1;
			eths->eth[0].flags = (ETH_RGMII_1G | ETH_PHYSICAL);
			eths->eth[0].phy_id = 0;
			strcpy(eths->eth[0].if_name, "eth0");

			eths->eth[1].num = 1;
			eths->eth[1].flags = (ETH_RGMII_1G | ETH_PHYSICAL);
			eths->eth[1].phy_id = 1;
			strcpy(eths->eth[1].if_name, "eth1");
			break;

		case BALTOS_IR_5221:
			eths->eth[1].num = 4;
			eths->eth[1].flags = (ETH_RMII_100M | ETH_PHYSICAL);
			eths->eth[1].phy_id = 0;
			strcpy(eths->eth[1].if_name, "eth0");
			break;

		case BALTOS_IR_3220:
			eths->eth[1].num = 2;
			eths->eth[1].flags = (ETH_RMII_100M | ETH_PHYSICAL);
			eths->eth[1].phy_id = 0;
			strcpy(eths->eth[1].if_name, "eth0");
			break;

		case BALTOS_IR_2110:
			eths->eth[1].num = 1;
			eths->eth[1].flags = (ETH_RMII_100M | ETH_PHYSICAL);
			eths->eth[1].phy_id = 1;
			strcpy(eths->eth[1].if_name, "eth0");
			break;

		case VS860:
			eths->eth[0].num = 1;
			eths->eth[0].flags = (ETH_RMII_100M | ETH_PHYSICAL);
			eths->eth[0].phy_id = 1;
			strcpy(eths->eth[0].if_name, "usb0");
			eths->eth[1].num = 1;
			eths->eth[1].flags = (ETH_RMII_100M | ETH_PHYSICAL);
			eths->eth[1].phy_id = 1;
			strcpy(eths->eth[1].if_name, "eth0");
			break;

		case NETCOM_PLUS_111:
		case NETCOM_PLUS_113:
		case NETCOM_PLUS_211:
		case NETCOM_PLUS_213:
		case NETCAN:
		case NETCOM_PLUS:
		case NETCOM_PLUS_811:
		case NETCOM_PLUS_413:
		case NETCOM_PLUS_813:
		case BALTOS_DIO_1080:
		case NETCON3:
			eths->eth[1].num = 0;
			eths->eth[1].flags = ETH_RMII_100M;
			eths->eth[1].phy_id = 0;
			strcpy(eths->eth[1].if_name, "eth0");
			break;
		case NETCOM_PLUS_ECO_111:
		case NETCOM_PLUS_ECO_113:
		case NETCOM_PLUS_ECO_111_WLAN:
		case NETCOM_PLUS_ECO_113_WLAN:
		case NETCOM_PLUS_ECO_411:
		case NETCOM_PLUS_ECO_111A:
		case NETCOM_PLUS_ECO_113A:
		case NETCOM_PLUS_ECO_211A:
		case NETCOM_PLUS_ECO_213A:
		case NETCAN_PLUS_ECO:
		case NETCAN_PLUS_ECO_WLAN:
		case NETCAN_PLUS_ECO_110A:
		case NETCAN_PLUS_ECO_210A:
		case NETIO:
		case NETIO_WLAN:
		case NETUSB:
			strcpy(eths->eth[0].if_name, "eth0");
			break;
		case MICROROUTER:
			//WAN
			strcpy(eths->eth[0].if_name, "eth0");
			eths->eth[0].num = 1;
			eths->eth[0].flags = (ETH_RMII_100M | ETH_PHYSICAL);
			//LAN
			strcpy(eths->eth[1].if_name, "eth1");
			eths->eth[1].num = 1;
			eths->eth[1].flags = (ETH_RMII_100M | ETH_PHYSICAL);
			break;
	}

	onrisc_capabilities.gpios = gpios;
	onrisc_capabilities.dips = dips;
	onrisc_capabilities.uarts = uarts;
	onrisc_capabilities.leds = leds;
	onrisc_capabilities.wlan_sw = wlan_sw;
	onrisc_capabilities.mpcie_sw = mpcie_sw;
	onrisc_capabilities.eths = eths;

	rc = EXIT_SUCCESS;
error:
	return rc;
}
