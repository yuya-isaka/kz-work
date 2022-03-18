// ソフトウェア・割込みベクタ関連の定義

#ifndef _INTERRUPT_H_INCLUDED_
#define _INTERRUPT_H_INCLUDED_

// リンカスクリプトで定義してあるシンボル--------------------------------
extern char softvec;
#define SOFTVEC_ADDR (&softvec)

// 型の定義 -----------------------------------------------------------------
// ソフトウェア・割込みベクタの種別を表す型の定義
typedef short softvec_type_t;
// 割込みハンドラの型の定義
typedef void (*softvec_handler_t)(softvec_type_t type, unsigned long sp);

// defineでわかりやすく定義 -------------------------------------------------
// ソフトウェア・割込みベクタの位置
#define SOFTVECS ((softvec_handler_t *)SOFTVEC_ADDR)
// 割込み有効化
#define INTR_ENABLE asm volatile("andc.b #0x3f, ccr")
// 割込み無効化（割込み禁止）
#define INTR_DISABLE asm volatile("orc.b #0xc0, ccr")

// 関数のプロトタイプ宣言 ---------------------------------------------------------
// ソフトウェア・割込みベクタの初期化
int softvec_init(void);
// ソフトウェア割込みベクタの設定
int softvec_setintr(softvec_type_t type, softvec_handler_t handler);
// 共通割込みハンドラ
// ソフトウェア割込みベクタを処理するための共通割込みハンドラ
void interrupt(softvec_type_t type, unsigned long sp);

#endif