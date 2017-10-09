#!/usr/bin/python

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

from onrisc import *

info = onrisc_system_t()

onrisc_init(info)

print("Serial number: {}".format(info.ser_nr))
print("Production date: {}".format(info.prd_date))

caps = onrisc_get_dev_caps()

# GPIO
if caps.gpios:
    gpios = onrisc_gpios_t()
    onrisc_gpio_get_value(gpios)
    print("Show GPIOs value: 0x{0:08x}".format(gpios.value))

# DIP
if caps.dips:
    rc, dips = onrisc_get_dips()
    print("Show DIPs value: 0x{0:08x}".format(dips))

# WLAN switch
if caps.wlan_sw:
    rc, wlan_sw = onrisc_get_wlan_sw_state()
    if wlan_sw:
        print("WLAN switch: on")
    else:
        print("WLAN switch: off")

# mPCIe switch
if caps.mpcie_sw:
    rc, mpcie_sw = onrisc_get_mpcie_sw_state()
    if mpcie_sw:
        print("mPCIe switch: on")
    else:
        print("mPCIe switch: off")

    onrisc_set_mpcie_sw_state(0)

# UARTs
if caps.uarts:
    uart = onrisc_uart_mode_t()
    uart.rs_mode = TYPE_RS485_HD
    onrisc_set_uart_mode(1, uart)
