#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
upstream="$root/.build/upsilon"
revision=539b90a9a8aaf3db9451da13505f938116650f14

if [[ ! -d "$upstream/.git" ]]; then
  git init -q "$upstream"
  git -C "$upstream" remote add origin https://github.com/UpsilonNumworks/Upsilon.git
fi
if ! git -C "$upstream" cat-file -e "$revision^{commit}" 2>/dev/null; then
  git -C "$upstream" fetch --depth 1 origin "$revision"
fi
if [[ $(git -C "$upstream" rev-parse --verify HEAD 2>/dev/null || true) != "$revision" ]]; then
  git -C "$upstream" checkout --detach "$revision"
fi

mkdir -p "$upstream/aos" "$root/dist"
cp "$root"/src/* "$upstream/aos/"

# The app container header is only used for Upsilon's external-app heap.
# AOS disables it, so this include would only pull in unrelated dependencies.
sed -i '/^#include "apps\/apps_container_storage\.h"$/d' \
  "$upstream/ion/src/device/bootloader/platform_info.cpp"
sed -i -E \
  -e "s/(m_externalAppsFlashStart = )0xFFFFFFFF,/\\1reinterpret_cast<uint32_t *>(0xFFFFFFFFu),/" \
  -e "s/(m_externalAppsRAM(Start|End) = )0xFFFFFFFF,/\\1reinterpret_cast<const uint8_t *>(0xFFFFFFFFu),/" \
  "$upstream/ion/src/device/bootloader/platform_info.cpp"

usb_sources='apps/usb/usb_connected_controller.cpp apps/battery_view.cpp
escher/src/view.cpp escher/src/window.cpp escher/src/text_view.cpp
escher/src/message_text_view.cpp escher/src/view_controller.cpp escher/src/responder.cpp
escher/src/transparent_view.cpp escher/src/palette.cpp'

make -C "$upstream" -f Makefile -f "$root/tools/device.mk" -j"${JOBS:-4}" MODEL=bootloader EPSILON_APPS= apps_src= \
  EPSILON_I18N=en HOME_DISPLAY_EXTERNALS= INCLUDE_ULAB=0 OMEGA_USERNAME=AOS \
  'epsilon_src=$(ion_src) $(liba_src) $(libaxx_src) $(default_kandinsky_src) aos/main.cpp aos/launcher.cpp aos/usb_screen.cpp '"${usb_sources//$'\n'/ }" \
  epsilon.A.bin epsilon.B.bin

output="$upstream/output/release/device/bootloader"
# Package only the upstream raw images, which already exclude RAM-only data.
python3 "$root/tools/package-images.py" "$output" "$root/dist"
(
  cd "$root/dist"
  sha256sum AOS-N0110-slot-{A,B}.{dfu,bin} > SHA256SUMS
)
