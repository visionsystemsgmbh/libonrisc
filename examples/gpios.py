#!/usr/bin/python

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

'''This example shows how to use the GPIOs of the Baltos iR 5221 & 3220 in a few variants.

Invocation:

python gpios.py 
'''

from onrisc import *

# initialize libonrisc
# This routine must be invoked before any other libonrisc call
onrisc_init(None)

caps = onrisc_get_dev_caps()

# only act when GPIOs are present
if caps.gpios:
    gpios = onrisc_gpios_t()
    
    #Get all GPIO states
    onrisc_gpio_get_value(gpios)
    print("Show GPIOs value: 0x{0:08x}".format(gpios.value))
    
    # => Show GPIOs value: 0x0000055a
    
    # Interpretation:
    #    Hex Digits 0 ... 0   5           5         |        a
    #    Binary     0 ... 0       0    1    0    1  |  1   0   1   0
    #    Labels               *  OUT3 OUT2 OUT1 OUT0| IN3 IN2 IN1 IN0      
    # 
    #   * is only relevant with the DIO-Extender

    # Set 0 1 0 1 pattern on OUT3 OUT2 OUT1 OUT0
    gpios.mask = 0xf0
    # ^ affect OUT3 OUT2 OUT1 OUT0
    gpios.value = 0x50
    # ^ define OUT3 and OUT1 as 0; OUT2 and OUT0 as 1
    onrisc_gpio_set_value(gpios)
    # ^ apply the pattern

    # React on changes of IN1 IN0 with a callback function

    #the callback function has to handle two arguments ( which GPIO triggered the callback and the current value of all GPIOs) and return an integer (no specific purpose) 
    def callback(triggered_gpio,value):
        if triggered_gpio & 0x2:
            print("IN1 triggered")
        if triggered_gpio & 0x1:
            print("IN0 triggered")
        return 0

    py_gpio_register_callback(0x3, callback, BOTH)
    # ^ register the callback (notice the "py_" prefix)
    #                         ^ triggered on IN1 & IN 0
    #                              ^ used callback
    #                                        # trigger on BOTH edges (0 -> 1 and 1 -> 0)


    # --> you may need a wait or infinite loop here

    # Do not react anymore (unregister callback for IN1 & IN 0) 
    py_gpio_cancel_callback(0x3)
    #                       ^ unregister callback for IN1 & IN 0

