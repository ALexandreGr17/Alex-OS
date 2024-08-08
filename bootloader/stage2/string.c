#include "string.h"
#define NULL 0

const char* strchr(const char* str, char c){
	if(str == NULL){
		return NULL;
	}

	while(*str){
		if(*str == c){
			return str;
		}
		str++;
	}
	return NULL;
}

char* strcpy(char* dst, const char* src){
	if(!src || !dst){
		return NULL;
	}

	int i = 0;
	for(; src[i]; i++){
		dst[i] = src[i];
	}
	dst[i] = 0;
	return dst;
}

unsigned strlen(const char* str){
	unsigned size = 0;
	for(;str[size];size++);
	return size;
}
