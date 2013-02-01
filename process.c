#include <LPC17xx.h>
#include "uart_polling.h"
#include "process.h"
#include "userproc.h"
#include "rtx.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */

ProcessQueue* readyQueue;
ProcessQueue** blockedQueues;
ProcessNode* curProcess;
ProcessNode* nullProcessNode;

int newProcessId = 0;				/* Must be unique */


void null_process(void) {
	while(1) {
		k_release_processor();
	}
}

void process_init() {
  int i;
	// Ready queue initialization
	readyQueue = (ProcessQueue*)s_requestion_memory_block();
	readyQueue->first = NULL;
	readyQueue->last = NULL;
	readyQueue->size = 0;
	
	// Blocked queues initialization
	blockedQueues = (ProcessQueue**)s_requestion_memory_block();
	for (i = 0; i < PRIORITY_COUNT; ++i) {
		blockedQueues[i] = (ProcessQueue*)s_requestion_memory_block();
		blockedQueues[i]->first = NULL;
		blockedQueues[i]->last = NULL;
		blockedQueues[i]->size = 0;
	}

	// Null process initialization
	curProcess = NULL;
	nullProcessNode = (ProcessNode*)s_requestion_memory_block();
	init_pcb(&null_process, nullProcessNode, 4);
	
	// User processes initialization
	assign_processes();
}

int set_process_priority(int process_ID, int priority) {
	ProcessNode* node = readyQueue->first;

	if(process_ID == 0){
		return -1;
	}
		
	if (priority < 0 || priority > 3)
		return -1;
	
	while(node != NULL) {
		if (node->pcb.m_pid == process_ID) {
			node->pcb.priority = priority;
			return 0;
		}
		node = node->next;
	}
	
	return -1;
}

int get_process_priority(int process_ID) {
		
	ProcessNode* node = readyQueue->first;
	
	while(node != NULL) {
		if (node->pcb.m_pid == process_ID) {
			return node->pcb.priority;
		}
		node = node->next;
	}

	return -1;
}

// Return the process that should be executed
ProcessNode* scheduler(void){
	if (curProcess == NULL) {
		// First time executing
	  curProcess = poll_process(readyQueue);
	  return curProcess;
	}
	else if (readyQueue->first != NULL) {
		return poll_process(readyQueue);
	}
	return nullProcessNode;
}

void init_pcb(void* process, ProcessNode* node, int priority) {
	int i;
	uint32_t* stackBlockStart = (uint32_t*)s_requestion_memory_block();
	node->next = NULL;
	
	// PCB initialization
	node->pcb.m_pid = newProcessId++;
	node->pcb.priority = priority;
	node->pcb.m_state = NEW;
	
	// Stack initialization
	node->pcb.mp_sp = stackBlockStart + USR_SZ_STACK;
	node->pcb.mp_sp--;
	/* 8 bytes alignment adjustment to exception stack frame */
	if (!(((uint32_t)node->pcb.mp_sp) & 0x04)) {
			node->pcb.mp_sp--;
	}
	
	*(--(node->pcb.mp_sp))  = INITIAL_xPSR;      /* user process initial xPSR */ 
	*(--(node->pcb.mp_sp))  = (uint32_t) process;  /* PC contains the entry point of the process */

	for (i = 0; i < 6; i++) { /* R0-R3, R12 are cleared with 0 */
		*(--(node->pcb.mp_sp)) = 0x0;
	}
}

// This is an API call for creating new user processes
int add_new_process(void* process) {
	return add_new_prioritized_process(process, 3);
}

// This is an internal call for creating new processes
int add_new_prioritized_process(void* process, int priority) {
	ProcessNode* node = (ProcessNode*)s_requestion_memory_block();
	
	
	if (readyQueue->first == NULL)
		readyQueue->first = node;
	else
		readyQueue->last->next = node;
	
	readyQueue->last = node;
	readyQueue->size++;
	init_pcb(process, node, priority);
	return 0;
}

// This is an API call for relasing current user process
int release_processor() {
	return release_processor_to_queue(RDY);
}

// This is an internal API call for releasing current process with a new specified state
int release_processor_to_queue(proc_state_t newState) {
	curProcess->pcb.m_state = newState;
	switch (newState) {
		case RDY:
			push_process(readyQueue, curProcess);
			break;
		case INSUFFICIENT_MEMORY:
			push_process(blockedQueues[curProcess->pcb.priority], curProcess);
			break;
	}
	manage_processor();
	return 0;
}

// Remove first process from the specified queue
ProcessNode* poll_process(ProcessQueue* queue) {
	ProcessNode* node = queue->first;
	if (node == NULL) {
		return NULL;
	}
	queue->first = node->next;
	if (queue->first == NULL) {
		queue->last = NULL;
	}
	node->next = NULL;
	queue->size--;
	return node;
}

// Add process to the back of the specified queue
void push_process(ProcessQueue* queue, ProcessNode* node) {
	if (queue->first == NULL) {
		queue->first = node;
	}
	else {
		queue->last->next = node;
	}
	queue->last = node;
	node->next = NULL;
	queue->size++;
}

// Internal call for releasing processor
int k_release_processor(void)
{
	volatile int i;
	volatile proc_state_t state;
	ProcessNode *oldProcess = NULL;
	ProcessNode *newProcess = NULL;

	// Unblock any processes if possible
	unblock_process();
	newProcess = scheduler();
	if (curProcess == NULL) {
	 return -1;
	}
	
	oldProcess = curProcess;
	curProcess = newProcess;

	state = curProcess->pcb.m_state;

	if (state == NEW) {
		if (oldProcess->pcb.m_state != NEW) {
			oldProcess->pcb.mp_sp = (uint32_t *) __get_MSP();
		}
		curProcess->pcb.m_state = RUN;
		__set_MSP((uint32_t) curProcess->pcb.mp_sp);
		__rte();  /* pop exception stack frame from the stack for a new process */
	} else if (state == RDY) {   
		oldProcess->pcb.mp_sp = (uint32_t *) __get_MSP(); /* save the old process's sp */
		curProcess->pcb.m_state = RUN;
		__set_MSP((uint32_t) curProcess->pcb.mp_sp); /* switch to the new proc's stack */		
	} else {
		curProcess = oldProcess; /* revert back to the old proc on error */
		uart1_put_string("Kernel Error: Failed to switch process.\n\rProcess ID:\n\r");
		uart1_put_hex(curProcess->pcb.m_pid);
		 return -1;
	}	 	 
	return 0;
}

// If memory is available, unblock the process with the highest priority
void unblock_process() {
	int i;
	ProcessNode* node;
	if (!hasFreeMemory())
		return;
	
	for (i = 0; i < PRIORITY_COUNT; ++i) {
		node = poll_process(blockedQueues[i]);
		if (node != NULL) {
			node->pcb.m_state = RDY;
			push_process(readyQueue, node);
			return;
		}
	}
}
