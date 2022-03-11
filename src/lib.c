// 各種ライブラリ関数の本体

// 生の入出力=シリアル
// 端末返還を行なっている入出力=コンソール

#include "defines.h"
#include "serial.h"
#include "lib.h"

// 1文字送信
// コンソールへの文字出力関数 (コンソールへの出力はこいつを呼び出せば良くなった)
// (出力先はdefines.hで定義したSERIAL_DEFAULT_DEVICE)
int putc(unsigned char c)
{
	// C言語．．．改行コードは\n
	// UNIXの世界...改行コードは0x0a
	// シリアル通信...歴史的経緯により改行コードは'\r'0x0d
	// このようなコード変換を「端末変換」という
	if (c == '\n')
		serial_send_byte(SERIAL_DEFAULT_DEVICE, '\r');
	return serial_send_byte(SERIAL_DEFAULT_DEVICE, c);
}

// 文字列送信
int puts(unsigned char *str)
{
	while (*str)
		putc(*(str++));
	return 0;
}