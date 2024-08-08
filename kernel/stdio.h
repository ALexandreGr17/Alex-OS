#pragma once
#include <stdint.h>

void clrscr();
void putc(char c);
void putc_color(char c, char color);
void puts(const char* str);
void puts_color(const char* str, char color);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);
