# Custom OLED widget for the Corne (Matrix-rain screensaver).
# This repo doubles as a Zephyr module: the ZMK build action sets
# ZMK_EXTRA_MODULES to the repo root when it sees zephyr/module.yml, which pulls
# this CMakeLists in. The source is only compiled for builds that have a display
# (so the settings_reset build, which has none, skips it).
if(CONFIG_ZMK_DISPLAY)
  target_sources(app PRIVATE src/custom_status_screen.c)
endif()
