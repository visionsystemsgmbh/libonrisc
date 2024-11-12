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
		"onrisc_wrap.cxx"
      ],
      "libraries": ['-lsoc'],
      "cflags_cc": [
        "-std=c++17"
      ],
      "dependencies": [
                "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_maybe",
      ],
    }
  ]
}
