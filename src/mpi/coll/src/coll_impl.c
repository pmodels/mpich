/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"
#include "coll_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_DEVICE_COLLECTIVES
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI collectives will allow the device to override
        the MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.  If set to false,
        the device-level collective function will not be called.

    - name        : MPIR_CVAR_COLLECTIVE_FALLBACK
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR controls what the MPI library should do if the
        user-specified collective algorithm does not work for the
        arguments passed in by the user.  "0" means that the MPI
        library will raise an error.  "1" means that the MPI library
        will print an error message and fallback to another algorithm.
        "2" means that the MPI library will silently fallback to
        another algorithm.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Nbc_progress_hook_id = 0;

MPIR_Tree_type_t MPIR_Iallreduce_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;

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

    /* Ibcast */
    if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "kary"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "knomial_1"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "knomial_2"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;

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
    mpi_errno = MPII_Stubtran_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Gentran_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Stubalgo_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Treealgo_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Recexchalgo_init();
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

    mpi_errno = MPII_Gentran_finalize();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Stubtran_finalize();
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
    return MPII_Gentran_scheds_are_pending() == false;
}

/* Function to initialze communicators for collectives */
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
    mpi_errno = MPII_Stubtran_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Gentran_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPID_Coll_comm_init_hook(comm);
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

    /* cleanup all collective communicators */
    mpi_errno = MPII_Stubalgo_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Treealgo_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* cleanup transport data */
    mpi_errno = MPII_Stubtran_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Gentran_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
