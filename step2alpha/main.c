// main関数
#include "defines.h"
#include "serial.h"
#include "lib.h"

int main(void)
{
	// SCIのレジスタをデータ長を8ビット，ストップビット長を1,パリティ無しで初期化
	// OSがインタフェースを用意する = デバイスドライバを書く
	serial_init(SERIAL_DEFAULT_DEVICE);

	puts("Hello World!\n");

	putxval(0x10, 0);
	puts("\n");
	putxval(0xffff, 0);
	puts("\n");
	putxval(3, 0);
	puts("\n");
	putxval(100, 0);
	puts("\n");
	putxval('a', 0);
	puts("\n");
	putxval('a', 2);
	puts("\n");
	putxval('b', 0);
	puts("\n");

	while (1)
		;

	return 0;
}
