#include "boot/arch/io.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define VGA_CRT_ADDRESS_PORT 0x3D4
#define VGA_CRT_DATA_PORT 0x3D5

static const unsigned  SCREEN_WIDTH = 80;
static const unsigned SCREEN_HEIGHT = 25;
static const uint8_t DEFAULT_COLOR = 0x7;

static uint8_t* __attribute__((section(".bootstrap_data"))) g_ScreenBuffer = (uint8_t*)0xB8000;
static int __attribute__((section(".bootstrap_bss"))) g_ScreenX = 0, g_ScreenY = 0;

static void __attribute__((section(".bootstrap"))) putchr(int x, int y, char c)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

static void __attribute__((section(".bootstrap"))) putcolor(int x, int y, uint8_t color)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

static char __attribute__((section(".bootstrap"))) getchr(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)];
}

static uint8_t __attribute__((section(".bootstrap"))) getcolor(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1];
}

void __attribute__((section(".bootstrap"))) enable_cursor() {
    bootstrap_io_out8(VGA_CRT_ADDRESS_PORT, 0x0A);
    bootstrap_io_out8(VGA_CRT_DATA_PORT, bootstrap_io_in8(VGA_CRT_DATA_PORT) & 0xC0);

    bootstrap_io_out8(VGA_CRT_ADDRESS_PORT, 0x0B);
    bootstrap_io_out8(VGA_CRT_DATA_PORT, (bootstrap_io_in8(VGA_CRT_DATA_PORT) & 0xD0) | SCREEN_HEIGHT);
}

static void __attribute__((section(".bootstrap"))) setcursor(int x, int y)
{
    int pos = y * SCREEN_WIDTH + x;

    bootstrap_io_out8(VGA_CRT_ADDRESS_PORT, 0x0F);  // 0xf -> cursor location low
    bootstrap_io_out8(VGA_CRT_DATA_PORT, (uint8_t)(pos & 0xFF)); // first Byte of location
    bootstrap_io_out8(VGA_CRT_ADDRESS_PORT, 0x0E); // 0xe -> cursor location high
    bootstrap_io_out8(VGA_CRT_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF)); // second Byte ogf location
}

void __attribute__((section(".bootstrap"))) bootstrap_clrscr()
{
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }

    g_ScreenX = 0;
    g_ScreenY = 0;
    setcursor(g_ScreenX, g_ScreenY);
}

static void __attribute__((section(".bootstrap"))) scrollback(int lines)
{
    for (int y = lines; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }

    for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }

    g_ScreenY -= lines;
}

static void __attribute__((section(".bootstrap"))) putc(char c)
{
    bootstrap_io_out8(0xE9, c);
    switch (c)
    {
        case '\n':
            g_ScreenX = 0;
            g_ScreenY++;
            break;
    
        case '\t':
            for (int i = 0; i < 4 - (g_ScreenX % 4); i++)
                putc(' ');
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        default:
            putchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;
    }

    if (g_ScreenX >= SCREEN_WIDTH)
    {
        g_ScreenY++;
        g_ScreenX = 0;
    }
    if (g_ScreenY >= SCREEN_HEIGHT)
        scrollback(1);

    setcursor(g_ScreenX, g_ScreenY);
}

// static void putc_color(char c, char color)
// {
//     switch (c)
//     {
//         case '\n':
//             g_ScreenX = 0;
//             g_ScreenY++;
//             break;
//
//         case '\t':
//             for (int i = 0; i < 4 - (g_ScreenX % 4); i++)
//                 putc(' ');
//             break;
//
//         case '\r':
//             g_ScreenX = 0;
//             break;
//
//         default:
//             putcolor(g_ScreenX, g_ScreenY, color);
//             putchr(g_ScreenX, g_ScreenY, c);
//             g_ScreenX++;
//             break;
//     }
//
//     if (g_ScreenX >= SCREEN_WIDTH)
//     {
//         g_ScreenY++;
//         g_ScreenX = 0;
//     }
//     if (g_ScreenY >= SCREEN_HEIGHT)
//         scrollback(1);
//
//     setcursor(g_ScreenX, g_ScreenY);
// }
//
static void __attribute__((section(".bootstrap"))) puts(const char* str)
{
    while(*str)
    {
        putc(*str);
        str++;
    }
}

// void puts_color(const char* str, char color){
//     while (*str) {
//         putc(*str);
//     }
// }

static void __attribute__((section(".bootstrap"))) reverse_buffer(char* buffer, int buffer_len) {
    for (int i = 0; i < buffer_len / 2; i++) {
        char tmp = buffer[i];
        buffer[i] = buffer[buffer_len - i - 1];
        buffer[buffer_len - i - 1] = tmp;
    }
}

static void __attribute__((section(".bootstrap"))) __log_check_fmt(char** fmt, va_list args) {
    switch (**fmt) {
        case 's': {
            puts(va_arg(args, char*));
            (*fmt)++;
        } break;

        case 'd':
        case 'i': {
            char buffer[21] = { 0 };
            int size = 0;
            uint64_t n = va_arg(args, uint64_t);

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
            uint64_t n = va_arg(args, uint64_t);

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

void __attribute__((section(".bootstrap"))) bootstrap_logf(char* fmt, ...) {

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
