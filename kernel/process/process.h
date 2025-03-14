#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

struct process_context {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eflags;
    uint32_t es;
    uint32_t ds;
    uint32_t cs;
    uint32_t ss;
    uint32_t eip;
} __attribute__((packed)) ;

void __attribute__((cdecl)) i686_save_context(struct process_context* ctx);
void __attribute__((cdecl)) i686_load_context(struct process_context* ctx);

void load_ctx();
void save_ctx();

#endif 
