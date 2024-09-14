#ifndef STRING_H
#define STRING_H

#include <stdint.h>
uint32_t strlen(const char* str);
void strcpy(char* dst, const char* src);
char toupper(char c); 
char* strchr(const char* str, char c);
uint8_t strcmp(const char* str1, const char* str2);

#endif
