#!/usr/bin/env python3
"""Bounded, non-terminating interpreter progress telemetry; no signals or guest edits."""
from pathlib import Path
root = Path("upstream-runtime/recomp/src/cpu")
p = root / "cpu.c"
s = p.read_text()
marker = "static BbCpu *current_cpu(void) {"
assert s.count(marker) == 1
code = r'''
#include <pthread.h>
#include <dlfcn.h>
extern uint64_t bb_image_base __attribute__((weak));
typedef struct BbIosProgress {
    struct BbIosProgress *next;
    char name[32];
    uint64_t instructions, rip, target, arg0, arg1;
    uint64_t reported; /* only the reporter writes this */
} BbIosProgress;
static BbIosProgress *bbios_progress_threads;
static _Thread_local BbIosProgress *bbios_progress_this;
static pthread_once_t bbios_report_once = PTHREAD_ONCE_INIT;
static void *bbios_progress_report(void *unused) {
    (void)unused;
    for (unsigned sample = 1; sample <= 18; ++sample) {
        sleep(10);
        fprintf(stderr, "\n=== iOS progress elapsed=%us (approximate snapshots) ===\n", sample * 10);
        for (BbIosProgress *p = __atomic_load_n(&bbios_progress_threads, __ATOMIC_ACQUIRE); p; p = p->next) {
            const uint64_t count = __atomic_load_n(&p->instructions, __ATOMIC_RELAXED);
            const uint64_t rip = __atomic_load_n(&p->rip, __ATOMIC_RELAXED);
            const uint64_t target = __atomic_load_n(&p->target, __ATOMIC_RELAXED);
            const uint64_t a0 = __atomic_load_n(&p->arg0, __ATOMIC_RELAXED);
            const uint64_t a1 = __atomic_load_n(&p->arg1, __ATOMIC_RELAXED);
            Dl_info symbol = {0};
            if (target) (void)dladdr((const void *)(uintptr_t)target, &symbol);
            fprintf(stderr, "Progress thread=%s instructions=%llu delta=%llu guest_offset=%#llx "
                "host=%s target=%#llx arg0=%#llx arg1=%#llx\n",
                p->name, (unsigned long long)count, (unsigned long long)(count - p->reported),
                (unsigned long long)(rip && &bb_image_base ? rip - bb_image_base : 0),
                target ? (symbol.dli_sname ? symbol.dli_sname : "unknown") : "none",
                (unsigned long long)target, (unsigned long long)a0, (unsigned long long)a1);
            p->reported = count;
        }
    }
    fputs("iOS progress: three-minute diagnostic sampling finished; game continues\n", stderr);
    return NULL;
}
static void bbios_progress_start(void) {
    pthread_t thread;
    const int error = pthread_create(&thread, NULL, bbios_progress_report, NULL);
    if (!error) pthread_detach(thread);
    else fprintf(stderr, "iOS progress reporter could not start: %d\n", error);
}
static void bbios_progress_register(void) {
    const char *enabled = getenv("BB_IOS_PROGRESS");
    if (!enabled || strcmp(enabled, "1")) return;
    BbIosProgress *p = calloc(1, sizeof(*p));
    if (!p) return;
    snprintf(p->name, sizeof(p->name), "%s", *thread_label ? thread_label : "unnamed");
    BbIosProgress *head = __atomic_load_n(&bbios_progress_threads, __ATOMIC_ACQUIRE);
    do { p->next = head; }
    while (!__atomic_compare_exchange_n(&bbios_progress_threads, &head, p, 0,
                                        __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));
    bbios_progress_this = p;
    pthread_once(&bbios_report_once, bbios_progress_start);
}
void bbios_progress_tick(BbCpu *cpu, uint64_t target) {
    BbIosProgress *p = bbios_progress_this;
    if (!p) return;
    if (bbcpu_is_guest_code(cpu->rip))
        __atomic_store_n(&p->rip, cpu->rip, __ATOMIC_RELAXED);
    __atomic_store_n(&p->instructions, cpu->instructions, __ATOMIC_RELAXED);
    __atomic_store_n(&p->arg0, cpu->r[RDI], __ATOMIC_RELAXED);
    __atomic_store_n(&p->arg1, cpu->r[RSI], __ATOMIC_RELAXED);
    __atomic_store_n(&p->target, target, __ATOMIC_RELAXED);
}

'''
s = s.replace(marker, code + marker)
marker = "    tls_cpu = cpu;\n    return cpu;"
assert s.count(marker) == 1
s = s.replace(marker, "    tls_cpu = cpu;\n    bbios_progress_register();\n    return cpu;")
p.write_text(s)
p = root / "interp.c"
s = p.read_text()
marker = "static void call_host(BbCpu *cpu, uint64_t target, uint64_t stack_args) {"
assert s.count(marker) == 1
s = s.replace(marker, "extern void bbios_progress_tick(BbCpu *cpu, uint64_t target);\n" + marker)
marker = "    bbcpu_hostcall((const void *)target, &a);"
assert s.count(marker) == 1
s = s.replace(marker, "    bbios_progress_tick(cpu, target);\n" + marker + "\n    bbios_progress_tick(cpu, 0);")
marker = "    while (cpu->rip != stop) {"
assert s.count(marker) == 1
s = s.replace(marker, "    static _Thread_local unsigned bbios_blocks;\n" + marker +
    "\n        if ((++bbios_blocks & 1023u) == 0) bbios_progress_tick(cpu, 0);")
p.write_text(s)
print("iOS progress telemetry: 10-second samples for three minutes; interpreter and host waits")
