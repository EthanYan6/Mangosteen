# Local firmware mirror

`mangosteen.bin` is copied here on every `./compile-with-docker.sh` build
(from the preset’s `build/<Preset>/mangosteen_*.bin`).

The flasher page loads this file first via the “远程获取” button; GitHub Releases
is only a fallback when the local mirror is missing.
