/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "lock/zm_tlp.h"

/* The algorithm follows this logic

acquire()
lock(high_p)
do {
    if(CAS(common, FREE, GRANTED);
        break;
    if (CAS(common, GRANTED, REQUEST {
        while(common != GRANTED) ;
        break;     }
} while (1)
unlock(high_p)

CS

release()
if(!CAS(common, REQUEST, GRANTED); //optimistic
    common = FREE

acquire_low()
lock(low_p)
do {
    while(common != FREE) ;
    if(CAS(common, FREE, GRANTED);
        break;
} whil (1)


CS

release_low()
if(!CAS(common, REQUEST, GRANTED); //optimistic
    common = FREE;
unlock(low_p)
*/

/* Helper functions */

#if (ZM_TLP_HIGH_P == ZM_TICKET)
#define zm_tlp_init_high_p(L) zm_ticket_init(&L->high_p)
#elif (ZM_TLP_HIGH_P == ZM_MCS)
#define zm_tlp_init_high_p(L) zm_mcs_init(&L->high_p)
#endif

#if (ZM_TLP_HIGH_P == ZM_TICKET)
#define zm_tlp_acquire_high_p(L) zm_ticket_acquire(&L->high_p)
#elif (ZM_TLP_HIGH_P == ZM_MCS)
#define zm_tlp_acquire_high_p(L)                        \
    zm_mcs_qnode_t local_ctxt;                          \
    zm_mcs_acquire(&L->high_p, &local_ctxt)
#endif

#if (ZM_TLP_HIGH_P == ZM_TICKET)
#define zm_tlp_release_high_p(L) zm_ticket_release(&L->high_p)
#elif (ZM_TLP_HIGH_P == ZM_MCS)
#define zm_tlp_release_high_p(L) zm_mcs_release(&L->high_p, &local_ctxt);
#endif

#if (ZM_TLP_LOW_P == ZM_TICKET)
#define zm_tlp_init_low_p(L) zm_ticket_init(&L->low_p)
#elif (ZM_TLP_LOW_P == ZM_MCS)
#define zm_tlp_init_low_p(L) zm_mcs_init(&L->low_p)
#endif

#if (ZM_TLP_LOW_P == ZM_TICKET)
#define zm_tlp_acquire_low_p(L) zm_ticket_acquire(&L->low_p)
#elif (ZM_TLP_LOW_P == ZM_MCS)
#define zm_tlp_acquire_low_p(L)                         \
    zm_mcs_qnode_t local_ctxt;                          \
    zm_mcs_acquire(&L->low_p, &local_ctxt)
#endif

#if (ZM_TLP_LOW_P == ZM_TICKET)
#define zm_tlp_release_low_p(L) zm_ticket_release(&L->low_p)
#elif (ZM_TLP_LOW_P == ZM_MCS)
#define zm_tlp_release_low_p(L) zm_mcs_release(&L->low_p, &local_ctxt);
#endif

int zm_tlp_init(zm_tlp_t *L)
{
   zm_tlp_init_high_p(L);
   zm_atomic_store(&L->common, ZM_STATUS_FREE, zm_memord_release);
   zm_tlp_init_low_p(L);
    return 0;
}

int zm_tlp_acquire(zm_tlp_t *L) {
    /* Acquire the high priority lock */
   zm_tlp_acquire_high_p(L);

   unsigned int old;
   old = zm_atomic_exchange(&L->common,
                            ZM_STATUS_REQUEST,
                            zm_memord_acq_rel);
   if (old == ZM_STATUS_FREE)
        zm_atomic_store(&L->common, ZM_STATUS_GRANTED, zm_memord_release);
   else {
        while(zm_atomic_load(&L->common, zm_memord_acquire) != ZM_STATUS_GRANTED)
                ; //SPIN
    }
    zm_tlp_release_high_p(L);
    return 0;
}

int zm_tlp_acquire_low(zm_tlp_t *L) {
    /* Acquire the low priority lock */
   zm_tlp_acquire_low_p(L);
   do {
        while(zm_atomic_load(&L->common, zm_memord_acquire) != ZM_STATUS_FREE)
            ; // SPIN
        unsigned int expected = ZM_STATUS_FREE;
        if(zm_atomic_compare_exchange_weak(&L->common,
                                          &expected,
                                          ZM_STATUS_GRANTED,
                                          zm_memord_release,
                                          zm_memord_acquire))
            break;
    } while (1);
    zm_tlp_release_low_p(L);
    return 0;
}

/* Release the lock */
int zm_tlp_release(zm_tlp_t *L) {
    unsigned int expected = ZM_STATUS_REQUEST;
    do {
        expected = ZM_STATUS_REQUEST;
        if(zm_atomic_compare_exchange_weak(&L->common,
                                           &expected,
                                           ZM_STATUS_GRANTED,
                                           zm_memord_release,
                                           zm_memord_acquire))
            break;

        expected = ZM_STATUS_GRANTED;
        if(zm_atomic_compare_exchange_weak(&L->common,
                                           &expected,
                                           ZM_STATUS_FREE,
                                           zm_memord_release,
                                           zm_memord_acquire))
            break;
    } while (1);
    return 0;
}
