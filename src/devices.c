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
	onrisc_led_caps_t *leds = NULL;
	onrisc_dip_caps_t *dips = NULL;
	onrisc_uart_caps_t *uarts = NULL;
	onrisc_sw_caps_t *wlan_sw = NULL;
	onrisc_sw_caps_t *mpcie_sw = NULL;

	switch(onrisc_system.model) {
		case ALEKTO:
		case ALENA:
		case ALEKTO_LAN:
			/* initialize LED caps */
			leds = malloc(sizeof(onrisc_led_caps_t));
			if (NULL == leds) {
				goto error;
			}
			memset(leds, 0, sizeof(onrisc_led_caps_t));

			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE);
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE);
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE);

			/* initialize UART caps */
			uarts = malloc(sizeof(onrisc_uart_caps_t));
			if (NULL == uarts) {
				goto error;
			}
			memset(uarts, 0, sizeof(onrisc_uart_caps_t));

			uarts->num = 2;
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
			/* initialize LED caps */
			leds = malloc(sizeof(onrisc_led_caps_t));
			if (NULL == leds) {
				goto error;
			}
			memset(leds, 0, sizeof(onrisc_led_caps_t));

			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
			leds->led[LED_POWER].pin = 96;
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 16;
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 17;

			/* initialize UART caps */
			uarts = malloc(sizeof(onrisc_uart_caps_t));
			if (NULL == uarts) {
				goto error;
			}
			memset(uarts, 0, sizeof(onrisc_uart_caps_t));

			uarts->num = 2;
			for (i = 0; i < uarts->num; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					(RS_HAS_485_SW | RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					8 + 4 * i,
					0x20);
			}

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
		case ALEKTO2:
			/* initialize LED caps */
			leds = malloc(sizeof(onrisc_led_caps_t));
			if (NULL == leds) {
				goto error;
			}
			memset(leds, 0, sizeof(onrisc_led_caps_t));

			leds->num = 1;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE | LED_NEEDS_I2C_ADDR);
			leds->led[LED_POWER].pin = 9;
			leds->led[LED_POWER].i2c_id = 0x21;

			/* initialize UART caps */
			uarts = malloc(sizeof(onrisc_uart_caps_t));
			if (NULL == uarts) {
				goto error;
			}
			memset(uarts, 0, sizeof(onrisc_uart_caps_t));

			uarts->num = 2;
			for (i = 0; i < uarts->num; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					4 * i,
					0x21);
			}

			break;
		/*TODO: detect hw rev */
		case BALTOS_DIO_1080:
		case NETCON3:
		case NETCOM_PLUS:
		case NETCOM_PLUS_811:
			/* initialize LED caps */
			leds = malloc(sizeof(onrisc_led_caps_t));
			if (NULL == leds) {
				goto error;
			}
			memset(leds, 0, sizeof(onrisc_led_caps_t));

			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
			leds->led[LED_POWER].pin = 96;
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 17;
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 16;

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

			/* initialize LED caps */
			leds = malloc(sizeof(onrisc_led_caps_t));
			if (NULL == leds) {
				goto error;
			}
			memset(leds, 0, sizeof(onrisc_led_caps_t));

			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
			leds->led[LED_POWER].pin = 96;
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 17;
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 16;

			/* initialize UART caps */
			uarts = malloc(sizeof(onrisc_uart_caps_t));
			if (NULL == uarts) {
				goto error;
			}
			memset(uarts, 0, sizeof(onrisc_uart_caps_t));

			if (NETCOM_PLUS_413 == onrisc_system.model) {
				uarts->num = 4;
			} else {
				uarts->num = 8;
			}

			for (i = 0; i < 4; i++) {
				onrisc_config_switch(&uarts->ctrl[i],
					(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
					4,
					4 * i,
					0x20);
			}

			if (NETCOM_PLUS_813 == onrisc_system.model) {
				for (i = 0; i < 4; i++) {
					onrisc_config_switch(&uarts->ctrl[i + 4],
						(RS_HAS_TERMINATION | RS_IS_GPIO_BASED | RS_NEEDS_I2C_ADDR),
						4,
						4 * i,
						0x21);
				}
			}

			/* initialize mPCIe switch caps */
			mpcie_sw = malloc(sizeof(onrisc_sw_caps_t));
			if (NULL == mpcie_sw) {
				goto error;
			}
			memset(mpcie_sw, 0, sizeof(onrisc_sw_caps_t));

			mpcie_sw->pin = 100;

			break;
		case NETCOM_PLUS_111:
		case NETCOM_PLUS_113:
		case NETCOM_PLUS_211:
		case NETCOM_PLUS_213:
		case NETCAN:
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

			/* initialize LED caps */
			leds = malloc(sizeof(onrisc_led_caps_t));
			if (NULL == leds) {
				goto error;
			}
			memset(leds, 0, sizeof(onrisc_led_caps_t));

			leds->num = 3;
			leds->led[LED_POWER].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
			leds->led[LED_POWER].pin = 96;
			leds->led[LED_WLAN].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_WLAN].pin = 16;
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 17;

			if (NETCAN == onrisc_system.model) {
				leds->num = 4;
				leds->led[LED_CAN_ERROR].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED);
				leds->led[LED_CAN_ERROR].pin = 15;
				break;
			}

			/* initialize UART caps */
			uarts = malloc(sizeof(onrisc_uart_caps_t));
			if (NULL == uarts) {
				goto error;
			}
			memset(uarts, 0, sizeof(onrisc_uart_caps_t));

			if (NETCOM_PLUS_111 == onrisc_system.model || NETCOM_PLUS_113 == onrisc_system.model) {
				uarts->num = 1;
			} else {
				uarts->num = 2;
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
	}

	onrisc_capabilities.dips = dips;
	onrisc_capabilities.uarts = uarts;
	onrisc_capabilities.leds = leds;
	onrisc_capabilities.wlan_sw = wlan_sw;
	onrisc_capabilities.mpcie_sw = mpcie_sw;

	rc = EXIT_SUCCESS;
error:
	return rc;
}
