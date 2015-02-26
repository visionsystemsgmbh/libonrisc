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
		case BALIOS_IR_5221:
		case BALIOS_IR_3220:
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
		case BALIOS_DIO_1080:
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
			leds->led[LED_WLAN].pin = 16;
			leds->led[LED_APP].flags = (LED_IS_AVAILABLE | LED_IS_GPIO_BASED | LED_IS_HIGH_ACTIVE);
			leds->led[LED_APP].pin = 17;

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
		case NETCOM_PLUS_413:
		case NETCOM_PLUS_813:
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

			if (NETCOM_PLUS_413 == onrisc_system.model) {
				uarts->num = 4;
			} else {
				uarts->num = 8;
			}

			for (i = 0; i < uarts->num; i++) {
				uarts->ctrl[i].flags = (RS_HAS_TERMINATION | RS_IS_GPIO_BASED);
			}

			break;
		case NETCOM_PLUS_111:
		case NETCOM_PLUS_113:
		case NETCOM_PLUS_211:
		case NETCOM_PLUS_213:
			/* initialize DIP caps */
			dips = malloc(sizeof(onrisc_dip_caps_t));
			if (NULL == dips) {
				goto error;
			}
			memset(dips, 0, sizeof(onrisc_dip_caps_t));

			dips->num = 1;
			dips->dip_switch[0].num = 4;
			dips->dip_switch[0].pin[0] = 114;
			dips->dip_switch[0].pin[1] = 115;
			dips->dip_switch[0].pin[2] = 137;
			dips->dip_switch[0].pin[3] = 138;

			break;
	}

	onrisc_system.caps.dips = dips;
	onrisc_system.caps.uarts = uarts;
	onrisc_system.caps.leds = leds;

	rc = EXIT_SUCCESS;
error:
	return rc;
}
