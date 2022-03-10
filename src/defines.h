// 様々な方の定義
// 頻繁に利用する型は定義しておく（ヘッダファイルの依存関係をなくすため）

#ifndef _DEFINES_H_INCLUDED_
#define _DEFINES_H_INCLUDED_

#define NULL ((void *)0)		// NULLポインタの定義
#define SERIAL_DEFAULT_DEVICE 1 // 標準のシリアルデバイス

// ビット幅固の整数型
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

#endif