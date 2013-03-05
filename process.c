#include <LPC17xx.h>
#include "uart.h"
#include "process.h"
#include "userproc.h"
#include "rtx.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */

ProcessQueue* readyQueue;
ProcessQueue** blockedQueues;
ProcessQueue* blockedMsgQueues;
ProcessNode* curProcess;
ProcessNode* nullProcessNode;

pcb_t* procArr[7];
MessageQueue* msgDelayQueue;
int newProcessId = 0;				/* Must be unique */


void null_process(void) {
	while(1) {
		release_processor();
	}
}

void process_init() {
  int i;
	// Ready queue initialization
	readyQueue = (ProcessQueue*)k_request_memory_block();
	readyQueue->first = NULL;
	readyQueue->last = NULL;
	readyQueue->size = 0;
	
	// Blocked queues initialization
	blockedQueues = (ProcessQueue**)k_request_memory_block();
	for (i = 0; i < PRIORITY_COUNT; ++i) {
		blockedQueues[i] = (ProcessQueue*)k_request_memory_block();
		blockedQueues[i]->first = NULL;
		blockedQueues[i]->last = NULL;
		blockedQueues[i]->size = 0;
	}
	
	

	// Initialization of blocked queues for messaging
	blockedMsgQueues = (ProcessQueue*)k_request_memory_block();
	blockedMsgQueues->first = NULL;
	blockedMsgQueues->last = NULL;
	blockedMsgQueues->size = 0;
	
	//Initialization of delayed message queue
	msgDelayQueue = (MessageQueue*)k_request_memory_block();
	msgDelayQueue->first = NULL;
	msgDelayQueue->last = NULL;
	msgDelayQueue->size = 0;
	
	// Null process initialization
	curProcess = NULL;
	nullProcessNode = (ProcessNode*)k_request_memory_block();
	init_pcb(&null_process, nullProcessNode, 4);
	
	// User processes initialization
	for (i = 0; i < ProcessCount; ++i)
		add_new_prioritized_process(ProcessTable[i], 0);
}

int k_set_process_priority(int process_ID, int priority) {
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

int k_get_process_priority(int process_ID) {
	
	int i = 0;
	ProcessNode* node = readyQueue->first;
	
	while(node != NULL) {
		if (node->pcb.m_pid == process_ID) {
			return node->pcb.priority;
		}
		node = node->next;
	}
	
	for (i = 0; i < PRIORITY_COUNT; ++i) {
		node = blockedQueues[i]->first;
		
		while(node != NULL) {
			if (node->pcb.m_pid == process_ID) {
				return node->pcb.priority;
			}
			node = node->next;
		}
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
	uint32_t* stackBlockStart = (uint32_t*)k_request_memory_block();
	node->next = NULL;
	
	// PCB initialization
	procArr[newProcessId] = &(node->pcb);
	node->pcb.m_pid = newProcessId++;
	
	node->pcb.priority = priority;
	node->pcb.m_state = NEW;
	
	node->pcb.msgQueue.first = NULL;
	node->pcb.msgQueue.last = NULL;
	node->pcb.msgQueue.size = 0;
	
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
int k_add_new_process(void* process) {
	return add_new_prioritized_process(process, 3);
}

// This is an internal call for creating new processes
int add_new_prioritized_process(void* process, int priority) {
	ProcessNode* node = (ProcessNode*)k_request_memory_block();
	
	
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
int k_voluntarily_release_processor() {
	return k_release_processor(RDY);
}

// This is an internal API call for releasing current process with a new specified state
int k_release_processor(proc_state_t newState) {
	if (curProcess != NULL)
		curProcess->pcb.m_state = newState;
	
	switch (newState) {
		case RDY:
			if (curProcess != NULL)
				push_process(readyQueue, curProcess);
			break;
		case INSUFFICIENT_MEMORY:
			push_process(blockedQueues[curProcess->pcb.priority], curProcess);
			break;
		case MSG_WAIT:
			push_process(blockedMsgQueues, curProcess);
			break;
	}
	switch_process();
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

// Remove process from the specified queue with specified process ID
ProcessNode* remove_process(ProcessQueue* queue, int pid) {
	ProcessNode* node = queue->first;
	ProcessNode* prev = NULL;
	while (node != NULL) {
		if (node->pcb.m_pid == pid) {
			if (prev != NULL)
				prev->next = node->next;
			if (queue->first == node)
				queue->first = node->next;
			if (queue->last == node)
				queue->last = prev;
			
			queue->size--;
			return node;
		}
		prev = node;
		node = node->next;
	}
	return NULL;
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

// Add process to the back of the specified queue
void push_process_to_front(ProcessQueue* queue, ProcessNode* node) {
	if (queue->first == NULL) {		
		queue->last = node;
	}
	node->next = queue->first;
	queue->first = node;
	queue->size++;
}

// Internal call for releasing processor
int switch_process(void)
{
	volatile int i;
	volatile proc_state_t state;
	ProcessNode *oldProcess = NULL;
	ProcessNode *newProcess = NULL;

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
		uart_put_string("Kernel Error: Failed to switch process.\n\rProcess ID:\n\r");
//		uart1_put_hex(curProcess->pcb.m_pid);
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
			push_process_to_front(readyQueue, node);
			
			//Preemption
			if(curProcess->pcb.priority > node->pcb.priority) {
				k_voluntarily_release_processor();
			}
			
			return;
		}
	}
}

int k_send_message(int process_ID, void *messageEnvelope) {
	return send_msg(process_ID, messageEnvelope, 1);
}

int send_msg(int process_ID, void *messageEnvelope, int allowPreempt) {
	ProcessNode* node;
	pcb_t* target = procArr[process_ID];
	Message* msg = (Message*)messageEnvelope;
	addMessage(&(target->msgQueue), msg, 0);
	
	node = remove_process(blockedMsgQueues, process_ID);
	if (node != NULL) {
		// Receiver is in blocked queue
		node->pcb.m_state = RDY;
		push_process_to_front(readyQueue, node);
		
		//Preemption
		if(allowPreempt && curProcess->pcb.priority > node->pcb.priority) {
			k_voluntarily_release_processor();
		}
	}
	return 0;
}


int k_delayed_send(int process_ID, void *MessageEnvelope, int delay){
	Message* msg = (Message*)MessageEnvelope;
	addMessage(msgDelayQueue, msg, delay);
	return 0;
}

void* k_receive_message(int* sender_id) {
    return removeMessage(&(curProcess->pcb.msgQueue), sender_id);
}

void k_dec_delay_msg_time(){
	MessageNode* temp;
	MessageNode* node = msgDelayQueue->first;
	MessageNode* 	prev = msgDelayQueue->first;
	Message* msg ;

	while(node!=NULL){
		node->delay--;
		if(node->delay <=0){
			msg = node->message;
			send_msg(msg->dest_pid, (void*)msg, 0);
			temp = node;
			if((node == msgDelayQueue->first) || (node == msgDelayQueue->last)){
				if(msgDelayQueue->first == msgDelayQueue->last){
					msgDelayQueue->first = NULL;
					msgDelayQueue->last = NULL;
				}else{
					if(node == msgDelayQueue->first){
						msgDelayQueue->first = node->next;
						prev = node->next;
					}
					if(node == msgDelayQueue->last){
						msgDelayQueue->last = prev;
						prev->next = NULL;
					}
				}
			}
			else{
				prev->next = node->next;
			}
			node = node->next;
			msgDelayQueue->size--;
			k_release_memory_block(temp);
		}
		else{
			prev = node;
			node = node->next;
		}
	}
}
