/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "utlist.h"
#if defined HAVE_HCOLL
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
static int comm_created(MPIR_Comm *comm, void *param);
static int comm_destroyed(MPIR_Comm *comm, void *param);

/* macros and head for list of communicators */
#define COMM_ADD(comm) DL_PREPEND_NP(comm_list, comm, dev.next, dev.prev)
#define COMM_DEL(comm) DL_DELETE_NP(comm_list, comm, dev.next, dev.prev)
#define COMM_FOREACH(elt) DL_FOREACH_NP(comm_list, elt, dev.next, dev.prev)
static MPIR_Comm *comm_list = NULL;

typedef struct hook_elt
{
    int (*hook_fn)(struct MPIR_Comm *, void *);
    void *param;
    struct hook_elt *prev;
    struct hook_elt *next;
} hook_elt;

static hook_elt *create_hooks_head = NULL;
static hook_elt *destroy_hooks_head = NULL;
static hook_elt *create_hooks_tail = NULL;
static hook_elt *destroy_hooks_tail = NULL;

int MPIDI_CH3I_Comm_init(void)
{
    int mpi_errno = MPI_SUCCESS;
#if defined HAVE_HCOLL && MPID_CH3I_CH_HCOLL_BCOL
    MPIR_CHKLMEM_DECL();
#endif

    MPIR_FUNC_ENTER;

    MPIR_Add_finalize(register_hook_finalize, NULL, MPIR_FINALIZE_CALLBACK_PRIO-1);

    /* register hooks for keeping track of communicators */
    mpi_errno = MPIDI_CH3U_Comm_register_create_hook(comm_created, NULL);
    MPIR_ERR_CHECK(mpi_errno);

#if defined HAVE_HCOLL
    {
        int r;

        /* check if the user is not trying to override the multicast
         * setting before resetting it */
        if (getenv("HCOLL_ENABLE_MCAST_ALL") == NULL) {
            /* FIXME: We should not unconditionally disable multicast.
             * Test to make sure it's available before choosing to
             * enable or disable it. */
            r = MPL_putenv("HCOLL_ENABLE_MCAST_ALL=0");
            MPIR_ERR_CHKANDJUMP(r, mpi_errno, MPI_ERR_OTHER, "**putenv");
        }

#if defined MPID_CH3I_CH_HCOLL_BCOL
        if (getenv("HCOLL_BCOL") == NULL) {
            char *envstr;
            int size = strlen("HCOLL_BCOL=") + strlen(MPID_CH3I_CH_HCOLL_BCOL) + 1;

            MPIR_CHKLMEM_MALLOC(envstr, size);
            snprintf(envstr, size, "HCOLL_BCOL=%s", MPID_CH3I_CH_HCOLL_BCOL);

            r = MPL_putenv(envstr);
            MPIR_ERR_CHKANDJUMP(r, mpi_errno, MPI_ERR_OTHER, "**putenv");
        }
#endif

        mpi_errno = MPIDI_CH3U_Comm_register_create_hook(hcoll_comm_create, NULL);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIDI_CH3U_Comm_register_destroy_hook(hcoll_comm_destroy, NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    mpi_errno = MPIDI_CH3U_Comm_register_destroy_hook(comm_destroyed, NULL);
    MPIR_ERR_CHECK(mpi_errno);
    
 fn_exit:
    MPIR_FUNC_EXIT;
#if defined HAVE_HCOLL && MPID_CH3I_CH_HCOLL_BCOL
    MPIR_CHKLMEM_FREEALL();
#endif
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static int create_vcrt_from_group(MPIR_Group *group, struct MPIDI_VCRT **vcrt_out)
{
    int mpi_errno = MPI_SUCCESS;

    if (group->ch3_vcrt) {
        MPIDI_VCRT_Add_ref(group->ch3_vcrt);
        *vcrt_out = group->ch3_vcrt;
        goto fn_exit;
    }

    struct MPIDI_VCRT *vcrt;
    mpi_errno = MPIDI_VCRT_Create(group->size, &vcrt);
    MPIR_ERR_CHECK(mpi_errno);

    *vcrt_out = vcrt;

    for (int i = 0; i < group->size; i++) {
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group, i);
        /* Currently ch3 does not synchronize pg with MPIR_worlds. All lpid are contiguous
         * with world_idx = 0. We can tell whether it is a spawned process by checking whether
         * it is >= world size.
         */
        if (lpid < MPIR_Process.size) {
            MPIDI_VCR_Dup(&MPIDI_Process.my_pg->vct[lpid], &vcrt->vcr_table[i]);
        } else {
            /* search PGs to find the vc. Not particularly efficient, but likely not critical */
            /* TODO: Build a vc hash for dynamic processes */
            MPIDI_PG_iterator iter;
            MPIDI_PG_Get_iterator(&iter);
            bool found_it = false;
            while (MPIDI_PG_Has_next(&iter)) {
                MPIDI_PG_t *pg;
                MPIDI_PG_Get_next(&iter, &pg);
                for (int j = 0; j < pg->size; j++) {
                    if (pg->vct[j].lpid == lpid) {
                        MPIDI_VCR_Dup(&pg->vct[j], &vcrt->vcr_table[i]);
                        found_it = true;
                        break;
                    }
                }
                if (found_it) {
                    break;
                }
                pg = pg->next;
            }
            MPIR_ERR_CHKANDJUMP1(!found_it, mpi_errno, MPI_ERR_OTHER, "**procnotfound",
                                 "**procnotfound %d", i);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_CH3I_Comm_commit_pre_hook(MPIR_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (comm == MPIR_Process.comm_world) {
        comm->rank        = MPIR_Process.rank;
        comm->remote_size = MPIR_Process.size;
        comm->local_size  = MPIR_Process.size;

        mpi_errno = MPIDI_VCRT_Create(comm->remote_size, &comm->dev.vcrt);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**dev|vcrt_create",
                                 "**dev|vcrt_create %s", "MPI_COMM_WORLD");
        }

        /* Initialize the connection table on COMM_WORLD from the process group's
        connection table */
        for (int p = 0; p < MPIR_Process.size; p++) {
            MPIDI_VCR_Dup(&MPIDI_Process.my_pg->vct[p], &comm->dev.vcrt->vcr_table[p]);
        }
        goto done_vcrt;
    } else if (comm == MPIR_Process.comm_self) {
        comm->rank        = 0;
        comm->remote_size = 1;
        comm->local_size  = 1;

        mpi_errno = MPIDI_VCRT_Create(comm->remote_size, &comm->dev.vcrt);
        if (mpi_errno != MPI_SUCCESS)
        {
            MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**dev|vcrt_create",
                                "**dev|vcrt_create %s", "MPI_COMM_SELF");
        }

        MPIDI_VCR_Dup(&MPIDI_Process.my_pg->vct[MPIR_Process.rank], &comm->dev.vcrt->vcr_table[0]);
        goto done_vcrt;
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = create_vcrt_from_group(comm->local_group, &comm->dev.vcrt);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = create_vcrt_from_group(comm->local_group, &comm->dev.local_vcrt);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = create_vcrt_from_group(comm->remote_group, &comm->dev.vcrt);
        MPIR_ERR_CHECK(mpi_errno);
    }

  done_vcrt:
    /* add vcrt to the comm groups if they are not there */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        if (comm->local_group->ch3_vcrt == NULL) {
            MPIDI_VCRT_Add_ref(comm->dev.vcrt);
            comm->local_group->ch3_vcrt = comm->dev.vcrt;
        }
    } else {
        if (comm->local_group->ch3_vcrt == NULL) {
            MPIDI_VCRT_Add_ref(comm->dev.local_vcrt);
            comm->local_group->ch3_vcrt = comm->dev.local_vcrt;
        }
        if (comm->remote_group->ch3_vcrt == NULL) {
            MPIDI_VCRT_Add_ref(comm->dev.vcrt);
            comm->remote_group->ch3_vcrt = comm->dev.vcrt;
        }
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        /* setup the vcrt for the local_comm in the intercomm */
        if (comm->local_comm) {
            comm->local_comm->dev.vcrt = comm->dev.local_vcrt;
            MPIDI_VCRT_Add_ref(comm->dev.local_vcrt);
        }
    }

    hook_elt *elt;
    LL_FOREACH(create_hooks_head, elt) {
        mpi_errno = elt->hook_fn(comm, elt->param);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);;
    }

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3I_Comm_commit_post_hook(MPIR_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_CH3I_Comm_destroy_hook(MPIR_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt;

    MPIR_FUNC_ENTER;

    LL_FOREACH(destroy_hooks_head, elt) {
        mpi_errno = elt->hook_fn(comm, elt->param);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_VCRT_Release(comm->dev.vcrt);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        mpi_errno = MPIDI_VCRT_Release(comm->dev.local_vcrt);
        MPIR_ERR_CHECK(mpi_errno);
    }

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3I_Comm_set_hints(MPIR_Comm *comm_ptr, MPIR_Info *info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_CH3U_Comm_register_create_hook(int (*hook_fn)(struct MPIR_Comm *, void *), void *param)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt;
    MPIR_CHKPMEM_DECL();

    MPIR_FUNC_ENTER;

    MPIR_CHKPMEM_MALLOC(elt, sizeof(hook_elt), MPL_MEM_OTHER);

    elt->hook_fn = hook_fn;
    elt->param = param;
    
    LL_PREPEND(create_hooks_head, create_hooks_tail, elt);

 fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_CH3U_Comm_register_destroy_hook(int (*hook_fn)(struct MPIR_Comm *, void *), void *param)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt;
    MPIR_CHKPMEM_DECL();

    MPIR_FUNC_ENTER;

    MPIR_CHKPMEM_MALLOC(elt, sizeof(hook_elt), MPL_MEM_OTHER);

    elt->hook_fn = hook_fn;
    elt->param = param;
    
    LL_PREPEND(destroy_hooks_head, destroy_hooks_tail, elt);

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

static int register_hook_finalize(void *param)
{
    int mpi_errno = MPI_SUCCESS;
    hook_elt *elt, *tmp;

    MPIR_FUNC_ENTER;

    LL_FOREACH_SAFE(create_hooks_head, elt, tmp) {
        LL_DELETE(create_hooks_head, create_hooks_tail, elt);
        MPL_free(elt);
    }
    
    LL_FOREACH_SAFE(destroy_hooks_head, elt, tmp) {
        LL_DELETE(destroy_hooks_head, destroy_hooks_tail, elt);
        MPL_free(elt);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}


int comm_created(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    comm->dev.anysource_enabled = TRUE;

    /* Use the VC's eager threshold by default if it is not set. */
    if (comm->hints[MPIR_COMM_HINT_EAGER_THRESH] == 0) {
        comm->hints[MPIR_COMM_HINT_EAGER_THRESH] = -1;
    }

    /* Initialize the last acked failure to -1 */
    comm->dev.last_ack_rank = -1;

    COMM_ADD(comm);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int comm_destroyed(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    COMM_DEL(comm);
    comm->dev.next = NULL;
    comm->dev.prev = NULL;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}


/* flag==TRUE iff a member of group is also a member of comm */
static int nonempty_intersection(MPIR_Comm *comm, MPIR_Group *group, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    int i_g, i_c;
    MPIDI_VC_t *vc_g, *vc_c;

    MPIR_FUNC_ENTER;

    /* handle common case fast */
    if (comm == MPIR_Process.comm_world) {
        *flag = TRUE;
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER, VERBOSE, "comm is comm_world");
        goto fn_exit;
    }
    *flag = FALSE;
    
    /* FIXME: This algorithm assumes that the number of processes in group is
       very small (like 1).  So doing a linear search for them in comm is better
       than sorting the procs in comm and group then doing a binary search */

    for (i_g = 0; i_g < group->size; ++i_g) {
        /* FIXME: This won't work for dynamic procs */
        MPIDI_PG_Get_vc(MPIDI_Process.my_pg, MPIR_Group_rank_to_lpid(group, i_g), &vc_g);
        for (i_c = 0; i_c < comm->remote_size; ++i_c) {
            MPIDI_Comm_get_vc(comm, i_c, &vc_c);
            if (vc_g == vc_c) {
                *flag = TRUE;
                goto fn_exit;
            }
        }
    }
    
 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}


int MPIDI_CH3I_Comm_handle_failed_procs(MPIR_Group *new_failed_procs)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm;
    int flag = FALSE;

    MPIR_FUNC_ENTER;

    /* mark communicators with new failed processes as collectively inactive and
       disable posting anysource receives */
    COMM_FOREACH(comm) {
        /* if this comm is already collectively inactive and
           anysources are disabled, there's no need to check */
        if (!comm->dev.anysource_enabled)
            continue;

        mpi_errno = nonempty_intersection(comm, new_failed_procs, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        if (flag) {
            MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER, VERBOSE,
                             (MPL_DBG_FDEST, "disabling AS on communicator %p (%#08x)",
                              comm, comm->handle));
            comm->dev.anysource_enabled = FALSE;
        }
    }

    /* Signal that something completed here to allow the progress engine to
     * break out and return control to the user. */
    MPIDI_CH3_Progress_signal_completion();

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

void MPIDI_CH3I_Comm_find(int context_id, MPIR_Comm **comm)
{
    MPIR_FUNC_ENTER;

    COMM_FOREACH((*comm)) {
        if ((*comm)->context_id == context_id || ((*comm)->context_id + MPIR_CONTEXT_COLL_OFFSET) == context_id ||
            ((*comm)->node_comm && ((*comm)->node_comm->context_id == context_id || ((*comm)->node_comm->context_id + MPIR_CONTEXT_COLL_OFFSET) == context_id)) ||
            ((*comm)->node_roots_comm && ((*comm)->node_roots_comm->context_id == context_id || ((*comm)->node_roots_comm->context_id + MPIR_CONTEXT_COLL_OFFSET) == context_id)) ) {
            MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,VERBOSE,"Found matching context id: %d", (*comm)->context_id);
            break;
        }
    }

    MPIR_FUNC_EXIT;
}

int MPID_Group_init_hook(MPIR_Group * group_ptr)
{
    group_ptr->ch3_vcrt = NULL;
    return MPI_SUCCESS;
}

int MPID_Group_free_hook(MPIR_Group * group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (group_ptr->ch3_vcrt) {
        mpi_errno = MPIDI_VCRT_Release(group_ptr->ch3_vcrt);
    }
    return mpi_errno;
}
