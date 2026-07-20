#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------
# Usage:
#   ./compile-with-docker.sh [Preset] [CMake options...]
# Examples:
#   ./compile-with-docker.sh Custom
#   ./compile-with-docker.sh Bandscope -DENABLE_SPECTRUM=ON
#   ./compile-with-docker.sh Broadcast -DENABLE_FEAT_F4HWN_GAME=ON -DENABLE_NOAA=ON
#   ./compile-with-docker.sh All
# Default preset: "Custom"
# ---------------------------------------------

IMAGE=uvk1-uvk5v3
PRESET=${1:-Custom}
shift || true  # remove preset from arguments if present

# Any remaining args will be treated as CMake cache variables
EXTRA_ARGS=("$@")

# ---------------------------------------------
# Validate preset name
# ---------------------------------------------
if [[ ! "$PRESET" =~ ^(Custom|Bandscope|Broadcast|Basic|RescueOps|Game|Fusion|All)$ ]]; then
  echo "❌ Unknown preset: '$PRESET'"
  echo "Valid presets are: Custom, Bandscope, Broadcast, Basic, RescueOps, Game, Fusion, All"
  exit 1
fi

# ---------------------------------------------
# Build the Docker image (only needed once)
# ---------------------------------------------
if [[ "$(docker images -q $IMAGE)" == "" ]]; then
  echo "Building Docker image..."
  docker build -t "$IMAGE" .
fi

# ---------------------------------------------
# Clean existing CMake cache to ensure toolchain reload
# ---------------------------------------------
rm -rf build
export MSYS_NO_PATHCONV=1
# ---------------------------------------------
# Function to build one preset
# ---------------------------------------------
build_preset() {
  local preset="$1"
  echo ""
  echo "=== 🚀 Building preset: ${preset} ==="
  echo "---------------------------------------------"
  docker run --rm \
    -u $(id -u):$(id -g) \
    -it -v "$PWD":/src -w /src "$IMAGE" \
    bash -c "which arm-none-eabi-gcc && arm-none-eabi-gcc --version && \
             cmake --preset ${preset} ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"} && \
             cmake --build --preset ${preset} -j"
  echo "✅ Done: ${preset}"
}

# ---------------------------------------------
# 将默认/Custom 产物同步到 docs/firmware（供刷机站同源加载）
# ---------------------------------------------
copy_firmware_to_docs() {
  local preset="$1"
  local build_dir="build/${preset}"
  local src=""
  if [[ -d "$build_dir" ]]; then
    # Prefer mangosteen_*.bin; fall back to any .bin in the preset build dir
    src=$(ls -1 "$build_dir"/mangosteen_*.bin 2>/dev/null | head -n 1 || true)
    if [[ -z "$src" ]]; then
      src=$(ls -1 "$build_dir"/*.bin 2>/dev/null | head -n 1 || true)
    fi
  fi
  if [[ -n "$src" && -f "$src" ]]; then
    mkdir -p docs/firmware
    cp -f "$src" docs/firmware/mangosteen.bin
    echo "📁 已复制固件到 docs/firmware/mangosteen.bin（来源: $src）"
  else
    echo "⚠️ 未找到 ${preset} 固件产物，跳过复制到 docs/firmware"
  fi
}

# ---------------------------------------------
# Handle 'All' preset
# ---------------------------------------------
if [[ "$PRESET" == "All" ]]; then
  PRESETS=(Bandscope Broadcast Basic RescueOps Game Fusion)
  for p in "${PRESETS[@]}"; do
    build_preset "$p"
  done
  # All 构建后用 Custom 产物作站点镜像（若无 Custom 则用 Fusion）
  if [[ -d build/Custom ]]; then
    copy_firmware_to_docs Custom
  else
    copy_firmware_to_docs Fusion
  fi
  echo ""
  echo "🎉 All presets built successfully!"
else
  build_preset "$PRESET"
  copy_firmware_to_docs "$PRESET"
fi
