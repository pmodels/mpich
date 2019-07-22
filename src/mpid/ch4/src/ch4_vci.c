/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ch4_vci.h"

#ifndef MPIDI_CH4_DIRECT_NETMOD
static void vci_arm(MPIDI_vci_t * vci, int vni, int vsi);
#else
static void vci_arm(MPIDI_vci_t * vci, int vni);
#endif
static void vci_unarm(MPIDI_vci_t * vci);
static void vci_get_free(int *free_vci);
static void vci_update_next_free();

#undef FUNCNAME
#define FUNCNAME MPIDI_vci_pool_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
#ifndef MPIDI_CH4_DIRECT_NETMOD
int MPIDI_vci_pool_alloc(int num_vsis, int num_vnis)
#else
int MPIDI_vci_pool_alloc(int num_vnis)
#endif
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    /* Statically allocate the maximum possible size of the CH4 VCI pool.
     *      * The root VCI takes 1 NM and 1 SHM VCI. The rest could be VSI-only
     *           * or VNI-only VCIs. It is guaranteed that we have at least 1 VNI and
     *                * at least 1 VSI. */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_VCI_POOL(max_vcis) = 1 + (num_vnis - 1) + (num_vsis - 1);
#else
    MPIDI_VCI_POOL(max_vcis) = num_vnis;
#endif

    MPIDI_VCI_POOL(vci) = MPL_malloc(MPIDI_VCI_POOL(max_vcis) * sizeof(MPIDI_vci_t), MPL_MEM_OTHER);

    /* Initialize all VCIs in the pool */
    for (i = 0; i < MPIDI_VCI_POOL(max_vcis); i++) {
        MPID_Thread_mutex_create(&MPIDI_VCI(i).lock, &mpi_errno);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        /* A SEND request covers all possible use-cases */
        MPIDI_VCI(i).lw_req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(MPIDI_VCI(i).lw_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                "**nomemreq");
        MPIDI_REQUEST(MPIDI_VCI(i).lw_req, vci) = i;
         MPIR_cc_set(&MPIDI_VCI(i).lw_req->cc, 0);
        
        MPIDI_VCI(i).ref_count = 0;

        MPIDI_VCI(i).vni = MPIDI_NM_VNI_INVALID;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        MPIDI_VCI(i).vsi = MPIDI_SHM_VSI_INVALID;
#endif
        MPIDI_VCI(i).is_free = 1;
    }

    MPID_Thread_mutex_create(&MPIDI_VCI_POOL(lock), &mpi_errno);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    MPIDI_VCI_POOL(next_free_vci) = MPIDI_VCI_ROOT;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

 /* This is called only during finalize time. So, no thread
  *  * safety required here. */
#undef FUNCNAME
#define FUNCNAME MPIDI_vci_pool_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_vci_pool_free(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    /* Free the VCIs that have not been freed yet */
    for (i = 0; i < MPIDI_VCI_POOL(max_vcis); i++) {
        if (!MPIDI_VCI(i).is_free) {
            mpi_errno = MPIDI_vci_free(i);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
    }

    MPID_Thread_mutex_destroy(&MPIDI_VCI_POOL(lock), &mpi_errno);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    MPL_free(MPIDI_VCI_POOL(vci));

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifndef MPIDI_CH4_DIRECT_NETMOD
static void vci_arm(MPIDI_vci_t * vci, int vni, int vsi)
#else
static void vci_arm(MPIDI_vci_t * vci, int vni)
#endif
{
    vci->vni = vni;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    vci->vsi = vsi;
#endif
    vci->is_free = 0;
}

static void vci_unarm(MPIDI_vci_t * vci)
{
    vci->vni = MPIDI_NM_VNI_INVALID;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    vci->vsi = MPIDI_SHM_VSI_INVALID;
#endif
    vci->is_free = 1;
}

static void vci_get_free(int *free_vci)
{
    int i;

    *free_vci = MPIDI_VCI_INVALID;

    for (i = 0; i < MPIDI_VCI_POOL(max_vcis); i++) {
        if (MPIDI_VCI(i).is_free) {
            *free_vci = i;
            return;
        }
    }
}

static void vci_update_next_free()
{
    int new_next_free_vci;

    new_next_free_vci = MPIDI_VCI_POOL(next_free_vci)++;

    if (new_next_free_vci >= MPIDI_VCI_POOL(max_vcis) || !MPIDI_VCI(new_next_free_vci).is_free) {
        vci_get_free(&new_next_free_vci);
    }

    MPIDI_VCI_POOL(next_free_vci) = new_next_free_vci;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_vci_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_vci_alloc(MPIDI_vci_type_t type, MPIDI_vci_resource_t resources,
                    MPIDI_vci_property_t properties, int *vci)
{
    int mpi_errno = MPI_SUCCESS;
    int my_vci, my_vni;

    my_vci = MPIDI_VCI_INVALID;
    my_vni = MPIDI_NM_VNI_INVALID;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    int my_vsi;
    my_vsi = MPIDI_SHM_VSI_INVALID;
#endif

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_POOL(lock));

    if (MPIDI_VCI_POOL(next_free_vci) == MPIDI_VCI_ROOT) {
        my_vci = MPIDI_VCI_ROOT;
        /* Allocate a VNI for the root VCI */
        mpi_errno =
            MPIDI_NM_vni_alloc(MPIDI_VCI_RESOURCE__GENERIC, MPIDI_VCI_PROPERTY__GENERIC, &my_vni);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        if (my_vni == MPIDI_NM_VNI_INVALID) {
            goto fn_exit;
        }
#ifndef MPIDI_CH4_DIRECT_NETMOD
        /* Allocate a VSI for the root VCI */
        mpi_errno =
            MPIDI_SHM_vsi_alloc(MPIDI_VCI_RESOURCE__GENERIC, MPIDI_VCI_PROPERTY__GENERIC, &my_vsi);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        if (my_vsi == MPIDI_SHM_VSI_INVALID) {
            MPIDI_NM_vni_free(my_vni);
            goto fn_exit;
        }
#endif
    } else {
        switch (type) {
            case MPIDI_VCI_TYPE__EXCLUSIVE:
                /* Obtain VNI, and/or VSI, and VCI in that order */
                if (resources & MPIDI_VCI_RESOURCE__NM) {
                    /* Allocate a VNI */
                    mpi_errno = MPIDI_NM_vni_alloc(resources, properties, &my_vni);
                    if (mpi_errno != MPI_SUCCESS) {
                        MPIR_ERR_POP(mpi_errno);
                    }
                    if (my_vni == MPIDI_NM_VNI_INVALID) {
                        goto fn_exit;
                    }
                }
#ifndef MPIDI_CH4_DIRECT_NETMOD
                if (resources & MPIDI_VCI_RESOURCE__SHM) {
                    /* Allocate a VSI */
                    mpi_errno = MPIDI_SHM_vsi_alloc(resources, properties, &my_vsi);
                    if (mpi_errno != MPI_SUCCESS) {
                        MPIR_ERR_POP(mpi_errno);
                    }
                    if (my_vsi == MPIDI_SHM_VSI_INVALID) {
                        if (resources & MPIDI_VCI_RESOURCE__NM) {
                            MPIDI_NM_vni_free(my_vni);
                        }
                        goto fn_exit;
                    }
                }
#endif
                /* Get a free VCI */
                my_vci = MPIDI_VCI_POOL(next_free_vci);
                if (my_vci == MPIDI_VCI_INVALID) {
                    /* Search for a free VCI. It must have been freed before this alloc.
                     *                      * Otherwise the VNI or VSI alloc should have failed. */
                    vci_get_free(&my_vci);
                }
                break;
            case MPIDI_VCI_TYPE__SHARED:
                /* Return the root VCI */
                my_vci = MPIDI_VCI_ROOT;
                goto update_ref_count;
        }
    }
    /* Arm this VCI */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    vci_arm(&MPIDI_VCI(my_vci), my_vni, my_vsi);
#else
    vci_arm(&MPIDI_VCI(my_vci), my_vni);
#endif
    /* Update next free VCI */
    vci_update_next_free();
  update_ref_count:
    /* Increment the reference counter for this VCI */
    MPIDI_VCI(my_vci).ref_count++;
  fn_exit:
    *vci = my_vci;
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_POOL(lock));
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_vci_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_vci_free(int vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_vci_t *this_vci;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_POOL(lock));

    this_vci = &MPIDI_VCI(vci);
    this_vci->ref_count--;

    if (this_vci->ref_count == 0) {
        if (this_vci->vni != MPIDI_NM_VNI_INVALID) {
            /* Free the VNI */
            mpi_errno = MPIDI_NM_vni_free(this_vci->vni);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (this_vci->vsi != MPIDI_SHM_VSI_INVALID) {
            /* Free the VSI */
            mpi_errno = MPIDI_SHM_vsi_free(this_vci->vsi);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
#endif
        /* Unarm this VCI */
        vci_unarm(this_vci);
    }
  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_POOL(lock));
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#if MPIDI_CH4_VCI_METHOD == MPICH_VCL__TLS

#ifdef MPL_TLS
MPL_TLS int MPIDI_vci_src = 0;
MPL_TLS int MPIDI_vci_dst = 0;
#else
int MPIDI_vci_src = 0;
int MPIDI_vci_dst = 0;
#endif

int MPIX_set_vci(int vci_src, int vci_dst)
{
    MPIDI_vci_src = vci_src % MPIDI_VCI_POOL(max_vcis);
    MPIDI_vci_dst = vci_dst % MPIDI_VCI_POOL(max_vcis);
    return MPI_SUCCESS;
}

#else
int MPIX_set_vci(int vci_src, int vci_dst)
{
    return MPI_SUCCESS;
}

#endif
