#ifndef	RAZORBACK_CONNECTED_ENTITY_PRIVATE_H
#define	RAZORBACK_CONNECTED_ENTITY_PRIVATE_H
#ifdef __cplusplus
extern "C" {
#endif
extern bool ConnectedEntityList_Start (void);
extern void ConnectedEntityList_Stop (void);
extern struct ConnectedEntity* ConnectedEntityList_GetDispatcher(void);
extern bool ConnectedEntityList_MarkDispatcherUnusable(uuid_t nuggetId);
#ifdef __cplusplus
}
#endif
#endif
