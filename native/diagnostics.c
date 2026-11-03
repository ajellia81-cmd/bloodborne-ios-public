/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static int diagnostic_fd = -1;
static volatile sig_atomic_t entered;
static void raw(const char *s) { if(diagnostic_fd>=0) write(diagnostic_fd,s,strlen(s)); }
void bb_diagnostic_mark(const char *s) {
    raw(s); raw("\n"); if(diagnostic_fd>=0) fsync(diagnostic_fd);
}
static void caught(int sig, siginfo_t *info, void *context) {
    (void)context;
    const char hex[]="0123456789abcdef";
    char msg[]="fatal signal=00 address=0x0000000000000000\n";
    msg[13]='0'+sig/10; msg[14]='0'+sig%10;
    uintptr_t a=(uintptr_t)info->si_addr;
    for(int i=0;i<16;i++) msg[25+i]=hex[(a >> ((15-i)*4))&15];
    if(diagnostic_fd>=0) { write(diagnostic_fd,msg,sizeof(msg)-1); fsync(diagnostic_fd); }
    signal(sig,SIG_DFL); kill(getpid(),sig);
}
/* Linked before runtime archives so this logs before their C++ constructors. */
__attribute__((constructor(101))) void bb_diagnostic_init(void) {
    if(entered) return; entered=1;
    const char *home=getenv("HOME");
    if(!home) return;
    char path[4096];
    snprintf(path,sizeof(path),"%s/Documents",home); mkdir(path,0700);
    snprintf(path,sizeof(path),"%s/Documents/Bloodborne",home); mkdir(path,0700);
    snprintf(path,sizeof(path),"%s/Documents/Bloodborne/startup.log",home);
    diagnostic_fd=open(path,O_WRONLY|O_CREAT|O_APPEND,0600);
    bb_diagnostic_mark("\n=== Bloodborne build 14 startup: early loader initializer entered ===");
    struct sigaction action; memset(&action,0,sizeof(action));
    sigemptyset(&action.sa_mask); action.sa_sigaction=caught; action.sa_flags=SA_SIGINFO;
    const int signals[]={SIGABRT,SIGSEGV,SIGBUS,SIGILL,SIGFPE};
    for(unsigned i=0;i<sizeof(signals)/sizeof(signals[0]);i++) sigaction(signals[i],&action,NULL);
    bb_diagnostic_mark("early loader initializer: signal logging ready");
    /* GPU UserPaths is constructed before main. Its parent must already exist. */
    snprintf(path,sizeof(path),"%s/Documents/Bloodborne/user",home);
    if (mkdir(path,0700) != 0 && access(path,W_OK) != 0) {
        bb_diagnostic_mark("early GPU user directory is not writable");
        return;
    }
    setenv("BB_GPU_USER_DIR",path,1);
    setenv("BB_USER_DIR",path,1);
    bb_diagnostic_mark("GPU user directory prepared before static initializers");
}

