#include "l_coroutine.h"
typedef unsigned int uint;

#define DEFAULT_STACK_SIZE (32 * 1024) //默认使用32KB的栈空间

typedef void(*task_func_t)(void*, void*);

typedef enum {

	TASK_DEAD = 0,
	TASK_READY,
	TASK_RUNING,
	TASK_EXIT,

}task_status_t;

typedef struct __scheduler {
	
	ucontext_t 			main_ctx;
	
	struct list_head 	task_list;

	int 				task_size;


} scheduler_t;

typedef struct __task {

	task_func_t 		func;	//task所指向的函数

	void				*arg;	//参数

	ucontext_t			ctx;	

	struct list_head  	list;	//挂载点

	uint 				stack_size; 

	task_status_t		status;

	scheduler_t			*scheduler;

} task_t;



void task_start(task_t *t) 
{

	assert(t);

	t->func(t, t->arg);

}


task_t* task_create(scheduler_t *s, task_func_t func, void *arg, uint stack_size)
{
	assert(s);
	assert(func);
	assert(arg);
	assert(stack_size > 0);

	task_t *t = NULL;

	t = malloc(sizeof(task_t) + stack_size);

	if (t == NULL)
		return NULL;

	t->func = func;
	t->arg = arg;
	t->stack_size = stack_size;
	t->scheduler = s;

	if (getcontext(&t->ctx) < 0) {
		printf("fail to getcontext\n");
		free(t);
		return NULL;
	}

	t->ctx.uc_stack.ss_sp = ((char*) t) + sizeof(task_t);

	t->ctx.uc_stack.ss_size = stack_size;
	
	makecontext(&t->ctx, (void (*)())task_start, 1, t);

	list_add_tail(&t->list, &s->task_list);

	s->task_size++;

	return t;
}

int task_yield(task_t *t) 
{
	assert(t);

	t->status = TASK_READY;

	list_add_tail(&t->list, &t->scheduler->task_list);
	
	t->scheduler->task_size++;

	return swapcontext(&t->ctx, &t->scheduler->main_ctx);

}

int task_exit(task_t *t) 
{
	assert(t);
	
	int ret = 0;

	t->status = TASK_EXIT;

	ret = swapcontext(&t->ctx, &t->scheduler->main_ctx);

	if (ret < 0) 
		return ret;
	
	free(t);

	return ret;
}


scheduler_t * open_coroutine() 
{
	scheduler_t *s = NULL;

	s = malloc(sizeof(scheduler_t));

	if (s == NULL) 
		return NULL;

	s->task_size = 0;

	INIT_LIST_HEAD(&s->task_list);

	return s;
	
}

void close_coroutine(scheduler_t *s) 
{
	assert(s);
	task_t *t = NULL;
	task_t *tmp = NULL;

	list_for_each_entry_safe(t, tmp, &s->task_list, list) {
		free(t);
	}
	
	free(s);
}

void schedule(scheduler_t *s)
{
	assert(s);

	int ret = 0;
	task_t *t = NULL;

	while(1) {
		if (list_empty(&s->task_list)) 
			break;

		t = list_entry(s->task_list.next, task_t, list);

		assert(t);

		list_del(&t->list);

		t->status = TASK_RUNING;

		ret = swapcontext(&t->scheduler->main_ctx, &t->ctx);

		assert(ret == 0);


	}

}

void foo(void *t_arg, void *real_arg)
{
    task_t *t = (task_t*)t_arg;
    int n = *(int*)real_arg;

    int i = 0;

    for (i =0 ; i < 10; i++) {
        printf("%d\n", n);
        task_yield(t);
    }
    
    task_exit(t);


}

int main(int argc, char **argv)
{
    int b = 66;
    int a = 44;
    scheduler_t *s = open_coroutine();     
    task_t *t =  task_create(s, foo, &a, DEFAULT_STACK_SIZE);
    t =  task_create(s, foo, &b, DEFAULT_STACK_SIZE);
    if (t == NULL)
        goto out;

    schedule(s); 

out:
    close_coroutine(s);
    return 0;
}
