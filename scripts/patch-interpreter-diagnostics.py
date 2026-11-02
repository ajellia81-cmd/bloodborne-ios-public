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
