# Build 22: native Metal pass scheduling and descriptor stability

Goal: 30 FPS at the existing phone display profile while preserving PS4 effects/LOD defaults. No graphics downgrade, dynamic resolution change, frame skipping, or simulation-time patch.

## Changes

Backports from [bloodborne-native/bloodborne-runtime](https://github.com/bloodborne-native/bloodborne-runtime), on pinned API-4 runtime `14dbd84263b3a08d001eb7fb1128e98f4c5f2be9`:

- [bce636d](https://github.com/bloodborne-native/bloodborne-runtime/commit/bce636dfdf8627599c3ea4cc0468936149dfcf45): track read dependencies at exact byte ranges by default. The previous 4 KiB rounding falsely conflicted with adjacent ring-buffer copies, splitting the G-buffer render pass and forcing attachment stores/reloads. The old coarse mode remains opt-in through its upstream toggle. Existing real dependencies are retained.
- [11116eb](https://github.com/bloodborne-native/bloodborne-runtime/commit/11116ebd222b3d506baf255f4cd6113fc62ac95c): keep a native Metal render pass open for barriers unrelated to graphics writes or its attachments. Graphics writes and attachment transitions still end the pass; unknown attachment identities conservatively end it. Non-native/Vulkan behavior is retained.
- [e207326](https://github.com/bloodborne-native/bloodborne-runtime/commit/e207326050a31ccd9e09d5d56bd1cc275816cbd8): map ISA-unused descriptor formats 15, 23–31, 42 and 43 to existing invalid-resource handling. Avoid the reported `Unknown data_format=15 and num_format=5` assertion without suppressing assertions for valid formats. Color targets with invalid formats are absent. This handles the invalid descriptor; it does not establish that every possible crash is fixed.

The runtime pin and personal AOT ABI remain unchanged. The upstream commits' other unrelated changes are not imported. Patch context must match or CI fails.

## Display and quality

- Native landscape output: 2868 × 1320, native aspect.
- MetalFX Performance internal scene configuration: 1434 × 660 (actual GPU alignment may be 1436 × 660).
- 30 FPS target/limit.
- PS4 effects/LOD defaults from build 21 retained, including SSR off. The one-time defaults marker and subsequent user settings are preserved. A previously saved user setting is not forcibly reset by this build.

Native output resolution does not mean full-native internal rendering. The existing MetalFX profile is unchanged.

## Verification boundaries

[Full macOS runner verification passed](https://github.com/ajellia81-cmd/bloodborne-ios-public/actions/runs/44266433945) for source `9569162bbcc32a1596e352bc6a5c8e39d8f7e368`. The first attempt exposed a fixture error (putting 6-bit encodings into a 5-bit color-target field); the corrected fixture passed without changing the GPU backports.

CI checks real GPU headers across all 64 descriptor encodings and the 32 representable color-target encodings, existing lifecycle concurrency, locale, display, PSO policy, haptics and tessellation SPIR-V validation, then compiles actual iOS Metal/runtime code and packages an unsigned IPA.

Compilation cannot establish sustained 30 FPS on a physical phone. Device testing should compare the same save, route, graphics settings and thermal conditions. Compare first traversal and repeat traversal separately; deleting shaders intentionally reintroduces first-use compilation. Remaining pipeline compilation, streaming and thermal limits may still cause stalls. No claim that every source bottleneck is eliminated.

Game data, private AOT and device shader caches are excluded from public source and Actions artifacts.
