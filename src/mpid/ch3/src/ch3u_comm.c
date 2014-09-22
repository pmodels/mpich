/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpl_utlist.h"
#if defined HAVE_LIBHCOLL
#include "../../common/hcoll/hcoll.h"
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_ENABLE_HCOLL
      category    : CH3
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, enable HCOLL collectives.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static int register_hook_finalize(void *param);
static int comm_created(MPID_Comm *comm, void *param);
static int comm_destroyed(MPID_Comm *comm, void *param);

/* macros and head for list of communicators */
#define COMM_ADD(comm) MPL_DL_PREPEND_NP(comm_list, comm, dev.next, dev.prev)
#define COMM_DEL(comm) MPL_DL_DELETE_NP(comm_list, comm, dev.next, dev.prev)
#define COMM_FOREACH(elt) MPL_DL_FOREACH_NP(comm_list, elt, dev.next, dev.prev)
static MPID_Comm *comm_list = NULL;

typedef struct hook_elt
{
    int (*hook_fn)(struct MPID_Comm *, void *);
    void *param;
    struct hook_elt *prev;
    struct hook_elt *next;
} hook_elt;

static hook_elt *create_hooks_head = NULL;
static hook_elt *destroy_hooks_head = NULL;
static hook_elt *create_hooks_tail = NULL;
static hook_elt *destroy_hooks_tail = NULL;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Comm_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_Comm_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_COMM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_COMM_INIT);

    MPIR_Add_finalize(register_hook_finalize, NULL, MPIR_FINALIZE_CALLBACK_PRIO-1);

    /* register hooks for keeping track of communicators */
    mpi_errno = MPIDI_CH3U_Comm_register_create_hook(comm_created, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

#if defined HAVE_LIBHCOLL
    if (MPIR_CVAR_CH3_ENABLE_HCOLL) {
        mpi_errno = MPIDI_CH3U_Comm_register_create_hook(hcoll_comm_create, NULL);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        mpi_errno = MPIDI_CH3U_Comm_register_destroy_hook(hcoll_comm_destroy, NULL);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
#endif

    mpi_errno = MPIDI_CH3U_Comm_register_destroy_hook(comm_destroyed, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_COMM_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Comm_create_hook
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_Comm_create_hook(MPID_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_COMM_CREATE_HOOK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_COMM_CREATE_HOOK);

    MPL_LL_FOREACH(create_hooks_head, elt) {
        mpi_errno = elt->hook_fn(comm, elt->param);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_COMM_CREATE_HOOK);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Comm_destroy_hook
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_Comm_destroy_hook(MPID_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_COMM_DESTROY_HOOK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_COMM_DESTROY_HOOK);

    MPL_LL_FOREACH(destroy_hooks_head, elt) {
        mpi_errno = elt->hook_fn(comm, elt->param);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_COMM_DESTROY_HOOK);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Comm_register_create_hook
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3U_Comm_register_create_hook(int (*hook_fn)(struct MPID_Comm *, void *), void *param)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_COMM_REGISTER_CREATE_HOOK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_COMM_REGISTER_CREATE_HOOK);

    MPIU_CHKPMEM_MALLOC(elt, hook_elt *, sizeof(hook_elt), mpi_errno, "hook_elt");

    elt->hook_fn = hook_fn;
    elt->param = param;
    
    MPL_LL_PREPEND(create_hooks_head, create_hooks_tail, elt);

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_COMM_REGISTER_CREATE_HOOK);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Comm_register_destroy_hook
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3U_Comm_register_destroy_hook(int (*hook_fn)(struct MPID_Comm *, void *), void *param)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_COMM_REGISTER_DESTROY_HOOK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_COMM_REGISTER_DESTROY_HOOK);

    MPIU_CHKPMEM_MALLOC(elt, hook_elt *, sizeof(hook_elt), mpi_errno, "hook_elt");

    elt->hook_fn = hook_fn;
    elt->param = param;
    
    MPL_LL_PREPEND(destroy_hooks_head, destroy_hooks_tail, elt);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_COMM_REGISTER_DESTROY_HOOK);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME register_hook_finalize
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int register_hook_finalize(void *param)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt, *tmp;
    MPIDI_STATE_DECL(MPID_STATE_REGISTER_HOOK_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_REGISTER_HOOK_FINALIZE);

    MPL_LL_FOREACH_SAFE(create_hooks_head, elt, tmp) {
        MPL_LL_DELETE(create_hooks_head, create_hooks_tail, elt);
        MPIU_Free(elt);
    }
    
    MPL_LL_FOREACH_SAFE(destroy_hooks_head, elt, tmp) {
        MPL_LL_DELETE(destroy_hooks_head, destroy_hooks_tail, elt);
        MPIU_Free(elt);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_REGISTER_HOOK_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME comm_created
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int comm_created(MPID_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_COMM_CREATED);

    MPIDI_FUNC_ENTER(MPID_STATE_COMM_CREATED);

    comm->dev.anysource_enabled = TRUE;

    /* Use the VC's eager threshold by default. */
    comm->dev.eager_max_msg_sz = -1;

    /* Initialize the last acked failure to -1 */
    comm->dev.last_ack_rank = -1;

    COMM_ADD(comm);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_COMM_CREATED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME comm_destroyed
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int comm_destroyed(MPID_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_COMM_DESTROYED);

    MPIDI_FUNC_ENTER(MPID_STATE_COMM_DESTROYED);

    COMM_DEL(comm);
    comm->dev.next = NULL;
    comm->dev.prev = NULL;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_COMM_DESTROYED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* flag==TRUE iff a member of group is also a member of comm */
#undef FUNCNAME
#define FUNCNAME nonempty_intersection
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int nonempty_intersection(MPID_Comm *comm, MPID_Group *group, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    int i_g, i_c;
    MPIDI_VC_t *vc_g, *vc_c;
    MPIDI_STATE_DECL(MPID_STATE_NONEMPTY_INTERSECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_NONEMPTY_INTERSECTION);

    /* handle common case fast */
    if (comm == MPIR_Process.comm_world || comm == MPIR_Process.icomm_world) {
        *flag = TRUE;
        MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "comm is comm_world or icomm_world");
        goto fn_exit;
    }
    *flag = FALSE;
    
    /* FIXME: This algorithm assumes that the number of processes in group is
       very small (like 1).  So doing a linear search for them in comm is better
       than sorting the procs in comm and group then doing a binary search */

    for (i_g = 0; i_g < group->size; ++i_g) {
        /* FIXME: This won't work for dynamic procs */
        MPIDI_PG_Get_vc(MPIDI_Process.my_pg, group->lrank_to_lpid[i_g].lpid, &vc_g);
        for (i_c = 0; i_c < comm->remote_size; ++i_c) {
            MPIDI_Comm_get_vc(comm, i_c, &vc_c);
            if (vc_g == vc_c) {
                *flag = TRUE;
                goto fn_exit;
            }
        }
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NONEMPTY_INTERSECTION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Comm_handle_failed_procs
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_Comm_handle_failed_procs(MPID_Group *new_failed_procs)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm;
    int flag = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMM_HANDLE_FAILED_PROCS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_COMM_HANDLE_FAILED_PROCS);

    /* mark communicators with new failed processes as collectively inactive and
       disable posting anysource receives */
    COMM_FOREACH(comm) {
        /* if this comm is already collectively inactive and
           anysources are disabled, there's no need to check */
        if (!comm->dev.anysource_enabled)
            continue;

        mpi_errno = nonempty_intersection(comm, new_failed_procs, &flag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (flag) {
            MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE,
                             (MPIU_DBG_FDEST, "disabling AS on communicator %p (%#08x)",
                              comm, comm->handle));
            comm->dev.anysource_enabled = FALSE;
        }
    }

    /* Now that we've marked communicators with disable anysource, we
       complete-with-an-error all anysource receives posted on those
       communicators */
    mpi_errno = MPIDI_CH3U_Complete_disabled_anysources();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_COMM_HANDLE_FAILED_PROCS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

void MPIDI_CH3I_Comm_find(MPIR_Context_id_t context_id, MPID_Comm **comm)
{
    MPIDI_STATE_DECL(MPIDI_STATE_MPIDI_CH3I_COMM_FIND);
    MPIDI_FUNC_ENTER(MPIDI_STATE_MPIDI_CH3I_COMM_FIND);

    COMM_FOREACH((*comm)) {
        if ((*comm)->context_id == context_id || ((*comm)->context_id + MPID_CONTEXT_INTRA_COLL) == context_id ||
            ((*comm)->node_comm && ((*comm)->node_comm->context_id == context_id || ((*comm)->node_comm->context_id + MPID_CONTEXT_INTRA_COLL) == context_id)) ||
            ((*comm)->node_roots_comm && ((*comm)->node_roots_comm->context_id == context_id || ((*comm)->node_roots_comm->context_id + MPID_CONTEXT_INTRA_COLL) == context_id)) ) {
            MPIU_DBG_MSG_D(CH3_OTHER,VERBOSE,"Found matching context id: %d", (*comm)->context_id);
            break;
        }
    }

    MPIDI_FUNC_EXIT(MPIDI_STATE_MPIDI_CH3I_COMM_FIND);
}
