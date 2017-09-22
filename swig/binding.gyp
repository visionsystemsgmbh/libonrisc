{
  "targets": [
    {
      "target_name": "onrisc",
      "sources": [ "onrisc.c", "onrisc_wrap.cxx", "devices.c", "dips.c", "gpios.c", "leds.c", "system.c", "uarts.c" ],
      "libraries": ['-ludev', '-lsoc'],
    }
  ]
}
