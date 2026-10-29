# Bloodborne iOS feasibility

Public experimental workspace for an iPhone 18 Pro Max port, using cloud tooling and GitHub Actions macOS runners. **No playable iOS IPA has been built or verified.**

## Verified repository state (2026-10-30, Korea)
- The original workflow only compiled a trivial C function to an iOS ARM64 object.
- The current workflow compiles the actual upstream runtime platform layer and every handwritten macos-on-ios shim separately, and uploads build diagnostics.
- Run #3, commit e77990c125ecf916453138852ecc67d464ba56c3, failed **before a runner started**. GitHub's annotation says: "The job was not started because recent account payments have failed or your spending limit needs to be increased."
- No compilation steps ran, no artifacts were produced, and no compiler success or compiler failure can be inferred from that run.
- Public repository migration was requested by the owner. Public runner availability must be tested independently. No game data is uploaded.

## Public repository compilation
The public repository successfully started a macOS ARM64 runner. Its first run compiled upstream src/platform.c to a Mach-O ARM64 iOS object. AppKit_RuntimeHooks.m failed because the runner defaulted to Xcode 16.4 / iOS SDK 18.5, which lacks Metal 4 compiler declarations. The workflow now explicitly selects the confirmed installed Xcode 26.3 toolchain; run 40976604972 passed with Xcode 26.3: the actual runtime platform layer and all handwritten shim translation units compiled successfully. This is object compilation only; linking, app packaging, rendering and game execution remain unverified.

## Source findings (not device validation)
- bloodborne-native/bloodborne-mac packages bloodborne-runtime, bloodborne-recomp and bloodborne-launcher.
- The runtime already contains limited TARGET_OS_IPHONE platform adaptations and ios/bbgpu_stub.c. That GPU stub draws nothing and is explicitly a CPU-only proof of concept.
- The normal runtime build still selects macOS dependencies, desktop packaging and helper executables. There is no complete iOS build target in the reviewed main CMakeLists.txt.
- macos-on-ios-public retags ARM64 Mach-O files and adds UIKit-backed shims. Its converter currently requires a signing identity, team and provisioning profile; it has no unsigned-output option.
- The macOS runtime uses dynamic CPU translation or an optional ahead-of-time compiled game library. macos-on-ios does not itself provide JIT. An iOS CPU execution strategy, including any remaining dynamic-code fallback, must be verified.
- Compiling a shim or platform object does not verify linking, app launch, Metal rendering, game execution, memory use or performance.

## Game data and compatibility
Never commit game assets, PS4 executables, firmware, generated game code or memory dumps.
The upstream expects a decrypted game folder merged with update 1.09: eboot.bin, sce_sys/ and dvdroot_ps4/.
Korean edition compatibility remains unverified: the executable must match the supported build.
Upstream documents GOTY-integrated The Old Hunters (including CUSA03023 and CUSA03173); standalone add-on packages are not currently loaded.

## Next gates
1. Restore Actions runner access and inspect actual compilation diagnostics; fix and rebuild.
2. Build and link the CPU runtime and an iOS app entry point without game data.
3. Evaluate the real Metal renderer and macos-on-ios integration; preserve upstream licenses and credits.
4. Produce an unsigned IPA and verify its Mach-O platforms, dependencies and package structure.
5. Test on the user's device with separately supplied game data; verify Korean text and integrated DLC.

## References
- https://github.com/bloodborne-native/bloodborne-mac
- https://github.com/bloodborne-native/bloodborne-runtime
- https://github.com/bloodborne-native/bloodborne-recomp
- https://github.com/zorrobyte/macos-on-ios-public
- https://github.com/bloodborne-native/bloodborne-mac/blob/main/docs/guide/game-files.md

Built on upstream Bloodborne-native, bbport and shadPS4 work; macos-on-ios is a separate compatibility project.
