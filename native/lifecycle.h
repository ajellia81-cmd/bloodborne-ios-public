#pragma once
void bb_ios_lifecycle_install(void);
void bb_ios_set_background(int background);
void bb_ios_wait_foreground(void);
void bb_ios_gpu_begin(void);
void bb_ios_gpu_end(void);
void bb_ios_gpu_quiesce_begin(void);
int bb_ios_is_background(void);

extern void (*bb_ios_main_pump)(void);


int bb_ios_gpu_in_lease(void);
