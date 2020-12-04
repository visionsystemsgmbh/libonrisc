var ffi = require('ffi-napi');
var ref = require('ref-napi');
var Struct = require('ref-struct-napi');
var ArrayType = require('ref-array-napi')

const rs_type = {
	TYPE_UNKNOWN: 0,
	TYPE_RS232: 1,
	TYPE_RS422: 2,
	TYPE_RS485_FD: 3,
	TYPE_RS485_HD: 4,
	TYPE_LOOPBACK: 5,
	TYPE_DIP: 6,
	TYPE_CAN: 7,
}

const led_type = {
	LED_POWER: 1,
	LED_WLAN: 2,
	LED_APP: 4
}

var onrisc_system_t = Struct({
  'model' : 'uint16',
  'hw_rev' : 'uint32',
  'ser_nr' : 'uint32',
  'prd_date' : ArrayType('char', 11),
  'mac1' : ArrayType('uint8', 6),
  'mac2' : ArrayType('uint8', 6),
  'mac3' : ArrayType('uint8', 6),
});
var onrisc_system_ptr = ref.refType(onrisc_system_t);

var onrisc_uart_mode_t = Struct({
  'rs_mode' : 'uint32',
  'termination' : 'uint32',
  'dir_ctrl' : 'uint32',
  'echo' : 'uint32',
  'src' : 'uint32',
});
var onrisc_uart_mode_ptr = ref.refType(onrisc_uart_mode_t);

var onrisc_gpios_t = Struct({
  'mask' : 'uint32',
  'value' : 'uint32',
});
var onrisc_gpios_ptr = ref.refType(onrisc_gpios_t);

// define the time types
var time_t = ref.types.long
var suseconds_t = ref.types.long

// define the "timeval" struct type
var timeval = Struct({
  tv_sec: time_t,
  tv_usec: suseconds_t
})

var blink_led_t = Struct({
  'interval' : timeval,
  'high_phase' : timeval,
  'led_type' : 'uint32',
  'count' : 'int32',
  'thread_id' : 'uint32',
  'led' : ref.refType('void'),
  'fd' : 'int',
  'leds_old' : 'uint32',
});
var blink_led_ptr = ref.refType(blink_led_t);

var lib_file = 'libonrisc.so';

var mpcie_state_ptr = ref.alloc('int32');
var wlan_sw_state_ptr = ref.alloc('int32');
var dips_state_ptr = ref.alloc('uint32');

// loads all symbols from the shared object for the libonrisc.so
ffi.DynamicLibrary(lib_file, ffi.DynamicLibrary.FLAGS.RTLD_NOW |
                             ffi.DynamicLibrary.FLAGS.RTLD_GLOBAL);

var onrisc = ffi.Library(lib_file, {
  'onrisc_init': ['int32', [onrisc_system_ptr]],
  'onrisc_get_uart_mode': ['int32', ['int', onrisc_uart_mode_ptr]],
  'onrisc_set_uart_mode': ['int32', ['int', onrisc_uart_mode_ptr]],
  'onrisc_get_wlan_sw_state': ['int32', ['int32*']],
  'onrisc_get_mpcie_sw_state': ['int32', ['int32*']],
  'onrisc_set_mpcie_sw_state': ['int32', ['int32']],
  'onrisc_get_dips': ['int32', ['uint32*']],
  'onrisc_blink_create': ['void', [blink_led_ptr]],
  'onrisc_switch_led': ['int32', [blink_led_ptr, 'uint8']],
  'onrisc_gpio_get_value': ['int32', [onrisc_gpios_ptr]],
  'onrisc_gpio_set_value': ['int32', [onrisc_gpios_ptr]],
});

exports.mpcie_state_ptr = mpcie_state_ptr
exports.wlan_sw_state_ptr = wlan_sw_state_ptr
exports.dips_state_ptr = dips_state_ptr
exports.rs_type = rs_type
exports.led_type = led_type
exports.onrisc_system_t = onrisc_system_t
exports.onrisc_uart_mode_t = onrisc_uart_mode_t
exports.blink_led_t = blink_led_t
exports.onrisc_gpios_t = onrisc_gpios_t
exports.onrisc_init = onrisc.onrisc_init
exports.onrisc_get_uart_mode = onrisc.onrisc_get_uart_mode
exports.onrisc_set_uart_mode = onrisc.onrisc_set_uart_mode
exports.onrisc_get_wlan_sw_state = onrisc.onrisc_get_wlan_sw_state
exports.onrisc_get_mpcie_sw_state = onrisc.onrisc_get_mpcie_sw_state
exports.onrisc_set_mpcie_sw_state = onrisc.onrisc_set_mpcie_sw_state
exports.onrisc_get_dips = onrisc.onrisc_get_dips
exports.onrisc_blink_create = onrisc.onrisc_blink_create
exports.onrisc_switch_led = onrisc.onrisc_switch_led
exports.onrisc_gpio_get_value = onrisc.onrisc_gpio_get_value
exports.onrisc_gpio_set_value = onrisc.onrisc_gpio_set_value
