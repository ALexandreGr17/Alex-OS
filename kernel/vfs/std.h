#ifndef STD_H
#define STD_H

#include <stdint.h>

typedef struct std_elt_s std_elt_t;

struct std_elt_s {
	char val;
	std_elt_t* next;
};

typedef struct {
	std_elt_t* first;
	std_elt_t* last;
	uint32_t   size;
} std_t;


std_t* init_std();
void std_put(std_t* std, char val);
char std_get(std_t* std);

#endif
