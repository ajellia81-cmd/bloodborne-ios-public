# Performance source audit: Build20

Graphics configuration stays unchanged. This review is not proof that every source bottleneck has been eliminated.

## Confirmed synchronization defect

`native/lifecycle.c` holds a single global mutex for each entire GPU lease. `scripts/patch-ios-lifecycle.py` puts the whole Metal presentation transaction inside that lease, including `nextDrawable`. Other recording threads need the same mutex to commit work. Consequently a drawable wait prevents unrelated foreground submissions. The new concurrency regression test fails on Build20 and passes with counted concurrent foreground leases. Background closes admission and waits for every active lease before draining. Nested leases and the old pause/resume regression test also pass. The shared NSMutableSet queue registry now has its own short mutex, because concurrent foreground leases no longer serialize its writes. No shader, AOT or graphics settings changed.

## Checks and remaining questions

- Runtime recording worker selection and frame-ahead admission match Build20's settings (two each); they are bounded, not an unlimited queue.
- The presentation path selects native Metal and returns before the Vulkan presentation path; reading the two branches does not establish duplicate rendering or duplicate frame limiting.
- Original game pacing below 30 FPS remains coupled to simulation slowdown. Changing its time step is not a safe substitute for performance work.
- `CommitOwned` commits command buffers needed to carry timeline signals/fences even when empty. The log's zero-command lines are not by themselves proof of useless work; deleting them could break synchronization.
- PSO persistence is disabled. The latest device session has no writes, normal thermal state and small recorded PSO/GC times; these cannot explain all persistent slow intervals. PSO timers do not cover every shader/library/data-loading cost.
- AOT logs show translator fallback remains, but counts are not CPU-time measurements; regenerating all AOT is not justified by those counts alone.
- Resource retention and scratch dependency barrier from Build20 stay intact because the owner confirmed rendering/capture stability. Removing those protections to chase FPS is not supported.

## Validation

Cloud C11 pthread build with warnings as errors: new parallel/nested/admission/drain/resume regression passes; existing lifecycle gate regression passes. Build20 baseline aborts at the bounded parallel-entry assertion, demonstrating the serialization difference. Objective-C/Metal SDK compilation, Thread Sanitizer, real-device frame timing and screenshot/background stability are still unverified for this change.

## Next measurement

Measure drawable acquisition, admission/lease wait, GPU completion duration and frame resource waits separately with bounded aggregate logging. Reproduce the same location/camera on device before changing resolution. GPU durations can overlap, so summed command durations must not be reported as a single frame's GPU time. Keep the graphics options unchanged until the remaining measured bottleneck is identified.
