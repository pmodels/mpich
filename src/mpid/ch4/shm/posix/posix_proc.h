#ifndef SHM_POSIX_PROC_H_INCLUDED
#define SHM_POSIX_PROC_H_INCLUDED

#include "posix_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_rank_is_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rank_is_local(int rank, MPIR_Comm * comm)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
#endif /* SHM_POSIX_PROC_H_INCLUDED */
