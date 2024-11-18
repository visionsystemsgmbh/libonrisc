var onrisc = require('libonrisc');

var obj = new onrisc.OnriscSystem();
var info = obj.getHwParams()


console.log("Serial number: %d", info.ser_nr);
console.log("Production date: %s", info.prd_date);

var caps = obj.getDevCaps();

// GPIO
if (caps.gpios) {
	var gpios = obj.getGpioValue();
	console.log("Show GPIOs value: 0x%s", gpios.value.toString(16));
}

// DIP
if (caps.dips) {
	val = obj.getDips();
	if (val & onrisc.DipMask.DIP_S1) {
		console.log("S1: on");
	} else {
		console.log("S1: off");
	}
	if (val & onrisc.DipMask.DIP_S2) {
		console.log("S2: on");
	} else {
		console.log("S2: off");
	}
	if (val & onrisc.DipMask.DIP_S3) {
		console.log("S3: on");
	} else {
		console.log("S3: off");
	}
	if (val & onrisc.DipMask.DIP_S4) {
		console.log("S4: on");
	} else {
		console.log("S4: off");
	}
}

// WLAN switch
if (caps.wlan_sw) {
	val = obj.getWlanSwState();
	if (val) {
		console.log("WLAN switch: on");
	} else {
		console.log("WLAN switch: off");
	}
}

// mPCIe switch
if (caps.mpcie_sw) {
	val = obj.getMpcieSwState();
	if (val) {
		console.log("mPCIe switch: on");
	} else {
		console.log("mPCIe switch: off");
	}
	obj.setMpcieSwState(1);
}

// UARTs
if (caps.uarts) {
	obj.setUartMode(1, onrisc.RsModes.TYPE_RS485_FD, 0)
}
