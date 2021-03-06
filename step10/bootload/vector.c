// 割り込みベクタの設定

// CPUには「割込み」という機能がある (割込みという指示が入ると別の処理に入るというもの)
// 割込み時に行う処理を「割込みハンドラ」と呼ぶ
// 割込みハンドラが終了すると，CPUは中断していた処理を再開する

// 具体的
// CPUが「割込み線」というピンを持っていて，ピンに電圧がかかるとCPUが割り込み発生を検知して割り込み処理に入る
// (不論理の場合はピンの電圧が落ちた時）)

// 割込みを実現するためには，　割込み発生時にどのハンドラを実行するのかを設定する必要がある．
// 2通りの方法
// 1. 割込み発生時には，特定のアドレスから実行する．
// 2. 割込み発生時には，どのアドレスから実行するか特定のアドレスに設定しておく．

// 1 = 割込みハンドラを配置するアドレスをCPU側で決め打ちして，割り込みが発生したら強制的にそのアドレスに強制的に飛ぶようにする
// 2 = 割り込みハンドラを配置するアドレスを特定のアドレスに記述しておくというもの → 「ベクタ割込み方式」

// 「ハンドラのアドレスを記述しておく特定のアドレス」＝割込みベクタ

// H8は「ベクタ割込み方式」で，割り込みが発生すると「割り込みベクタ」を参照しに行く．
// 割込みベクタは，　割込みの種類に応じて「0x000000 - 0x0000ff」のメモリ上に配置．

// 割込みの種類
// ・ 電源ON ... リセットベクタ
// ・ リセット ... リセットベクタ　（H8では割込みベクタの先頭がリセット・ベクタになっている）

// C言語で定義された関数は， 「_」がついてシンボルが作成される
// リンカは関数名や変数名をシンボルとして扱う
// 関数や変数の実体はメモリ上にあるから， -> シンボルはそのメモリのアドレスの別名とも言える

#include "defines.h"

// H8が一番最初に実行開始する関数
// extern ... 関数のプロトタイプ宣言
extern void start(void);		// スタートアップ
extern void intr_softerr(void); // ソフトウェアエラー（トラップ割込み）
extern void intr_syscall(void); // システムコール（トラップ割込み）
extern void intr_serintr(void); // シリアル割込み

// vectors[]の内容は0x000000 - 0x0000ffにないといけない
// リンカスクリプトの定義で先頭番地に配置する
// リンカスクリプト ... オブジェクトファイル->実行形式の時に，　関数や変数をアドレス上にどのように配置するかなどを指定する, 「メモリマップ」

// 割込みベクタ (関数へのポインタの配列)
// 割込みベクタは，　割込みハンドラを指すアドレスのこと
// 割込みハンドラは， 処理を書いた関数
// -> vectors[] は，　割込みハンドラ関数のアドレスを格納している（割込み処理の種類にってハンドラも複数あるから配列）
// 「　CPUの割り込みベクタの設定　」
void (*vectors[])(void) = {
	start, // start() へのポインタ 一番最初に実行開始するアドレス (H8では割込みベクタの先頭がリセット・ベクタになっている，　したがって今はリセットベクタにstart()を設定している)
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	intr_syscall,
	intr_softerr,
	intr_softerr,
	intr_softerr,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	intr_serintr, // シリアル関連の割り込みが発生したら，intr_serintr()という関数が呼ばれる -> 実体はintr.Sにある
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
	intr_serintr,
};

// vector[52] - vectors[63]
