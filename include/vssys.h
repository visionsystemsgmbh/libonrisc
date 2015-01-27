#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <libudev.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Linux header */
#include <linux/sockios.h>
#include <linux/mii.h>
#include <linux/ethtool.h>

#include "onrisc.h"

/* driver−specific ioctls : */
#define TIOCGRS485 0x542E
#define TIOCSRS485 0x542F

extern int init_flag;
extern onrisc_system_t onrisc_system;
extern onrisc_gpios_int_t onrisc_gpios;
extern int serial_mode_first_pin;
