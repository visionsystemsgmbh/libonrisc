{
  "targets": [
    {
      "target_name": "libonrisc",
      "sources": [
		"onrisc.c",
		"uarts.c",
		"devices.c",
		"dips.c",
		"gpios.c",
		"leds.c",
		"system.c",
		"onrisc_wrap.cxx"],
      "libraries": ['-ludev', '-lsoc'],
    }
  ]
}
