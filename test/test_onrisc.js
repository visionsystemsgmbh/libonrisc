var onrisc = require('libonrisc');

var info = new onrisc.onrisc_system_t();
onrisc.onrisc_init(info);

console.log("Serial number: %d", info.ser_nr);
console.log("Production date %s", info.prd_date);
