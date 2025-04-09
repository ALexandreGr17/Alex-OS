#include "process.h"

#include "memory/memory.h"

#include <ELF/ELF.h>
#include <stdio.h>

struct process_context* kernel_ctx = NULL;
struct process_list* processes = NULL;

uint32_t current_pid = 0;

void init_process_management() {
    kernel_ctx = malloc(sizeof(struct process_context));
    processes = calloc(sizeof(struct process_list), 1);
    processes->ctx = kernel_ctx;
    processes->next = NULL;
}

uint32_t add_process(struct process_context* ctx) {
    struct process_list* tmp = processes;
    uint32_t pid = 1;
    while(tmp->next != NULL) {
        tmp = tmp->next;
        if (pid == tmp->pid) {
            pid++;
        }
    }
    tmp->next = malloc(sizeof(struct process_list));
    tmp->next->ctx = ctx;
    tmp->next->pid = pid;
    return pid;
}

void remove_process(uint32_t pid) {
    struct process_list* tmp = processes;
    uint32_t i = 0;
    while(tmp->next != NULL && tmp->next->pid < pid) {
        tmp = tmp->next;
    }
    if (tmp->next == NULL) {
        return;
    }
    struct process_list* process = tmp->next;
    tmp->next = process->next;
    free(process->ctx);
    free(process);
}

struct process_context* get_ctx(uint32_t pid) {
    struct process_list* tmp = processes;
    while(tmp != NULL && tmp->pid < pid) {
        tmp = tmp->next;
    }
    if (tmp == NULL) {
        return NULL;
    }
    return tmp->ctx;
}

int exec(char* file) {
    void* entry_point = NULL;
    if (load_elf_file(file, &entry_point) < 0) {
        return -1;
    }

    struct process_context* new_ctx = calloc(sizeof(struct process_context*), 1);
    uint32_t pid = add_process(new_ctx);
    void* stack_top = calloc(1, 4096);
    uint32_t* stack = (uint32_t*)stack_top;
    *(--stack) = (uint32_t) entry_point;
    new_ctx->esp = (uint32_t)stack;

    current_pid = pid;
    int return_val = context_switch(kernel_ctx, new_ctx, 0);

    printf("return val: %d\n", return_val);
    remove_process(pid);
    free(stack_top);
    if (unload_elf_file(file)) {
        return -1;
    }
    return return_val;
}

void exit_process() {
    uint32_t val = 0;
    __asm__ volatile ("mov %%edi, %0" : "=r" (val) : : );
    context_switch(get_ctx(current_pid), get_ctx(0), val);
}
