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

int contains_prefix(char*a, char* pre){
	while(*pre != '\0'){
		if(*a != *pre)
			return FALSE;
		a++;
		pre++;
	}
	return TRUE;
}

void* string_copy(void* target, char* s) {
	char* t = (char*)target;
	while (*s != '\0') {
		*t = *s;
		t++;
		s++;
	}
	*t = '\0';
	return target;
}
