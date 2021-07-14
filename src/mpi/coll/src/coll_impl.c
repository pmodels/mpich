/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
   - name : COLLECTIVE
     description : A category for collective communication variables.

cvars:
    - name        : MPIR_CVAR_DEVICE_COLLECTIVES
      category    : COLLECTIVE
      type        : enum
      default     : percoll
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select whether the device can override the
        MPIR-level collective algorithms.
        all     - Always prefer the device collectives
        none    - Never pick the device collectives
        percoll - Use the per-collective CVARs to decide

    - name        : MPIR_CVAR_COLLECTIVE_FALLBACK
      category    : COLLECTIVE
      type        : enum
      default     : silent
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to control what the MPI library should do if the
        user-specified collective algorithm does not work for the
        arguments passed in by the user.
        error   - throw an error
        print   - print an error message and fallback to the internally selected algorithm
        silent  - silently fallback to the internally selected algorithm

    - name        : MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of tuning file.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Nbc_progress_hook_id = 0;

MPIR_Tree_type_t MPIR_Iallreduce_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Allreduce_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Bcast_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;
void *MPIR_Csel_root = NULL;

int MPII_Coll_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Iallreduce */
    if (0 == strcmp(MPIR_CVAR_IALLREDUCE_TREE_TYPE, "kary"))
        MPIR_Iallreduce_tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_TREE_TYPE, "knomial_1"))
        MPIR_Iallreduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_TREE_TYPE, "knomial_2"))
        MPIR_Iallreduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;

    /* Allreduce */
    if (0 == strcmp(MPIR_CVAR_ALLREDUCE_TREE_TYPE, "knomial_1"))
        MPIR_Allreduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_ALLREDUCE_TREE_TYPE, "knomial_2"))
        MPIR_Allreduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        MPIR_Allreduce_tree_type = MPIR_TREE_TYPE_KARY;

    /* Ibcast */
    if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "kary"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "knomial_1"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "knomial_2"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;

    /* Bcast */
    if (0 == strcmp(MPIR_CVAR_BCAST_TREE_TYPE, "kary"))
        MPIR_Bcast_tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_BCAST_TREE_TYPE, "knomial_1"))
        MPIR_Bcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_BCAST_TREE_TYPE, "knomial_2"))
        MPIR_Bcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        MPIR_Bcast_tree_type = MPIR_TREE_TYPE_KARY;

    /* Ireduce */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_TREE_TYPE, "kary"))
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_TREE_TYPE, "knomial_1"))
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_TREE_TYPE, "knomial_2"))
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;

    /* register non blocking collectives progress hook */
    mpi_errno = MPIR_Progress_hook_register(MPIDU_Sched_progress, &MPIR_Nbc_progress_hook_id);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize transports */
    mpi_errno = MPII_TSP_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Stubalgo_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Treealgo_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Recexchalgo_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize selection tree */
    if (!strcmp(MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE, "")) {
        mpi_errno = MPIR_Csel_create_from_buf(MPII_coll_generic_json,
                                              MPII_Create_container, &MPIR_Csel_root);
    } else {
        mpi_errno = MPIR_Csel_create_from_file(MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE,
                                               MPII_Create_container, &MPIR_Csel_root);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_Coll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* deregister non blocking collectives progress hook */
    MPIR_Progress_hook_deregister(MPIR_Nbc_progress_hook_id);

    mpi_errno = MPII_TSP_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Csel_free(MPIR_Csel_root);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Function used by CH3 progress engine to decide whether to
 * block for a recv operation */
int MPIR_Coll_safe_to_block(void)
{
    return MPII_TSP_scheds_are_pending() == false;
}

/* Function to initialize communicators for collectives */
int MPIR_Coll_comm_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    comm->coll.pof2 = MPL_pof2(comm->local_size);

    /* initialize any stub algo related data structures */
    mpi_errno = MPII_Stubalgo_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);
    /* initialize any tree algo related data structures */
    mpi_errno = MPII_Treealgo_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize any transport data structures */
    mpi_errno = MPII_TSP_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Recexchalgo_comm_init(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Csel_prune(MPIR_Csel_root, comm, &comm->csel_comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Function to cleanup any communicators for collectives */
int MPII_Coll_comm_cleanup(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Csel_free(comm->csel_comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* cleanup all collective communicators */
    mpi_errno = MPII_Stubalgo_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Treealgo_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* cleanup transport data */
    mpi_errno = MPII_TSP_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Recexchalgo_comm_cleanup(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* For reduction-type collective, this routine swaps the (potential) device buffers
 * with allocated host-buffer */
void MPIR_Coll_host_buffer_alloc(const void *sendbuf, const void *recvbuf, MPI_Aint count,
                                 MPI_Datatype datatype, void **host_sendbuf, void **host_recvbuf)
{
    void *tmp;
    if (sendbuf != MPI_IN_PLACE) {
        tmp = MPIR_gpu_host_swap(sendbuf, count, datatype);
        *host_sendbuf = tmp;
    } else {
        *host_sendbuf = NULL;
    }

    if (recvbuf == NULL) {
        *host_recvbuf = NULL;
    } else if (sendbuf == MPI_IN_PLACE) {
        tmp = MPIR_gpu_host_swap(recvbuf, count, datatype);
        *host_recvbuf = tmp;
    } else {
        tmp = MPIR_gpu_host_alloc(recvbuf, count, datatype);
        *host_recvbuf = tmp;
    }
}

void MPIR_Coll_host_buffer_free(void *host_sendbuf, void *host_recvbuf)
{
    MPL_free(host_sendbuf);
    MPL_free(host_recvbuf);
}

void MPIR_Coll_host_buffer_swap_back(void *host_sendbuf, void *host_recvbuf, void *in_recvbuf,
                                     MPI_Aint count, MPI_Datatype datatype, MPIR_Request * request)
{
    if (!host_sendbuf && !host_recvbuf) {
        /* no copy (or free) at completion necessary, just return */
        return;
    }

    if (request == NULL || MPIR_Request_is_complete(request)) {
        /* operation is complete, copy the data and return */
        if (host_sendbuf) {
            MPIR_gpu_host_free(host_sendbuf, count, datatype);
        }
        if (host_recvbuf) {
            MPIR_gpu_swap_back(host_recvbuf, in_recvbuf, count, datatype);
        }
        return;
    }

    /* data will be copied later during request completion */
    request->u.nbc.coll.host_sendbuf = host_sendbuf;
    request->u.nbc.coll.host_recvbuf = host_recvbuf;
    if (host_recvbuf) {
        request->u.nbc.coll.user_recvbuf = in_recvbuf;
    }
    request->u.nbc.coll.count = count;
    request->u.nbc.coll.datatype = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
}

void MPIR_Coll_host_buffer_persist_set(void *host_sendbuf, void *host_recvbuf, void *in_recvbuf,
                                       MPI_Aint count, MPI_Datatype datatype,
                                       MPIR_Request * request)
{
    if (!host_sendbuf && !host_recvbuf) {
        /* no copy (or free) at completion necessary, just return */
        return;
    }

    /* data will be copied later during request completion */
    request->u.persist_coll.coll.host_sendbuf = host_sendbuf;
    request->u.persist_coll.coll.host_recvbuf = host_recvbuf;
    if (host_recvbuf) {
        request->u.persist_coll.coll.user_recvbuf = in_recvbuf;
        request->u.persist_coll.coll.count = count;
        request->u.persist_coll.coll.datatype = datatype;
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
    }
}
