#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

extern int test08_1_main(int argc, char *argv[]);

static int start_threads(int argc, char *argv[])
{
	kz_run(test08_1_main, "command", 0x100, 0, NULL);
	return 0;
}

int main(void)
{
	INTR_DISABLE;

	puts("kozos boot succeed!\n");

	// "start"という名前のスレッドが，start_threads()をメイン関数として動作開始
	//       メイン関数　　　　名前　　　スタックサイズ    argc   argv
	kz_start(start_threads, "start", 0x100, 0, NULL);

	return 0;
}