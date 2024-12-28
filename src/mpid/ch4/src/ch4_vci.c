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
    bool same_world = true;
    int nprocs = comm->local_size;
    int *granks;
    MPIR_CHKLMEM_MALLOC(granks, nprocs * sizeof(int));
    for (int i = 0; i < nprocs; i++) {
        int avtid;
        MPIDIU_comm_rank_to_pid(comm, i, &granks[i], &avtid);
        MPIR_Assert(avtid == 0);
    }

    /* for now, we only allow setup setup vcis for each remote once */
    for (int i = 0; i < nprocs; i++) {
        MPIR_Assert(MPIDI_global.all_num_vcis[granks[i]] == 0);
    }

    /* set up local vcis */
    int num_vcis_actual;
    mpi_errno = MPIDI_NM_init_vcis(MPIDI_global.n_total_vcis, &num_vcis_actual);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_global.n_total_vcis = num_vcis_actual;

    /* gather the number of remote vcis */
    int *all_num_vcis;
    MPIR_CHKLMEM_MALLOC(all_num_vcis, nprocs * sizeof(int));
    mpi_errno = MPIR_Allgather_impl(num_vcis_actual, 1, MPIR_INT_INTERNAL,
                                    all_num_vcis, 1, MPIR_INT_INTERNAL, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    for (int i = 0; i < nprocs; i++) {
        MPIDI_global.all_num_vcis[granks[i]] = all_num_vcis[i];
    }

    comm->vcis_enabled = true;

    for (int vci = 1; vci < MPIDI_global.n_total_vcis; vci++) {
        mpi_errno = MPIDI_init_per_vci(vci);
        MPIR_ERR_CHECK(mpi_errno);
    }

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
