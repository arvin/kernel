#include "rtx.h"
#include "uart_polling.h"
#include "userproc.h"
#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */

#define true 1

void proc1(void) {
	int errorFlag = 1;
	uart1_put_string("Test 1: Deallocate non-existing blocks.\n\r");
	
	if (s_release_memory_block(0))
		uart1_put_string("PASSED\n\r");
	else
		uart1_put_string("FAILED\n\r");
	
	do
		release_processor();
	while (true);
}

void proc2(void){
	uart1_put_string("Test 2: Release processor (passed if gets to test 3)\n\r");
	
  do
		release_processor();
	while (true);
}

void proc3(void){
	int i = 0;
	int errorFlag = 0;
	
	uart1_put_string("Test 3: Allocate and deallocate memory 50 times.\n\r");
	
	for(i; i < 50; i++){
		errorFlag |= s_release_memory_block(s_requestion_memory_block());
	}
	
	if (errorFlag)	
		uart1_put_string("FAILED\n\r");
	else
		uart1_put_string("PASSED\n\r");
	
	do
		release_processor();
	while (true);
	
}

void proc4(void){
	//Allocate to max and then deallocate all
	void** arr[3];
	int i = 0;
	int errorFlag = 0;
	
	uart1_put_string("Test 4a: Allocate 95 blocks.\n\r");
	for (i = 0; i < 3; ++i)
		arr[i] = s_requestion_memory_block();
	
	i = 0;
	while(i < 92) {
		arr[i/32][i%32] = (void *)s_requestion_memory_block();
		i++;
	}
	
	release_processor();
	
	uart1_put_string("Test 4b: Deallocate 94 blocks.\n\r");
	i--;
	
	for(i; i >=0; i--) {
		errorFlag |= s_release_memory_block(arr[i/32][i%32]);
	}
	for (i = 0; i < 3; ++i)
		errorFlag |= s_release_memory_block(arr[i]);
	
	if (errorFlag)
		uart1_put_string("FAILED\n\r");
	else
		uart1_put_string("PASSED\n\r");
	
	do
		release_processor();
	while (true);
}

void proc5(void) {
	void* block;
	uart1_put_string("Test 5: Request for memory block while all memory blocks are used.\n\r");
	block = s_requestion_memory_block();
	uart1_put_string("This message should be printed only if blocks within test 4 are deallocated.\n\r");
	
	do
		release_processor();
	while (true);
}

void proc6(void) {
	int result = set_process_priority(1, 2);
	uart1_put_string("Test 6a: Set priority to a valid number.\n\r");
	if(result == 0)
		uart1_put_string("PASSED\n\r");
	else
		uart1_put_string("FAILED\n\r");
	
	result = set_process_priority(1, 9000);
	
	uart1_put_string("Test 6b: Set priority to an invalid number.\n\r");
	if(result == -1)
		uart1_put_string("PASSED.\n\r");
	else
		uart1_put_string("FAILED.\n\r");
	
	uart1_put_string("Test 6c: Setting priority of null process should be prohibited.\n\r");
	set_process_priority(0, 1);
	if(result == -1)
		uart1_put_string("PASSED\n\r");
	else
		uart1_put_string("FAILED\n\r");
	
	do
		release_processor();
	while (true);
}


void assign_processes() {
  add_new_process(&proc1);
	add_new_process(&proc2);
	add_new_process(&proc3);
	add_new_process(&proc4);
	add_new_process(&proc5);
	add_new_process(&proc6);
}