#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"

// TCBの個数
#define THEREAD_NUM 6
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
}

static void thread_init(kz_thread *thp)
{
}

static kz_thread_id_t thread_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
}

static int thread_exit(void)
{
}

// 割り込みハンドラの登録 -------------------------------------------------------------------------------------------------

static int setintr(softvec_type_t type, kz_handler_t handler)
{
}

// システムコールの実行 -------------------------------------------------------------------------------------------------

static void call_function(kz_syscall_type_t type, kz_syscall_param_t *p)
{
}

static void syscall_prc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
}

// 割り込み処理 -------------------------------------------------------------------------------------------------------

static void schedule(void)
{
}

static void syscall_intr(void)
{
}

static void softerr_intr(void)
{
}

static void thread_intr(softvec_type_t type, unsigned long sp)
{
}

// 初期スレッドの起動 -------------------------------------------------------------------------------------------------------

void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
}

void kz_sysdown(void)
{
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
}