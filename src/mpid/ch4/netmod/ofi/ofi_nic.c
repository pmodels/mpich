/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"
#include "mpir_hwtopo.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_OFI_PREF_NIC
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Accept the NIC value from a user
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

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

/* Return true if the NIC is close to the group of the calling process */
static bool is_nic_close_snc4(const MPIDI_OFI_nic_info_t * nic_info, int num_parents)
{
    int nic_socket_gid = MPIR_hwtopo_get_parent_socket(nic_info->parent);
    int rank_socket_gid = MPIR_hwtopo_get_parent_socket(MPIR_hwtopo_get_first_pu_group());

    /* In SNC4 mode, when there are 4 groups that have nics, it means that there are 4
     * other adjacent groups with no nics. This leads to each set of 2 groups having 2 nics
     * such that, the first group has no nics and the second group has 2 nics.
     * The correct assignment strategy is such the 2 nics of the second group is considered
     * close to the ranks on both the groups.*/
    if (num_parents == 4) {
        /* Check that the parent socket of the rank and the nic is the same */
        if (nic_socket_gid == rank_socket_gid) {
            int nic_group_lid = MPIR_hwtopo_get_lid(nic_info->parent);
            int rank_group_lid = MPIR_hwtopo_get_lid(MPIR_hwtopo_get_first_pu_group());
            if (nic_group_lid == rank_group_lid || nic_group_lid - rank_group_lid == 1) {
                struct fi_info *info = (struct fi_info *) (nic_info->nic);
                if (info->nic->bus_attr->bus_type == FI_BUS_PCI) {
                    struct fi_pci_attr pci = info->nic->bus_attr->attr.pci;

                    int nic_lid = MPIR_hwtopo_get_pci_network_lid(pci.domain_id,
                                                                  pci.bus_id,
                                                                  pci.device_id,
                                                                  pci.function_id);

                    /* Map 1st nic of the group to the previous group */
                    if (nic_lid == 0 && nic_group_lid - rank_group_lid == 1)
                        return 1;
                    /* Map 2nd nic of the group to the current group */
                    else if (nic_lid == 1 && nic_group_lid == rank_group_lid)
                        return 1;
                }
            }
        }
    } else {
        /* On using a different configuration than having 4 num_parents, simply
         * compare parent socket of the nic and the rank */
        if (nic_socket_gid == rank_socket_gid)
            return 1;
    }
    return 0;
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
#ifdef HAVE_LIBFABRIC_NIC
static int setup_multi_nic(int nic_count);
#endif
static bool match_prov_addr(struct fi_info *prov, const char *hostname);

int MPIDI_OFI_init_multi_nic(struct fi_info *prov)
{
    int mpi_errno = MPI_SUCCESS;
    int nic_count = 0;

    /* Count the number of NICs */
    struct fi_info *pref_prov = NULL;
    for (struct fi_info * p = prov; p; p = p->next) {
        /* additional filtering */
        if (MPIR_CVAR_OFI_SKIP_IPV6 && p->addr_format == FI_SOCKADDR_IN6) {
            continue;
        }
        if (!MPIDI_OFI_nic_is_up(p)) {
            continue;
        }
        if (!pref_prov || match_prov_addr(p, MPIR_pmi_hostname())) {
            pref_prov = p;
        }
#ifdef HAVE_LIBFABRIC_NIC
        /* check the nic */
        struct fid_nic *nic = p->nic;
        if (nic && nic->bus_attr->bus_type == FI_BUS_PCI &&
            !MPIDI_OFI_nic_already_used(p, MPIDI_OFI_global.prov_use, nic_count)) {
            MPIDI_OFI_global.prov_use[nic_count] = fi_dupinfo(p);
            MPIR_Assert(MPIDI_OFI_global.prov_use[nic_count]);
            nic_count++;
            if (nic_count == MPIDI_OFI_MAX_NICS) {
                break;
            }
        }
#endif
    }

    if (nic_count == 0) {
        MPIR_ERR_CHKANDJUMP(!pref_prov, mpi_errno, MPI_ERR_OTHER, "**ofi_no_prov");
        /* If no NICs are detected, then force using first provider */
        MPIDI_OFI_global.prov_use[0] = fi_dupinfo(pref_prov);
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
/* Comparison function for NICs in SPR SNC4 mode. This function is used in qsort(). */
static int compare_nics_snc4(const void *nic1, const void *nic2)
{
    const MPIDI_OFI_nic_info_t *i1 = (const MPIDI_OFI_nic_info_t *) nic1;
    const MPIDI_OFI_nic_info_t *i2 = (const MPIDI_OFI_nic_info_t *) nic2;

    if (i1->close && !i2->close)
        return -1;
    else if (i2->close && !i1->close)
        return 1;
    return compare_nic_names(&(i1->nic), &(i2->nic));
}

/* TODO: Now that multiple NICs are detected, sort them based on preferred-ness,
 * closeness and count of other processes using the NIC. */
static int setup_multi_nic(int nic_count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_hwtopo_gid_t parents[MPIDI_OFI_MAX_NICS] = { 0 };
    int num_parents = 0;
    int local_rank = MPIR_Process.local_rank;
    MPIDI_OFI_nic_info_t *nics = MPIDI_OFI_global.nic_info;
    MPIR_CHKLMEM_DECL();

    int num_nics = 1;
    if (MPIR_CVAR_CH4_OFI_MAX_NICS == -1) {
        /* use all nics */
        num_nics = nic_count;
    } else if (MPIR_CVAR_CH4_OFI_MAX_NICS > 1) {
        /* use multiple nics */
        num_nics = MPL_MIN(MPIR_CVAR_CH4_OFI_MAX_NICS, nic_count);
    } else {
        /* default single nic (will selelct closest nic if nic_count > 1) */
        num_nics = 1;
    }

    bool pref_nic_set = false;
    if (MPIR_CVAR_CH4_OFI_PREF_NIC > -1 && MPIR_CVAR_CH4_OFI_PREF_NIC < nic_count) {
        pref_nic_set = true;
    }

    /* Initially sort the NICs by name. This way all intranode ranks have a consistent view. */
    qsort(MPIDI_OFI_global.prov_use, nic_count, sizeof(struct fi_info *), compare_nic_names);

    int num_numa_nodes = MPIR_hwtopo_get_num_numa_nodes();
    bool is_snc4_with_cxi_nics = false;

    if ((num_numa_nodes == 8 || num_numa_nodes == 16))
        if (nic_count > 1)
            if (strstr(MPIDI_OFI_global.prov_use[0]->domain_attr->name, "cxi"))
                is_snc4_with_cxi_nics = true;

    int num_close_nics = 0;

    /* Special case of nic assignment for SPR in SNC4 mode */
    if (is_snc4_with_cxi_nics && !pref_nic_set) {
        for (int i = 0; i < nic_count; ++i) {
            nics[i].nic = MPIDI_OFI_global.prov_use[i];
            nics[i].id = i;
            /* Set the preference of all NICs to least preferable (lower is more preferable) */
            nics[i].prefer = nic_count + 1;
            nics[i].count = 0;
            nics[i].num_close_ranks = 0;

            nics[i].parent = get_nic_parent(nics[i].nic);

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
        /* Use num_parents to determine nic closeness */
        for (int i = 0; i < nic_count; ++i) {
            nics[i].close = is_nic_close_snc4(&nics[i], num_parents);
            if (nics[i].close)
                num_close_nics++;
        }

    } else {
        /* General case of nic assignment */

        /* Now go through every NIC and set initial information
         * from current process's perspective */
        for (int i = 0; i < nic_count; ++i) {
            nics[i].nic = MPIDI_OFI_global.prov_use[i];
            nics[i].id = i;
            /* Determine NIC's "closeness" to current process */
            nics[i].close = is_nic_close(nics[i].nic);
            if (nics[i].close)
                num_close_nics++;
            /* Set the preference of all NICs to least preferable (lower is more preferable) */
            nics[i].prefer = nic_count + 1;
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
    }

    /* If there were zero NICs on my socket, then just consider every NIC close
     * and share them among all ranks with a similar view */
    if (num_close_nics == 0) {
        for (int i = 0; i < nic_count; ++i)
            nics[i].close = 1;
        num_close_nics = nic_count;
    }

    if (pref_nic_set) {
        /* When using this CVAR, the rank can only use 1 NIC. We do not reset num_close_nics again
         * in case a NIC is down and it needs to use another NIC. */
        MPIDI_OFI_nic_info_t *tmp_nic;
        MPIR_CHKLMEM_MALLOC(tmp_nic, sizeof(MPIDI_OFI_nic_info_t));
        int idx_to = 0;
        int idx_from = MPIR_CVAR_CH4_OFI_PREF_NIC;
        memcpy(tmp_nic, &nics[idx_from], sizeof(MPIDI_OFI_nic_info_t));
        memcpy(&nics[idx_from], &nics[idx_to], sizeof(MPIDI_OFI_nic_info_t));
        memcpy(&nics[idx_to], tmp_nic, sizeof(MPIDI_OFI_nic_info_t));
    } else {
        /* Sort the NICs array based on closeness first. This way all the close
         * NICs are at the beginning of the array */
        if (is_snc4_with_cxi_nics) {
            /* Use a separate sorting function for snc4 nics in order to just compare
             * closeness followed by nic name */
            qsort(nics, nic_count, sizeof(nics[0]), compare_nics_snc4);
        } else {
            qsort(nics, nic_count, sizeof(nics[0]), compare_nics);
        }

        /* Because we cannot communicate with the other local processes to avoid collisions with the
         * same NICs, just shift NICs that have multiple close NICs around according to their local
         * rank. This will likely give the same result as long as processes have been bound properly. */
        int old_idx = (num_close_nics == 0) ? 0 : local_rank % num_close_nics;

        if (old_idx != 0) {
            MPIDI_OFI_nic_info_t *old_nics;
            MPIR_CHKLMEM_MALLOC(old_nics, sizeof(MPIDI_OFI_nic_info_t) * nic_count);
            memcpy(old_nics, nics, sizeof(MPIDI_OFI_nic_info_t) * nic_count);

            /* Rotate the preferred NIC for each process starting at old_idx. */
            for (int new_idx = 0; new_idx < num_close_nics; new_idx++) {
                if (new_idx != old_idx)
                    memcpy(&nics[new_idx], &old_nics[old_idx], sizeof(MPIDI_OFI_nic_info_t));

                if (++old_idx >= num_close_nics)
                    old_idx = 0;
            }
        }
    }

    /* Reorder the prov_use array based on nic_info array */
    for (int i = 0; i < nic_count; ++i) {
        MPIDI_OFI_global.prov_use[i] = nics[i].nic;
    }

    /* Set globals */
    for (int i = num_nics; i < nic_count; i++) {
        fi_freeinfo(MPIDI_OFI_global.prov_use[i]);
    }
    MPIDI_OFI_global.num_nics = MPL_MIN(nic_count, num_nics);

    /* Set some info keys on MPI_INFO_ENV to reflect the number of available (close) NICs */
    char nics_str[32];
    MPIR_Info *info_ptr = NULL;
    MPIR_Info_get_ptr(MPI_INFO_ENV, info_ptr);
    snprintf(nics_str, 32, "%d", MPIDI_OFI_global.num_nics);
    MPIR_Info_set_impl(info_ptr, "num_nics", nics_str);
    snprintf(nics_str, 32, "%d", num_close_nics);
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

static bool match_prov_addr(struct fi_info *prov, const char *hostname)
{
    bool match = false;

    if (!hostname) {
        goto fn_exit;
    }

    char addr_buf[500];
    switch (prov->addr_format) {
        case FI_SOCKADDR_IN:
            inet_ntop(AF_INET, &((struct sockaddr_in *) prov->src_addr)->sin_addr, addr_buf, 500);
            match = (strcmp(hostname, addr_buf) == 0);
            break;
        case FI_SOCKADDR_IN6:
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *) prov->src_addr)->sin6_addr,
                      addr_buf, 500);
            match = (strcmp(hostname, addr_buf) == 0);
            break;
        case FI_SOCKADDR_IB:
            break;
        case FI_ADDR_PSMX:
            break;
        case FI_ADDR_GNI:
            break;
        case FI_ADDR_STR:
            match = (strcmp(hostname, (char *) prov->src_addr) == 0);
            break;
        default:
            break;
    }
  fn_exit:
    return match;
}
