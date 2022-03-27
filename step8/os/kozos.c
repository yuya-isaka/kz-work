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

// スレッドを始めるときにディスパッチによって呼び出される処理だ．
// thread_init自体がスレッドだと思えばいい（ディスパッチに呼び出されるのはこの関数）
// kz_startからのディスパッチ -> startスレッドの処理開始(thread_initの処理開始) -> startスレッド内からkz_runからのシステムコールからのディスパッチ -> startスレッドに戻る(thread_initに戻る) -> thread_end()発動 -> kz_exit()発動 -> startスレッドからシステムコールからのディスパッチ (currentが更新されている) -> commandスレッドが始まる(thread_initが始まる）．
static void thread_init(kz_thread *thp)
{
	thp->init.func(thp->init.argc, thp->init.argv); // スレッドのメイン関数（この中にシステムコールとかが含まれている）
	thread_end();									// スレッドが終わった時 (スレッド終了のシステムコールが含まれている)
}

// 新規スレッドの作成（OSの機能，『kz_runシステムコール』で使われる）
static kz_thread_id_t thread_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
	int i;
	kz_thread *thp;
	uint32 *sp;
	extern char userstack;					// リンカスクリプトで定義されるスタック領域
	static char *thread_stack = &userstack; // -> 使用できるように定義

	// 空いているTCBを検索
	for (i = 0; i < THREAD_NUM; i++)
	{
		thp = &threads[i];
		// 見つかった！
		if (!thp->init.func)
			break;
	}

	// 見つからなかった;;
	if (i == THREAD_NUM)
		return -1;

	// 初期化
	memset(thp, 0, sizeof(*thp));
	// thpを新規スレッドとして育成

	// TCBの設定
	strcpy(thp->name, name); // 名前
	thp->next = NULL;		 // 次のスレッド
	thp->init.func = func;	 // スレッドの処理関数
	thp->init.argc = argc;	 // 引数
	thp->init.argv = argv;	 // 引数

	// スレッド用のスタック領域の確保
	memset(thread_stack, 0, stacksize); // リンカスクリプトで定義された場所を０で事前初期化
	thread_stack += stacksize;			// スタックを加算で確保 (0xfff400 ~ 0xfff464)
	thp->stack = thread_stack;			// TCBのスタックに設定 (0xfff464を指す)

	// スタックの初期化（適切な値を設定）
	sp = (uint32 *)thp->stack;	  // 設定するときは間接的なspポインタを利用 -> スレッドのコンテキストとしてTCBに設定
	*(--sp) = (uint32)thread_end; // スレッド終了用 ()

	*(--sp) = (uint32)thread_init; // rteで呼び出される -> CPUによってここをPCとして設定される -> エントリー関数を設定
	// -> スレッドのスタートアップが『thread_init関数』

	*(--sp) = 0; // ER6
	*(--sp) = 0; // ER5
	*(--sp) = 0; // ER4
	*(--sp) = 0; // ER3
	*(--sp) = 0; // ER2
	*(--sp) = 0; // ER1
				 // -> 『dispatch関数』の呼び出しと逆順に格納

	//『thread_init関数』の引数
	*(--sp) = (uint32)thp; // ER0

	// スレッドのコンテキストを設定
	thp->context.sp = (uint32)sp;
	// -> 『dispatch関数』の引数
	//    『スレッドのコンテキスト』は汎用レジスタの復旧に利用される
	//	   スタックポインタが分かれば復旧できる

	putcurrent(); // 現在のこのthread_runを呼び出したスレッドをレディーキューに戻す, kz_startから呼び出したやつはcurrent==NULLだから，特に影響はない

	current = thp;
	putcurrent(); // 新しく追加するスレッドを尻尾につなげる

	return (kz_thread_id_t)current; // アドレスをスレッドIDとして戻す
}

// スレッドを終わらせる
static int thread_exit(void)
{
	puts(current->name);
	puts(" EXIT.\n");
	memset(current, 0, sizeof(*current));
	return 0;
}

// システムコールの実行 -------------------------------------------------------------------------------------------------

// システムコールの種別に応じた処理が行われる．
static void call_function(kz_syscall_type_t type, kz_syscall_param_t *p)
{
	switch (type)
	{
	case KZ_SYSCALL_TYPE_RUN: // kz_run
		p->un.run.ret = thread_run(p->un.run.func, p->un.run.name, p->un.run.stacksize, p->un.run.argc, p->un.run.argv);
		break;
	case KZ_SYSCALL_TYPE_EXIT: // kz_exit
		thread_exit();
		break;
	default:
		break;
	}
}

static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
	getcurrent(); // システムコールを呼び出したスレッドをレディーキューから外した状態で処理関数を呼び出す -> システムコールを呼び出したスレッドをそのまま動作継続させたい場合は，処理関数の内部でputcurrent()がいる
	// ちなみに外すだけで，currentは更新していない
	call_function(type, p); // システムコールの処理関数呼び出し
}

// 割り込み処理 -------------------------------------------------------------------------------------------------------

static void schedule(void)
{
	if (!readyque.head) // 次に実行するスレッドがなかったら終わり
		kz_sysdown();

	current = readyque.head;
}

// 登録するシステムコール関数
static void syscall_intr(void)
{
	syscall_proc(current->syscall.type, current->syscall.param);
}

// 登録するソフトエラー関数
static void softerr_intr(void)
{
	puts(current->name);
	puts(" DOWN.\n");
	getcurrent();  // このソフトエラー割込みを呼び出したスレッド（current）をレディーキューから外す
	thread_exit(); // スレッド終了
}

// OSの処理
// OSの本体
static void thread_intr(softvec_type_t type, unsigned long sp)
{
	current->context.sp = sp; // カレントスレッドのコンテキストを保存

	// 1. スレッドを生成して，レディーキューに繋げる．
	// 2. スレッドを削除
	if (handlers[type]) // 割込みの種類ごとに処理
		handlers[type]();
	// これが終わった時点では，新しく追加されたスレッドが，currentになっている
	// レディーキューの先頭は, 元々のスレッド

	schedule(); // レディーキューの先頭をカレントスレッドとしてcurrentに代入（ラウンドロビン）
	// 次の実行するスレッドがレディーキューになかったらここで処理が終わる．

	dispatch(&current->context); // カレントスレッドのディスパッチ (引数としてスレッドのコンテキスト情報の領域のアドレス)
								 // カレンストレッドのスタックポインタ情報を渡しておかないと，割込み処理を実行したあと，どのスタックを用いればよいかわからなくなる
}

// 割り込みハンドラの登録 -------------------------------------------------------------------------------------------------

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
	// カレントスレッドの初期化
	current = NULL;
	// カレントスレッドは実行中のスレッドのこと
	// NULLにすることで何もないことを表現

	// レディーキューの初期化
	readyque.head = readyque.tail = NULL;
	// レディーキューは実行可能なスレッド（実行中を含む）のこと
	// 先頭と末尾をNULLにすることでスレッドが繋がれていないことを表現

	// タスクコントロールブロック(TCB)の初期化
	memset(threads, 0, sizeof(threads));
	// TCBはスレッド(タスク)の情報を格納する領域のこと
	// スレッドがないので全てを０で初期化

	// 割込みハンドラの初期化
	memset(handlers, 0, sizeof(handlers));
	// 割込みハンドラは誤動作防止のためすべて０で初期化

	// 割込みハンドラの登録
	setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr); // ダウン要因発生, 『softerr_intr』関数が呼ばれるように登録
	setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr); // システムコール, 『syscall_intr』関数が呼ばれるように登録
	// -> 登録した割込みハンドラは直接呼ばれない．割込み要因がシステムコールであろうがダウン要因発生だろうが，必ず『thread_intr関数』が呼ばれる．
	//    thread_intr関数の中で，『syscall_intr関数』『softerr_intr関数』が呼び分けられる

	// 初期スレッドの作成
	current = (kz_thread *)thread_run(func, name, stacksize, argc, argv);
	// （システムコールを使って初期スレッドを作成したいが，システムコールはスレッドからしか呼べない仕様になっている...）
	// -> OSの機能(『thread_run関数』)を直接使用して初期スレッドを生成
	// thread_run関数の説明はその関数にLet's Go

	INTR_ENABLE; // 本にはなかったけど，ここらへんでするべき

	dispatch(&current->context);
}

void kz_sysdown(void)
{
	puts("system error!\n");
	while (1)
		;
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
	current->syscall.type = type;
	current->syscall.param = param;
	asm volatile("trapa #0");
	// -> CPUがPCとCCRをスタックに移動させる -> スレッドの処理が停止
	// -> vector.cで8番目のintr_syscallが呼ばれる -> 割込みの処理開始
	// -> intr.Sに定義したintr_syscallがinterruptを実行
	// -> interruptはSOFTVECSから指定された割り込みを実行 (ここではthread_intrが実行される) -> ここからがOSの処理
	// -> thread_intrでhandler配列に登録された関数を実行（ここではsyscall_intrが実行される）
	// -> syscall_intrで種類に応じて関数実行
	// -> syscall_intrでは，システムコールを呼び出したスレッドをレディーキューから取り除いた後，thread_runかthread_exitを実行
	// -> thread_runならスレッドが生成されて，レディーキューに繋がれる，thread_exitならcurrentを真っ白にする
	// -> thread_intrに戻ってきて，scheduleしてスレッドをcurrentにセット，ディスパッチしてスレッドの処理を再開する． (ここで２回目のディスパッチ, startスレッドをdispatchしたから，戻る)
}