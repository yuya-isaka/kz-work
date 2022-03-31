#include "defines.h"
#include "kozos.h"
#include "syscall.h"

// システムコールのAPIとなる関数たち

// それぞれの実際のシステムコールたちは，『構造体』の中身を確認しながら実行する

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

// どこから？
// 『test09_1_main関数』『test09_2main関数』『test09_3_main関数』
int kz_wait(void)
{
	kz_syscall_param_t param;
	kz_syscall(KZ_SYSCALL_TYPE_WAIT, &param);
	return param.un.wait.ret;
}

// どこから？
// 『test09_1_main関数』『test09_2main関数』
int kz_sleep(void)
{
	kz_syscall_param_t param;
	kz_syscall(KZ_SYSCALL_TYPE_SLEEP, &param);
	return param.un.sleep.ret;
}

// どこから？
// 『test09_3_main関数』
int kz_wakeup(kz_thread_id_t id)
{
	kz_syscall_param_t param;
	param.un.wakeup.id = id;
	kz_syscall(KZ_SYSCALL_TYPE_WAKEUP, &param);
	return param.un.wakeup.ret;
}

// どこから？
kz_thread_id_t kz_getid(void)
{
	kz_syscall_param_t param;
	kz_syscall(KZ_SYSCALL_TYPE_GETID, &param);
	return param.un.getid.ret;
}

// どこから？
// 『test09_1_main関数』『test09_2main関数』
int kz_chpri(int priority)
{
	kz_syscall_param_t param;
	param.un.chpri.priority = priority;
	kz_syscall(KZ_SYSCALL_TYPE_CHPRI, &param);
	return param.un.chpri.ret;
}