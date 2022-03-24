#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"

// TCBの個数
#define THREAD_NUM 6
// スレッド名の最大長
#define THREAD_NAME_SIZE 15

// スレッドコンテキスト
// スレッドのコンテキスト保存用の構造体の定義
// そのスレッドの実行を中断・再開する際に，保存が必要なCPU情報
// 汎用レジスタはスタックに保存されるので，TCBにコンテキストとして保存するのはスタックポインタのみ
typedef struct _kz_context
{
	uint32 sp;
} kz_context;

// タスク・コントロール・ブロック（TCB）
typedef struct _kz_thread
{
	struct _kz_thread *next;		 // レディーキューへの接続に利用するnextポインタ
	char name[THREAD_NAME_SIZE + 1]; // スレッド名
	char *stack;					 // スレッドのスタック

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
} readyque;

static kz_thread *current;						// カレントスレッド（現在実行中のスレッド, TCBへのポインタ）
static kz_thread threads[THREAD_NUM];			// タスクコントロールブロック（TCB）の実態
static kz_handler_t handlers[SOFTVEC_TYPE_NUM]; // OSが管理する割り込みハンドラ

// スレッドのディスパッチ用関数（実態はstartup.sにアセンブラで記述）
void dispatch(kz_context *context);

// レディーキューの操作 (リンクリストの操作，ポインタの操作) -----------------------------------------------------------------------------------------

// カレントスレッドをレディーキューから抜き出す（デキュー）
static int getcurrent(void)
{
	if (current == NULL)
	{
		return -1;
	}

	// カレントスレッドは必ず先頭
	readyque.head = current->next;
	if (readyque.head == NULL)
	{
		// headがNULLになった鐡ことは，headとtailが同じとこかつNULLってこと
		readyque.tail = NULL;
	}
	current->next = NULL; // エントリをリンクリストから抜き出したからnextポインタをクリアしておく

	return 0;
}

// カレントスレッドをレディーキューに繋げる（エンキュー）
// レディーキューの末尾にカレントスレッドをつなげる
static int putcurrent(void)
{
	if (current == NULL)
	{
		return -1;
	}

	// レディーキューの末尾に接続
	if (readyque.tail)
	{
		// 現状の尻尾があるならそこにつなげる
		readyque.tail->next = current;
	}
	else
	{
		// 尻尾がないなら，先頭ってことでよし
		readyque.head = current;
	}
	readyque.tail = current; // 尻尾後塵

	return 0;
}

// スレッドの起動と終了 -------------------------------------------------------------------------------------------------

static void thread_end(void)
{
	kz_exit();
}

// スレッドの実行関数
static void thread_init(kz_thread *thp)
{
	// スレッドのメイン処理を実行
	thp->init.func(thp->init.argc, thp->init.argv);
	// スレッドを終わらせる．
	thread_end();
}

// スレッドの生成
// 初期スレッドだったら，(starts_thread, "start", 0x100, 0, NULL)
static kz_thread_id_t thread_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
	// 空きのTCBを検索
	int i;
	kz_thread *thp;
	for (i = 0; i < THREAD_NUM; i++)
	{
		thp = &threads[i];
		if (!thp->init.func)
			break;
	}
	if (i == THREAD_NUM)
		return -1;

	memset(thp, 0, sizeof(*thp));

	// TCB設定
	strcpy(thp->name, name);
	thp->next = NULL;
	thp->init.func = func; // スレッドのメイン関数
	thp->init.argc = argc;
	thp->init.argv = argv;

	// スタック初期化 (dispatchで呼び出す前の準備)
	extern char userstack;
	static char *thread_stack = &userstack;
	memset(thread_stack, 0, stacksize);
	thread_stack += stacksize;
	thp->stack = thread_stack;

	uint32 *sp;
	sp = (uint32 *)thp->stack;
	// *(--sp) = (uint32)thread_end;

	// dispatchで呼ばれるのと逆順にいれていく
	*(--sp) = (uint32)thread_init; // rteで呼び出される
	*(--sp) = 0;				   // ER6
	*(--sp) = 0;				   // ER5
	*(--sp) = 0;				   // ER4
	*(--sp) = 0;				   // ER3
	*(--sp) = 0;				   // ER2
	*(--sp) = 0;				   // ER1
	*(--sp) = (uint32)thp;		   // thread_initの引数

	thp->context.sp = (uint32)sp; // dispatchに渡される？

	putcurrent(); // 現在のこのthread_runを呼び出したスレッドをレディーキューに戻す
				  // システムコールを呼び出したカレントスレッドは，レディーキューから一旦外した状態でシステムコールの処理関数が呼ばれる
				  // なので，システムコール(kz_run)を呼び出したカレントスレッドを，戻してあげるy必要がある．
				  // kz_start の時は，カレントスレッドはNULLなので特に影響のない処理となる．

	current = thp;
	putcurrent(); // 新しく作ったスレッドをレディーキューに追加

	return (kz_thread_id_t)current; // アドレスをスレッドIDとして戻す
}

// スレッドを終わらせる
// kz_exit か ソフトエラー割込みで発生
// スレッドの終わりで，thread_end()->kz_exit()->thread_exit()という流れで呼ばれる．
static int thread_exit(void)
{
	puts(current->name);
	puts(" EXIT.\n");
	memset(current, 0, sizeof(*current));
	return 0;
}

// システムコールの実行 -------------------------------------------------------------------------------------------------

static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
}

// 割り込み処理 -------------------------------------------------------------------------------------------------------

static void schedule(void)
{
	if (!readyque.head) // 次に実行するスレッドがなかったら終わり
		kz_sysdown();

	current = readyque.head;
}

// handler
// 登録するシステムコール関数
static void syscall_intr(void)
{
	getcurrent(); // システムコールを呼び出したスレッドをレディーキューから外した状態で処理関数を呼び出す -> システムコールを呼び出したスレッドをそのまま動作継続させたい場合は，処理関数の内部でputcurrent()がいる
	// KOZOSの設計ではこうなっている
	// システムコールを呼び出したカレントスレッドは，レディーキューから一旦外した状態でシステムコールの処理関数が呼ばれる
	kz_syscall_type_t type = current->syscall.type;
	kz_syscall_param_t *p = current->syscall.param;
	switch (type)
	{
	case KZ_SYSCALL_TYPE_RUN: // kz_run
		// threadの結果をretに格納
		p->un.run.ret = thread_run(p->un.run.func, p->un.run.name, p->un.run.stacksize, p->un.run.argc, p->un.run.argv);
		break;
	case KZ_SYSCALL_TYPE_EXIT: // kz_exit
		thread_exit();
		break;
	default:
		break;
	}
}

// handler
// 登録するソフトエラー関数
static void softerr_intr(void)
{
	puts(current->name);
	puts(" DOWN.\n");
	getcurrent();  // このソフトエラー割込みを呼び出したスレッド（current）をレディーキューから外す
	thread_exit(); // スレッド終了
}

// 割込みがあったら起動する関数
static void thread_intr(softvec_type_t type, unsigned long sp)
{
	current->context.sp = sp; // カレントスレッドのコンテキストを保存
	// なぜ保存する？
	// 割込みを実行したスレッド（current）は，handlers[type]()で，getcurrent()で取り除かれるから，残し解かないとわからなくなる

	if (handlers[type]) // 割込みの種類ごとに処理
		handlers[type]();
	// thread_runが終わったらここに戻ってくる

	schedule(); // レディーキューの先頭をカレントスレッドとしてcurrentに代入（ラウンドロビン）

	// scheduleされた新しいスレッドを実行（スレッドはすべて，dispatchによって実行できる形式でthread_runで保存されているという設定）
	dispatch(&current->context); // カレントスレッドのディスパッチ (引数としてスレッドのコンテキスト情報の領域のアドレス)
								 // カレンストレッドのスタックポインタ情報を渡しておかないと，割込み処理を実行したあと，どのスタックを用いればよいかわからなくなる

	// ここには返ってこないらしい
}

static int setintr(softvec_type_t type, kz_handler_t handler)
{
	extern void thread_intr(softvec_type_t type, unsigned long sp);

	softvec_setintr(type, thread_intr); // ソフトウェア割込みベクタにthread_intr設定

	handlers[type] = handler; // ハンドラーに登録

	return 0;
}

// 初期スレッドの起動 -------------------------------------------------------------------------------------------------------

void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
	current = NULL;

	readyque.head = readyque.tail = NULL;
	memset(threads, 0, sizeof(threads));
	memset(handlers, 0, sizeof(handlers));

	setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr); // 1
	setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr); // 0

	// 新しく作ったスレッドが帰ってくる
	current = (kz_thread *)thread_run(func, name, stacksize, argc, argv);

	// 初期スレッドの動作開始
	dispatch(&current->context);
}

// OS内部の致命的なエラーが発生した時停止させるサービス関数
void kz_sysdown(void)
{
	puts("system error!\n");
	while (1)
		;
}

// 割込みベース
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
	current->syscall.type = type;
	current->syscall.param = param;
	// 8番の割り込み
	// ブートローダの割込みベクタのintr_syscall (vector.c)
	asm volatile("trapa #0"); // 割込みを発生？
							  // -> 結果的に，SOFTVEC _TYPE _SYSCALLのソフトウェア割込みベクタが実行
							  // -> thread_intr()
							  // -> syscall_intr()
							  // run:  -> thread_run()
							  // exit: -> thread_end()
}