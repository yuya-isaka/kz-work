#include "defines.h"
#include "kozos.h"
#include "lib.h"

// どこから？
// 『kozos.c』の『thread_init関数』（init.func）
// スレッドとして生成されて実行される関数は，すべて『thread_init関数』（init.func）
int test08_1_main(int argc, char *argv[])
{
	char buf[32];
	puts("test08_1 started.\n");

	while (1)
	{
		puts("> ");
		gets(buf);

		if (!strncmp(buf, "echo", 4))
		{
			puts(buf + 4);
			puts("\n");
		}
		else if (!strcmp(buf, "exit"))
		{
			break;
		}
		else
		{
			puts("unknown.\n");
		}
	}

	puts("test08_1 exit.\n");

	return 0;
}
