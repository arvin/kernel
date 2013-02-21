#ifndef _RTX_H
#define _RTX_H

// Constants
#define NULL 0

typedef unsigned int U32;

// User Defined Variables
extern int ProcessCount;
extern void* ProcessTable[];

// User API
#define __SVC_0  __svc_indirect(0)

extern int k_release_processor(void);
#define manage_processor() _release_processor((U32)k_release_processor)
int _release_processor(U32 p_func) __SVC_0;

extern int release_processor(void);
extern void* s_requestion_memory_block(void);
extern int s_release_memory_block(void* memory_block);

extern int k_set_process_priority(int process_ID, int priority);
#define set_process_priority(process_ID, priority) _set_process_priority((U32)k_set_process_priority, process_ID, priority)
extern int _set_process_priority(U32 p_func, int process_ID, int priority) __SVC_0;

extern int k_get_process_priority(int process_ID);
#define get_process_priority(process_ID) _get_process_priority((U32)k_get_process_priority, process_ID)
extern int _get_process_priority(U32 p_func, int process_ID) __SVC_0;

#endif /* !_RTX_H_ */
