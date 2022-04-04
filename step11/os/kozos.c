#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"
#include "memory.h"

// スレッドの最大個数
#define THREAD_NUM 6
// スレッド名の最大長
#define THREAD_NAME_SIZE 15
// 優先度の個数
#define PRIORITY_NUM 16

// スレッドコンテキスト
// スレッドのコンテキスト保存用の構造体の定義
// そのスレッドの実行を中断・再開する際に，保存が必要なCPU情報
// 汎用レジスタはスタックに保存されるので，TCBにコンテキストとして保存するのはスタックポインタのみ
typedef struct _kz_context
{
	uint32 sp;
} kz_context;

#define KZ_THREAD_FLAG_READY (1 << 0) // レディーフラグ

// タスク・コントロール・ブロック（TCB）
typedef struct _kz_thread
{
	struct _kz_thread *next;		 // レディーキューへの接続に利用するnextポインタ
	char name[THREAD_NAME_SIZE + 1]; // スレッド名
	int priority;					 // 優先度
	char *stack;					 // スレッドのスタック
	uint32 flags;					 // 各種フラグを管理する変数

	// スレッドのスタートアップに渡すパラメータ
	struct
	{
		kz_func_t func; // スレッドのメイン関数
		int argc;		// メイン関数の引数
		char **argv;	// メイン関数の引数
	} init;

	// システムコール用のバッファ
	struct
	{
		kz_syscall_type_t type;
		kz_syscall_param_t *param;
	} syscall;

	// スレッドのコンテキスト情報の保存領域
	kz_context context;
} kz_thread;

// メッセージバッファ
// メッセージ１つに相当
typedef struct _kz_msgbuf
{
	struct _kz_msgbuf *next;
	// メッセージを送信したスレッド
	kz_thread *sender;
	// メッセージのパラメータ保存領域
	struct
	{
		int size;
		char *p;
	} param;
} kz_msgbuf;

// メッセージボックス
// メッセージIDごとに用意される
typedef struct
{
	// 受信待ち状態のスレッド（ウェイクアップされるスレッド）
	kz_thread *receiver;
	// メッセージ・キュー
	kz_msgbuf *head;
	// 終端のエントリを記憶しておくことで，終端のnextに即座に連結できる設計にしている
	kz_msgbuf *tail;

	/*
		H8は16ビットCPUなので，32ビット整数に対しての乗算命令がない．
		よって，構造体のサイズが2の累乗になっていなと，構造体の配列のインデックス計算が乗算が使われて「__mulsi3がない」などのリンクエラーになる場合がある．
		（2の累乗ならシフト演算が利用されるので問題ない）
		対策として，サイズが2の累乗になるようにダミーメンバで調整する
		他構造体で同様のエラーが出た場合には，同様の処理を使用
	*/
	long dummy[1];
} kz_msgbox;

// スレッドのレディー・キュー
// TCBを繋いでいるキュー
// カレントスレッドの実行に区切りがついたらシステムコールが発行され，レディーキューの終端につなげられる
// 実行可能なスレッドを公平に実行する方式（ラウンドロビン・スケジューリング）
// リンクリスト構造
// 構造体にnextポインタという次のエントリを指すのをチェインさせたデータ構造
static struct
{
	kz_thread *head;
	kz_thread *tail;
} readyque[PRIORITY_NUM];

// 現在のスレッド
static kz_thread *current; // カレントスレッド（現在実行中のスレッド, TCBへのポインタ）
// スレッドの実体
// スレッドの個数分用意
static kz_thread threads[THREAD_NUM]; // タスクコントロールブロック（TCB）の実体
// ハンドラの実体
// ハンドラの個数分用意 -> 3個（システムコール，ソフトエラー，シリアル割込み）
static kz_handler_t handlers[SOFTVEC_TYPE_NUM]; // OSが管理する割り込みハンドラ
// メッセージボックスの実体
// メッセージIDの個数分用意
static kz_msgbox msgboxes[MSGBOX_ID_NUM];

// スレッドのディスパッチ用関数（実態はstartup.sにアセンブラで記述）
void dispatch(kz_context *context);

// レディーキューの操作 (リンクリストの操作，ポインタの操作) -----------------------------------------------------------------------------------------

// どこから？
// 『kozos.c』の『syscall_proc関数』，『softerr_intr関数』
// カレントスレッドをレディーキューから抜き出す（デキュー）
static int getcurrent(void)
{
	if (current == NULL)
	{
		return -1;
	}

	// 既にレディーフラグが落ちてるなら無視
	// -> なぜなら，フラグが既に落ちてるならレディーキューにないってことだから
	if (!(current->flags & KZ_THREAD_FLAG_READY))
	{
		// 実行中スレッドがレディーじゃない場合は無視
		return 1;
	}

	// readyque.head = current->next;
	// if (readyque.head == NULL)
	// {
	// 	// headがNULLになった鐡ことは，headとtailが同じとこかつNULLってこと
	// 	readyque.tail = NULL;
	// }
	// カレントスレッドは必ず先頭
	readyque[current->priority].head = current->next;
	if (readyque[current->priority].head == NULL)
	{
		readyque[current->priority].tail = NULL;
	}
	// レディーフラグをマスクする
	// レディーフラグを落とす
	// 最下位ビットのみゼロにする
	current->flags &= ~KZ_THREAD_FLAG_READY;
	// エントリをリンクリストから抜き出したからnextポインタをクリアしておく
	current->next = NULL;

	return 0;
}

// どこから？
// 『kozos.c』の『thread_???関数』
// カレントスレッドをレディーキューに繋げる（エンキュー）
// レディーキューの末尾にカレントスレッドをつなげる
static int putcurrent(void)
{
	if (current == NULL)
	{
		return -1;
	}

	// レディービットを参照し，レディービットが立ってるなら何もしない
	// -> なぜなら，レディービットが立ってるってことは，レディーキューに既に入っているから
	if (current->flags & KZ_THREAD_FLAG_READY)
	{
		return 1;
	}

	// レディーキューの末尾に接続
	if (readyque[current->priority].tail)
	{
		// 現状の尻尾があるならそこにつなげる
		readyque[current->priority].tail->next = current;
	}
	else
	{
		// 尻尾がないなら，先頭ってことでよし
		readyque[current->priority].head = current;
	}
	readyque[current->priority].tail = current; // 尻尾後塵

	// レディーフラグを立てる
	// 最下位ビットがたつ
	current->flags |= KZ_THREAD_FLAG_READY;

	return 0;
}

// スレッドの起動と終了 -------------------------------------------------------------------------------------------------

// どこから？
// 『kozos.c』の『thread_init関数』で呼び出し
static void thread_end(void)
{
	kz_exit();
}

// どこから？
// 『os/startup.s』の『_dispatch関数』
// スレッドを始めるときにディスパッチによって呼び出される処理だ．
// thread_init自体がスレッドだと思えばいい（ディスパッチに呼び出されるのはこの関数）
// kz_startからのディスパッチ -> startスレッドの処理開始(thread_initの処理開始) -> startスレッド内からkz_runからのシステムコールからのディスパッチ -> startスレッドに戻る(thread_initに戻る) -> thread_end()発動 -> kz_exit()発動 -> startスレッドからシステムコールからのディスパッチ (currentが更新されている) -> commandスレッドが始まる(thread_initが始まる）．
static void thread_init(kz_thread *thp)
{
	thp->init.func(thp->init.argc, thp->init.argv); // スレッドのメイン関数（この中にシステムコールとかが含まれている）
	thread_end();									// スレッドが終わった時 (スレッド終了のシステムコールが含まれている)
}

// どこから？
// 『kozos.c』の『call_function（0(run)システムコール, sys_type）
// 新規スレッドの作成（OSの機能，『kz_runシステムコール』で使われる）
/*
	- 1. 空いているTCBの検索
	- 2. TCBの設定
	- 3. メモリ上にスタック領域の確保
	- 4. スタック領域の初期化
	- 5. 新規スレッドをレディーキューに接続
*/
static kz_thread_id_t thread_run(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[])
{
	int i;
	kz_thread *thp;
	uint32 *sp;
	extern char userstack;					// リンカスクリプトで定義されるスタック領域
	static char *thread_stack = &userstack; // -> 使用できるように定義
	// -> staticを付けることで，関数を抜けても値が記憶される
	// -> スタック領域は重複されることなく確保される!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1111

	// 1. 空いているTCBを検索
	for (i = 0; i < THREAD_NUM; i++)
	{
		thp = &threads[i];
		// 見つかった！
		if (!thp->init.func) // 空いているスレッドはinit.funcが登録されていない
			break;
	}

	// 見つからなかった;;
	if (i == THREAD_NUM)
		return -1;

	// 初期化
	memset(thp, 0, sizeof(*thp));
	// ゴミが残ってるかもなので0埋め
	// thpを新規スレッドとして育成

	// 2. TCBの設定
	/*
		- スレッド名前
		- 次のスレッド
		- スレッド起動直後に呼ばれる関数
		- 引数
		- 引数
		- スレッドを再開するためのスタックポインタ
	*/
	strcpy(thp->name, name);  // スレッド名前
	thp->next = NULL;		  // 次のスレッド (新規スレッドはレディーキューの末尾に登録されるから，nextは『NULL』)
	thp->priority = priority; // 優先度
	thp->flags = 0;			  // フラグ
	thp->init.func = func;	  // スレッド起動後に呼ばれる関数
	thp->init.argc = argc;	  // 引数
	thp->init.argv = argv;	  // 引数

	// 3. スレッド用のスタック領域の確保
	memset(thread_stack, 0, stacksize); // リンカスクリプトで定義された場所を０で事前初期化
	thread_stack += stacksize;			// スタックを加算で確保 (0xfff400 ~ 0xfff464)
	// -> staticで定義されているから，重複することなく確保できる
	thp->stack = thread_stack; // スレッドを再開するためのスタックポインタ（コンテキスト情報）(0xfff464を指す)
	// -> スレッドは下方伸長（アドレスが小さい方向に伸びる）ので，確保直後の一番大きいアドレスを初期値として持つ
	// thp->stackはスタックの一番大きい値

	// 4. スタックの初期化（適切な値を設定）
	sp = (uint32 *)thp->stack;	  // 設定するときは間接的なspポインタを利用 -> スレッドのコンテキストとしてTCBに設定
	*(--sp) = (uint32)thread_end; // スレッド終了用 ()

	// プログラムカウンタ（エントリー関数）を設定
	// rteで呼び出される -> CPUによってここをPCとして設定される -> エントリー関数を設定
	// スレッドの優先度がゼロの場合には，『割り込み禁止スレッド』とする
	// -> 最上位の１バイトに0xc0が代入される,　24bit(３バイト)左にずらす
	// CCRが上位１バイトなので，CCRを0xc0にしているのと一緒 -> 立てると割込み無効になる（ CCRの最上位ビット（Iビット，ついでにUIビット））
	// 立てると割り込み禁止になる．
	// これは，割り込み処理を行うスレッドで『割り込み禁止で動作させたい』時に利用するためのもの
	*(--sp) = (uint32)thread_init | ((uint32)(priority ? 0 : 0xc0) << 24);
	// -> スレッドのスタートアップが『thread_init関数』

	// 汎用レジスタ復旧用の設定
	*(--sp) = 0; // ER6
	*(--sp) = 0; // ER5
	*(--sp) = 0; // ER4
	*(--sp) = 0; // ER3
	*(--sp) = 0; // ER2
	*(--sp) = 0; // ER1
				 // -> 『dispatch関数』の呼び出しと逆順に格納
				 //     （ER0から順番に加算しながら復帰するから）

	//『thread_init関数』の引数
	*(--sp) = (uint32)thp; // ER0

	// スレッドのコンテキストを設定
	thp->context.sp = (uint32)sp;
	// -> 『dispatch関数』の引数
	//    『スレッドのコンテキスト』は汎用レジスタの復旧に利用される
	//	   KOZOSではスタックポインタが分かれば復旧できる！！！！！！！！！！！！！！
	// -> 復旧したのち，『thread_init関数』が呼び出される．
	//    その際，汎用レジスタは復旧されている且つ，スタックポインタも最大値を指しており，下方伸長して利用するのに問題なくなっている．

	// システムコールを呼び出したスレッドをレディーキューに戻す
	putcurrent();
	// kz_startから呼ばれた場合は，NULLだから影響はない

	// 5. 新規スレッドをレディーキューに接続（末尾）
	current = thp;
	putcurrent();

	// 新規スレッドのアドレスを返却
	return (kz_thread_id_t)current;
}

// どこから？
// 『kozos.c』の『call_function関数（1(exit)システムコール(KZ_SYSCALL_TYPE_EXIT),sys_type)』，『softerr_intr関数』
// スレッドを終わらせる
// システムコール
static int thread_exit(void)
{
	puts(current->name);
	puts(" EXIT.\n");
	memset(current, 0, sizeof(*current));
	return 0;
}

// どこから？
// 『kozos.c』の『call_function関数（2(wait)システムコール(KZ_SYSCALL_TYPE_WAIT),sys_type)』
// call_functionで呼び出されているから，getcurrentが事前に呼ばれてて，カレントスレッドは取り除かれている
// それを末尾につなげるということは，他のスレッドが一旦スケジューリングされる
// 『CPUを離す』
// スレッドの処理内容が重い時に利用できるシステムコール
// kz_wait()を適当なタイミングで読んで，重い処理を行なっている最中に，他のスレッドを動作させることができる．
static int thread_wait(void)
{
	// レディーキューから一旦外して接続し直すことで，ラウンドロビンで他のスレッドを動作させる
	putcurrent();
	return 0;
}

// どこから？
// 『kozos.c』の『call_function関数（3(sleep)システムコール(KZ_SYSCALL_TYPE_SLEEP),sys_type)』
// 事前にsyscall_procのgetcurrent()でカレントスレッドが抜かれる
// つまり，カレントスレッドをスリープ状態にする
static int thread_sleep(void)
{
	return 0;
}

// どこから？
// 『kozos.c』の『call_function関数（4(wakeup)システムコール(KZ_SYSCALL_TYPE_WAKEUP),sys_type)』
// スレッドをレディー状態に戻すことをウェイクアップ
// 指定したidをウェイクアップ
static int thread_wakeup(kz_thread_id_t id)
{
	// ウェイクアップを呼び出したスレッドをレディーキューに戻す
	putcurrent();

	// 指定されたスレッドをレディーキューに接続してウェイクアップ
	current = (kz_thread *)id;
	putcurrent();

	return 0;
}

// どこから？
// 『kozos.c』の『call_function関数（5(getid)システムコール(KZ_SYSCALL_TYPE_GETID),sys_type)』
// thread_waitとは同じ動作
// しかし，システムコールを実行しているスレッド自身のスレッドIDが返る
// 他のスレッドを動作させつつ，IDを取得
static kz_thread_id_t thread_getid(void)
{
	putcurrent();
	// TCBのアドレスがスレッドIDとなる
	return (kz_thread_id_t)current;
}

// どこから？
// 『kozos.c』の『call_function関数（6(chpri)システムコール(KZ_SYSCALL_TYPE_CHPRI),sys_type)』
// カレントスレッドを，優先度を変更してレディーキューに接続する．
static int thread_chpri(int priority)
{
	int old = current->priority;
	if (priority >= 0)
		// 優先度変更
		current->priority = priority;
	// 新しい優先度のレディーキューに繋ぎ直す
	putcurrent();
	return old;
}

// どこから？
// 『call_function関数』
// 動的メモリ獲得
static void *thread_kmalloc(int size)
{
	// システムコールを呼び出した関数をレディーキューに戻す
	putcurrent();
	return kzmem_alloc(size);
}

// どこから？
// 『call_function関数』
// メモリ解放
// charで受け取るんか．．
static int thread_kmfree(char *p)
{
	kzmem_free(p);
	putcurrent();
	return 0;
}

// システムコールの実行 -------------------------------------------------------------------------------------------------

// どこから？
// 『kozos.c』の『syscall_proc関数』
// システムコールの種別に応じた処理が行われる．
// -> thread_? が呼ばれる
static void call_function(kz_syscall_type_t sys_type, kz_syscall_param_t *p)
{
	switch (sys_type)
	{
	case KZ_SYSCALL_TYPE_RUN: // kz_run
		p->un.run.ret = thread_run(p->un.run.func, p->un.run.name, p->un.run.priority, p->un.run.stacksize, p->un.run.argc, p->un.run.argv);
		break;
	case KZ_SYSCALL_TYPE_EXIT: // kz_exit
		thread_exit();
		break;
	case KZ_SYSCALL_TYPE_WAIT: // kz_wait
		p->un.wait.ret = thread_wait();
		break;
	case KZ_SYSCALL_TYPE_SLEEP: // kz_sleep
		p->un.sleep.ret = thread_sleep();
		break;
	case KZ_SYSCALL_TYPE_WAKEUP: // kz_wakeup
		p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
		break;
	case KZ_SYSCALL_TYPE_GETID: // kz_getid
		p->un.getid.ret = thread_getid();
		break;
	case KZ_SYSCALL_TYPE_CHPRI: // kz_chpri
		p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
		break;
	case KZ_SYSCALL_TYPE_KMALLOC: // kz_kmalloc
		p->un.kmalloc.ret = thread_kmalloc(p->un.kmalloc.size);
		break;
	case KZ_SYSCALL_TYPE_KMFREE: // kz_kmree
		p->un.kmfree.ret = thread_kmfree(p->un.kmfree.p);
		break;
	default:
		break;
	}
}

// どこから？
// 『kozos.c』の『syscall_intr関数』
static void syscall_proc(kz_syscall_type_t sys_type, kz_syscall_param_t *p)
{
	getcurrent(); // システムコールを呼び出したスレッドをレディーキューから外した状態で処理関数を呼び出す -> システムコールを呼び出したスレッドをそのまま動作継続させたい場合は，処理関数の内部でputcurrent()がいる
	// ちなみに外すだけで，currentは更新していない
	call_function(sys_type, p); // システムコールの処理関数呼び出し
}

// どこから？
// 『kozos.c』の『thread_intr関数』（handlers[sof_type]()）
// 割り込みハンドラ
static void syscall_intr(void)
{
	syscall_proc(current->syscall.type, current->syscall.param);
}

// どこから？
// 『kozos.c』の『thread_intr関数』
static void schedule(void)
{
	int i;

	// 優先度の高い順（優先度の数値の小さい順）にレディーキューを見て，動作可能なスレッドを検索する
	for (i = 0; i < PRIORITY_NUM; i++)
	{
		// 見つかったら終わり
		if (readyque[i].head)
			break;
	}

	// 次に実行するスレッドがなかったら終わり
	if (i == PRIORITY_NUM)
		kz_sysdown();

	// カレントスレッドに設定
	current = readyque[i].head;
}

// どこから？
// 『kozos.c』の『thread_intr関数』（handlers[sof_type]()）
// 割り込みハンドラ
static void softerr_intr(void)
{
	puts(current->name);
	puts(" DOWN.\n");
	getcurrent();  // このソフトエラー割込みを呼び出したスレッド（current）をレディーキューから外す
	thread_exit(); // スレッド終了
}

// どこから？
// 『interrupt.c』の『interrupt関数』から（thread_intrがSOFTVECS配列に登録されている）
// 割込みハンドラ＝＝OSの処理
static void thread_intr(softvec_type_t sof_type, unsigned long sp)
{
	// カレントスレッドのコンテキストを保存
	current->context.sp = sp;

	// syscall_intr, softerr_intr （システムコール，ソフトウェアエラー）
	if (handlers[sof_type])
		handlers[sof_type]();

	// レディーキューの先頭をカレントスレッドとしてcurrentに代入（ラウンドロビン）
	// 次の実行するスレッドがレディーキューになかったらここで処理が終わる．
	schedule();

	// カレントスレッドのディスパッチ (引数としてスレッドのスタック領域（コンテキスト情報）のアドレス)
	// -> 割込みハンドラ（thread_intr関数）は割込みスタック領域を使用している．スレッドの処理を再開するとき，スタックをスレッドスタック領域に変更する必要がある）
	dispatch(&current->context);
}

// 割り込みハンドラの登録 -------------------------------------------------------------------------------------------------

static void thread_intr(softvec_type_t sof_type, unsigned long sp);
// どこから？
// 『kozos.c』の『kz_start関数』
// ↓ 現状は１つだが増えるかも？
// SOFTVECS配列...thread_intr
// handlers配列...syscall_intr, softerr_intr (システムコール or ソフトウェアエラー)
static int setintr(softvec_type_t sof_type, kz_handler_t handler)
{

	softvec_setintr(sof_type, thread_intr); // ソフトウェア割込みベクタにthread_intr設定

	handlers[sof_type] = handler; // ハンドラーに登録

	return 0;
}

// 初期スレッドの起動 -------------------------------------------------------------------------------------------------------

// どこから？
// 『main.c』の『main関数』
void kz_start(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[])
{
	// メモリプールの初期化
	kzmem_init();

	// カレントスレッドの初期化
	current = NULL;
	// カレントスレッドは実行中のスレッドのこと
	// NULLにすることで何もないことを表現

	// レディーキューの初期化
	// readyque.head = readyque.tail = NULL;
	// 配列になったので，memset()でゼロクリア
	memset(readyque, 0, sizeof(readyque));
	// レディーキューは実行可能なスレッド（実行中を含む）のこと
	// 先頭と末尾をNULLにすることでスレッドが繋がれていないことを表現

	// タスクコントロールブロック(TCB)の初期化
	memset(threads, 0, sizeof(threads));
	// TCBはスレッド(タスク)の情報を格納する領域のこと
	// スレッドがないので全てを０で初期化

	// 割込みハンドラの初期化
	memset(handlers, 0, sizeof(handlers));
	// 割込みハンドラは誤動作防止のためすべて０で初期化
	// syscall_intrとsofterr_intrを登録する

	// 割込みハンドラの登録
	setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr); // ダウン要因発生, 『softerr_intr』関数が呼ばれるように登録
	setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr); // システムコール, 『syscall_intr』関数が呼ばれるように登録
	// -> 登録した割込みハンドラは直接呼ばれない．割込み要因がシステムコールであろうがダウン要因発生だろうが，必ず『thread_intr関数』が呼ばれる．
	//    thread_intr関数の中で，『syscall_intr関数』『softerr_intr関数』が呼び分けられる

	// 初期スレッドの新規作成
	current = (kz_thread *)thread_run(func, name, priority, stacksize, argc, argv);
	// （システムコールを使って初期スレッドを作成したいが，システムコールはスレッドからしか呼べない仕様になっている...）
	// -> OSの機能(『thread_run関数』)を直接使用して初期スレッドを生成
	// thread_run関数の説明はその関数にLet's Go

	// スレッド処理再開（上の『thread_run』で新規作成したスレッドを処理開始）
	dispatch(&current->context);
	// 『startup.s』に書かれている
}

// どこから？
// 『kozos.c』の『schedule関数』
void kz_sysdown(void)
{
	puts("system error!\n");
	while (1)
		;
}

// どこから？
// 『syscall.c』の『kz_run関数』と『kz_exit関数』
void kz_syscall(kz_syscall_type_t sys_type, kz_syscall_param_t *param)
{
	current->syscall.type = sys_type; // 0(run) or 1(exit)
	current->syscall.param = param;
	// TRAP0命令発効
	asm volatile("trapa #0");
	/*
	CPUによってPCとCCRがスタックに退避
	↓
	『intr_syscall関数』 (『vector.c』で8番目のintr_syscall, 『intrS』に定義)
		- 汎用レジスタを『スレッドスタック領域（0xfff400)』に退避
		- スタック領域を『割込みスタック領域(0xffff00)』に切り替え
	↓
	『interrupt関数』　(『interrupt.c』で定義)
		- ソフトウェア割込みハンドラが登録されていたら，　割込みハンドラを呼ぶ
	↓
	『thread_intr関数』 (割込みハンドラ==OS)
		- システムコールを発行したスレッドのスタックポインタを保持
		↓
		- 割込み要因に応じて割込みハンドラの呼び分け （システムコールなら『syscall_intr関数』，　ダウン要因発生なら『soft_intr関数』が呼ばれる）
			『syscall_intr関数』
				- 以下の処理がシステムコール特有の『割込みハンドラ』だということを明示する（ラッパー関数）
			↓
			『syscall_proc関数』
				- レディーキューからカレントスレッドを抜く（システムコール種別に依存しない処理）
				- システムコール種別に応じた処理を行う（『call_functions関数』)
			↓
			『call_functions関数』
				- システムコール種別に応じた処理
					- 『thread_run関数』．．．新規スレッドができてレディーキューの末尾に繋がれる
					- 『thread_exit関数』．．．currentを真っ白にする
		↓
		- スレッドのスケジューリング
			『schedule関数』
				- スレッドのスケジューリング（ラウンドロビン方式なので，レディーキューの先頭が次に実行されるスレッドになる）
		↓
		- スレッドのディスパッチ
			『dispatch関数』
				- カレントスレッドのスタック領域から以下を復旧する．
					- 汎用レジスタ
					- プログラムカウンタ
					- CCR
				- スレッドの処理を再開する．
	*/
}
