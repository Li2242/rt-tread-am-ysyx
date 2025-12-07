#include "rtdef.h"
#include <am.h>
#include <klib.h>
#include <rtthread.h>

// 全局上下文
typedef struct {
	Context **from;
	Context **to;
} pcb_switch;

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
void rt_hw_context_switch_to(rt_ubase_t to);

//返回的Context就是下次切换要执行的
static Context* ev_handler(Event e, Context *c) {
	// printf("ev_handler\n");
  switch (e.event) {
		case EVENT_YIELD:
			rt_thread_t current = rt_thread_self();
			pcb_switch *temp = (pcb_switch *)current->user_data;
			if (temp->from) {
    		*(temp->from) = c;   // 只在有 from 的时候保存
			}
			return *(temp->to);
		case EVENT_IRQ_TIMER:
		printf("EVENT_IRQ_TIMER\n");
			return NULL;
    default: 
			printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
}

void __am_cte_init() {
  cte_init(ev_handler);
	printf("__am_cte_init()\n");
}

void rt_hw_context_switch_to(rt_ubase_t to) {
	// printf("rt_hw_context_switch_to\n");
	//当前的线程的pcb
	rt_thread_t current = rt_thread_self();
	//保存线程的私有数据
	rt_ubase_t old_user_data = current->user_data;
	pcb_switch p ={
		.from = NULL,
		.to   = (Context **)to
	};
	//存进去
	current->user_data = (rt_ubase_t)&p;
	yield();
	//归还线程的私有数据
	current->user_data = old_user_data;
}


void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
	// printf("rt_hw_context_switch\n");
	rt_thread_t current = rt_thread_self();
	rt_base_t old_user_data = current->user_data;
	pcb_switch p={
		.from = (Context **)from,
		.to   = (Context **)to
	};
	current->user_data = (rt_base_t)&p;
	yield();
	current->user_data = old_user_data;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}



typedef struct {
	void (*tentry)(void *);
	void *parameter;
	void (* texit)(void);
} wrapped;

void fun_wrapped(void* t){
	wrapped *f = (wrapped*)t;
	f->tentry(f->parameter);
	f->texit();
}

//上下文的创建
rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
	//对齐
	uintptr_t top = ((uintptr_t)stack_addr & ~(sizeof(uintptr_t)-1));
	//给包裹函数的参数一个栈空间
	top = top - sizeof(wrapped);
	wrapped *wra = (wrapped *)top;

	wra->tentry    = tentry;
	wra->parameter = parameter;
	wra->texit     = texit;

	Area kstack = {
		.start = (void*)(top - sizeof(Context)),
		.end = (void*)(top)
	};

	Context *c = kcontext(kstack, (void *)fun_wrapped, (void*)wra);
	// printf("rt_hw_stack_init\n");
  return (rt_uint8_t *)c;
}
