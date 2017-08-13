/** @file judgment.h
 * Judgment transmission functions.
 */
#ifndef RAZORBACK_JUDGMENT_H
#define RAZORBACK_JUDGMENT_H

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

SO_PUBLIC extern struct Judgment * Judgment_Create (struct EventId *eventId, struct BlockId *blockId);
SO_PUBLIC extern void Judgment_Destroy (struct Judgment *judgment);
SO_PUBLIC extern uint32_t Judgment_BinaryLength (struct Judgment *judgment);

/** Render a verdict on a block
 */
SO_PUBLIC extern bool
Judgment_Render_Verdict (uint8_t p_iLevel, struct Judgment *p_pJudgment);
#ifdef __cplusplus
}
#endif
#endif
