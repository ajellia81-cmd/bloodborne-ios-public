# Bloodborne iOS experimental runtime

A game-free iOS shell for Bloodborne-native, built entirely with cloud tooling and GitHub Actions macOS runners. Target: iPhone 18 Pro Max. **Game execution on that device has not been verified.**

## Verified results
| Check | Result |
| --- | --- |
| Original private workspace | Deleted at the owner's request after public files were verified |
| Runtime platform and macos-on-ios handwritten source compilation | Passed with Xcode 26.3 (run 40976604972) |
| Complete CPU runtime link, iOS ARM64 | Passed |
| Real CPU interpreter executing a synthetic x86-64 addition in cloud iOS Simulator 18.5 | Passed: 19 + 23 returned 42 (run 40978238814) |
| Actual native Metal GPU, core, app link and unsigned IPA packaging | Passed for the earlier fixed-resolution shell (run 40979870364) |
| Native-display profile behavior | Passed: portrait normalization, landscape pixels, aspect auto and preservation of unrelated settings |
| Updated native-display IPA | Passed: run 41284868696; unsigned iOS ARM64, bundled patch XML, system framework/library imports only |
| Physical iPhone launch, game loading, Korean rendering, DLC, frame rate and memory | Not verified |

The earlier compiler failure in macos-on-ios used the runner's default Xcode 16.4 / SDK 18.5, which lacks Metal 4 declarations. The workflow explicitly selects Xcode 26.3. The upstream iOS core script also needs modern Bash to handle its empty GPU-stub array under nounset; the workflow uses Homebrew Bash.

## Native display and aspect ratio
The shell queries the fullscreen high-density drawable in **pixels**, normalizes it to landscape, and uses it for both output and scene rendering. It writes `aspect_ratio=auto`, `live_resolution=0` and the measured `output_res`, while preserving other settings.

There is no forced 16:9 or 960x540 output. For example, a 2868x1320 drawable uses 2868x1320 at 239:110. The launcher canvas scales uniformly. Status bars are hidden and the game window requests fullscreen Retina rendering. The patch XML is bundled, and the current upstream C patch builder generates resolution patches at launch using `--patches -`; no prebuilt `patches.bin` is needed.

These are verified configuration and build behaviors. Physical-device rendering, camera framing, HUD placement and performance still require testing with the user's own data.

## Build path
- `.github/workflows/ios-feasibility.yml`: source-level iOS compilation diagnostics, including macos-on-ios.
- `.github/workflows/ios-cpu.yml`: complete CPU-only runtime, diagnostic IPA and simulator execution.
- `.github/workflows/ios-metal.yml`: pinned dependency downloads with upstream checksums, native iOS dependencies, real Metal GPU and core, display-profile check, full native runtime IPA.
- `native/`: SDL iOS shell and tested display-profile implementation.
- `scripts/package-native.sh`: app link, notices, platform/linkage diagnostics and unsigned IPA packaging.

The upstream now publishes a direct iOS runtime path. Its `docs/IOS_POC.md` reports an iPhone 17 Pro Max PoC with Metal plus ahead-of-time game code and interpreter fallback. That is an **upstream report**, not a test reproduced here. This project uses the native iOS path; macos-on-ios remains an analyzed and compile-tested alternative, not a JIT provider.

## Personal game data
Never commit game assets, PS4 executables, firmware, generated game code, memory dumps or personalized AOT libraries. Ignore rules cover common data and build artifacts. The public IPA includes no game data or generated game code.

The game folder must be a decrypted PS4 dump with the clean **1.09** update merged in. Korean edition executable compatibility must be checked. GOTY-integrated The Old Hunters is the current compatibility goal; standalone add-on package loading is not established.

The native shell expects:
```text
Documents/Bloodborne/
  boot-linked.bin
  content.bin
  game/
    eboot.bin
    sce_sys/param.sfo
    sce_module/libc.prx
    sce_module/libSceFios2.prx
    dvdroot_ps4/...
  user/
  bbport.ini
  runtime.log
```

`scripts/prepare-personal-data.py` prepares personal boot metadata outside Git and refuses execution in public GitHub Actions. The open-source runtime currently falls back to its interpreter without a personal AOT library, so compilation success is not evidence of playable performance. AOT code must be prepared privately from the owner's executable, embedded in a personal app build and signed appropriately; it must never be published.

Defaults: Korean game language (PS4 language 9), offline play, native device resolution, 30fps limit, no upscaler. These settings do not establish Korean/DLC compatibility or measured performance.

## Credits and source
Built on Bloodborne-native, bbport by deadinside28 and shadPS4. macos-on-ios is a separate compatibility project. Runtime-derived code is GPL-2.0-or-later; see LICENSE and bundled dependency notices.

- https://github.com/bloodborne-native/bloodborne-mac
- https://github.com/bloodborne-native/bloodborne-runtime
- https://github.com/bloodborne-native/bloodborne-recomp
- https://github.com/zorrobyte/macos-on-ios-public

The native-display IPA was structurally validated after download: ZIP integrity, ARM64 CPU type, iOS Mach-O platform, no code-signature load command, landscape orientations, hidden status bar and bundled resolution patch XML. SHA256: 3822953e50149bce94038732b881042599036a0721890ee13f874b6d31c44e44. The latest shader scanner also requires zlib; the app link includes -lz. These checks do not substitute for physical-device or game testing.
