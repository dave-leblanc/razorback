#ifndef RAZORBACK_BLOCK_POOL_PRIVATE_H
#define RAZORBACK_BLOCK_POOL_PRIVATE_H
#include <razorback/block_pool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define BLOCK_POOL_KEEP 0
#define BLOCK_POOL_DESTROY 2

bool BlockPool_Init(struct RazorbackContext *p_pContext);
void BlockPool_ForEachItem(int (*function) (struct BlockPoolItem *, void *), void *userData);
void BlockPool_Item_Lock(void *a);
void BlockPool_Item_Unlock(void *a);

void BlockPool_SetStatus(struct BlockPoolItem *p_pItem, uint32_t p_iStatus);
uint32_t BlockPool_GetStatus(struct BlockPoolItem *p_pItem);
void BlockPool_SetFlags(struct BlockPoolItem *p_pItem, uint32_t p_iFlags);
void BlockPool_DestroyItemDataList(struct BlockPoolItem *p_pItem);
#ifdef __cplusplus
}
#endif
#endif
