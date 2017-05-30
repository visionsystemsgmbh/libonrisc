#include "vssys.h"

int dip_init_flag = 0;
onrisc_capabilities_t onrisc_capabilities;
onrisc_dip_caps_t * onrisc_dips;

int onrisc_dip_init()
{
	int rc = EXIT_SUCCESS;
	gpio_level level;
	int i;
	onrisc_dip_caps_t *dip_caps = onrisc_capabilities.dips;

	assert(init_flag == 1);


	if (NULL == dip_caps) {
		rc = EXIT_FAILURE;
		goto error;
	}

	for (i = 0; i < dip_caps->dip_switch[0].num; i++) {
		uint32_t pin = dip_caps->dip_switch[0].pin[i];

		/* export GPIO */
		dip_caps->dip_switch[0].gpio[i] = libsoc_gpio_request(pin, LS_GPIO_SHARED);
		if (dip_caps->dip_switch[0].gpio[i] == NULL) {
			rc = EXIT_FAILURE;
			goto error;
		}

		/* set direction to input */
		if (libsoc_gpio_set_direction(dip_caps->dip_switch[0].gpio[i], INPUT) ==
		    EXIT_FAILURE) {
			rc = EXIT_FAILURE;
			goto error;
		}
	}

	dip_init_flag = 1;

 error:
	return rc;
}

int onrisc_get_dips(uint32_t * dips)
{
	int rc = EXIT_SUCCESS;
	gpio_level level;
	int i;
	onrisc_dip_caps_t *dip_caps = onrisc_capabilities.dips;

	assert(init_flag == 1);

	*dips = 0;

	if (NULL == dip_caps) {
		rc = EXIT_FAILURE;
		goto error;
	}

	if (!dip_init_flag) {
		if (onrisc_dip_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	for (i = 0; i < dip_caps->dip_switch[0].num; i++) {
		uint32_t pin = dip_caps->dip_switch[0].pin[i];

		/* get level */
		level = libsoc_gpio_get_level(dip_caps->dip_switch[0].gpio[i]);
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

int generic_dip_callback(void * arg)
{
	callback_int_arg_t * params = (callback_int_arg_t *) arg;
	onrisc_gpios_t val;
	val.mask = 1 << (params->index);
	gpio_level level;
	int i;

	onrisc_dips = onrisc_capabilities.dips;

	val.value = 0;

	for (i = 0; i < onrisc_dips->dip_switch[0].num; i++) {
		level = libsoc_gpio_get_level(onrisc_dips->dip_switch[0].gpio[i]);

		if (level == HIGH) {
			val.value &= ~(1 << i);
		} else {
			val.value |= 1 << i;
		}
	}

	return params->callback_fn(val, params->args);
}

int onrisc_dip_register_callback(uint32_t idx, int (*callback_fn) (onrisc_gpios_t, void *), void *arg, gpio_edge edge)
{
	int i, rc = EXIT_FAILURE;
	callback_int_arg_t * params;

	if (!onrisc_capabilities.dips){
		goto error;
	}

	if (onrisc_capabilities.dips->num <= idx){
		goto error;
	}
	onrisc_dips = onrisc_capabilities.dips;

	if (!dip_init_flag) {
		if (onrisc_dip_init() == EXIT_FAILURE) {
			goto error;
		}
	}

	for (i = 0; i < onrisc_dips->dip_switch[idx].num; i++) {
		/* set trigger edge */
		libsoc_gpio_set_edge(onrisc_dips->dip_switch[idx].gpio[i], edge);	
		params = malloc(sizeof(callback_int_arg_t));
		if (NULL == params) {
			goto error;
		}
		memset(params, 0, sizeof(callback_int_arg_t));
		
		params->callback_fn = callback_fn;				
		params->index = i;				
		params->args = arg;				

		/* register ISR */
		libsoc_gpio_callback_interrupt(onrisc_dips->dip_switch[idx].gpio[i], generic_dip_callback, (void *) params);
	}
	rc = EXIT_SUCCESS;
 error:
	return rc;
}

int onrisc_dip_cancel_callback(uint32_t idx)
{
	int i, rc = EXIT_FAILURE;

	if (!onrisc_capabilities.dips){
		goto error;
	}

	if (onrisc_capabilities.dips->num <= idx){
		goto error;
	}
	onrisc_dips = onrisc_capabilities.dips;

	for (i = 0; i < onrisc_dips->dip_switch[idx].num; i++) {
		/* register ISR */
		libsoc_gpio_callback_interrupt_cancel(onrisc_dips->dip_switch[idx].gpio[i]);
	}
	rc = EXIT_SUCCESS;
 error:
	return rc;
}

