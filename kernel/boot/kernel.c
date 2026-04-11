void puts(char* s) {
    volatile char* buffer = (char*)0xb8000;
    int i = 0;
    while(*s) {
        buffer[i] = *s;
        buffer[i+1]= 0x0F;
        i += 2;
        s++;
    }
}

int main(void) {
    puts("Hello fom x86_64 c");
    return 0;
}
