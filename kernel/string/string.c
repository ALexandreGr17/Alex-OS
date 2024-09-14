#include <stdint.h>
#include <string/string.h>

#define NULL 0

uint32_t strlen(const char* str){
	uint32_t size = 0;
	for(;str[size];size++);
	return size;
}

void strcpy(char* dst, const char* src){
	for(int i = 0; src[i]; i++){
		dst[i] = src[i];
	}
}

char toupper(char c){
	if(c >= 'a' && c <= 'z'){
		return  c - 'a' + 'A';
	}
	return c;
}

char* strchr(const char* str, char c){
	while (*str && *str != c) {
		str++;
	}

	return *str == 0 ? NULL : str;
}

uint8_t strcmp(const char* str1, const char* str2){
	int i = 0;
	for(i = 0; str1[i] && str2[i]; i++){
		if(str1[i] != str2[i]){
			return 0;
		}
	}

	return !((str1[i] == 0) ^ (str2[i] == 0));
}
