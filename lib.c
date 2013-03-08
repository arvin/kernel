#include "lib.h"
#include "rtx.h"

int string_equals(char* a, char* b){
	while(*a != '\0' && *b != '\0'){
		if(*a != *b)
			return FALSE;
		a++;
		b++;
	}
	return (*a == '\0' && *b == '\0');
}
