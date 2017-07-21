#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <onrisc.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TEST_BAUDRATE 	B9600

/*
 * This example shows how to setup RS485 half-duplex mode with termination
 * and send some characters at 9600 baud.
 *
 * Compilation instructions:
 * 
 * gcc -o rs485 rs485.c -l onrisc
 */
int main(int argc, char **argv)
{
	int rc = EXIT_FAILURE;
	int fd = -1, ret;
	struct termios ser_termios;
	onrisc_uart_mode_t mode;
	char buf[32];

	if (argc != 3) {
		printf("Wrong parameter count. \nUsage:\nrs485 device port-number\nExmaple:\nrs485 /dev/ttyO1 1\n");
		goto error;
	}

	/* 
	 * initialize libonrisc
	 * This routine must be invoked before any other libonrisc call
	 */
	if (onrisc_init(NULL) == EXIT_FAILURE) {
		perror("init libonrisc: ");
		goto error;
	}

	/* set RS mode to RS485 half-duplex and enable termination */
	mode.rs_mode = TYPE_RS485_HD;
	mode.termination = 1;

	if (onrisc_set_uart_mode(atoi(argv[2]), &mode) == EXIT_FAILURE) {
		perror("failed to set serial port mode: ");
		goto error;
	}

	/* open serial port */
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("open: ");
		goto error;
	}

	ret = tcgetattr(fd, &ser_termios);
	if (ret < 0) {
		perror("Getting attributes");
		goto error;
	}
	cfmakeraw(&ser_termios);

	/* configure local flags */
	ser_termios.c_lflag = 0;

	/* configure input flags */
	ser_termios.c_iflag = IGNPAR;

	/* configure output flags */
	ser_termios.c_oflag = 0;

	/* configure control flags */
	ser_termios.c_cflag = CS8 | CLOCAL | CREAD;

	ser_termios.c_cc[VMIN] = 0;
	ser_termios.c_cc[VTIME] = 0;

	ret = cfsetispeed(&ser_termios, TEST_BAUDRATE);
	ret = cfsetospeed(&ser_termios, TEST_BAUDRATE);
	ret = tcsetattr(fd, TCSANOW, &ser_termios);
	if (ret < 0) {
		perror("Setting attributes");
		goto error;
	}

	/* send test string */
	sprintf(buf, "hello");
	ret = write(fd, buf, strlen(buf));

	rc = EXIT_SUCCESS;
 error:
	if (fd > 0) {
		close(fd);
	}

	return rc;
}
