// ソフトウェア・割込みベクタ関連の定義

#ifndef _INTERRUPT_H_INCLUDED_
#define _INTERRUPT_H_INCLUDED_

// リンカスクリプトで定義してあるシンボル--------------------------------
extern char softvec;
#define SOFTVEC_ADDR (&softvec)

// 型の定義 -----------------------------------------------------------------
// ソフトウェア・割込みベクタの種別を表す型の定義
// shortのエイリアスなだけだが，プリミティブ型ではなく値の種類の応じて型を定義するのは，プログラムが明白になるから良い
// コメントはあくまで注釈で，プログラムの意味はプログラムで表現するべき
// -> 文章がわかりにくいと指摘されたら，注釈を増やすのではなく，章構成や段落などを工夫して文章をわかりやすくする
// -> コメントは処理の目的や例外的な対処に対して書くもの
typedef short softvec_type_t;
// 割込みハンドラの型の定義
// ハンドラ関数の型を定義
// 例えば，ハンドラは，void handler(softvec_type_t type, unsigned long sp);のように考えている
// 1. 割込みの種別  2. スタックポインタ
// ハンドラの設定の操作を行うときは，ハンドラへのポインタ，関数へのポインタを使用する必要があります．
// 「関数へのポインタ」を複数扱う時とか，配列として準備できるといい感じに使える
// 定義したハンドラ関数を受け取る時に使ったりする
// 関数へのポインタ定義の記述
// -> 「関数のプロトタイプ宣言をして，関数名の部分を(*f)に置き換える．」終わり
typedef void (*softvec_handler_t)(softvec_type_t sof_type, unsigned long sp);

// defineでわかりやすく定義 -------------------------------------------------
// ソフトウェア・割込みベクタの位置
// アドレスをキャストすると，アドレスの場所にキャストした型が用意される？？
// ここではキャストした型は，関数へのポインタだから，SOFTVECSには関数へのポインタを入れられる．
#define SOFTVECS ((softvec_handler_t *)SOFTVEC_ADDR)
// 割込み有効化
// インラインアセンブラ（C言語のプログラム中でのアセンブラの記述をかのうにするためのもの）
// 割込みの有効化/無効化は，CCRの操作なので，アセンブラでしかできないが，わざわざ関数を用意するのはめんどくさい．のでインラインアセンブラで書いてしまう
// CCRはCPUで用意されていうレジスタだが，レジスタ単位を直接指定できるのはアセンブラだけ（メモリマップI/Oとかでメモリにレジスタのアドレスがマップされているなら話は別だが，CCRはそうではない）
#define INTR_ENABLE asm volatile("andc.b #0x3f, ccr")
// 割込み無効化（割込み禁止）
// 無効化する時，処理途中に再度割込みが入って有効化になるのを防ぐために，アトミックに割込みを無固化する必要がある．CPUが専用の命令を用意しているケースが多い，H8はorc.bでいける
#define INTR_DISABLE asm volatile("orc.b #0xc0, ccr")

// 関数のプロトタイプ宣言 ---------------------------------------------------------
// ソフトウェア・割込みベクタの初期化
int softvec_init(void);
// ソフトウェア割込みベクタの設定
int softvec_setintr(softvec_type_t sof_type, softvec_handler_t handler);
// 共通割込みハンドラ
// ソフトウェア割込みベクタを処理するための共通割込みハンドラ
void interrupt(softvec_type_t sof_type, unsigned long sp);

#endif
