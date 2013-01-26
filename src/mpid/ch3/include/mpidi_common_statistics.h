/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _MPIDI_COMMON_STATISTICS_H_
#define _MPIDI_COMMON_STATISTICS_H_

/* Statically decides whether or not to perform 'action'.
 *
 * This has the unfortunate side-effect that the compiler will complain about
 * undeclared variables in the 'action' parameter.
 * Thankfully, since by linking time they will be long gone (due to the compiler
 * optimizing away the whole macro), a simple 'extern' declaration suffices.
 * This is why some statistics headers declare their variables as extern
 * outside of the macro scope.
 */
#define MPIR_T_GATE(PVAR_CLASS, action) \
    do {                                \
        if (ENABLE_##PVAR_CLASS) {      \
            action;                     \
        }                               \
    } while(0)


/* TODO add some code here and probably elsewhere to make these show up in the
 * MPI_T_pvar_ interface */
#define MPIR_T_INC_impl(x) (++(x))
#define MPIR_T_DEC_impl(x) (--(x))

#define MPIR_T_INC(PVAR_CLASS, x) MPIR_T_GATE(PVAR_CLASS, MPIR_T_INC_impl(x))
#define MPIR_T_DEC(PVAR_CLASS, x) MPIR_T_GATE(PVAR_CLASS, MPIR_T_DEC_impl(x))


#define MPIR_T_START_TIMER_impl(start) MPID_Wtime(&start)
#define MPIR_T_END_TIMER_impl(start, mpit_variable)     \
    do {                                                \
        MPID_Time_t end;                                \
        double temp_delta = 0.0;                        \
        MPID_Wtime(&(end));                             \
        MPID_Wtime_diff(&(start), &end, &temp_delta);   \
        (mpit_variable) += temp_delta;                  \
    } while(0)

#define MPIR_T_START_TIMER(PVAR_CLASS, start) MPIR_T_GATE(PVAR_CLASS, MPIR_T_START_TIMER_impl(start))
#define MPIR_T_END_TIMER(PVAR_CLASS, start, pvar) MPIR_T_GATE(PVAR_CLASS, MPIR_T_END_TIMER_impl(start, pvar))


#define MPIR_T_SUBTRACT_impl(x, y) ((x) -= (y))
#define MPIR_T_ADD_impl(x, y)      ((x) += (y))

#define MPIR_T_SUBTRACT(PVAR_CLASS, x, y) MPIR_T_GATE(PVAR_CLASS, MPIR_T_SUBTRACT_impl(x, y))
#define MPIR_T_ADD(PVAR_CLASS, x, y) MPIR_T_GATE(PVAR_CLASS, MPIR_T_ADD_impl(x, y))


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
    *countp = (COUNT);                                                          \
    return MPI_SUCCESS;                                                         \
}

#endif  /* _MPIDI_COMMON_STATISTICS_H_ */
