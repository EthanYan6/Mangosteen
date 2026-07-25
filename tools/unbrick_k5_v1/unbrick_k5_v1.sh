#!/usr/bin/env bash
# Mangosteen Firmware
#
# Copyright (c) 2026 BD1AHN
#
# Licensed under the Apache License, Version 2.0
#
# Project:
#     山竹 (Mangosteen)
#
# Maintainer:
#     BD1AHN
#
# Official Website:
#     https://ethanyan6.github.io/Mangosteen/
#
# The Mangosteen name, logo, and official project identity
# are protected separately from the source code license.
openocd -f ./interface/stlink.cfg -f ./target/dp32g030.cfg -c "init; reset halt; uv_flash_bl bootloader.bin; shutdown"