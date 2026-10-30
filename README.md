# Bloodborne iOS feasibility

Public experimental workspace for an iPhone 18 Pro Max port, using cloud tooling and GitHub Actions macOS runners. **No playable iOS game build has been verified.** A CPU-only diagnostic unsigned IPA has been built and structurally validated.

## Verified repository state (2026-10-30, Korea)
- The original workflow only compiled a trivial C function to an iOS ARM64 object.
- The current workflow compiles the actual upstream runtime platform layer and every handwritten macos-on-ios shim separately, and uploads build diagnostics.
- The historical private run failed **before a runner started**. It supplied no compiler evidence. Public macOS runner access has since been verified.
- No compilation steps ran, no artifacts were produced, and no compiler success or compiler failure can be inferred from that run.
- Public repository migration was requested by the owner. Public runner availability must be tested independently. No game data is uploaded.

## Public repository compilation
The public repository successfully started a macOS ARM64 runner. Its first run compiled upstream src/platform.c to a Mach-O ARM64 iOS object. AppKit_RuntimeHooks.m failed because the runner defaulted to Xcode 16.4 / iOS SDK 18.5, which lacks Metal 4 compiler declarations. The workflow now explicitly selects the confirmed installed Xcode 26.3 toolchain; run 40976604972 passed with Xcode 26.3: the actual runtime platform layer and all handwritten shim translation units compiled successfully. This is object compilation only; linking, app packaging, rendering and game execution remain unverified.

## Source findings (not device validation)
- bloodborne-native/bloodborne-mac packages bloodborne-runtime, bloodborne-recomp and bloodborne-launcher.
- Upstream docs/IOS_POC.md describes an iPhone 17 Pro Max proof of concept using native Metal, AOT game code and interpreter fallback. This is an upstream report, not a result reproduced here. scripts/ios/build-deps.sh, build-gpu.sh, build-core.sh and recomp.sh are public. The upstream app shell remains private. ios/bbgpu_stub.c is a separate CPU-only stub that draws nothing.
- The normal runtime build still selects macOS dependencies, desktop packaging and helper executables. There is no complete iOS build target in the reviewed main CMakeLists.txt.
- macos-on-ios-public retags ARM64 Mach-O files and adds UIKit-backed shims. Its converter currently requires a signing identity, team and provisioning profile; it has no unsigned-output option.
- The CPU source disables JIT under TARGET_OS_IPHONE and falls back to its interpreter. Upstream's iOS PoC uses an ahead-of-time game library for performance. No generated game code is included here. macos-on-ios has been compile-tested as an alternative, but its retagging does not supply JIT; the native iOS runtime is the primary build path.
- Compiling a shim or platform object does not verify linking, app launch, Metal rendering, game execution, memory use or performance.

## Game data and compatibility
Never commit game assets, PS4 executables, firmware, generated game code or memory dumps.
The upstream expects a decrypted game folder merged with update 1.09: eboot.bin, sce_sys/ and dvdroot_ps4/.
Korean edition compatibility remains unverified: the executable must match the supported build.
Upstream documents GOTY-integrated The Old Hunters (including CUSA03023 and CUSA03173); standalone add-on packages are not currently loaded.

## Next gates
1. Inspect actual compilation diagnostics, fix failures and rebuild. Public runner access is verified.
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

## Current build results
- The private ajellia81-cmd/bloodborne-ios repository was deleted at the owner's request after the public files were verified.
- Run 40977401736 successfully compiled and linked the complete CPU runtime, SDL 3.4.18, Zydis 4.1.1, LibAtrac9 and the upstream CPU-only GPU stub for iOS ARM64.
- Bloodborne-CPUProbe-unsigned.ipa contains a UIKit test that calls the real interpreter with a self-authored seven-byte addition function. The IPA's executable is ARM64, has an iOS build-version command, and has no code-signature load command. The test has not been executed on a physical device.
- The full native Metal runtime workflow builds the upstream iOS dependencies, GPU library and core, then links the new SDL app shell. Its results must be checked before claiming a full runtime IPA.
- The native shell expects separately prepared boot-linked.bin and 960x540 patches.bin plus the user's game/ directory in Documents/Bloodborne. The loader additionally requires content.bin. It defaults to Korean game language (PS4 language 9), 960x540, Metal, interpreter fallback and offline play. Language, DLC, game loading and rendering remain unverified.

## Simulator execution
Run 40978238814 passed the device build, simulator build and a cloud iOS simulator execution of the diagnostic app. The real bbcpu interpreter returned 42 for the synthetic x86-64 addition function with arguments 19 and 23. This verifies that narrow interpreter/ABI/stack path in the simulator, not physical-device address space, rendering, the game or performance.

## Native Metal build result
Run 40978956252 built the actual upstream iOS Metal GPU static library successfully. The subsequent core step stopped before compilation because macOS's default Bash 3.2 treats the upstream script's empty stub array as unbound under nounset. The workflow now installs and explicitly uses modern Bash for that step. Final app linking and native runtime IPA packaging remain pending.
