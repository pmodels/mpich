/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiatomic.h"
#include "mpidu_process_locks.h"

/*
    These macros are analogous to the MPIDU_THREAD_XXX_CS_{ENTER,EXIT} macros.
    TODO Consider putting debugging macros in here that utilize 'msg'.
*/
#define MPIDU_IPC_SINGLE_CS_ENTER(msg)          \
    do {                                        \
        MPIU_Assert(emulation_lock);            \
        MPIDU_Process_lock(emulation_lock);     \
    } while (0)

#define MPIDU_IPC_SINGLE_CS_EXIT(msg)           \
    do {                                        \
        MPIU_Assert(emulation_lock);            \
        MPIDU_Process_unlock(emulation_lock);   \
    } while (0)


static MPIDU_Process_lock_t *emulation_lock = NULL;

int MPIDU_Interprocess_lock_init(MPIDU_Process_lock_t *shm_lock, int isLeader)
{
    int mpi_errno = MPI_SUCCESS;
    emulation_lock = shm_lock;

    if (isLeader) {
        MPIDU_Process_lock_init(emulation_lock);
    }

    return mpi_errno;
}

/*
    Emulated atomic primitives
    --------------------------

    These are versions of the atomic primitives that emulate the proper behavior
    via the use of an inter-process lock.  For more information on their
    individual behavior, please see the comment on the corresponding top level
    function.

    In general, these emulated primitives should _not_ be used.  Most algorithms
    can be more efficiently implemented by putting most or all of the algorithm
    inside of a single critical section.  These emulated primitives exist to
    ensure that there is always a fallback if no machine-dependent version of a
    particular operation has been defined.  They also serve as a very readable
    reference for the exact semantics of our MPIDU_Atomic ops.
*/


void MPIDU_Atomic_add_by_lock(int *ptr, int val)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_add");
    *ptr += val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_add");
}

int *MPIDU_Atomic_cas_int_ptr_by_lock(int * volatile *ptr, int *oldv, int *newv)
{
    volatile int *prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_cas");
    prev = *ptr;
    if (prev == oldv) {
        *ptr = newv;
    }
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_cas");
    return (int *)prev;
}

char *MPIDU_Atomic_cas_char_ptr_by_lock(char * volatile *ptr, char *oldv, char *newv)
{
    return (char *)MPIDU_Atomic_cas_int_ptr_by_lock((int *volatile*)ptr, (int *)oldv, (int *)newv);
}

int MPIDU_Atomic_cas_int_by_lock(volatile int *ptr, int oldv, int newv)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_cas");
    prev = *ptr;
    if (prev == oldv) {
        *ptr = newv;
    }
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_cas");
    return prev;
}

MPI_Aint MPIDU_Atomic_cas_aint_by_lock(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
    MPI_Aint prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_cas_aint");
    prev = *ptr;
    if (prev == oldv) {
        *ptr = newv;
    }
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_cas_aint");
    return (MPI_Aint)prev;
}

int MPIDU_Atomic_decr_and_test_by_lock(volatile int *ptr)
{
    int new_val;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_decr_and_test");
    new_val = --(*ptr);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_decr_and_test");
    return (0 == new_val);
}

void MPIDU_Atomic_decr_by_lock(volatile int *ptr)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_decr");
    --(*ptr);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_decr");
}

int MPIDU_Atomic_fetch_and_add_by_lock(volatile int *ptr, int val)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_fetch_and_add");
    prev = *ptr;
    *ptr += val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_fetch_and_add");
    return prev;
}

int MPIDU_Atomic_fetch_and_decr_by_lock(volatile int *ptr)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_fetch_and_decr");
    prev = *ptr;
    --(*ptr);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_fetch_and_decr");
    return prev;
}

int MPIDU_Atomic_fetch_and_incr_by_lock(volatile int *ptr)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_fetch_and_incr");
    prev = *ptr;
    ++(*ptr);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_fetch_and_incr");
    return prev;
}

void MPIDU_Atomic_incr_by_lock(volatile int *ptr)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_incr");
    ++(*ptr);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_incr");
}

int *MPIDU_Atomic_swap_int_ptr_by_lock(int * volatile *ptr, int *val)
{
    volatile int *prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_swap_int_ptr");
    prev = *ptr;
    *ptr = val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_swap_int_ptr");
    return (int *)prev;
}

char *MPIDU_Atomic_swap_char_ptr_by_lock(char * volatile *ptr, char *val)
{
    return (char *)MPIDU_Atomic_swap_int_ptr_by_lock((int *volatile*)ptr, (int *)val);
}

int MPIDU_Atomic_swap_int_by_lock(volatile int *ptr, int val)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_swap_int");
    prev = *ptr;
    *ptr = val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_swap_int");
    return (int)prev;
}

MPI_Aint MPIDU_Atomic_swap_aint_by_lock(volatile MPI_Aint *ptr, MPI_Aint val)
{
    MPI_Aint prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_swap_aint");
    prev = *ptr;
    *ptr = val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_swap_aint");
    return (MPI_Aint)prev;
}

