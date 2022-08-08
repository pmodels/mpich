/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMI_COMMON_H_INCLUDED
#define PMI_COMMON_H_INCLUDED

/* Set PMI_initialized to 1 for singleton init but no process manager
   to help.  Initialized to 2 for normal initialization.  Initialized
   to values higher than 2 when singleton_init by a process manager.
   All values higher than 1 involve a PM in some way.
*/
typedef enum {
    PMI_UNINITIALIZED = 0,
    SINGLETON_INIT_BUT_NO_PM = 1,
    NORMAL_INIT_WITH_PM,
    SINGLETON_INIT_WITH_PM
} PMIState;

extern PMIState PMI_initialized;

extern int PMI_fd;
extern int PMI_size;
extern int PMI_rank;

#endif
