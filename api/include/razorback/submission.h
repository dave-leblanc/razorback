/** Submission API.
 * @file submission.h
 */
#ifndef RAZORBACK_SUBMISSION_H
#define RAZORBACK_SUBMISSION_H

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RZB_SUBMISSION_OK 0
#define RZB_SUBMISSION_ERROR 1
#define RZB_SUBMISSION_NO_TYPE 2

/** Submit a block pool item.
 * @param p_pItem The item.
 * @param p_iFlags The submission flags.
 * @param p_pSf_Flags Pointer to a uint32_t to store the SourceFire threat flags in.
 * @param p_pEnt_Flags Pointer to a uint32_t to store the Enterprise threat flags in.
 * @return true on success false on error.
 */
SO_PUBLIC extern int Submission_Submit(struct BlockPoolItem *p_pItem, int p_iFlags, uint32_t *p_pSf_Flags, uint32_t *p_pEnt_Flags);
#ifdef __cplusplus
}
#endif
#endif
