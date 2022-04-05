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
	// ブロックがどのプールに所属するか，情報を持たせておく．メモリ解放の時に使う
	int size;
} kzmem_block;

/*
	メモリ・プールの構造体
	ブロックサイズ（16, 32, 64）ごとに確保される
*/
typedef struct _kzmem_pool
{
	// メモリプールのサイズ（メモリ解放の時に，対象のブロックがどのプールに属するか確認するときに使う）
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

// どこから？
// 『kzmem_init関数』
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

	/*
		- mpp自体のアドレスは，　プールごとの解放済みリストの先頭アドレス
		- mppに格納するアドレスは， 領域の構造体のアドレス
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
		// ③『格納後に初期化』 (ヘッダだけ初期化してないか？, mp+p->size分の初期化はしていないはず．．)
		memset(mp, 0, sizeof(*mp));
		mp->size = p->size;
		// 解放済みリンクリストの次
		// ① 『まずアドレスを決める』
		mpp = &(mp->next);
		// 切り分け更新
		// 更新する際は，用意しているメモリプールサイズを確保（ヘッダだけ(sizeof(*mp))だけではダメ）
		mp = (kzmem_block *)((char *)mp + p->size);
		// 最初に切り分ける箇所を更新
		area += p->size;
	}

	return 0;
}

// どこから？
// 『kozos.c』の『kz_start関数』
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

// どこから？
// 『kozos.c』の『thread_kmalloc関数』『sendmsg関数』<-メッセージのバッファで使用
// 動的メモリの確保
void *kzmem_alloc(int size)
{
	int i;
	kzmem_block *mp;
	kzmem_pool *p;

	for (i = 0; i < MEMORY_AREA_NUM; i++)
	{
		// なぜポインタ？
		// コピーによる余計な変数の使用をなくすためか
		p = &pool[i];
		// 要求されたサイズが収まるかチェック
		// 用意したメモリプールサイズ - メモリヘッダ
		if (size <= p->size - sizeof(kzmem_block))
		{
			// 解放済み領域がない（メモリブロック不足）
			if (p->free == NULL)
			{
				// メモリ枯渇
				kz_sysdown();
				return NULL;
			}
			// 解放済みリンクリストから領域を取得
			mp = p->free;
			// リンクリストの更新（先頭を変える）
			// リンクリストから抜き出す
			p->free = p->free->next;
			// リンクリストから抜き出したmpはnextを持たない
			// 安全のためにNULLクリア
			mp->next = NULL;

			// 実際に利用可能な領域は，メモリブロック構造体（メモリヘッダ）の直後の領域になる．
			// ので直後のアドレスを返す．
			// 初期化はされてなさそう？
			return mp + 1;
		}
	}

	// 指定されたサイズの領域を格納できるメモリプールが無い
	// 64より大きい
	kz_sysdown();
	return NULL;
}

// どこから？
// 『kozos.c』の『thread_kmfree関数』
// データ領域を指すメモリアドレスが渡される．汎用ポインタで受け取る．
void kzmem_free(void *mem)
{
	int i;
	kzmem_block *mp;
	kzmem_pool *p;

	// 領域の直前にある（はずの）メモリブロック構造体を取得（ヘッダ）
	// kzmem_blockポインタで『キャスト』してから-1することで，ヘッダの先頭を取得できる．
	mp = ((kzmem_block *)mem - 1);

	for (i = 0; i < MEMORY_AREA_NUM; i++)
	{
		p = &pool[i];
		// どのプール(どのサイズ)か調べる
		if (mp->size == p->size)
		{
			// 解放済みリンクリストの先頭につなげる（再利用可能になる）
			// mpの次に今の先頭を
			mp->next = p->free;
			// 先頭をmpに更新
			p->free = mp;
			return;
		}
	}

	kz_sysdown();
}
