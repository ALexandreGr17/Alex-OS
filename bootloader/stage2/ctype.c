#include "ctype.h"

bool islower(char c){
	return c >= 'a' && c <= 'z';
}

char toupper(char chr)
{
    return islower(chr) ? (chr - 'a' + 'A') : chr;
}
