#ifndef _KOZOS_H_INCLUDED_
#define _KOZOS_H_INCLUDED_

#include "defines.h"
#include "syscall.h"

// スレッド起動のシステムコール
/*
	スレッドを生成する．
	引数として関数へのポインタを渡すとスレッドを生成し，その関数の実行を開始
	引数．．．スレッド名，　スレッドのスタックサイズ，　関数の引数
	戻り値．．．生成したスレッドのID番号．　これを『スレッドID』と呼ぶ
*/
// 優先度追加
kz_thread_id_t kz_run(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]);

// スレッド終了のシステムコール
/*
	スレッドを終了する
	C言語のexit()に相当
*/
void kz_exit(void);

// カレントスレッドをレディーキューの後ろに繋ぎ直すことでカレントスレッドを切り替える
int kz_wait(void);
// スレッドをレディーキューから外してスリープ状態にする
int kz_sleep(void);
// スリープ状態のスレッドをレディーキューに繋ぎ直して，レディー状態に戻す
int kz_wakeup(kz_thread_id_t id);
// 自分のスレッドIDを取得する
kz_thread_id_t kz_getid(void);
// スレッドの優先度を変更する（変更前の優先度が返る）
int kz_chpri(int priority);
// 領域を確保
void *kz_malloc(int size);
// 領域を解放
int kz_kmfree(void *p);

// 初期スレッドを起動し，OSの動作を開始
/*
	最初のスレッドを生成
	スレッド生成はkz_runだが，それはシステムコールなのでスレッドからしか実行できない．（鶏と卵）
	一番最初の初代スレッドは明示的に起動する必要がある
	「スレッド生成用のスレッド」としてkz_start()を生成し，そのスレッドから本来必要なスレッドをkz_runで生成していく
	『初期スレッド』と呼ぶことにする
	スレッドの生成後は，スレッドの動作に入るから，この関数が戻ってくることはない．．．儚い存在
*/
void kz_start(kz_func_t func, char *name, int priority, int stacksize, int arvc, char *argv[]);

// 致命的エラーの時に呼び出す
/*
	組込みプログラミングやOSのプログラミングでは，些細なバグで挙動不審になる
	エラー処理が必要な箇所で，kz_sysdown()を利用して停止させると，暴走する前にメッセージを出力して終了できる
*/
void kz_sysdown(void);

// システムコールを実行する
/*
	システムコールの呼び出しを行う共通関数
*/
void kz_syscall(kz_syscall_type_t sys_type, kz_syscall_param_t *param);

// ユーザスレッド
/*
	ユーザスレッドのメイン関数
	C言語のmainに相当
	（本来はkozos.hではなく別ファイルでプロトタイプ宣言するべき）
*/
// extern int test08_1_main(int argc, char *argv[]);
// int test09_1_main(int argc, char *argv[]);
// int test09_2_main(int argc, char *argv[]);
// int test09_3_main(int argc, char *argv[]);
// extern kz_thread_id_t test09_1_id;
// extern kz_thread_id_t test09_2_id;
// extern kz_thread_id_t test09_3_id;
int test10_1_main(int argc, char *argv[]);

#endif
