#include "defines.h"
#include "kozos.h"
#include "lib.h"
#include "memory.h"

/*
	メモリ・ヘッダ
	メモリ・ブロック構造体
	メモリ領域の先頭に付加されるヘッダ
	（獲得された各領域は，先頭に以下の構造体を持っている）
*/
typedef struct _kzmem_block
{
	// 次のメモリ領域のポインタ
	struct _kzmem_block *next;
	// 実際の大きさ
	int size;
} kzmem_block;

/*
	メモリ・プールの構造体
	ブロックサイズ（16, 32, 64）ごとに確保される
*/
typedef struct _kzmem_pool
{
	// 16 or 32 or 64
	// このサイズにはメモリ・ヘッダも含まれているため，メモリブロック中で実際に利用できるのはメモリ・ヘッダ（sizeof(kzmem_block)==8）を引いたバイト数になる．
	int size;
	// リンクリストの最大数
	int num;
	// 解放済みリンクリストの先頭のポインタ
	kzmem_block *free;
} kzmem_pool;

// メモリ・プールの定義（個々のヘッダ・サイズ・個数）
// ３種類定義（16, 32, 64 バイト）
// あらかじめ静的に確保する領域
static kzmem_pool pool[] = {
	{16, 8, NULL},
	{32, 8, NULL},
	{64, 4, NULL},
};

// メモリ・プールの種類の個数（今回は３種類）
#define MEMORY_AREA_NUM (sizeof(pool) / sizeof(*pool))

// メモリプールの初期化
static int kzmem_init_pool(kzmem_pool *p)
{
	int i;
	kzmem_block *mp;
	// 『ポインタ変数のアドレス』を格納
	kzmem_block **mpp;
	// リンカスクリプトで定義されている動的メモリ用の領域を取得
	extern char freearea;
	// 静的に保持
	static char *area = &freearea;

	// 切り分け
	mp = (kzmem_block *)area;
	// -> mp自体はアドレス
	/*
		実体を確保
	mp -> -------------
		 |            |
		 |            |
		 |            |
		 --------------
	*/

	// 解放済みリストの先頭
	// ①『まずアドレスを決める』
	mpp = &p->free;

	for (i = 0; i < p->num; i++)
	{
		// 切り分けた領域を解放済みリンクリストの先頭に格納
		// ②『あとで中身を格納』
		*mpp = mp;
		// 切り分けた領域を初期化
		memset(mp, 0, sizeof(*mp));
		mp->size = p->size;
		// 解放済みリンクリストの次
		// ① 『まずアドレスを決める』
		mpp = &(mp->next);
		// 切り分け更新
		mp = (kzmem_block *)((char *)mp + p->size);
		// 最初に切り分ける箇所を更新
		area += p->size;
	}

	return 0;
}

// 動的メモリの初期化
int kzmem_init(void)
{
	int i;
	for (i = 0; i < MEMORY_AREA_NUM; i++)
	{
		// 各メモリプールを初期化
		kzmem_init_pool(&pool[i]);
	}
	return 0;
}