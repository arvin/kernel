#include "rtx.h"
#include "uart_polling.h"
#include "userproc.h"
#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */

#define true 1

int passCount = 0;
int proc4Stage = 0;

void proc1(void) {
	uart1_put_string("-- RTX Test Start --\n\r");
	uart1_put_string("Test 1: Deallocate non-existing blocks.\n\r");
	
	
	if (s_release_memory_block(0)) {
		passCount++;
		uart1_put_string("PASSED\n\r");
	}
	else
		uart1_put_string("FAILED\n\r");
	
	do
		release_processor();
	while (true);
}

void proc2(void){
	uart1_put_string("Test 2: Release processor\n\r");
	
  do
		release_processor();
	while (true);
}

void proc3(void){
	int i;
	int errorFlag = 0;
	
	passCount++;
	uart1_put_string("PASSED\n\r");
	
	uart1_put_string("Test 3: Allocate and deallocate memory 50 times.\n\r");
	
	for(i = 0; i < 50; i++){
		errorFlag |= s_release_memory_block(s_requestion_memory_block());
	}
	
	if (errorFlag)
		uart1_put_string("FAILED\n\r");
	else {
		passCount++;
		uart1_put_string("PASSED\n\r");
	}
	
	do
		release_processor();
	while (true);
	
}

void proc4(void){
	//Allocate to max and then deallocate all
	void** arr[3];
	int i = 0;
	int errorFlag = 0;
	
	uart1_put_string("Test 4 (Step 1): Allocate 95 blocks.\n\r");
	for (i = 0; i < 3; ++i)
		arr[i] = s_requestion_memory_block();
	
	i = 0;
	while(i < 92) {
		arr[i/32][i%32] = (void *)s_requestion_memory_block();
		i++;
	}
	
	proc4Stage++;
	release_processor();
	
	uart1_put_string("Test 4 (Step 2): Deallocate 95 blocks.\n\r");
	i--;
	
	while(i >= 0) {
		errorFlag |= s_release_memory_block(arr[i/32][i%32]);
		i--;
	}
	for (i = 0; i < 3; ++i)
		errorFlag |= s_release_memory_block(arr[i]);
	
	if (errorFlag)
		uart1_put_string("FAILED\n\r");
	else {
		passCount++;
		uart1_put_string("PASSED\n\r");
	}
	
	proc4Stage++;
	do
		release_processor();
	while (true);
}

void proc5(void) {
	void* block;
	block = s_requestion_memory_block();
	s_release_memory_block(block);
	uart1_put_string("Test 5: Request for memory block while all memory blocks are used.\n\r");
	if (proc4Stage == 2) {
		passCount++;
		uart1_put_string("PASSED\n\r");
	}
	else
		uart1_put_string("FAILED\n\r");
	
	do
		release_processor();
	while (true);
}

void proc6(void) {
	int result;
	
	release_processor();
	
	result = set_process_priority(1, 2);
	uart1_put_string("Test 6a: Set priority to a valid number.\n\r");
	if(result == 0) {
		passCount++;
		uart1_put_string("PASSED\n\r");
	}
	else
		uart1_put_string("FAILED\n\r");
	
	result = set_process_priority(1, 9000);
	
	uart1_put_string("Test 6b: Set priority to an invalid number.\n\r");
	if(result == -1) {
		passCount++;
		uart1_put_string("PASSED\n\r");
	}
	else
		uart1_put_string("FAILED\n\r");
	
	uart1_put_string("Test 6c: Setting priority of null process should be prohibited.\n\r");
	set_process_priority(0, 1);
	if(result == -1) {
		passCount++;
		uart1_put_string("PASSED\n\r");
	}
	else
		uart1_put_string("FAILED\n\r");
	
	uart1_put_string("Total number of pass count:\n\r");
	uart1_put_int(passCount);
	uart1_put_string(" out of 8 passed in total.\n\r");
	uart1_put_string("-- RTX Test End --\n\r");
	do
		release_processor();
	while (true);
}

int ProcessCount = 6;
void* ProcessTable[] = {
	&proc1,
	&proc2,
	&proc3,
	&proc4,
	&proc5,
	&proc6
};
