# This repo doubles as a Zephyr module so the ZMK build action compiles our
# custom OLED widget (it sets ZMK_EXTRA_MODULES to the repo root on seeing
# zephyr/module.yml). Only compiled for builds that have a display.
if(CONFIG_ZMK_DISPLAY)
  target_sources(app PRIVATE src/custom_status_screen.c)
endif()
