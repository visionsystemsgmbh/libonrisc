%module onrisc

%{
#include "include/onrisc.h"
%}

#define __attribute__(x)
%include "stdint.i"
%include "include/onrisc.h"
