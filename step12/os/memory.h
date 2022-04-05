#ifndef _KOZOS_MEMORY_H_INCLUDED_
#define _KOZOS_MEMORY_H_INCLUDED_

// メモリプールの初期化（動的に確保するメモリの初期化）
int kzmem_init(void);

// 動的メモリ領域の獲得
void *kzmem_alloc(int size);

// 動的メモリ領域の解放
void kzmem_free(void *mem);

#endif
