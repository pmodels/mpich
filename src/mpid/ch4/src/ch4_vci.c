/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_NUM_VCIS
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Sets the number of VCIs to be implicitly used (should be a subset of MPIDI_CH4_MAX_VCIS).

    - name        : MPIR_CVAR_CH4_RESERVE_VCIS
      category    : CH4
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Sets the number of VCIs that user can explicitly allocate (should be a subset of MPIDI_CH4_MAX_VCIS).

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIDI_vci_init(void)
{
    /* Initialize multiple VCIs */
    /* TODO: add checks to ensure MPIDI_vci_t is padded or aligned to MPL_CACHELINE_SIZE */
    MPIR_Assert(MPIR_CVAR_CH4_NUM_VCIS >= 1);   /* number of vcis used in implicit vci hashing */
    MPIR_Assert(MPIR_CVAR_CH4_RESERVE_VCIS >= 0);       /* maximum number of vcis can be reserved */

    MPIDI_global.n_vcis = 1;
    MPIDI_global.n_total_vcis = 1;
    MPIDI_global.n_reserved_vcis = 0;
    MPIDI_global.share_reserved_vcis = false;

    MPIDI_global.all_num_vcis = MPL_calloc(MPIR_Process.size, sizeof(int), MPL_MEM_OTHER);
    MPIR_Assert(MPIDI_global.all_num_vcis);

    return MPI_SUCCESS;
}

int MPIDI_vci_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int vci = 1; vci < MPIDI_global.n_total_vcis; vci++) {
        mpi_errno = MPIDI_destroy_per_vci(vci);
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPL_free(MPIDI_global.all_num_vcis);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_Comm_set_vcis(MPIR_Comm * comm, int num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    /* FIXME: currently ofi require each process to have the same number of nics,
     *        thus need access to world_comm for collectives. We should remove
     *        this restriction, then we can move MPIDI_NM_init_vcis to
     *        MPIDI_world_pre_init.
     */
    MPIR_Assert(n_total_vcis <= MPIDI_CH4_MAX_VCIS);
    MPIR_Assert(n_total_vcis <= MPIR_REQUEST_NUM_POOLS);

    int num_vcis_actual;
    mpi_errno = MPIDI_NM_init_vcis(n_total_vcis, &num_vcis_actual);
    MPIR_ERR_CHECK(mpi_errno);

#if MPIDI_CH4_MAX_VCIS == 1
    MPIR_Assert(num_vcis_actual == 1);
#else
    MPIR_Assert(num_vcis_actual > 0 && num_vcis_actual <= MPIDI_global.n_total_vcis);

    MPIDI_global.n_total_vcis = num_vcis_actual;
    MPIDI_global.n_vcis = MPL_MIN(MPIR_CVAR_CH4_NUM_VCIS, MPIDI_global.n_total_vcis);

    mpi_errno = MPIR_Allgather_fallback(&MPIDI_global.n_vcis, 1, MPIR_INT_INTERNAL,
                                        MPIDI_global.all_num_vcis, 1, MPIR_INT_INTERNAL,
                                        MPIR_Process.comm_world, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    for (int vci = 1; vci < MPIDI_global.n_total_vcis; vci++) {
        mpi_errno = MPIDI_init_per_vci(vci);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Allocate_vci(int *vci, bool is_shared)
{
    int mpi_errno = MPI_SUCCESS;

    *vci = 0;
#if MPIDI_CH4_MAX_VCIS == 1
    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ch4nostream");
#else

    if (MPIDI_global.n_vcis + MPIDI_global.n_reserved_vcis >= MPIDI_global.n_total_vcis) {
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**outofstream");
    } else {
        MPIDI_global.n_reserved_vcis++;
        for (int i = MPIDI_global.n_vcis; i < MPIDI_global.n_total_vcis; i++) {
            if (!MPIDI_VCI(i).allocated) {
                MPIDI_VCI(i).allocated = true;
                *vci = i;
                break;
            }
        }
    }
#endif
    if (is_shared) {
        MPIDI_global.share_reserved_vcis = true;
    }
    return mpi_errno;
}

int MPID_Deallocate_vci(int vci)
{
    MPIR_Assert(vci < MPIDI_global.n_total_vcis && vci >= MPIDI_global.n_vcis);
    MPIR_Assert(MPIDI_VCI(vci).allocated);
    MPIDI_VCI(vci).allocated = false;
    MPIDI_global.n_reserved_vcis--;
    return MPI_SUCCESS;
}
