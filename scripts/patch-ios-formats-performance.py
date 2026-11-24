#!/usr/bin/env python3
"""GPL-2.0-or-later: compatible upstream backports on the pinned API-4 runtime.
Sources: bloodborne-native/bloodborne-runtime commits named below.
Fail closed when patch context changes; never silently skip a backport.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
runtime = root / "upstream-runtime"
for name in ("metal-single-region", "tess-state", "metal-encoder", "adaptive-time", "metal-playback", "raster-inputs", "compact-bindings", "slice-view-fast-path"):
    patch = root / "scripts" / "backports" / (name + ".patch")
    subprocess.run(["git", "-C", str(runtime), "apply", "--check", str(patch)], check=True)
    subprocess.run(["git", "-C", str(runtime), "apply", str(patch)], check=True)
    print("Applied upstream backport", name, flush=True)
