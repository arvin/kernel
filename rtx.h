/* *
 * @file:   rtx.h
 * @brief:  User API prototype, this is an example only
 * @author: Yiqing Huang
 * @date:   2013/01/12
 */
#ifndef _RTX_H
#define _RTX_H

typedef unsigned int U32;

#define __SVC_0  __svc_indirect(0)

extern int k_release_processor(void);
#define manage_processor() _release_processor((U32)k_release_processor)
int __SVC_0 _release_processor(U32 p_func);
//int _release_processor(U32 p_func) __SVC_0; // __SVC_0 can also be at the end

extern int add_new_process(void*);
extern int release_processor(void);
extern void* s_requestion_memory_block(void);
extern int s_release_memory_block(void* memory_block);
extern int set_process_priority(int process_ID, int priority);
extern int get_process_priority(int process_ID);

#endif /* !_RTX_H_ */
