#!/usr/bin/env python3
"""Add terminating-fault diagnostics for the pinned interpreter (no recovery)."""
from pathlib import Path
root = Path("upstream-runtime/recomp/src/cpu")
cpu = root / "cpu.c"
text = cpu.read_text()
assert "uint64_t bbcpu_guest_rip(void)" in text
text += r'''
extern uint64_t bb_image_base __attribute__((weak));
/* iOS diagnostic snapshot: do not allocate a CPU from a signal handler. */
int bbios_interpreter_regs(BbGuestRegs *regs) {
    if (!tls_cpu) return 0;
    for (unsigned i = 0; i < 16; ++i) regs->r[i] = tls_cpu->r[i];
    regs->rip = tls_cpu->rip;
    return 1;
}
int bbios_interpreter_fault(char *out, size_t size) {
    BbGuestRegs regs;
    if (!bbios_interpreter_regs(&regs)) return 0;
    int n = snprintf(out, size,
        "  interpreter guest snapshot rip=%#llx (guest offset %#llx)\n"
        "  interpreter state may be a call continuation when faulting inside a host import\n",
        (unsigned long long)regs.rip,
        (unsigned long long)(regs.rip - (&bb_image_base ? bb_image_base : 0)));
    static const char *names[16] = {
        "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
        "r8","r9","r10","r11","r12","r13","r14","r15"
    };
    for (unsigned i = 0; i < 16 && n > 0 && (size_t)n < size; ++i) {
        int added = snprintf(out + n, size - (size_t)n, "  %s=%#llx%s",
            names[i], (unsigned long long)regs.r[i], i % 4 == 3 ? "\n" : "");
        if (added < 0) break;
        n += added;
    }
    return 1;
}
'''
cpu.write_text(text)
jit = root / "jit_arm64.c"
text = jit.read_text()
needle = "int bbcpu_fault_guest_regs(void *context, BbGuestRegs *regs) {"
assert text.count(needle) == 2
pos = text.index(needle)
text = text[:pos] + "extern int bbios_interpreter_regs(BbGuestRegs *regs);\nextern int bbios_interpreter_fault(char *out, size_t size);\n" + text[pos:]
old = "const CodeRange *range = fault_range(context);\n    if (!range) return 0;"
assert text.count(old) == 2
text = text.replace(old, "const CodeRange *range = fault_range(context);\n    if (!range) return bbios_interpreter_regs(regs);", 1)
text = text.replace(old, "const CodeRange *range = fault_range(context);\n    if (!range) return bbios_interpreter_fault(out, size);", 1)
jit.write_text(text)
print("Interpreter terminating-fault register and guest-stack reporting enabled")

# Observe allocator initialization and the failing arena without altering guest state.
probe = Path("upstream-runtime/src/probe.c")
text = probe.read_text()
marker = "static void fault(int sig, siginfo_t *info, void *context) {"
assert text.count(marker) == 1
observer = r'''
#ifdef BB_HAVE_BBCPU
static void bbios_heap_probe(const uint64_t regs[16], uint64_t address, uint32_t tag) {
    static unsigned counts[4];
    const uint64_t object = (tag == 1 || tag == 2) ? regs[3] : regs[7];
    if (tag == 3 && object != bb_image_base + UINT64_C(0x569cdf8)) return;
    const unsigned count = __atomic_fetch_add(&counts[tag], 1, __ATOMIC_RELAXED);
    if (count >= 32) return;
    uint64_t fields[24] = {0}, caller = 0;
    const size_t got = bb_read_memory(fields, object, sizeof(fields));
    (void)bb_read_memory(&caller, regs[4], sizeof(caller));
    fprintf(stderr, "Heap trace tag=%u site=%#llx object=%#llx thread=%s caller=%#llx "
        "rsi=%#llx rdx=%#llx rcx=%#llx rax=%#llx readable=%zu\n",
        tag, (unsigned long long)(address - bb_image_base),
        (unsigned long long)object, runtime_thread_name(),
        (unsigned long long)(caller - bb_image_base),
        (unsigned long long)regs[6], (unsigned long long)regs[2],
        (unsigned long long)regs[1], (unsigned long long)regs[0], got);
    for (size_t i = 0; i < got / 8; ++i)
        fprintf(stderr, "  heap[%#zx]=%#llx%s", i * 8,
            (unsigned long long)fields[i], i % 4 == 3 ? "\n" : "");
    if ((got / 8) % 4) fputc('\n', stderr);
}
static void bbios_heap_probes(void) {
    const char *enabled = getenv("BB_HEAP_TRACE");
    if (!enabled || strcmp(enabled, "1")) return;
    bbcpu_set_probe(bb_image_base + 0x2087e50, bbios_heap_probe, 0);
    bbcpu_set_probe(bb_image_base + 0x2087f83, bbios_heap_probe, 1);
    bbcpu_set_probe(bb_image_base + 0x2087f8b, bbios_heap_probe, 2);
    bbcpu_set_probe(bb_image_base + 0x2088030, bbios_heap_probe, 3);
    puts("Heap diagnostics: allocator init, metadata allocation and failing arena observed");
}
#endif

'''
text = text.replace(marker, observer + marker)
marker = "        bbcpu_set_tls_displacement(tls_displacement);"
assert text.count(marker) == 1
text = text.replace(marker, marker + "\n        bbios_heap_probes();")
probe.write_text(text)
