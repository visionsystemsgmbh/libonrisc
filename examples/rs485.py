 #!/usr/bin/python

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

'''This example shows how to setup RS485 half-duplex mode with termination
and send some characters at 115200 baud.

Invocation:

python rs485.py /dev/ttyS1 1
'''

import sys
import serial
import onrisc


def switch_to_rs485(port_nr):
    '''Setup RS mode via libonrisc to RS485 and enable termination.'''
    uart = onrisc.onrisc_uart_mode_t()
    uart.rs_mode = onrisc.TYPE_RS485_HD
    uart.termination = 1
    onrisc.onrisc_set_uart_mode(port_nr, uart)


if len(sys.argv) != 3:
    print("Wrong parameter count. \nUsage:\npython rs485.py device port-number \
        \nExmaple:\npython rs485.py /dev/ttyS1 1\n")
    sys.exit(1)

# initialize libonrisc
# This routine must be invoked before any other libonrisc call
onrisc.onrisc_init(None)

switch_to_rs485(int(sys.argv[2]))

# open serial port and send a string
try:
    with serial.serial_for_url(sys.argv[1]) as ser:
        ser.baudrate = 115200
        ser.timeout = 1
        ser.write('hello'.encode('ascii'))
except IOError as err:
    print(err)
    sys.exit(1)
