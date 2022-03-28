#include "defines.h"
#include "kozos.h"
#include "lib.h"

// 優先度２
int test09_2_main(int argc, char *argv[])
{
	puts("test09_2 started.\n");

	puts("test09_2 sleep in.\n");
	// スレッドをスリープ
	kz_sleep();
	puts("test09_2 sleep out.\n");

	puts("test09_2 chpri in.\n");
	// 優先度を2->3
	kz_chpri(3);
	puts("test09_2 chpri out.\n");

	puts("test09_2 wait in.\n");
	// CPUを離し，他のスレッドを動作させる
	kz_wait();
	puts("test09_2 wait out.\n");

	puts("test09_2 exit.\n");

	return 0;
}