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

%include "onrisc.h"
