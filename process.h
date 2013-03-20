#ifndef _PROCESS_H_
#define _PROCESS_H_

#define NULL 0
#define PRIORITY_COUNT 4
#define INITIAL_xPSR 0x01000000    /* user process initial xPSR value */

#include <stdint.h>
#include "memory.h"
#include "message.h"

/* process states, note we only assume three states in this example */
typedef enum {NEW = 0, RDY, RUN, INSUFFICIENT_MEMORY, MSG_WAIT, WAIT_FOR_INTERRUPT, INTERRUPTED} proc_state_t;  


/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/

typedef struct pcb PCB;

typedef struct pcb { 
  //struct pcb *mp_next;     /* next pcb, not used in this example, RTX project most likely will need it, keep here for reference */
	uint32_t *stack_boundary;
  uint32_t *mp_sp;         /* stack pointer of the process */
  uint32_t m_pid;          /* process id */
  proc_state_t m_state;    /* state of the process */
	int priority;
	MessageQueue msgQueue;
} pcb_t;

typedef struct command_entry{
	char cmd[4];
	int pid;
} command_entry;

typedef struct ProcessNode {
	pcb_t pcb;
	struct ProcessNode* next;
} ProcessNode;

typedef struct ProcessQueue {
	ProcessNode* first;
	ProcessNode* last;
	int size;
} ProcessQueue;


void process_init(void);    /* initialize all procs in the system */
void init_pcb(void* process, ProcessNode* node, int priority, int isStackRequired);
void init_proc_stack(void* process, ProcessNode* node);
ProcessNode* scheduler(void);               /* pick the pid of the next to run process */
int k_add_new_process(void*);
int add_new_prioritized_process(void*, int priority);
int k_voluntarily_release_processor(void);				/* user release_process function */
int switch_process(void);       /* kernel release_process function */
int k_set_process_priority(int process_ID, int priority);
int k_get_process_priority(int process_ID);
int k_release_processor(proc_state_t newState);
ProcessNode* poll_process(ProcessQueue* queue);
ProcessNode* remove_process(ProcessQueue* queue, int pid);
void push_process(ProcessQueue* queue, ProcessNode* node);
void push_process_to_front(ProcessQueue* queue, ProcessNode* node);
void unblock_process(void);
extern void __rte(void);           /* pop exception stack frame */

int k_send_message(int process_ID, void *messageEnvelope);
int k_delayed_send(int process_ID, void *MessageEnvelope, int delay);
void* k_receive_message(int* sender_id);
void* system_proc_receive_message(system_proc_type type);
int k_dec_delay_msg_time(void);
void shift_ready_process(int pid);
int send_msg(int process_ID, void *messageEnvelope, int allowPreempt);
void k_display_time(void);
int k_get_system_pid(system_proc_type type);
void print_process(void);
char* append_to_block(char* block, char* str);
void eos(char* block);
void clear(char* block);
uint32_t get_current_process_id(void);
void set_process_state(uint32_t process_ID, proc_state_t state);
void clearString(char* str);
#endif /* ! _PROCESS_H_ */
