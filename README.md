# Bloodborne iOS experimental runtime

## Build 17 — pipeline cache candidate, device verification pending

- Build 16 device feedback confirmed Korean Default works. The latest device log measured individual Metal pipeline creations up to 262.22 ms, versus texture collection calls up to 1.82 ms. First-use pipeline creation is a confirmed source of long blocking work; texture uploads and other stages may also contribute.
- Cache individual Metal pipeline descriptors in device-local binary archive shards. Read one shard at a time and release the archive reference after the lookup; the monolithic archive stays disabled. Oversized or invalid shards fall back to normal creation. GPU/OS cache namespaces, a loading marker and failed-entry removal limit reuse of invalid data.
- Serialized entry limit: 1 MiB. Per-namespace disk budget: 128 MiB / 2048 entries. At most 64 queued descriptor snapshots and one writer (the device log showed bursts of 37 new creations). Cache IO is skipped while the application is inactive or below 512 MiB reported allocation headroom. These are file/queue limits, not a verified cap on the Metal driver's resident memory.
- Write descriptor snapshots on a utility worker after a two-second quiet interval, with no foreground pipeline creation in progress at the time the writer takes a job. A new foreground creation can begin while a write is already in progress.
- Coalesce concurrent creation of the same render, compute, tessellation or geometry map key. Recheck the original exact-key map after waiting; unrelated keys proceed independently. Do not skip draws or substitute a mismatched pipeline.
- Newly encountered pipelines can still compile synchronously. Improvements are expected primarily for cached pipelines across launches and duplicate concurrent requests. No claim of eliminating all first-visit stutter or of verified device memory behavior.
- Device-created shader caches and personal AOT code are not included in this public repository or Actions artifacts. Korean selection, output resolution, input and lifecycle fixes continue from build 16.


## Build 16 — compiled successfully; device verification pending

- Default Korean now corrects the game's regional mapping for Korean index 19 after a complete resolver layout hash check. It preserves the other 20 mappings and covers inlined resolver copies without modifying the personal AOT library.
- Discover nonempty message archives under case-insensitive `kor*` directories. Alias only game locale `korkr` to the detected directory (including `korko`); English/French/other selected locale paths are unchanged. Actual message opens remain logged.
- Spread noncritical texture collection across submissions (8 ms minimum interval, 2 ms candidate-scan budget). Individual frees may exceed that budget. Critical memory collection keeps the existing emergency passes; budgets and protected GPU-only image contents are unchanged.
- Log collector time and Metal pipeline creation time separately as `iOS streaming GC` and `iOS streaming PSO`. This does not eliminate first-use pipeline compilation; Metal binary archives stay disabled because of prior memory failures.
- macOS Actions build [41657669932](https://github.com/ajellia81-cmd/bloodborne-ios-public/actions/runs/41657669932) passed for code commit `c129a5cbd5f2ddbe421787bc99a8f454ec29952b`. Native unsigned IPA packaging, ARM64/iOS platform checks, Mach-O segment sizes, preserved guard segment, required locale/streaming symbols and ZIP integrity were verified. The personal AOT library was combined privately and was not uploaded to this repository or Actions.
- Host fixtures passed: directory casing/alias, empty resource detection, preservation of every other mapping, and invalid-layout fallback. A private test against the supplied executable also passed its resolver hash guard and data-only replacement; no game bytes are included here or uploaded to CI. Korean display, frame pacing, and memory behavior still require device validation.


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
| Game rendering and gameplay | Former heap fault passed; character creation reached; owner reports exit during world loading (build 13) |
| Korean rendering, DLC, frame rate | Not verified |

The earlier compiler failure in macos-on-ios used the runner's default Xcode 16.4 / SDK 18.5, which lacks Metal 4 declarations. The workflow explicitly selects Xcode 26.3. The upstream iOS core script also needs modern Bash to handle its empty GPU-stub array under nounset; the workflow uses Homebrew Bash.

## Native display and aspect ratio
The shell queries the fullscreen high-density drawable in **pixels**, normalizes it to landscape, for native output. Build 15 uses a half-width/half-height internal scene with MetalFX Performance to address confirmed GPU out-of-memory errors. It writes `aspect_ratio=auto`, `live_resolution=0` and the measured `output_res`, while preserving other settings.

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

The owner reports build 11 starts the game after a long wait and runs slowly. Its log shows ongoing main/render/resource/audio instruction progress rather than a process-wide deadlock. This app still has no personalized AOT library: the interpreter remains a major performance limitation, and GPU submit/draw counts are not gameplay FPS. Build 12 initializes and refreshes touch safe-area geometry on the window thread before input, adds direct finger input and a large Close button to the settings menu, bundles pinned OFL Noto Sans KR, and removes periodic interpreter/heap instrumentation. UI behavior and performance gains still require device verification. Native scene/output dimensions remain 2868x1320 on the owner's device.


Build 13 maps the physical controller Guide/Home event to toggle settings; L3+R3 remains available to the game. SDL must deliver the Guide event on the connected controller; physical-device verification is pending. Touch Settings remains available.

The launcher now detects `Frameworks/librecomp.dylib` inside a personal app and sets `BB_RECOMP_LIB` before runtime startup. Without it, the public IPA remains interpreter-only. The runtime checks the library API and per-function hashes before registering translated functions; presence alone does not prove successful activation. The cloud AOT toolchain workflows contain only compiler/dependency sources and binaries. Personal game-code generation, compilation and IPA injection occur outside public GitHub Actions. Native display dimensions and aspect are preserved. Loading time/FPS improvements require a device comparison after successful private AOT compilation; none are claimed from compilation alone.


Build 13 public Actions run `41577577514` completed successfully. A separate personal variant was compiled privately from the owner's previously supplied executable: 241,821 functions, 384 objects, LLVM 18.1.8, `-O2`, ARM64 iOS 26.0 and recompiler API 4. The personal library and personalized IPA are not published in this repository or Actions. Its platform, segment sizes, unsigned state, exported initialization symbol and IPA ZIP integrity were checked. The base Xcode-linked app already imports the same three 128-bit arithmetic helpers from libSystem, confirming the SDK-less library's required system imports. This is structural/build validation, not a physical-device loading-time or FPS measurement. Native output dimensions/aspect are unchanged. The public IPA still uses interpreter fallback unless a personal library is bundled.



## Build 14: lifecycle and world-loading stability
The owner's build 13 log confirms 241,004 of 241,031 library functions activated. It includes a GPU-watchdog stall in one session and, in the last session, texture-memory pressure and a 10-second Metal archive write before logging stops. The last session does not include a fatal signal or a jetsam report: the termination cause is not established.

Build 14 closes a cooperative guest-execution gate on UIKit inactivity, pauses/clears SDL audio streams and resets their pacing on return. Foreground leases serialize Metal submissions and cover drawable acquisition through presentation; existing queues are drained with a bounded wait before background. The UI thread can continue pumping foreground notifications if startup encounters the gate. Foreground guest checks avoid thread-local lookup. The renderer's binary archive is disabled, following the upstream iOS memory guidance, without changing scene/output dimensions. Metal completion errors, memory allowance and lifecycle transitions are logged. Controller Guide and the existing Korean UI font remain.

Cloud checks: repeated guest/GPU pause/resume, nested submission leases and in-flight drain ordering pass; the UIKit/Metal Objective-C bridge compiled against the actual iPhoneOS SDK (run 41616770350). Full IPA validation is recorded after the main build. Device background/resume and character-to-world entry still require testing; these changes are not a claim that the reported exit has been reproduced or cured in cloud.

Korean remains the requested system default (`BB_LANGUAGE=9`). Expected Korean message directories are checked at launch and the result is logged. The European game's own language menu and missing translation assets cannot be supplied by changing the system request or adding the launcher's CJK font. Actual Korean game text remains unverified. Game assets and the personalized AOT library stay outside the public repository and Actions.

Build 14 final CI run `41618475369` (code revision `aeb263fe40487070ca4fa8a0bf071ba7d020cd03`) passed. The private package passed ARM64/iOS 26 platform checks, PAGEZERO preservation, segment bounds, absence of signature/chained-fixup commands, lifecycle symbol checks and ZIP integrity. Its API-4 AOT library is byte-identical to the previously activated personal library. Device background/resume, world-entry stability and actual Korean game text remain unverified for build 14.



## Build 15: confirmed GPU memory failure and korko selection
The build 14 device log reaches the first cutscene, then records a UIKit memory warning, repeated Metal command-buffer Code 8 (`kIOGPUCommandBufferCallbackErrorOutOfMemory`), and driver-reported usage above the critical texture-GC mark. This establishes GPU allocation failures before the reported exit; the exact process-termination mechanism is not captured.

Output remains native 2868x1320 and fullscreen, with the same aspect ratio. To reduce resolution-dependent render-target memory, the default scene is now 1434x660, enlarged by MetalFX Performance. This deliberately trades internal rendering resolution for headroom; it is not native-resolution scene rendering. Other devices use the equivalent even-sized half-resolution profile, or unscaled rendering on small displays. The binary archive remains off. Physical-device stability and resulting image quality still need verification.

The owner identified the real Korean folder as `game/dvdroot_ps4/msg/korko`. The previous checker missed that exact name; its miss was not evidence that Korean assets were absent. Build 15 checks readable nonempty message archives there and logs actual message-file opens separately.

Per the owner's requested semantics, only the game's Default/system language is Korean (`BB_LANGUAGE=9`). Explicitly selected languages continue through the game's normal path. There is no global message/font path rewrite. System-language queries and actual Korean/other message-file opens are logged so this can be verified on device. Actual Korean rendering remains unverified.

Cloud fixture tests distinguish an empty locale folder from readable message archives and verify that no global locale override is enabled. Display tests cover native output, a half-size internal scene, MetalFX configuration, portrait normalization and small-display fallback.

Build 15 final CI run `41627006284`, code revision `a706050882cc263518e7513149a23031c87db6c8`, passed. The strict macOS fixture's mkdtemp declaration was fixed with the Darwin feature macro. The downloaded display test confirms 2868x1320 output with a 1434x660 MetalFX scene. The private unsigned package passed ARM64/iOS platform checks, segment bounds, PAGEZERO/signature/fixup checks, preserved AOT identity and ZIP integrity. The locale diagnostic symbols are present and the removed global-remap symbol is absent. Cutscene/world-entry stability and actual Default-language Korean rendering are still awaiting device verification.


