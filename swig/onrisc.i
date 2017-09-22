%module onrisc

%{
#include "onrisc.h"
%}

#define __attribute__(x)
%include "stdint.i"
%include "typemaps.i"
%include "carrays.i"

#ifdef SWIGPYTHON
%typemap(out) uint8_t[6] {
char mac[13] = "000000000000";
sprintf(mac,"%02X%02X%02X%02X%02X%02X",$1[0], $1[1], $1[2], $1[3], $1[4] ,$1[5]);
$result = PyString_FromString(mac);
}

%typemap(in) uint8_t[6] {
        uint8_t mac[6] = {0,0,0,0,0,0};
        sscanf(PyString_AsString($input),"%02X%02X%02X%02X%02X%02X",&mac[0], &mac[1], &mac[2], &mac[3], &mac[4] ,&mac[5]);
	$1=mac;
}

%typemap(in) char[ANY] {
        $1=PyString_AsString($input);
}

/*%typemap(in) gpio_level {
	$1=(gpio_level) PyInt_AsLong($input);
}
%typemap(out) gpio_level* {
	$result=PyInt_FromLong((long) *$1);
}*/


%typemap(out) uint8_t[ANY] {
	$result = PyList_New($1_dim0);
	int i = 0;
	for (; i < $1_dim0; ++i) {
		PyList_SetItem($result, i, PyInt_FromLong((long) $1[i]));
	}
	
}

%typemap(out) uint32_t[ANY] {
	$result = PyList_New($1_dim0);
	int i = 0;
	for (; i < $1_dim0; ++i) {
		PyList_SetItem($result, i, PyInt_FromLong((long) $1[i]));
	}
	
}

%typemap(out) onrisc_dip_switch_t[ANY] {
	$result = PyList_New($1_dim0);
	int i = 0;
	for (; i < $1_dim0; ++i) {
		PyObject *element = SWIG_NewPointerObj(SWIG_as_voidptr(&$1[i]), SWIGTYPE_p_onrisc_dip_switch_t, SWIG_POINTER_OWN);
		PyList_SetItem($result, i, element);
	}
	
}

//for datetime in python

%{
#include <datetime.h>
%}

%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) timeval {
    PyDateTime_IMPORT;
    $1 = PyDelta_Check($input) ? 1 : 0;
}

%typemap(out) struct timeval {
    PyDateTime_IMPORT;
    // Assume interval < a day
    $result = PyDelta_FromDSU(0, $1.tv_sec, $1.tv_usec);
}

%typemap(in) struct timeval
{
    PyDateTime_IMPORT;
    if (!PyDelta_Check($input)) {
	PyErr_SetString(PyExc_ValueError,"Expected a datetime.timedelta");
	return NULL;
    }

    struct timeval ret = {((PyDateTime_Delta*)$input)->seconds, ((PyDateTime_Delta*)$input)->microseconds};
    $1 = ret;
}
#endif
typedef int gpio_level;

%array_class(onrisc_eth_t, ethArray);
%array_class(onrisc_led_t, ledArray);
%array_class(onrisc_gpio_t, gpioArray);
%array_class(onrisc_dip_switch_t, dipSwitchArray);

int onrisc_get_uart_dips(int port_nr, uint32_t *OUTPUT);
int onrisc_get_uart_mode_raw(int port_nr, uint32_t *OUTPUT);
int onrisc_get_dips(uint32_t *OUTPUT);
int onrisc_get_wlan_sw_state(gpio_level *OUTPUT);
int onrisc_get_mpcie_sw_state(gpio_level *OUTPUT);
int onrisc_read_mdio_reg(int phy_id, int reg, int *OUTPUT);

%include "onrisc.h"

