/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_DEBUGGER_H_INCLUDED
#define HYDRA_DEBUGGER_H_INCLUDED

#include "hydra_base.h"
#include "hydra_exec.h"
#include "hydra_node.h"

/* This structure is defined by the debugger interface */
typedef struct MPIR_PROCDESC {
    char *host_name;
    char *executable_name;
    int pid;
} MPIR_PROCDESC;

/* Two global variables which a debugger can use for: (1) finding out
 * the state of the program, (2) informing the process that it has
 * been attached to */
enum {
    MPIR_NULL = 0,
    MPIR_DEBUG_SPAWNED = 1,
    MPIR_DEBUG_ABORTING = 2
};

extern volatile int MPIR_debug_state;
extern char *MPIR_debug_abort_string;

/* Set by the debugger when it attaches to this process. */
extern volatile int MPIR_being_debugged;

/* An array of processes */
extern struct MPIR_PROCDESC *MPIR_proctable;
/* This global variable defines the number of MPIR_PROCDESC entries
 * for the debugger */
extern int MPIR_proctable_size;

/* The presence of this variable tells the debugger that this process
 * starts MPI jobs and isn't part of the MPI_COMM_WORLD */
extern int MPIR_i_am_starter;

/* The presence of this variable tells the debugger that it need not
 * attach to all processes to get them running */
extern int MPIR_partial_attach_ok;

/* We make this a global pointer to prohibit the compiler from
 * inlining the breakpoint functions (without this, gcc often removes
 * the call) Neither the attribute __attribute__((noinline)) nor the
 * use of asm(""), recommended by the GCC manual, prevented the
 * inlining of this call.  Rather than place it in a separate file
 * (and still risk whole-program analysis removal), we use a globally
 * visable function pointer. */
extern int (*MPIR_breakpointFn) (void);
int MPIR_Breakpoint(void);

HYD_status HYD_dbg_setup_procdesc(int process_count, struct HYD_exec *exec_list, int *pid,
                                  int node_count, struct HYD_node * node_list);
void HYD_dbg_free_procdesc(void);

#endif /* HYDRA_DEBUGGER_H_INCLUDED */
