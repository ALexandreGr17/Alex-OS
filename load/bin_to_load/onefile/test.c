#include <stddef.h>
#include <syscall.h>

int strlen(char* s) {
    size_t i = 0;
    for(; s[i]; i++);
    return i;
}

int main(void) {
    char* msg = "Hello world\n";
    write(1, msg, strlen(msg));
   return 0;
}
