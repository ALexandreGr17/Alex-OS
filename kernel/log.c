#include "io.h"
#include <stdint.h>
#include <stdarg.h>


#define VGA_CRT_ADDRESS_PORT 0x3D4
#define VGA_CRT_DATA_PORT 0x3D5

#define BUFFER_WIDTH 80
#define BUFFER_HEIGHT 25
#define DEFAULT_COLOR 0x7


static volatile char* vga_buffer = (char*)0xb8000;
static uint8_t x, y;

#define PUTCHR(c_x, c_y, c) vga_buffer[2 * (c_y * BUFFER_WIDTH + c_x)] = c
#define PUTCOLOR(c_x, c_y, c) vga_buffer[2 * (c_y * BUFFER_WIDTH + c_x) + 1] = c

void putchr(char c) {
    PUTCHR(x, y, c);
}

void putcolor(char color) {
    PUTCOLOR(x, y, color);
}

void enable_cursor() {
    io_out8(VGA_CRT_ADDRESS_PORT, 0x0A);
    io_out8(VGA_CRT_DATA_PORT, io_in8(VGA_CRT_DATA_PORT) & 0xC0);

    io_out8(VGA_CRT_ADDRESS_PORT, 0x0B);
    io_out8(VGA_CRT_DATA_PORT, (io_in8(VGA_CRT_DATA_PORT) & 0xD0) | BUFFER_HEIGHT);
}

void setcursor() {
    int pos = y * BUFFER_WIDTH + x;

    io_out8(VGA_CRT_ADDRESS_PORT, 0x0F);
    io_out8(VGA_CRT_DATA_PORT, pos & 0xFF);
    io_out8(VGA_CRT_ADDRESS_PORT, 0x0E);
    io_out8(VGA_CRT_DATA_PORT, (pos >> 8) & 0xFF);
}

void clrscr() {
    for (int i = 0; i < BUFFER_WIDTH; i++) {
        for (int j = 0; j < BUFFER_HEIGHT; j++) {
            PUTCHR(i, j, '\0');
            PUTCOLOR(i, j, DEFAULT_COLOR);
        }
    }

    x = 0;
    y = 0;

    setcursor();
}

void putc(char c) {
    switch (c) {
        case '\n':{
            x = 0;
            y++;
        } break;
        case '\t':{
            for (int i = 0; i < 4; i++) {
                putc(' ');
            }
        } break;
        default: {
            putchr(c);
            x++;
        } break;
    }

    if (x >= BUFFER_WIDTH) {
        y++;
        x = 0;
    }
    if (y >= BUFFER_HEIGHT) {
        y = BUFFER_HEIGHT;
    }

    setcursor();
}

void puts(char* s) {
    while (*s) {
        putc(*s);
        s++;
    }
}

static void reverse_buffer(char* buffer, int buffer_len) {
    for (int i = 0; i < buffer_len / 2; i++) {
        char tmp = buffer[i];
        buffer[i] = buffer[buffer_len - i - 1];
        buffer[buffer_len - i - 1] = tmp;
    }
}

static void __log_check_fmt(char** fmt, va_list args) {
    switch (**fmt) {
        case 's': {
            puts(va_arg(args, char*));
            (*fmt)++;
        } break;

        case 'i': {
            char buffer[21] = { 0 };
            int size = 0;
            int n = va_arg(args, int);

            do {
                buffer[size] = (n % 10) + '0';
                n /= 10;
                size++;
            } while(n > 0);

            reverse_buffer(buffer, size);
            puts(buffer);
            (*fmt)++;
        } break;

        case 'x': {
            char buffer[21] = { 0 };
            int size = 0;
            int n = va_arg(args, int);

            do {
                int m = (n % 16);
                if (m < 10) {
                    buffer[size] = m + '0';
                }
                else {
                    buffer[size] = (m - 10) + 'A';
                }
                n /= 16;
                size++;
            } while(n > 0);

            reverse_buffer(buffer, size);
            puts(buffer);
            (*fmt)++;
        } break;

        case 'c': {
            putc(va_arg(args, int));
            (*fmt)++;
        } break;

        default: {
            putc('%');
        } break;
    }
}

void logf(char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char* old_fmt = fmt;

    while (*fmt) {
        if (*fmt == '%') {
            *fmt = '\0';
            puts(old_fmt);
            *fmt = '%';
            fmt++;
            __log_check_fmt(&fmt, args);

            old_fmt = fmt;

        }
        else {
            fmt++;
        }
    }

    puts(old_fmt);
}
