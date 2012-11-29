/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _MPIDI_COMMON_STATISTICS_H_
#define _MPIDI_COMMON_STATISTICS_H_

#if ENABLE_STATISTICS

/* TODO add some code here and probably elsewhere to make these show up in the
 * MPI_T_pvar_ interface */

#define MPIR_T_INC(x) (++(x))
#define MPIR_T_DEC(x) (--(x))

#define MPIR_T_START_TIMER(start) MPID_Wtime(&start)
#define MPIR_T_END_TIMER(start, mpit_variable)          \
    do {                                                \
        MPID_Time_t end;                                \
        double temp_delta = 0.0;                        \
        MPID_Wtime(&(end));                             \
        MPID_Wtime_diff(&(start), &end, &temp_delta);   \
        (mpit_variable) += temp_delta;                  \
    } while(0)

#define MPIR_T_SUBTRACT(x, y) ((x) -= (y))
#define MPIR_T_ADD(x, y)      ((x) += (y))

#else

#define MPIR_T_INC(x)
#define MPIR_T_DEC(x)

#define MPIR_T_START_TIMER(start)
#define MPIR_T_END_TIMER(start, mpit_variable)

#define MPIR_T_SUBTRACT(x, y)
#define MPIR_T_ADD(x, y)

#endif /* ENABLE_STATISTICS */

#define MPIR_T_SIMPLE_HANDLE_CREATOR(TAG, TYPE, COUNT)                          \
static int TAG(void *obj_handle,                                                \
               struct MPIR_T_pvar_handle *handle,                               \
               int *countp)                                                     \
{                                                                               \
    /* the IMPL_SIMPLE code reads/writes "bytes" bytes from the location given  \
     * by the "handle_state" pointer */                                         \
    handle->handle_state = handle->info->var_state;                             \
    handle->bytes = sizeof(TYPE);                                               \
                                                                                \
    *countp       = (COUNT);                                                    \
    return MPI_SUCCESS;                                                         \
}

#endif  /* _MPIDI_COMMON_STATISTICS_H_ */
