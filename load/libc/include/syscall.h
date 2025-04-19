#ifndef SYSCALL_H
#define SYSCALL_H

#include <stddef.h>

size_t __attribute__((cdecl)) read(int fd, void *out, size_t size);
size_t __attribute__((cdecl)) write(int fd, void *out, size_t size);
int __attribute__((cdecl)) open(char* filename);
int __attribute__((cdecl)) close(int fd);
void __attribute__((cdecl)) exit(int exit_code);

#endif
