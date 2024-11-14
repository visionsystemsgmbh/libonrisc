OnRISC API Library
==================

Features
--------

1. get system's hardware info like serial number. MACs etc.
2. control LEDs
3. control UART's RS232/422/485 driver
4. control digital I/O
5. read DIP switch
6. control mPCIe slot

Installation
------------

**libonrisc** has the following requirements: **CMake** as a build system
and it depends on **libsoc** library.

1. `cmake -B build`
2. `cmake --build build`
3. `cmake --install build`

Python Bindings
---------------

**libonrisc** also provides Python bindings generated via SWIG. In order to
build them install **swig** and **libpython** development packages and tell
CMake to create Python bindings and proceed as described in the "Installation"
section:

    cmake -B build -DPYTHON_WRAP=ON

In Debian, the **python3-onrisc1** package provides Python bindings.

The following example shows how to get device's serial number:

```python
import onrisc

info = onrisc.onrisc_system_t()
onrisc.onrisc_init(info)
print(info.ser_nr)
```

Further Python examples can be found in the `test` folder.

Node.js Bindings
----------------

**libonrisc** also provides Node.js bindings. In order to build them in Debian,
install **nodejs** package and tell CMake to create Node.js bindings:

    cmake -B build -DNODEJS_WRAP=ON

Now perform the following actions:

1. ``npm install -g `npm pack ./build/nodejs` ``
2. `echo "export NODE_PATH=/usr/lib/node_modules" >> /etc/profile.d/vscom.sh`
3. `source /etc/profile.d/vscom.sh`

The following example shows how to get device's serial number:

```javascript
var onrisc = require('libonrisc');

var obj = new onrisc.OnriscSystem();
console.log(obj.getHwParams().ser_nr)
```

Further Node.js examples can be found in `test` folder.

pkg-config Support
------------------

**libonrisc** automatically installs `libonrisc.pc` file, so that one can query
its version, linker flags, etc. To query **libonrisc** version invoke:

    pkg-config --modversion libonrisc

In CMake you can use the following code fragment to find `libonrisc`:

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PC_LIBONRISC REQUIRED libonrisc)

If `libonrisc` was found its include and linker flags will be stored in
`${PC_LIBONRISC_INCLUDE_DIRS}` and `${PC_LIBONRISC_LIBRARIES}` respectively.

API
---

All routines and data structures are described in `include/onrisc.h` using **Doxygen** syntax. It is mandatory to invoke `onrisc_init()` before you use any other API calls otherwise you'll get assertion triggered. `cli/onrisctool.c` provides usage examples to the most API calls.

### Get system information

```C
typedef struct {
  uint16_t model;
  uint32_t hw_rev;
  uint32_t ser_nr;
  char prd_date[11];
  uint8_t mac1[6];
  uint8_t mac2[6];
  uint8_t mac3[6];
} __attribute__ ((packed)) onrisc_system_t;

int onrisc_init(onrisc_system_t *data);
```

`onrisc_init()` routine initializes internal `onrisc_system_t` structure with data found in EEPROM/NOR flash and copies it to `*data`.

### LEDs

```C
void onrisc_blink_create(blink_led_t *blinker);
void onrisc_blink_destroy(blink_led_t *blinker);
int onrisc_blink_led_start(blink_led_t *blinker);
int onrisc_blink_led_stop(blink_led_t *blinker);
int onrisc_switch_led(blink_led_t *led, uint8_t state);
```

### UARTs

```C
int onrisc_set_uart_mode(int port_nr, onrisc_uart_mode_t *mode);

typedef struct {
  uint32_t rs_mode;
  uint32_t termination;
  uint32_t dir_ctrl;
  uint32_t echo;
} __attribute__ ((packed)) onrisc_uart_mode_t;
```

`onrisc_set_uart_mode()` routine configures RS232/422/485 driver. `onrisc_uart_mode_t` structure holds data for both KS8695 and OMAP3 based systems. `rs_mode` is common for both systems and configures RS modes and full/half-duplex modes. `termination` is available only for OMAP3 based devices. `dir_ctrl` and `echo` will be used on KS8695 based devices only.

### Digital I/O

```C
typedef struct  {
  uint32_t mask;
  uint32_t value;
} __attribute__ ((packed)) onrisc_gpios_t;

int onrisc_gpio_set_direction(onrisc_gpios_t * gpio_dir);
```

`onrisc_gpio_set_direction()` sets digital I/O direction to input/output for devices without fixed I/Os. `gpio_dir.mask` defines which pins to modify, `gpio_dir.values` defines I/O directions for relevant pins.

```C
int onrisc_gpio_set_value(onrisc_gpios_t * gpio_val);
```

`onrisc_gpio_set_value()` sets digital outputs. `gpio_val.mask` defines which pins to modify, `gpio_val.values` defines output levels. Inputs, that are selected in the mask, will be silently ignored.

```C
int onrisc_gpio_get_value(onrisc_gpios_t * gpio_val);
```

`onrisc_gpio_get_value()` gets digital inputs and outputs values. `gpio_val.values` holds the values for both inputs and outputs.

### DIP switch

```C
int onrisc_get_dips(uint32_t *dips);
```

`onrisc_get_dips()` returns DIP switch state. You can check, whether
particular switch is set via the following macros and *&* operator:

```C
#define DIP_S1	0x01
#define DIP_S2	0x02
#define DIP_S3	0x04
#define DIP_S4	0x08
```

### WLAN switch

```C
int onrisc_get_wlan_sw_state(gpio_level *state);
```

`onrisc_get_wlan_sw_state()` returns WLAN switch state, i.e. 1 - on, 0 - off

### mPCIe switch

```C
int onrisc_get_mpcie_sw_state(gpio_level *state);
int onrisc_set_mpcie_sw_state(gpio_level state);
```

`onrisc_get_mpcie_sw_state()` returns mPCIe switch state, i.e. 1 - on, 0 - off
`onrisc_set_mpcie_sw_state()` sets mPCIe switch state, i.e. 1 - on, 0 - off
