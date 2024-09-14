#include <mem_management.h>
#include <stdarg.h>
#include <stdint.h>
#include <vfs/std.h>

std_t* init_std(){
	std_t* std = malloc(sizeof(std_t));
	std->first = NULL;
	std->last = NULL;
	std->size = 0;
	return std;
}

void std_put(std_t* std, char val){
	std_elt_t* elt = malloc(sizeof(std_elt_t));
	elt->next = NULL;
	elt->val = val;

	if(std->last != NULL){
		std->last->next = elt;
	}
	std->last = elt;
	if(std->first == NULL){
		std->first = elt;
	}
	std->size++;
}

char std_get(std_t* std){
	if(std->first == NULL){
		return 0;
	}
	
	char c = std->first->val;
	std_elt_t* old = std->first;
	std->first = old->next;
	if(std->size == 1){
		std->last = NULL;
	}
	std->size--;
	free(old);
	return c;
}
