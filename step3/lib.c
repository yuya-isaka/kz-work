// 各種ライブラリ関数の本体

// 生の入出力=シリアル
// 端末返還を行なっている入出力=コンソール

#include "defines.h"
#include "serial.h"
#include "lib.h"

// メモリ関連のライブラリ---------------------------------------------

// mem型は戻り値や引数にいろんな型を取るために，
// 汎用ポインタ型 void *　をとる

// メモリを特定パターンで埋める
void *memset(void *b, int c, long len)
{
	char *p;
	for (p = b; len > 0; len--) // p経由で変換(char型に対応させるため？)
		*(p++) = c;
	return b;
}

// メモリのコピーを行う
void *memcpy(void *dst, const void *src, long len)
{
	char *d = dst;
	const char *s = src;
	for (; len > 0; len--)
		*(d++) = *(s++); // d経由で変換（char型に対応させるため？）
	return dst;
}

// メモリ上のデータの比較を行う
int memcmp(const void *b1, const void *b2, long len)
{
	const char *p1 = b1, *p2 = b2;
	for (; len > 0; len--)
	{
		if (*p1 != *p2)
			return (*p1 > *p2) ? 1 : -1;
		p1++;
		p2++;
	}
	return 0;
}

// 文字列関連のライブラリ-----------------------------------------------

// 文字列の長さ
int strlen(const char *s)
{
	int len;
	for (len = 0; *s; s++, len++)
		;
	return len;
}

// 文字列のコピーを行う
char *strcpy(char *dst, const char *src)
{
	char *d = dst;
	for (;; dst++, src++)
	{
		*dst = *src;
		if (!*src)
			break;
	}
	return d;
}

// 文字列の比較
int strcmp(const char *s1, const char *s2)
{
	while (*s1 || *s2)
	{
		if (*s1 != *s2)
			return (*s1 > *s2) ? 1 : -1;
		s1++;
		s2++;
	}
	return 0;
}

// 長さ制限有りで文字列の比較を行う
int strncmp(const char *s1, const char *s2, int len)
{
	while ((*s1 || *s2) && (len > 0))
	{
		if (*s1 != *s2)
			return (*s1 > *s2) ? 1 : -1;
		s1++;
		s2++;
		len--;
	}
	return 0;
}

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

// 文字列送信(文字列型の先頭のアドレスを受け取る)
int puts(unsigned char *str)
{
	while (*str)
		putc(*(str++));
	return 0;
}

// 整数値を16進数で出力
// 表示のためには数値から文字列への変換が必要
// 理由：　シリアルコントローラがシリアル通信で文字しか扱えないから
int putxval(unsigned long value, int column)
{
	// 文字列出力用バッファ
	char buf[9];

	// 下の桁から処理するのでバッファの終端から利用する
	char *p;
	p = buf + sizeof(buf) - 1; // 終端のアドレス取得
	*(p--) = '\0';			   // 逆順に格納

	if (!value && !column)
		column++;

	while (value || column)
	{
		*(p--) = "0123456789abcdef"[value & 0xf]; // 16新文字に変換してバッファに格納する(下位4ビットを変換し逆順に格納)
		value >>= 4;							  // 次の桁に進める
		if (column)
			column--; // 桁数指定がある場合にはカウントする
	}

	puts(p + 1); // バッファの内容を出力する(pは最終的に知りたいアドレス-1になってる)

	return 0;
}
