#include <am.h>
#include <klib.h>
#include <rtthread.h>

// 全局上下文
Context **g_from;
Context **g_to;
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
void rt_hw_context_switch_to(rt_ubase_t to);

//返回的Context就是下次切换要执行的
static Context* ev_handler(Event e, Context *c) {
	printf("ev_handler\n");
  switch (e.event) {
		case EVENT_YIELD: 

			break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return *g_to;
}

void __am_cte_init() {
  cte_init(ev_handler);
	printf("__am_cte_init()\n");
}

void rt_hw_context_switch_to(rt_ubase_t to) {
	printf("rt_hw_context_switch_to\n");
	g_to = (Context **)to;
	yield();
}

//上下文切换保护机制，保证任务切换原子、安全
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
	printf("rt_hw_context_switch\n");
	g_from = (Context **)from;
	g_to   = (Context **)to;
	yield();
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
	printf("rt_hw_stack_init\n");
  return (rt_uint8_t *)c;
}
