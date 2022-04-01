#include "defines.h"
#include "kozos.h"
#include "lib.h"

// 優先度３
int test09_3_main(int argc, char *argv[])
{
	puts("test09_3 started.\n");

	puts("test09_3 wakeup in (test09_1).\n");
	// test09_1をウェイクアップ
	kz_wakeup(test09_1_id);
	puts("test09_3 wakeup out.\n");

	puts("test09_3 wakeup in (test09_2).\n");
	// test09_2をウェイクアップ
	kz_wakeup(test09_2_id);
	puts("test09_3 wakeup out.\n");

	puts("test09_3 wait in.\n");
	// CPUを離し，他のスレッドを動作させる
	kz_wait();
	puts("test09_3 wait out.\n");

	puts("test09_3 exit in.\n");
	// スレッドを終了する
	kz_exit();
	puts("test09_3 exit out.\n");

	return 0;
}
