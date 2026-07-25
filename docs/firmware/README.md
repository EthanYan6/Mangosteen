<!--
  Mangosteen Firmware

  Copyright (c) 2026 BD1AHN

  Licensed under the Apache License, Version 2.0

  Project:
      山竹 (Mangosteen)

  Maintainer:
      BD1AHN

  Official Website:
      https://ethanyan6.github.io/Mangosteen/

  The Mangosteen name, logo, and official project identity
  are protected separately from the source code license.
-->
# Local firmware mirror

`mangosteen.bin` is copied here on every `./compile-with-docker.sh` build
(from the preset’s `build/<Preset>/mangosteen_*.bin`).

The flasher page loads this file first via the “远程获取” button; GitHub Releases
is only a fallback when the local mirror is missing.
