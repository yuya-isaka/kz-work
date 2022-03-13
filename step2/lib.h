// 各種ライブラリ関数のヘッダファイル

#ifndef _LIB_H_INCLUDED_
#define _LIB_H_INCLUDED_

// *注意
// メモリ関連のライブラリでは長さ指定が必要なやつは，intの代わりにlong型で指定している
// 理由：H8が16ビットCPUなので，int型は16ビットとなる．　メモリサイズを表す値としては小さい．
// mem系の関数は、文字列というよりメモリブロックを処理対象にしています。そのため、あらゆる型での処理が可能になります。ですから、戻り値や引数も汎用ポインタ型である「void *」を用います。
void *memset(void *b, int c, long len);				  // メモリを特定のバイトで埋める(セット先のメモリブロックのポインタ，　セットする文字，　セットバイトサイズ), bの値
void *memcpy(void *dst, const void *src, long len);	  // メモリのコピー(コピー先のメモリブロックのポインタ，　　コピー元のメモリブロックのポインタ，　　コピーバイトサイズ), dstの値
int memcmp(const void *b1, const void *b2, long len); // メモリの比較(比較元メモリブロックのポインタ, 比較元のメモリブロックのポインタ, 比較バイトサイズ) b1 > b2 正, b1 < b2 負, b1 = b2 0
int strlen(const char *s);							  // 文字列の長さ(文字列，長さに\0は含まない)
char *strcpy(char *dst, const char *src);			  // 文字列のコピー(コピー先の文字型配列のポインタ，　コピー元の文字型配列のポインタ) dstの値
int strcmp(const char *s1, const char *s2);			  // 文字列の比較（比較元の文字型配列のポインタ， 比較元の文字型配列のポインタ) s1 > s2 正, s1 < s2 負, s1 = s2 0
int strncmp(const char *s1, const char *s2, int len); // 長さ指定での文字列の比較(比較文字列， 比較文字列，　　比較文字数) s1 > s2 正, s1 < s2 負, s1 = s2 0

int putc(unsigned char c);	  // 1文字送信
int puts(unsigned char *str); // 文字列送信

int putxval(unsigned long value, int column); // 整数値を16進数で出力（表示したい値，　表示桁数） 戻り値は今の所必ず０

#endif
