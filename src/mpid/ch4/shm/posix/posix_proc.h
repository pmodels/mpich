#ifndef SHM_POSIX_PROC_H_INCLUDED
#define SHM_POSIX_PROC_H_INCLUDED

#include "posix_impl.h"

static inline int MPIDI_SHM_rank_is_local(int rank, MPIR_Comm * comm)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
#endif /* SHM_POSIX_PROC_H_INCLUDED */
