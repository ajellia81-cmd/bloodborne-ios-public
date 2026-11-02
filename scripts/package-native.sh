#!/usr/bin/env bash
set -euo pipefail
runtime="$PWD/upstream-runtime"
prefix="$runtime/deps/ios-arm64"
app="$PWD/output/Payload/BloodborneIOS.app"
mkdir -p "$app/Licenses"
cp native/Info.plist "$app/Info.plist"
mkdir -p "$app/patches"
cp "$runtime/patches/Bloodborne.xml" "$app/patches/Bloodborne.xml"
plutil -replace UIStatusBarHidden -bool YES "$app/Info.plist"
plutil -replace UIViewControllerBasedStatusBarAppearance -bool NO "$app/Info.plist"

# User-supplied artwork, cropped to square and resized for iOS icons.
base64 --decode < native/AppIcon.jpg.base64 > output/AppIcon.jpg
sips -s format png -z 120 120 output/AppIcon.jpg --out "$app/AppIcon60x60@2x.png" >/dev/null
sips -s format png -z 180 180 output/AppIcon.jpg --out "$app/AppIcon60x60@3x.png" >/dev/null
sips -s format png -z 152 152 output/AppIcon.jpg --out "$app/AppIcon76x76@2x.png" >/dev/null
sips -s format png -z 167 167 output/AppIcon.jpg --out "$app/AppIcon83.5x83.5@2x.png" >/dev/null
python3 - "$app/Info.plist" <<'PY'
import plistlib,sys
p=sys.argv[1]
with open(p,"rb") as f: info=plistlib.load(f)
info["CFBundleVersion"]="5"
info["CFBundleName"]="Bloodborne"
info["CFBundleDisplayName"]="Bloodborne"
info["CFBundleIcons"]={"CFBundlePrimaryIcon":{"CFBundleIconFiles":["AppIcon60x60"],"UIPrerenderedIcon":False}}
info["CFBundleIcons~ipad"]={"CFBundlePrimaryIcon":{"CFBundleIconFiles":["AppIcon60x60","AppIcon76x76","AppIcon83.5x83.5"],"UIPrerenderedIcon":False}}
with open(p,"wb") as f: plistlib.dump(info,f)
PY

export PKG_CONFIG_LIBDIR="$prefix/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR=
xcrun --sdk iphoneos clang -target arm64-apple-ios26.0 -O2 -I"$prefix/include" -c native/main.c -o output/native-main.o
xcrun --sdk iphoneos clang -target arm64-apple-ios26.0 -O2 -I"$prefix/include" -c native/display.c -o output/native-display.o
xcrun --sdk iphoneos clang -target arm64-apple-ios26.0 -O2 -c native/diagnostics.c -o output/native-diagnostics.o
archives=("$prefix"/lib/*.a)
gpuarchives=()
while IFS= read -r archive; do
  if [ "$archive" != "$runtime/out/ios/gpu/libbbgpu.a" ]; then gpuarchives+=("$archive"); fi
done < <(find "$runtime/out/ios/gpu" -type f -name '*.a')
# Upstream libraries only: generated game code is never linked into a public artifact.
xcrun --sdk iphoneos clang++ -target arm64-apple-ios26.0 \
  output/native-main.o output/native-display.o output/native-diagnostics.o "$runtime/out/ios/libbbios.a" "$runtime/out/ios/gpu/libbbgpu.a" \
  "${gpuarchives[@]}" "${archives[@]}" $(pkg-config --libs --static sdl3 libavformat libavcodec libavutil libswscale libswresample) \
  -framework UIKit -framework Foundation -framework Metal -framework QuartzCore \
  -framework CoreGraphics -framework CoreVideo -framework CoreMedia -framework VideoToolbox \
  -framework AudioToolbox -framework AVFoundation -framework GameController -framework CoreHaptics \
  -framework Security -framework SystemConfiguration -framework Accelerate \
  -lz -Wl,-export_dynamic -Wl,-no_fixup_chains \
  -Wl,-init,_bb_diagnostic_init -Wl,-headerpad,0x1000 -Wl,-map,output/native-link-map.txt -o "$app/BloodborneIOS"

# LiveContainer converts MH_EXECUTE to MH_DYLIB. Keep the leading guard segment; avoid
# chained, offset-based initializers. LiveContainer rewrites the first zero-address segment.
python3 - "$app/BloodborneIOS" <<'PY'
import struct,sys
b=open(sys.argv[1],"rb").read(); p=32; inits=0
for _ in range(struct.unpack_from("<I",b,16)[0]):
    cmd,size=struct.unpack_from("<II",b,p)
    assert cmd!=0x80000034, "chained fixups remain"
    if cmd==0x19:
        name=b[p+8:p+24].split(b"\0")[0]
        if name==b"__PAGEZERO":
            assert struct.unpack_from("<Q",b,p+48)[0]==0, "guard segment must have no file content"
        for j in range(struct.unpack_from("<I",b,p+64)[0]):
            q=p+72+j*80
            name=b[q:q+16].split(b"\0")[0]
            assert name!=b"__init_offsets", "offset initializers remain"
            if name==b"__mod_init_func":
                inits+=struct.unpack_from("<Q",b,q+40)[0]//8
    p+=size
assert inits>0, "static initializers must be preserved"
print("LiveContainer link layout: standard guard segment, no chained fixups; preserved",inits,"initializers")
PY

for dir in "$runtime" "$runtime/recomp" "$runtime/third_party/LibAtrac9" "$runtime"/deps/build/src/*; do
  for notice in "$dir"/LICENSE "$dir"/LICENSE.txt "$dir"/LICENSE.md "$dir"/COPYING; do
    if [ -f "$notice" ]; then
      cp "$notice" "$app/Licenses/$(basename "$dir")-$(basename "$notice")"
    fi
  done
done
git -C "$runtime" rev-parse HEAD > "$app/Licenses/runtime-source-version.txt"
git -C "$runtime/recomp" rev-parse HEAD > "$app/Licenses/recompiler-source-version.txt"
cp README.md "$app/Licenses/build-notes.md"
codesign --remove-signature "$app/BloodborneIOS" 2>/dev/null || true
xcrun strip -S "$app/BloodborneIOS"
file "$app/BloodborneIOS" | tee output/native-binary.txt
xcrun vtool -show-build "$app/BloodborneIOS" | tee output/native-platform.txt
otool -L "$app/BloodborneIOS" | tee output/native-linkage.txt
(cd output && zip -qr Bloodborne-iOS-experimental-unsigned.ipa Payload)
