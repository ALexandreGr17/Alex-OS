#include "vfs/vfs.h"
#include <arch/i686/isr.h>
#include <process/process.h>
#include <stdio.h>

void i686_syscall_handler(Register* regs) {
    __asm__ volatile("sti");  // Réactiver les interruptions
    
    if (regs->eax == 4) {
        exit_process(regs->edi);
    }
    else if (regs->eax == 1) {
        write(regs->edi, regs->edx, (char*)regs->esi);
    }
    else {
        printf("syscall: %x\n", regs->eax);
    }
}

void init_syscall_handler() {
    i686_ISR_Registerhandler(0x80, i686_syscall_handler);
}
