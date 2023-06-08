/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"
#include "mpir_hwtopo.h"

#ifdef HAVE_LIBFABRIC_NIC
/* Return the parent object (typically socket) of the NIC */
static MPIR_hwtopo_gid_t get_nic_parent(struct fi_info *info)
{
    if (info->nic->bus_attr->bus_type == FI_BUS_PCI) {
        struct fi_pci_attr pci = info->nic->bus_attr->attr.pci;
        return MPIR_hwtopo_get_dev_parent_by_pci(pci.domain_id, pci.bus_id, pci.device_id,
                                                 pci.function_id);
    }
    return MPIR_hwtopo_get_obj_by_name(info->domain_attr->name);
}

/* Return true if the NIC is bound to the same socket as calling process */
static bool is_nic_close(struct fi_info *info)
{
    if (info->nic->bus_attr->bus_type == FI_BUS_PCI) {
        struct fi_pci_attr pci = info->nic->bus_attr->attr.pci;
        return MPIR_hwtopo_is_dev_close_by_pci(pci.domain_id, pci.bus_id, pci.device_id,
                                               pci.function_id);
    }
    return MPIR_hwtopo_is_dev_close_by_name(info->domain_attr->name);
}

/* Comparison function for NIC names. Used in qsort() */
static int compare_nic_names(const void *info1, const void *info2)
{
    const struct fi_info **n1 = (const struct fi_info **) info1;
    const struct fi_info **n2 = (const struct fi_info **) info2;
    return strcmp((*n1)->domain_attr->name, (*n2)->domain_attr->name);
}

/* Comparison function for NICs. This function is used in qsort(). */
static int compare_nics(const void *nic1, const void *nic2)
{
    const MPIDI_OFI_nic_info_t *i1 = (const MPIDI_OFI_nic_info_t *) nic1;
    const MPIDI_OFI_nic_info_t *i2 = (const MPIDI_OFI_nic_info_t *) nic2;
    if (i1->close && !i2->close)
        return -1;
    else if (i2->close && !i1->close)
        return 1;
    else if (i1->prefer - i2->prefer)
        return i1->prefer - i2->prefer;
    else if (i1->count - i2->count)
        return i1->count - i2->count;
    else if (i1->id - i2->id)
        return i1->id - i2->id;
    return compare_nic_names(&(i1->nic), &(i2->nic));
}

/* Determine if NIC has already been included in others */
bool MPIDI_OFI_nic_already_used(const struct fi_info * prov, struct fi_info ** others,
                                int nic_count)
{
    for (int i = 0; i < nic_count; ++i) {
        if (prov->nic->bus_attr->bus_type == FI_BUS_PCI &&
            others[i]->nic->bus_attr->bus_type == FI_BUS_PCI) {
            struct fi_pci_attr pci = prov->nic->bus_attr->attr.pci;
            struct fi_pci_attr others_pci = others[i]->nic->bus_attr->attr.pci;
            if (pci.domain_id == others_pci.domain_id && pci.bus_id == others_pci.bus_id &&
                pci.device_id == others_pci.device_id && pci.function_id == others_pci.function_id)
                return true;
        } else {
            if (strcmp(prov->domain_attr->name, others[i]->domain_attr->name) == 0)
                return true;
        }
    }
    return false;
}
#endif

/* Setup the multi-NIC data structures to use the fi_info structure given in prov */
static int setup_single_nic(void);
static int setup_multi_nic(int nic_count);

int MPIDI_OFI_init_multi_nic(struct fi_info *prov)
{
    int mpi_errno = MPI_SUCCESS;
    int nic_count = 0;
    int max_nics = MPIR_CVAR_CH4_OFI_MAX_NICS;

    if (MPIR_CVAR_CH4_OFI_MAX_NICS == 0 || MPIR_CVAR_CH4_OFI_MAX_NICS <= -2) {
        /* Invalid values for the CVAR will default to single nic */
        max_nics = 1;
    }

    /* Count the number of NICs */
    struct fi_info *first_prov = NULL;
    for (struct fi_info * p = prov; p; p = p->next) {
        /* additional filtering */
        if (MPIR_CVAR_OFI_SKIP_IPV6 && p->addr_format == FI_SOCKADDR_IN6) {
            continue;
        }
        if (!MPIDI_OFI_nic_is_up(p)) {
            continue;
        }
        if (!first_prov) {
            first_prov = p;
        }
#ifdef HAVE_LIBFABRIC_NIC
        /* check the nic */
        struct fid_nic *nic = p->nic;
        if (nic && nic->bus_attr->bus_type == FI_BUS_PCI &&
            !MPIDI_OFI_nic_already_used(p, MPIDI_OFI_global.prov_use, nic_count)) {
            MPIDI_OFI_global.prov_use[nic_count] = fi_dupinfo(p);
            MPIR_Assert(MPIDI_OFI_global.prov_use[nic_count]);
            nic_count++;
            if (nic_count == max_nics) {
                break;
            }
        }
#endif
    }

    if (nic_count == 0) {
        MPIR_Assert(first_prov);
        /* If no NICs are detected, then force using first provider */
        MPIDI_OFI_global.prov_use[0] = fi_dupinfo(first_prov);
        MPIR_Assert(MPIDI_OFI_global.prov_use[0]);
        mpi_errno = setup_single_nic();
        MPIR_ERR_CHECK(mpi_errno);
    }
#ifdef HAVE_LIBFABRIC_NIC
    else if (nic_count >= 1) {
        mpi_errno = setup_multi_nic(nic_count);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif
    else {
        mpi_errno = setup_single_nic();
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPIR_Assert(MPIDI_OFI_global.num_nics > 0);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int setup_single_nic(void)
{
    MPIDI_OFI_global.num_nics = 1;
    MPIDI_OFI_global.num_close_nics = 1;
    MPIDI_OFI_global.nic_info[0].nic = MPIDI_OFI_global.prov_use[0];
    MPIDI_OFI_global.nic_info[0].id = 0;
    MPIDI_OFI_global.nic_info[0].close = 1;
    MPIDI_OFI_global.nic_info[0].prefer = 0;
    MPIDI_OFI_global.nic_info[0].count = MPIR_Process.local_size;
    MPIDI_OFI_global.nic_info[0].num_close_ranks = MPIR_Process.local_size;

    char nics_str[32];
    MPIR_Info *info_ptr = NULL;
    MPIR_Info_get_ptr(MPI_INFO_ENV, info_ptr);
    snprintf(nics_str, 32, "%d", 1);
    MPIR_Info_set_impl(info_ptr, "num_nics", nics_str);
    snprintf(nics_str, 32, "%d", 1);
    MPIR_Info_set_impl(info_ptr, "num_close_nics", nics_str);

    return MPI_SUCCESS;
}

#ifdef HAVE_LIBFABRIC_NIC
/* TODO: Now that multiple NICs are detected, sort them based on preferred-ness,
 * closeness and count of other processes using the NIC. */
static int setup_multi_nic(int nic_count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_hwtopo_gid_t parents[MPIDI_OFI_MAX_NICS] = { 0 };
    int num_parents = 0;
    int local_rank = MPIR_Process.local_rank;
    MPIDI_OFI_nic_info_t *nics = MPIDI_OFI_global.nic_info;
    MPIR_CHKLMEM_DECL(1);

    MPIDI_OFI_global.num_nics = nic_count;

    /* Initially sort the NICs by name. This way all intranode ranks have a consistent view. */
    qsort(MPIDI_OFI_global.prov_use, MPIDI_OFI_global.num_nics, sizeof(struct fi_info *),
          compare_nic_names);

    /* Limit the number of physical NICs depending on the CVAR */
    if (MPIR_CVAR_CH4_OFI_MAX_NICS > 0 && MPIDI_OFI_global.num_nics > MPIR_CVAR_CH4_OFI_MAX_NICS) {
        for (int i = MPIR_CVAR_CH4_OFI_MAX_NICS; i < MPIDI_OFI_global.num_nics; ++i) {
            fi_freeinfo(MPIDI_OFI_global.prov_use[i]);
        }
        MPIDI_OFI_global.num_nics = MPIR_CVAR_CH4_OFI_MAX_NICS;
    }

    /* Now go through every NIC and set initial information
     * from current process's perspective */
    for (int i = 0; i < MPIDI_OFI_global.num_nics; ++i) {
        nics[i].nic = MPIDI_OFI_global.prov_use[i];
        nics[i].id = i;
        /* Determine NIC's "closeness" to current process */
        nics[i].close = is_nic_close(nics[i].nic);
        if (nics[i].close)
            MPIDI_OFI_global.num_close_nics++;
        /* Set the preference of all NICs to least preferable (lower is more preferable) */
        nics[i].prefer = MPIDI_OFI_global.num_nics + 1;
        nics[i].count = 0;
        nics[i].num_close_ranks = 0;
        /* Determine NIC's first normal parent topology
         * item (e.g., typically the socket parent) */
        nics[i].parent = get_nic_parent(nics[i].nic);
        /* Expand list of close NIC-parent topology items or increment */
        if (nics[i].close) {
            int found = 0;
            for (int j = 0; j < num_parents; ++j) {
                if (parents[j] == nics[i].parent) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                parents[num_parents] = nics[i].parent;
                num_parents++;
            }
        }
    }

    /* If there were zero NICs on my socket, then just consider every NIC close
     * and share them among all ranks with a similar view */
    if (MPIDI_OFI_global.num_close_nics == 0) {
        for (int i = 0; i < MPIDI_OFI_global.num_nics; ++i)
            nics[i].close = 1;
        MPIDI_OFI_global.num_close_nics = MPIDI_OFI_global.num_nics;
    }

    /* Sort the NICs array based on closeness first. This way all the close
     * NICs are at the beginning of the array */
    qsort(nics, MPIDI_OFI_global.num_nics, sizeof(nics[0]), compare_nics);

    /* Because we cannot communicate with the other local processes to avoid collisions with the
     * same NICs, just shift NICs that have multiple close NICs around according to their local
     * rank. This will likely give the same result as long as processes have been bound properly. */
    int old_idx = (MPIDI_OFI_global.num_close_nics == 0) ? 0 :
        local_rank % MPIDI_OFI_global.num_close_nics;

    if (old_idx != 0) {
        MPIDI_OFI_nic_info_t *old_nics;
        MPIR_CHKLMEM_MALLOC(old_nics, MPIDI_OFI_nic_info_t *, sizeof(MPIDI_OFI_nic_info_t) *
                            MPIDI_OFI_global.num_nics, mpi_errno, "temporary nic info",
                            MPL_MEM_ADDRESS);
        memcpy(old_nics, nics, sizeof(MPIDI_OFI_nic_info_t) * MPIDI_OFI_global.num_nics);

        /* Rotate the preferred NIC for each process starting at old_idx. */
        for (int new_idx = 0; new_idx < MPIDI_OFI_global.num_close_nics; new_idx++) {
            if (new_idx != old_idx)
                memcpy(&nics[new_idx], &old_nics[old_idx], sizeof(MPIDI_OFI_nic_info_t));

            if (++old_idx >= MPIDI_OFI_global.num_close_nics)
                old_idx = 0;
        }
    }

    /* Reorder the prov_use array based on nic_info array */
    for (int i = 0; i < MPIDI_OFI_global.num_nics; ++i) {
        MPIDI_OFI_global.prov_use[i] = nics[i].nic;
    }

    /* Set some info keys on MPI_INFO_ENV to reflect the number of available (close) NICs */
    char nics_str[32];
    MPIR_Info *info_ptr = NULL;
    MPIR_Info_get_ptr(MPI_INFO_ENV, info_ptr);
    snprintf(nics_str, 32, "%d", MPIDI_OFI_global.num_nics);
    MPIR_Info_set_impl(info_ptr, "num_nics", nics_str);
    snprintf(nics_str, 32, "%d", MPIDI_OFI_global.num_close_nics);
    MPIR_Info_set_impl(info_ptr, "num_close_nics", nics_str);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif

bool MPIDI_OFI_nic_is_up(struct fi_info *prov)
{
#ifdef HAVE_LIBFABRIC_NIC
    /* Make sure the NIC returned by OFI is not down. Some providers don't include NIC
     * information so we need to skip those. */
    if (prov->nic != NULL && prov->nic->link_attr->state == FI_LINK_DOWN) {
        return false;
    }
#endif

    return true;
}
