/**
 * @brief timer.h - Timer header file
 * @author Y. Huang
 * @date 2013/02/12
 */
#ifndef _TIMER_H_
#define _TIMER_H_

extern uint32_t timer_init ( uint8_t n_timer );  /* initialize timer n_timer */

void trigger_timer_i_process(void);
int timer_i_process(void);
uint32_t get_wall_clk_handler(void);
void set_wall_clk_handler(uint32_t pid);
void k_set_timer_count(int time);
int get_timer(void);

#endif /* ! _TIMER_H_ */
