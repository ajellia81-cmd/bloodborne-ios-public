#!/usr/bin/env bash
set -euo pipefail
runtime="$PWD/upstream-runtime"
prefix="$runtime/deps/ios-arm64"
app="$PWD/output/Payload/BloodborneIOS.app"
mkdir -p "$app/Licenses"
cp native/Info.plist "$app/Info.plist"
export PKG_CONFIG_LIBDIR="$prefix/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR=
xcrun --sdk iphoneos clang -target arm64-apple-ios26.0 -O2 -I"$prefix/include" -c native/main.c -o output/native-main.o
archives=("$prefix"/lib/*.a)
gpuarchives=()
while IFS= read -r archive; do
  if [ "$archive" != "$runtime/out/ios/gpu/libbbgpu.a" ]; then gpuarchives+=("$archive"); fi
done < <(find "$runtime/out/ios/gpu" -type f -name '*.a')
# Upstream libraries only: generated game code is never linked into a public artifact.
xcrun --sdk iphoneos clang++ -target arm64-apple-ios26.0 \
  output/native-main.o "$runtime/out/ios/libbbios.a" "$runtime/out/ios/gpu/libbbgpu.a" \
  "${gpuarchives[@]}" "${archives[@]}" $(pkg-config --libs --static sdl3 libavformat libavcodec libavutil libswscale libswresample) \
  -framework UIKit -framework Foundation -framework Metal -framework QuartzCore \
  -framework CoreGraphics -framework CoreVideo -framework CoreMedia -framework VideoToolbox \
  -framework AudioToolbox -framework AVFoundation -framework GameController -framework CoreHaptics \
  -framework Security -framework SystemConfiguration -framework Accelerate \
  -Wl,-export_dynamic -o "$app/BloodborneIOS"
for dir in "$runtime" "$runtime/recomp" "$runtime/third_party/LibAtrac9" "$runtime"/deps/build/src/*; do
  for notice in "$dir"/LICENSE "$dir"/LICENSE.txt "$dir"/LICENSE.md "$dir"/COPYING; do
    if [ -f "$notice" ]; then
      cp "$notice" "$app/Licenses/$(basename "$dir")-$(basename "$notice")"
    fi
  done
done
git -C "$runtime" rev-parse HEAD > "$app/Licenses/runtime-source-version.txt"
cp README.md "$app/Licenses/build-notes.md"
codesign --remove-signature "$app/BloodborneIOS" 2>/dev/null || true
xcrun strip -S "$app/BloodborneIOS"
file "$app/BloodborneIOS" | tee output/native-binary.txt
xcrun vtool -show-build "$app/BloodborneIOS" | tee output/native-platform.txt
otool -L "$app/BloodborneIOS" | tee output/native-linkage.txt
(cd output && zip -qr Bloodborne-iOS-experimental-unsigned.ipa Payload)
