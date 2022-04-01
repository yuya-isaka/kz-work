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
	// 『メモリ保護機能のあるCPU』
	// 不正なメモリ領域をアクセスすると -> 不正アドレスアクセスとして例外割り込みを発生させる
	// 不正アドレスアクセスなどの不正な処理が行われてしまっているものとして扱う
	// 実際に不正アクセスが発生したときにスレッドの動作を中断できるか実験
	asm volatile("trapa #1");
	puts("test09_1 trap out.\n");

	puts("test09_1 exit.\n");

	return 0;
}
