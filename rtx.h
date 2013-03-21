#ifndef _RTX_H
#define _RTX_H

// Constants
#define NULL 0
#define TRUE 1
#define FALSE 0
#define MEMORY_BLOCK_SIZE 32

typedef unsigned int U32;

// User Defined Variables
extern int ProcessCount;
extern void* ProcessTable[];
extern int start_clk;

// User API

typedef struct Message{
    int sender_pid;
	  int dest_pid;
	  int type;
	  void* data;
}Message;

typedef enum{KEYBOARD_INPUT = 0, CRT_DISPLAY, COMMAND_REG, COMMAND, DISPLAY_TIME} msg_type;

typedef enum {TIMER = 0, UART, KCD, CRT} system_proc_type;


#define __SVC_0  __svc_indirect(0)

extern int k_voluntarily_release_processor(void);
#define release_processor() _release_processor((U32)k_voluntarily_release_processor)
int _release_processor(U32 p_func) __SVC_0;

extern void* k_persistent_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_persistent_request_memory_block)
void* _request_memory_block(U32 p_func) __SVC_0;

extern int k_has_free_memory(int multiplier);
#define has_free_memory() _has_free_memory((U32)k_has_free_memory, 1)
void* _has_free_memory(U32 p_func, int multiplier) __SVC_0;

extern int k_release_memory_block(void* memory_block);
#define release_memory_block(memory_block) _release_memory_block((U32)k_release_memory_block, memory_block)
int _release_memory_block(U32 p_func, void* memory_block) __SVC_0;

extern int k_set_process_priority(int process_ID, int priority);
#define set_process_priority(process_ID, priority) _set_process_priority((U32)k_set_process_priority, process_ID, priority)
int _set_process_priority(U32 p_func, int process_ID, int priority) __SVC_0;

extern int k_get_process_priority(int process_ID);
#define get_process_priority(process_ID) _get_process_priority((U32)k_get_process_priority, process_ID)
int _get_process_priority(U32 p_func, int process_ID) __SVC_0;

extern int k_send_message(int process_ID, void *messageEnvelope);
#define send_message(process_ID, messageEnvelope) _send_message((U32)k_send_message, process_ID, messageEnvelope)
int _send_message(U32 p_func, int process_ID, void *messageEnvelope) __SVC_0;

extern void* k_receive_message(int* sender_id);
#define receive_message(sender_id) _receive_message((U32)k_receive_message, sender_id)
void* _receive_message(U32 p_func, int* sender_ID) __SVC_0;

extern int k_delayed_send(int process_ID, void *MessageEnvelope, int delay);
#define delayed_send(process_ID, MessageEnvelope, delay) _delayed_send((U32)k_delayed_send, process_ID, MessageEnvelope, delay)
int _delayed_send(U32 p_func, int process_ID, void* MessageEnvelope, int delay) __SVC_0;

extern int k_dec_delay_msg_time(void);
#define dec_delay_msg_time() _dec_delay_msg_time((U32)k_dec_delay_msg_time)
int _dec_delay_msg_time(U32 p_func) __SVC_0;

extern int k_get_system_pid(system_proc_type type);
#define get_system_pid(type) _get_system_pid((U32)k_get_system_pid, type)
int _get_system_pid(U32 p_func, system_proc_type type) __SVC_0;

extern void k_display_time(void);
#define display_time() _display_time((U32)k_display_time)
void _display_time(U32 p_func) __SVC_0;

extern void k_set_timer_count(int time);
#define set_timer_count(time) _set_timer_count((U32)k_set_timer_count, time)
void _set_timer_count(U32 p_func, int time) __SVC_0;

#endif /* !_RTX_H_ */
