#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

struct process_context {
    uint32_t esp, ebp, ebx, esi, edi, eip;
};

struct process_list {
    struct process_list* next;
    struct process_context* ctx;
    uint32_t pid;
};

int __attribute__((cdecl)) context_switch(struct process_context* old, struct process_context* new, int return_value);
int exec(char* file);
void init_process_management();
void exit_process();

#endif 
