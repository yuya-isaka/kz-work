#include "defines.h"
#include "kozos.h"
#include "syscall.h"

// システムコールのAPIとなる関数たち

// どこから呼び出されてる？
// 『main.c』の『start_threads関数』
// スレッドの新規生成
kz_thread_id_t kz_run(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[])
{
	kz_syscall_param_t param;
	param.un.run.func = func;
	param.un.run.name = name;
	param.un.run.priority = priority;
	param.un.run.stacksize = stacksize;
	param.un.run.argc = argc;
	param.un.run.argv = argv;
	kz_syscall(KZ_SYSCALL_TYPE_RUN, &param);
	return param.un.run.ret;
}

// どこから？
// 『kozos.c』の『thread_end関数』
void kz_exit(void)
{
	kz_syscall(KZ_SYSCALL_TYPE_EXIT, NULL);
}

int kz_wait(void)
{
	kz_syscall_param_t param;
	
}