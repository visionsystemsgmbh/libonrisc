#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <libudev.h>

#include "onrisc.h"

extern int init_flag;
extern onrisc_system_t onrisc_system;
extern onrisc_gpios_int_t onrisc_gpios;
extern int serial_mode_first_pin;
