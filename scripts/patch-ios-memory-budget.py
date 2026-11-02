#!/usr/bin/env python3
"""Keep iOS native viewport patches within its fixed direct-memory budget."""
from pathlib import Path
p = Path("upstream-runtime/src/runtime_patches.c")
s = p.read_text()
old = "if ((long)width * height > (long)OUTPUT_W * OUTPUT_H) {"
assert s.count(old) == 1
s = s.replace(old, 'if ((long)width * height > (long)OUTPUT_W * OUTPUT_H &&\n            strcmp(env_or("BB_AUTO_GRAPHICS_HEAP", "1"), "0")) {')
p.write_text(s)
# Explain allocation failures instead of silently leaving game heaps uninitialized.
p = Path("upstream-runtime/src/runtime_memory.c")
s = p.read_text()
old = "if ((uint64_t)low >= end || size > end - (uint64_t)low) return NO_SPACE;"
new = '''if ((uint64_t)low >= end || size > end - (uint64_t)low) {
        fprintf(stderr, "Runtime: direct allocation rejected: size=%" PRIu64
                ", range=%" PRIu64 "..%" PRIu64 ", pool=%" PRIu64 "\\n",
                size, (uint64_t)low, end, (uint64_t)GAME_POOL_SIZE);
        return NO_SPACE;
    }'''
# Preserve literal C newline escape.
new = new.replace('PRIu64 "\n"', 'PRIu64 "\\n"')
assert s.count(old) == 1
s = s.replace(old, new)
old = "if (slot<0 || start > end || size > end-start) { write_unlock(); return NO_SPACE; }"
new = '''if (slot<0 || start > end || size > end-start) {
            fprintf(stderr, "Runtime: direct allocation exhausted: size=%" PRIu64
                    ", next=%" PRIu64 ", end=%" PRIu64 ", pool=%" PRIu64 "\\n",
                    size, start, end, (uint64_t)GAME_POOL_SIZE);
            write_unlock();
            return NO_SPACE;
        }'''
new = new.replace('PRIu64 "\n"', 'PRIu64 "\\n"')
assert s.count(old) == 1
s = s.replace(old, new)
p.write_text(s)
print("iOS memory budget: optional automatic graphics heap expansion; allocation failures logged")
