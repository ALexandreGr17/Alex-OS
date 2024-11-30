#include <stdint.h>
#include <stddef.h>

extern uint32_t __end;

static void * get_new_page(size_t size){
	return __end + size;
}
