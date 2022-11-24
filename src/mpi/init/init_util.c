/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

/* The following routines provide a callback facility for modules that need
   some code called on exit.  This method allows us to avoid forcing
   MPI_Finalize to know the routine names a priori.  Any module that wants to
   have a callback calls MPIR_Add_finalize(routine, extra, priority).

 */
typedef struct Finalize_func_t {
    int (*f) (void *);          /* The function to call */
    void *extra_data;           /* Data for the function */
    int priority;               /* priority is used to control the order
                                 * in which the callbacks are invoked */
} Finalize_func_t;
/* When full debugging is enabled, each MPI handle type has a finalize handler
   installed to detect unfreed handles.  */
#define MAX_FINALIZE_FUNC 256
static MPL_initlock_t fstack_lock = MPL_INITLOCK_INITIALIZER;
static Finalize_func_t fstack[MAX_FINALIZE_FUNC];
static int fstack_sp = 0;
static int fstack_max_priority = 0;

void MPIR_Add_finalize(int (*f) (void *), void *extra_data, int priority)
{
    /* MPIR_Add_finalize may be called both inside and outside init */
    MPL_initlock_lock(&fstack_lock);
    /* --BEGIN ERROR HANDLING-- */
    if (fstack_sp >= MAX_FINALIZE_FUNC) {
        /* This is a little tricky.  We may want to check the state of
         * MPIR_Process.mpich_state to decide how to signal the error */
        (void) MPL_internal_error_printf("overflow in finalize stack! "
                                         "Is MAX_FINALIZE_FUNC too small?\n");
        if (MPIR_Errutil_is_initialized()) {
            MPID_Abort(NULL, MPI_SUCCESS, 13, NULL);
        } else {
            exit(1);
        }
    }
    /* --END ERROR HANDLING-- */
    fstack[fstack_sp].f = f;
    fstack[fstack_sp].priority = priority;
    fstack[fstack_sp++].extra_data = extra_data;

    if (priority > fstack_max_priority)
        fstack_max_priority = priority;
    MPL_initlock_unlock(&fstack_lock);
}

/* Invoke the registered callbacks */
void MPII_Call_finalize_callbacks(int min_prio, int max_prio)
{
    int i, j;

    if (max_prio > fstack_max_priority)
        max_prio = fstack_max_priority;
    for (j = max_prio; j >= min_prio; j--) {
        for (i = fstack_sp - 1; i >= 0; i--) {
            if (fstack[i].f && fstack[i].priority == j) {
                fstack[i].f(fstack[i].extra_data);
                fstack[i].f = 0;
            }
        }
    }
}

static const char *threadlevel_name(int threadlevel)
{
    if (threadlevel == MPI_THREAD_SINGLE) {
        return "MPI_THREAD_SINGLE";
    } else if (threadlevel == MPI_THREAD_FUNNELED) {
        return "MPI_THREAD_FUNNELED";
    } else if (threadlevel == MPI_THREAD_SERIALIZED) {
        return "MPI_THREAD_SERIALIZED";
    } else if (threadlevel == MPI_THREAD_MULTIPLE) {
        return "MPI_THREAD_MULTIPLE";
    } else {
        return "UNKNOWN";
    }
}

static void print_setting(const char *label, const char *value)
{
    printf("%-18s: %s\n", label, value);
}

void MPII_dump_debug_summary(void)
{
#ifdef HAVE_ERROR_CHECKING
    print_setting("error checking", "enabled");
#else
    print_setting("error checking", "disabled");
#endif
#ifdef ENABLE_QMPI
    print_setting("QMPI", "enabled");
#else
    print_setting("QMPI", "disabled");
#endif
#ifdef HAVE_DEBUGGER_SUPPORT
    print_setting("debugger support", "enabled");
#else
    print_setting("debugger support", "disabled");
#endif
    print_setting("thread level", threadlevel_name(MPIR_ThreadInfo.thread_provided));
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
    print_setting("thread CS", "global");
#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    print_setting("thread CS", "per-vci");
#endif

    printf("==== data structure summary ====\n");
    printf("sizeof(MPIR_Comm): %zd\n", sizeof(MPIR_Comm));
    printf("sizeof(MPIR_Request): %zd\n", sizeof(MPIR_Request));
    printf("sizeof(MPIR_Datatype): %zd\n", sizeof(MPIR_Datatype));
    printf("================================\n");
}
