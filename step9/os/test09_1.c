#include "defines.h"
#include "kozos.h"
#include "lib.h"

// 優先度１
int test09_1_main(int argc, char *argv[])
{
	puts("test09_1 started.\n");

	puts("test09_1 sleep in.\n");
	// スレッドをスリープ
	kz_sleep();
	puts("test09_1 sleep out.\n");

	puts("test09_1 chpri in.\n");
	// 優先度を1->3
	kz_chpri(3);
	puts("test09_1 chpri out.\n");

	puts("test09_1 wait in.\n");
	// CPUを離し，他のスレッドを動作させる
	kz_wait();
	puts("test09_1 wait out.\n");

	puts("test09_1 trap in.\n");
	// トラップを発行
	asm volatile("trapa #1");
	puts("test09_1 trap out.\n");

	puts("test09_1 exit.\n");

	return 0;
}