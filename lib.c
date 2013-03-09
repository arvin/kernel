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


int size(char *ptr)
{
    //variable used to access the subsequent array elements.
    int offset = 0;
    //variable that counts the number of elements in your array
    int count = 0;

    //While loop that tests whether the end of the array has been reached
    while (*(ptr + offset) != '\0')
    {
        //increment the count variable
        ++count;
        //advance to the next element of the array
        ++offset;
    }
    //return the size of the array
    return count;
}