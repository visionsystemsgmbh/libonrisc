var onrisc = require('libonrisc');

var info = new onrisc.onrisc_system_t();
onrisc.onrisc_init(info);

console.log("Serial number: %d", info.ser_nr);
console.log("Production date: %s", info.prd_date);

var caps = onrisc.onrisc_get_dev_caps();

// GPIO
if (caps.gpios) {
	var gpios = new onrisc.onrisc_gpios_t();
	onrisc.onrisc_gpio_get_value(gpios);
	console.log("Show GPIOs value: 0x%s", gpios.value.toString(16));
}

// DIP
if (caps.dips) {
	dip = onrisc.new_uint32_tp();
	onrisc.onrisc_get_dips(dip);
	val = onrisc.uint32_tp_value(dip);
	if (val & onrisc.DIP_S1) {
		console.log("S1: on");
	} else {
		console.log("S1: off");
	}
	if (val & onrisc.DIP_S2) {
		console.log("S2: on");
	} else {
		console.log("S2: off");
	}
	if (val & onrisc.DIP_S3) {
		console.log("S3: on");
	} else {
		console.log("S3: off");
	}
	if (val & onrisc.DIP_S4) {
		console.log("S4: on");
	} else {
		console.log("S4: off");
	}
	onrisc.delete_uint32_tp(dip);
}

// WLAN switch
if (caps.wlan_sw) {
	wlan_sw  = onrisc.new_intp();
	onrisc.onrisc_get_wlan_sw_state(wlan_sw);
	val = onrisc.intp_value(wlan_sw);
	if (val) {
		console.log("WLAN switch: on");
	} else {
		console.log("WLAN switch: off");
	}
	onrisc.delete_intp(wlan_sw);
}

// mPCIe switch
if (caps.mpcie_sw) {
	mpcie_sw  = onrisc.new_intp();
	onrisc.onrisc_get_mpcie_sw_state(mpcie_sw);
	val = onrisc.intp_value(mpcie_sw);
	if (val) {
		console.log("mPCIe switch: on");
	} else {
		console.log("mPCIe switch: off");
	}
	onrisc.delete_intp(mpcie_sw);

	onrisc.onrisc_set_mpcie_sw_state(0);
}

// UARTs
if (caps.uarts) {
	var uart = new onrisc.onrisc_uart_mode_t();
	uart.rs_mode = onrisc.TYPE_RS485_HD;
	onrisc.onrisc_set_uart_mode(1, uart);
}
