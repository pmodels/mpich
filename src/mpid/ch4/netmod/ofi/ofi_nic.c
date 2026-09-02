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

    - name        : MPIR_CVAR_CH4_OFI_NIC_GPU_AFFINITY
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Controls whether NIC selection considers GPU locality, rather than CPU/NUMA
        locality. Disabled by default (0). When set to 1, selects NIC(s) with the
        closest PCI proximity to active GPU(s) (i.e. behind the same PCIe switch).
        This is currently only implemented for CUDA.

        IMPORTANT: MPICH has no notion of a per-rank GPU assignment; it inspects the
        first CUDA device visible to the process (local ordinal 0). For best results
        per-rank CUDA_VISIBLE_DEVICES should be constrained to device(s) that could
        effectively share the same NIC assignments.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* ---------------------------------- */
/* NIC affinity mask type and macros  */
/* ---------------------------------- */

#define MPIDI_OFI_NIC_MASK_WORD_SIZE 64
#define MPIDI_OFI_NIC_MASK_WORDS (MPIDI_OFI_MAX_NICS / MPIDI_OFI_NIC_MASK_WORD_SIZE)
MPL_static_assert(MPIDI_OFI_MAX_NICS % MPIDI_OFI_NIC_MASK_WORD_SIZE == 0,
                  "MPIDI_OFI_MAX_NICS must be a multiple of MPIDI_OFI_NIC_MASK_WORD_SIZE");

typedef struct {
    uint64_t bits[MPIDI_OFI_NIC_MASK_WORDS];
} MPIDI_OFI_nic_mask_t;

#define MPIDI_OFI_NIC_MASK_SET(mask, nic) \
    ((mask).bits[(nic) / 64] |= (uint64_t) 1 << ((nic) % 64))

/* ---------------------------------- */
/* Forward declarations for static    */
/* functions called from public APIs  */
/* ---------------------------------- */

static bool match_prov_addr(struct fi_info *prov, const char *hostname);
static int compare_nic_names(const void *info1, const void *info2);
static int compare_nic_closeness(const void *nic1, const void *nic2);
static int order_multi_nic_by_pref(int pref);
#ifdef HAVE_LIBFABRIC_NIC
static bool is_nic_pci_valid(struct fi_info *info);
static void refine_nic_close_by_gpu(MPIDI_OFI_nic_info_t * nics, int nic_count);
static int set_nic_info(void);
#endif

/* ================================== */
/* Public MPIDI_OFI functions         */
/* ================================== */

/* -- NIC status ------------------------------------------------ */

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

#ifdef HAVE_LIBFABRIC_NIC
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

/* -- NIC initialization ---------------------------------------- */

int MPIDI_OFI_fill_prov_use(struct fi_info *prov)
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

        MPIDI_OFI_global.num_nics_available = 1;
        MPIDI_OFI_global.num_nics = 1;
        MPIDI_OFI_global.nic_info[0].nic = MPIDI_OFI_global.prov_use[0];
        MPIDI_OFI_global.nic_info[0].id = 0;
        MPIDI_OFI_global.nic_info[0].close = 1;
    } else {
        MPIDI_OFI_global.num_nics_available = nic_count;

        /* nic_count >= 1 */
        /* Initially sort the NICs by name. This way all intranode ranks have a consistent view. */
        qsort(MPIDI_OFI_global.prov_use, nic_count, sizeof(struct fi_info *), compare_nic_names);

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
        MPIDI_OFI_global.num_nics = num_nics;

        for (int i = 0; i < MPIDI_OFI_global.num_nics_available; i++) {
            MPIDI_OFI_global.nic_info[i].nic = MPIDI_OFI_global.prov_use[i];
            MPIDI_OFI_global.nic_info[i].id = i;
            MPIDI_OFI_global.nic_info[i].close = 0;
        }

        if (MPIR_CVAR_CH4_OFI_PREF_NIC > -1 &&
            MPIR_CVAR_CH4_OFI_PREF_NIC < MPIDI_OFI_global.num_nics_available) {
            /* set the pref_nic's close to 1, all other nics' close to 0 */
            MPIDI_OFI_global.nic_info[MPIR_CVAR_CH4_OFI_PREF_NIC].close = 1;
        } else {
#ifdef HAVE_LIBFABRIC_NIC
            /* check all nics have valid pci info */
            bool all_valid = true;
            for (int i = 0; i < nic_count; ++i) {
                if (!is_nic_pci_valid(MPIDI_OFI_global.prov_use[i])) {
                    all_valid = false;
                    break;
                }
            }
            if (all_valid) {
                /* determine close via nic_pci */
                mpi_errno = set_nic_info();
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                /* just choose the first nic */
                MPIDI_OFI_global.nic_info[0].close = 1;
            }
        }
#else
            /* just choose the first nic */
            MPIDI_OFI_global.nic_info[0].close = 1;
        }
#endif
    }

    int num_close_nics = 0;
    for (int i = 0; i < MPIDI_OFI_global.num_nics_available; i++) {
        if (MPIDI_OFI_global.nic_info[i].close) {
            num_close_nics++;
        }
    }
    /* If there were zero NICs on my socket, then just consider every NIC close
     * and share them among all ranks with a similar view */
    if (num_close_nics == 0) {
        for (int i = 0; i < MPIDI_OFI_global.num_nics_available; i++) {
            MPIDI_OFI_global.nic_info[i].close = 1;
        }
        num_close_nics = MPIDI_OFI_global.num_nics_available;
    }

    MPIDI_OFI_global.num_close_nics = num_close_nics;

    MPIR_Assert(MPIDI_OFI_global.num_nics_available > 0);
    MPIR_Assert(MPIDI_OFI_global.num_close_nics > 0);

    char nics_str[32];
    MPIR_Info *info_ptr = NULL;
    MPIR_Info_get_ptr(MPI_INFO_ENV, info_ptr);

    snprintf(nics_str, 32, "%d", MPIDI_OFI_global.num_nics_available);
    MPIR_Info_set_impl(info_ptr, "num_nics_available", nics_str);
    snprintf(nics_str, 32, "%d", MPIDI_OFI_global.num_close_nics);
    MPIR_Info_set_impl(info_ptr, "num_close_nics", nics_str);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- NIC ordering ---------------------------------------------- */

int MPIDI_OFI_order_multi_nic_local(void)
{
    /* TODO: pass comm and use comm->local_rank */
    int pref = MPIR_Process.local_rank % MPIDI_OFI_global.num_close_nics;

    return order_multi_nic_by_pref(pref);
}

/* Exchange NIC affinity masks across node-local ranks and compute a preferred
 * NIC index that distributes load across sharing sets. Returns the preferred
 * NIC index into close NICs, or -1 on failure (caller should fall back to local
 * assignment). */
static int compute_nic_pref_global(MPIR_Comm * node_comm)
{
    int mpi_errno = MPI_SUCCESS;
    int local_rank = node_comm->rank;
    int local_size = node_comm->local_size;
    int num_nics = MPIDI_OFI_global.num_nics_available;
    int num_close = MPIDI_OFI_global.num_close_nics;
    int pref = -1;
    MPIDI_OFI_nic_mask_t *all_masks = NULL;

    /* Build local close_mask */
    MPIDI_OFI_nic_mask_t my_mask;
    memset(&my_mask, 0, sizeof(my_mask));
    for (int i = 0; i < num_nics; i++) {
        if (MPIDI_OFI_global.nic_info[i].close) {
            MPIDI_OFI_NIC_MASK_SET(my_mask, i);
        }
    }

    /* Allgather masks over node_comm */
    all_masks = MPL_malloc(local_size * sizeof(MPIDI_OFI_nic_mask_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!all_masks, mpi_errno, MPI_ERR_OTHER, "**nomem");

    memcpy(&all_masks[local_rank], &my_mask, sizeof(my_mask));
    mpi_errno = MPIR_Allgather_fallback(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                        all_masks, sizeof(MPIDI_OFI_nic_mask_t),
                                        MPIR_BYTE_INTERNAL, node_comm, MPIR_COLL_ATTR_SYNC);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDI_OFI_global.fabric_initialized) {
        goto fn_exit;
    }

    /* detect overlapping but unequal masks. this would imply a very strange
     * architecture and equitable distribution of NICs in that case would be
     * complicated. Warn and fallback to naive rotation. */
    for (int i = 0; i < local_size; i++) {
        for (int j = i + 1; j < local_size; j++) {
            if (memcmp(&all_masks[i], &all_masks[j], sizeof(my_mask)) == 0)
                continue;
            for (int w = 0; w < MPIDI_OFI_NIC_MASK_WORDS; w++) {
                if (all_masks[i].bits[w] & all_masks[j].bits[w]) {
                    MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                "NIC affinity masks overlap but are not equal; "
                                "falling back to local_rank assignment");
                    pref = -1;
                    goto fn_exit;
                }
            }
        }
    }

    /* Compute my index within my sharing set (ranks with identical mask) */
    int my_index_in_set = 0;
    for (int i = 0; i < local_rank; i++) {
        if (memcmp(&all_masks[i], &my_mask, sizeof(my_mask)) == 0) {
            my_index_in_set++;
        }
    }

    /* Sort NICs so that close NICs come first */
    qsort(MPIDI_OFI_global.nic_info, num_nics,
          sizeof(MPIDI_OFI_global.nic_info[0]), compare_nic_closeness);

    /* Round-robin close NICs across ranks in the same sharing set */
    pref = my_index_in_set % num_close;

  fn_exit:
    MPL_free(all_masks);
    return pref;
  fn_fail:
    pref = -1;
    goto fn_exit;
}

int MPIDI_OFI_order_multi_nic_global(MPIR_Comm * node_comm)
{
    int pref = compute_nic_pref_global(node_comm);

    if (MPIDI_OFI_global.fabric_initialized) {
        return MPI_SUCCESS;
    }

    if (pref < 0) {
        /* Fallback: no global info available or unsupported topology */
        pref = node_comm->rank % MPIDI_OFI_global.num_close_nics;
    }
    return order_multi_nic_by_pref(pref);
}

/* ================================== */
/* Static internal functions          */
/* ================================== */

/* -- Provider matching ----------------------------------------- */

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

/* -- NIC comparison and ordering ------------------------------- */

/* Comparison function for NIC names. Used in qsort() */
static int compare_nic_names(const void *info1, const void *info2)
{
    const struct fi_info **n1 = (const struct fi_info **) info1;
    const struct fi_info **n2 = (const struct fi_info **) info2;
    return strcmp((*n1)->domain_attr->name, (*n2)->domain_attr->name);
}

/* Comparison function for NICs. This function is used in qsort(). */
static int compare_nic_closeness(const void *nic1, const void *nic2)
{
    const MPIDI_OFI_nic_info_t *i1 = (const MPIDI_OFI_nic_info_t *) nic1;
    const MPIDI_OFI_nic_info_t *i2 = (const MPIDI_OFI_nic_info_t *) nic2;
    if (i1->close && !i2->close)
        return -1;
    else if (i2->close && !i1->close)
        return 1;
    return compare_nic_names(&(i1->nic), &(i2->nic));
}

static int order_multi_nic_by_pref(int pref)
{
    MPIDI_OFI_nic_info_t *nics = MPIDI_OFI_global.nic_info;

    /* 1. move all close nics to the front of the list. */
    qsort(nics, MPIDI_OFI_global.num_nics_available, sizeof(nics[0]), compare_nic_closeness);

    /* 2. rotate the close nics by pref. */
    if (pref > 0) {
        MPIDI_OFI_nic_info_t *tmp_nics = MPL_malloc(pref * sizeof(nics[0]), MPL_MEM_OTHER);
        MPIR_Assert(tmp_nics);
        for (int i = 0; i < pref; i++) {
            tmp_nics[i] = nics[i];
        }
        for (int i = 0; i < MPIDI_OFI_global.num_close_nics - pref; i++) {
            nics[i] = nics[i + pref];
        }
        for (int i = 0; i < pref; i++) {
            nics[MPIDI_OFI_global.num_close_nics - pref + i] = tmp_nics[i];
        }
        MPL_free(tmp_nics);
    }

    /* 3. order the prov_use array as well */
    for (int i = 0; i < MPIDI_OFI_global.num_nics_available; ++i) {
        MPIDI_OFI_global.prov_use[i] = nics[i].nic;
    }

    return MPI_SUCCESS;
}

/* -- NIC closeness detection ----------------------------------- */

#ifdef HAVE_LIBFABRIC_NIC
/* Sometime the provider may report "null" pci info (looking at you: opx) */
static bool is_nic_pci_valid(struct fi_info *info)
{
    if (info->nic->bus_attr->bus_type == FI_BUS_PCI) {
        struct fi_pci_attr pci = info->nic->bus_attr->attr.pci;
        return (pci.domain_id > 0 || pci.bus_id > 0 || pci.device_id > 0 || pci.function_id > 0);
    }
    return false;
}

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

static bool get_is_snc4_with_cxi_nics(void)
{
    int num_numa_nodes = MPIR_hwtopo_get_num_numa_nodes();

    if ((num_numa_nodes == 8 || num_numa_nodes == 16))
        if (MPIDI_OFI_global.num_nics_available > 1)
            if (strstr(MPIDI_OFI_global.prov_use[0]->domain_attr->name, "cxi"))
                return true;
    return false;
}

static void set_nic_close_snc4_with_cxi_nics(MPIDI_OFI_nic_info_t * nics, int nic_count)
{
    MPIR_hwtopo_gid_t parents[MPIDI_OFI_MAX_NICS] = { 0 };
    int num_parents = 0;

    /* Special case of nic assignment for SPR in SNC4 mode */
    for (int i = 0; i < nic_count; ++i) {
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
    }
}

/* Obtain the PCI BDF of the GPU this process is using. Returns true and fills
 * *domain, *bus, *dev, *func on success; returns false when GPU affinity is
 * unavailable (non-CUDA build, or no CUDA device / BDF resolvable).
 *
 * If the process has already bound a CUDA context, we use that context's
 * device; otherwise fall back to the first visible CUDA device; it's the user's
 * responsibility to constrain each rank to its intended GPU via
 * CUDA_VISIBLE_DEVICES (see the MPIR_CVAR_CH4_OFI_NIC_GPU_AFFINITY docs), or
 * make a CUDA context before getting this far. */
static bool get_selected_gpu_bdf(int *domain, int *bus, int *dev, int *func)
{
#ifdef MPL_HAVE_CUDA
    int rc = MPL_gpu_cuda_get_current_pci_bdf(domain, bus, dev, func);
    if (rc != MPL_SUCCESS) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

/* Extract a NIC's PCI BDF from its fi_info. Returns true only for NICs that
 * expose valid PCI bus info. */
static bool get_nic_bdf(struct fi_info *info, int *domain, int *bus, int *dev, int *func)
{
    if (!is_nic_pci_valid(info)) {
        return false;
    }
    struct fi_pci_attr pci = info->nic->bus_attr->attr.pci;
    *domain = pci.domain_id;
    *bus = pci.bus_id;
    *dev = pci.device_id;
    *func = pci.function_id;
    return true;
}

/* Replace the previously-computed (CPU/NUMA-affinity) "close" flags with a
 * GPU-locality-based selection. Closeness is based on direct GPU-NIC proximity
 * (e.g., sharing a PCI bridge), rather than NUMA node.
 *
 * If no NIC has a determinable proximity to the GPU, the CPU/NUMA result is
 * left intact so we never produce zero close NICs from GPU locality alone. */
static void refine_nic_close_by_gpu(MPIDI_OFI_nic_info_t * nics, int nic_count)
{
    if (MPIR_CVAR_CH4_OFI_NIC_GPU_AFFINITY != 1) {
        return;
    }

    int gdom, gbus, gdev, gfunc;
    if (!get_selected_gpu_bdf(&gdom, &gbus, &gdev, &gfunc)) {
        return;
    }

    /* Score each NIC's PCI proximity to the GPU and track the closest tier. */
    int best = MPIR_HWTOPO_PCI_PROXIMITY_NONE;
    int scores[MPIDI_OFI_MAX_NICS];
    for (int i = 0; i < nic_count; ++i) {
        int ndom, nbus, ndev, nfunc;
        int score = MPIR_HWTOPO_PCI_PROXIMITY_NONE;
        if (get_nic_bdf(nics[i].nic, &ndom, &nbus, &ndev, &nfunc)) {
            score = MPIR_hwtopo_get_pci_proximity(gdom, gbus, gdev, gfunc, ndom, nbus, ndev, nfunc);
        }
        scores[i] = score;
        if (score > best) {
            best = score;
        }
    }

    if (best <= MPIR_HWTOPO_PCI_PROXIMITY_ROOT) {
        /* Either no NIC's relationship to the GPU could be determined, or the
         * best any NIC achieves is a machine-root-only common ancestor. In the
         * latter case we successfully determined that GPU locality does not
         * favor any NIC (all are equally far), so overriding would just flatten
         * the CPU/NUMA distinctions. Either way, keep the CPU/NUMA selection. */
        return;
    }

    /* Keep only NICs in the closest tier (proximity == best). */
    for (int i = 0; i < nic_count; ++i) {
        nics[i].close = (scores[i] == best) ? 1 : 0;
    }
}

static int set_nic_info(void)
{
    MPIDI_OFI_nic_info_t *nics = MPIDI_OFI_global.nic_info;

    bool is_snc4_with_cxi_nics = get_is_snc4_with_cxi_nics();
    if (is_snc4_with_cxi_nics) {
        set_nic_close_snc4_with_cxi_nics(nics, MPIDI_OFI_global.num_nics_available);
    } else {
        for (int i = 0; i < MPIDI_OFI_global.num_nics_available; ++i) {
            nics[i].close = is_nic_close(nics[i].nic);
        }
    }

    refine_nic_close_by_gpu(nics, MPIDI_OFI_global.num_nics_available);

    return MPI_SUCCESS;
}

#endif
