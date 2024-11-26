/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_SESSION_H_INCLUDED
#define MPIR_SESSION_H_INCLUDED

#include "mpiimpl.h"

#define MPIR_SESSION_WORLD_PSET_NAME "mpi://WORLD"
#define MPIR_SESSION_SELF_PSET_NAME "mpi://SELF"

/* Session structure */
struct MPIR_Session {
    MPIR_OBJECT_HEADER;
    MPID_Thread_mutex_t mutex;
    MPIR_Errhandler *errhandler;
    struct MPII_BsendBuffer *bsendbuffer;       /* for MPI_Session_attach_buffer */
    int requested_thread_level;
    int thread_level;
    bool strict_finalize;
    char *memory_alloc_kinds;
};

extern MPIR_Object_alloc_t MPIR_Session_mem;

/* Preallocated session object */
extern MPIR_Session MPIR_Session_direct[];

/* Util functions and macros */

#define MPIR_Session_add_ref(_session) \
    do { MPIR_Object_add_ref(_session); } while (0)

#define MPIR_Session_release_ref(_session, _inuse) \
    do { MPIR_Object_release_ref(_session, _inuse); } while (0)

int MPIR_Session_create(MPIR_Session **, int);
int MPIR_Session_release(MPIR_Session * session_prt);

/* thread level util */
int MPIR_Session_get_thread_level_from_info(MPIR_Info * info_ptr, int *threadlevel);

/* strict finalize util */
int MPIR_Session_get_strict_finalize_from_info(MPIR_Info * info_ptr, bool * strict_finalize);

/* memory allocation kinds util */
int MPIR_Session_get_memory_kinds_from_info(MPIR_Info * info_ptr, char **out_kinds);

/* API Implementations */

/* temporary declarations until they are autogenerated */
int MPIR_Session_finalize_impl(MPIR_Session * session_ptr);
int MPIR_Session_get_info_impl(MPIR_Session * session_ptr, MPIR_Info ** info_used_ptr);
int MPIR_Session_get_nth_pset_impl(MPIR_Session * session_ptr, MPIR_Info * info_ptr, int n,
                                   int *pset_len, char *pset_name);
int MPIR_Session_get_num_psets_impl(MPIR_Session * session_ptr, MPIR_Info * info_ptr,
                                    int *npset_names);
int MPIR_Session_get_pset_info_impl(MPIR_Session * session_ptr, const char *pset_name,
                                    MPIR_Info ** info_ptr);
int MPIR_Session_init_impl(MPIR_Info * info_ptr, MPIR_Errhandler * errhandler_ptr,
                           MPIR_Session ** session_ptr);

#endif /* MPIR_SESSION_H_INCLUDED */
