#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
upstream="$root/.build/upsilon-icon"
revision=539b90a9a8aaf3db9451da13505f938116650f14

if [[ ! -e "$upstream/.git" ]]; then
  git init -q "$upstream"
  git -C "$upstream" remote add origin https://github.com/UpsilonNumworks/Upsilon.git
fi
if ! git -C "$upstream" cat-file -e "$revision^{commit}" 2>/dev/null; then
  git -C "$upstream" fetch --depth 1 origin "$revision"
fi
if [[ $(git -C "$upstream" rev-parse --verify HEAD 2>/dev/null || true) != "$revision" ]]; then
  git -C "$upstream" checkout --detach "$revision"
fi

mkdir -p "$root/dist"
patch_file="$root/tools/upsilon-aos.patch"
if ! git -C "$upstream" apply --reverse --check "$patch_file" 2>/dev/null; then
  git -C "$upstream" apply "$patch_file"
fi
cp "$root/src/usb_connection.h" "$upstream/apps/"
for app in latex notes; do
  mkdir -p "$upstream/apps/$app"
  cp "$root/src/$app/"* "$upstream/apps/$app/"
done

make -C "$upstream" -j"${JOBS:-4}" MODEL=bootloader EPSILON_APPS="calculation latex notes" \
  EPSILON_I18N=en HOME_DISPLAY_EXTERNALS= INCLUDE_ULAB=0 OMEGA_USERNAME=AOS \
  epsilon.A.bin epsilon.B.bin

output="$upstream/output/release/device/bootloader"
# Package only the upstream raw images, which already exclude RAM-only data.
python3 "$root/tools/package-images.py" "$output" "$root/dist"
(
  cd "$root/dist"
  sha256sum AOS-N0110-slot-{A,B}.{dfu,bin} > SHA256SUMS
)
