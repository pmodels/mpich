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

/* enable multiple vcis for this comm.
 * The number of vcis below MPIR_CVAR_CH4_NUM_VCIS will be used for implicit vcis.
 * The number of vcis above MPIR_CVAR_CH4_NUM_VCIS will be used as explicit (reserved) vcis.
 * The netmod may create less than the requested number of vcis.
 */
int MPIDI_Comm_set_vcis(MPIR_Comm * comm, int num_vcis)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    /* for now, intracomm only. I believe we can enable it for intercomm in the future */
    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    /* make sure multiple vcis are not previously enabled. Or it will mess up the
     * internal communication during setting up vcis. */
    MPIR_Assert(!comm->vcis_enabled);
    /* actually, only do it once for now */
    MPIR_Assert(MPIDI_global.n_total_vcis == 1);

    /* get global ranks */
    int nprocs = comm->local_size;
    int *granks;
    MPIR_CHKLMEM_MALLOC(granks, nprocs * sizeof(int));
    for (int i = 0; i < nprocs; i++) {
        MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(comm, i);
        MPIR_Assert(MPIR_LPID_WORLD_INDEX(lpid) == 0);
        granks[i] = MPIR_LPID_WORLD_RANK(lpid);
    }

    /* for now, we only allow setup setup vcis for each remote once */
    for (int i = 0; i < nprocs; i++) {
        MPIR_Assert(MPIDI_global.all_num_vcis[granks[i]] == 0);
    }

    /* setup vcis in netmod and shm */
    /* Netmod will decide that actual number of vcis (and nics) and gather from all ranks in all_num_vcis */
    int *all_num_vcis;
    MPIR_CHKLMEM_MALLOC(all_num_vcis, nprocs * sizeof(int));
    mpi_errno = MPIDI_NM_comm_set_vcis(comm, num_vcis, all_num_vcis);
    MPIR_ERR_CHECK(mpi_errno);

    int n_total_vcis = all_num_vcis[comm->rank];
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_comm_set_vcis(comm, n_total_vcis);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    for (int vci = 1; vci < n_total_vcis; vci++) {
        mpi_errno = MPIDI_init_per_vci(vci);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* update global vci settings */
    MPIDI_global.n_total_vcis = all_num_vcis[comm->rank];
    MPIDI_global.n_vcis = MPL_MIN(MPIR_CVAR_CH4_NUM_VCIS, MPIDI_global.n_total_vcis);
    MPIDI_global.n_reserved_vcis = MPIDI_global.n_total_vcis - MPIDI_global.n_vcis;
    for (int i = 0; i < nprocs; i++) {
        MPIDI_global.all_num_vcis[granks[i]] = all_num_vcis[i];
    }

    /* enable multiple vcis */
    comm->vcis_enabled = true;

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
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
