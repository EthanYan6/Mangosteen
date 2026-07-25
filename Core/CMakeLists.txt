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

add_library(Core INTERFACE)
target_include_directories(Core INTERFACE Inc)
target_link_libraries(Core INTERFACE PY32F071_Driver)
target_compile_definitions(Core INTERFACE 
    PY32F071x8 
    USE_FULL_LL_DRIVER
)

target_sources(Core INTERFACE
    Src/main.c
    Src/py32f071_it.c
    Src/system_py32f071.c
    startup_py32f071xx.s
)

target_link_options(Core INTERFACE "-T" "${CMAKE_CURRENT_SOURCE_DIR}/py32f071xb.ld")
