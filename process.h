#ifndef _PROCESS_H_
#define _PROCESS_H_

#define USR_SZ_STACK 0x020

#define NULL 0
#define PRIORITY_COUNT 4
#define INITIAL_xPSR 0x01000000    /* user process initial xPSR value */

#include <stdint.h>
#include "memory.h"

/* process states, note we only assume three states in this example */
typedef enum {NEW = 0, RDY, RUN, INSUFFICIENT_MEMORY} proc_state_t;  

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/

typedef struct pcb PCB;

typedef struct pcb { 
  //struct pcb *mp_next;     /* next pcb, not used in this example, RTX project most likely will need it, keep here for reference */  
  uint32_t *mp_sp;         /* stack pointer of the process */
  uint32_t m_pid;          /* process id */
  proc_state_t m_state;    /* state of the process */
	int priority;
} pcb_t;

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
void init_pcb(void* process, ProcessNode* node, int priority);
ProcessNode* scheduler(void);               /* pick the pid of the next to run process */
int add_new_process(void*);
int add_new_prioritized_process(void*, int priority);
int release_processor(void);				/* user release_process function */
int k_release_processor(void);       /* kernel release_process function */
int set_process_priority(int process_ID, int priority);
int get_process_priority(int process_ID);
int release_processor_to_queue(proc_state_t newState);
ProcessNode* poll_process(ProcessQueue* queue);
void push_process(ProcessQueue* queue, ProcessNode* node);
void unblock_process(void);
extern void __rte(void);           /* pop exception stack frame */

#endif /* ! _PROCESS_H_ */