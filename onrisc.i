%module onrisc

%{
#include "onrisc.h"
%}

#define __attribute__(x)
%include "stdint.i"

#ifdef SWIGPYTHON
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
#endif

%include "onrisc.h"
