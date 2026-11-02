# Bloodborne iOS experimental runtime

A game-free iOS shell for Bloodborne-native, built entirely with cloud tooling and GitHub Actions macOS runners. Target: iPhone 18 Pro Max. **The launcher and early game initialization have run on the user's device. Gameplay has not been verified.**

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
| Physical-device launcher and early game initialization | User logs: build 7 reaches SDL, native Metal, game worker threads and audio initialization |
| Game rendering and gameplay | Unresolved: GXRenderThread faults reading address 0x10 in the CPU interpreter |
| Korean rendering, DLC, frame rate | Not verified |

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

## Current device diagnostics
Build 8 opens controllers connected before launch and on hotplug so SDL can deliver launcher button events. This change has not yet been verified on the physical device. The startup log is `Documents/Bloodborne/startup.log`; runtime output is `Documents/Bloodborne/runtime.log`. Terminating interpreter faults now report a guest register snapshot and guest stack using the existing safe-memory reader, in addition to the host backtrace. This is diagnostic reporting; it does not recover from or fix the graphics-thread fault.

The pre-main startup abort was resolved by preparing the writable GPU user directory before the upstream static path initializer. Keep the leading Mach-O `__PAGEZERO` segment for LiveContainer conversion. Build 8 retains the user artwork and the display name `Bloodborne`.

Build 8 device log identifies the failing instruction at guest offset `0x2088090`: the engine's `DLSegregatedBiHeap` allocator reads a null list node plus 0x10 while allocating 0x800040 bytes. The allocator's source-path assertion string and instruction were checked privately against the user's previously supplied boot image. This establishes the crash site, not the cause of the invalid list. Build 9 observes heap setup, metadata allocation and the affected arena without modifying guest registers or heap contents. No original executable bytes are committed.

Build 9 device tracing shows the affected allocator body is entirely zero at first use, while a different arena initializes correctly. The automatic above-1080p graphics heap patch changes the selected table's total from 4,478 MiB to 7,817 MiB (graphics entries 2,287/886 MiB become 3,952/2,560 MiB), exceeding this iOS app's 4,608 MiB direct pool before other allocations. Build 10 sets `BB_AUTO_GRAPHICS_HEAP=0` and makes that automatic expansion optional. Scene/output dimensions and aspect patches remain native. Allocation rejection/exhaustion is now logged explicitly. Build 10 passing this crash and native-resolution gameplay memory usage still need physical-device verification.

Build 10 device log passes the former null-heap fault: the affected arena is initialized, game services continue, and native Metal draw/dispatch counters are nonzero. A gray screen remains. A missing `menu/logo.tpf.dcx` lookup occurs before further initialization; it does not by itself establish an incomplete dump or explain the gray screen. Build 11 records approximate per-thread instruction totals/deltas, recent guest offsets, active host-call symbols and two arguments every ten seconds for three minutes. It uses atomic diagnostic snapshots, no thread suspension or signals, and leaves guest registers/memory unchanged. Sampling stops while the game continues; screenshot appearance alone is not proof of deadlock or slow loading.
