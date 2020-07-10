/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

/* find best matching provider and populate hints */
static int find_provider(struct fi_info *hints);

/* picks one matching provider from the list or return NULL */
static struct fi_info *pick_provider_from_list(const char *provname, struct fi_info *prov_list);
static struct fi_info *pick_provider_by_name(const char *provname, struct fi_info *prov_list);
static struct fi_info *pick_provider_by_global_settings(struct fi_info *prov_list);

/* Need hold prov_list until we done setting up multi-nic */
static struct fi_info *prov_list = NULL;

void MPIDI_OFI_find_provider_cleanup(void)
{
    if (prov_list) {
        fi_freeinfo(prov_list);
    }
}

int MPIDI_OFI_find_provider(struct fi_info **prov_out)
{
    int mpi_errno = MPI_SUCCESS;

    /* First, find the provider and prepare the hints */
    struct fi_info *hints = fi_allocinfo();
    MPIR_Assert(hints != NULL);

    mpi_errno = find_provider(hints);
    MPIR_ERR_CHECK(mpi_errno);

    /* Second, get the actual fi_info * prov */
    MPIDI_OFI_CALL(fi_getinfo(MPIDI_OFI_get_ofi_version(), NULL, NULL, 0ULL, hints, &prov_list),
                   getinfo);

    struct fi_info *prov = prov_list;
    /* fi_getinfo may ignore the addr_format in hints, filter it again */
    if (hints->addr_format != FI_FORMAT_UNSPEC) {
        while (prov && prov->addr_format != hints->addr_format) {
            prov = prov->next;
        }
    }
    MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");
    if (!MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        int set_number = MPIDI_OFI_get_set_number(prov->fabric_attr->prov_name);
        MPIR_ERR_CHKANDJUMP(MPIDI_OFI_SET_NUMBER != set_number,
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
    }

    /* Third, update global settings */
    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        MPIDI_OFI_update_global_settings(prov, hints);
    }

    MPIR_Assert(prov);
    *prov_out = prov;

  fn_exit:
    /* prov_name is from MPL_strdup, can't let fi_freeinfo to free it */
    MPL_free(hints->fabric_attr->prov_name);
    hints->fabric_attr->prov_name = NULL;
    fi_freeinfo(hints);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Pick best matching provider and populate the hints accordingly */
static int find_provider(struct fi_info *hints)
{
    int mpi_errno = MPI_SUCCESS;

    const char *provname = MPIR_CVAR_OFI_USE_PROVIDER;
    int ofi_version = MPIDI_OFI_get_ofi_version();

    if (!MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        MPIDI_OFI_init_global_settings(MPIR_CVAR_OFI_USE_PROVIDER);
    } else {
        MPIDI_OFI_init_global_settings(MPIR_CVAR_OFI_USE_PROVIDER ? MPIR_CVAR_OFI_USE_PROVIDER :
                                       MPIDI_OFI_SET_NAME_DEFAULT);
    }

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        struct fi_info *prov_list, *prov_use;
        MPIDI_OFI_CALL(fi_getinfo(ofi_version, NULL, NULL, 0ULL, NULL, &prov_list), getinfo);

        prov_use = pick_provider_from_list(provname, prov_list);

        MPIR_ERR_CHKANDJUMP(prov_use == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        /* Initialize hints based on MPIDI_OFI_global.settings (updated by pick_provider_from_list()) */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = MPL_strdup(prov_use->fabric_attr->prov_name);
        hints->caps = prov_use->caps;
        hints->addr_format = prov_use->addr_format;

        fi_freeinfo(prov_list);
    } else {
        /* Make sure that the user-specified provider matches the configure-specified provider. */
        MPIR_ERR_CHKANDJUMP(provname != NULL &&
                            MPIDI_OFI_SET_NUMBER != MPIDI_OFI_get_set_number(provname),
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
        /* Initialize hints based on MPIDI_OFI_global.settings (config macros) */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = provname ? MPL_strdup(provname) : NULL;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */

#define DBG_TRY_PICK_PROVIDER(round) /* round is a str, eg "Round 1" */ \
    if (NULL == prov_use) { \
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, \
                        (MPL_DBG_FDEST, round ": find_provider returned NULL\n")); \
    } else { \
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, \
                        (MPL_DBG_FDEST, round ": find_provider returned %s\n", \
                        prov_use->fabric_attr->prov_name)); \
    }

static struct fi_info *pick_provider_from_list(const char *provname, struct fi_info *prov_list)
{
    struct fi_info *prov_use = NULL;
    /* We'll try to pick the best provider three times.
     * 1 - Check to see if any provider matches an existing capability set (e.g. sockets)
     * 2 - Check to see if any provider meets the default capability set
     * 3 - Check to see if any provider meets the minimal capability set
     */
    bool provname_is_set = (provname &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_DEFAULT) != 0 &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_MINIMAL) != 0);
    if (NULL == prov_use && provname_is_set) {
        prov_use = pick_provider_by_name((char *) provname, prov_list);
        DBG_TRY_PICK_PROVIDER("[match name]");
    }

    bool provname_is_minimal = (provname && strcmp(provname, MPIDI_OFI_SET_NAME_MINIMAL) == 0);
    if (NULL == prov_use && !provname_is_minimal) {
        MPIDI_OFI_init_global_settings(MPIDI_OFI_SET_NAME_DEFAULT);
        prov_use = pick_provider_by_global_settings(prov_list);
        DBG_TRY_PICK_PROVIDER("[match default]");
    }

    if (NULL == prov_use) {
        MPIDI_OFI_init_global_settings(MPIDI_OFI_SET_NAME_MINIMAL);
        prov_use = pick_provider_by_global_settings(prov_list);
        DBG_TRY_PICK_PROVIDER("[match minimal]");
    }

    return prov_use;
}

static struct fi_info *pick_provider_by_name(const char *provname, struct fi_info *prov_list)
{
    struct fi_info *prov, *prov_use = NULL;

    prov = prov_list;
    while (NULL != prov) {
        /* Match provider name exactly */
        if (0 != strcmp(provname, prov->fabric_attr->prov_name)) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST, "Skipping provider: name mismatch"));
            prov = prov->next;
            continue;
        }

        MPIDI_OFI_init_global_settings(prov->fabric_attr->prov_name);

        if (!MPIDI_OFI_match_global_settings(prov)) {
            prov = prov->next;
            continue;
        }

        prov_use = prov;

        break;
    }

    return prov_use;
}

static struct fi_info *pick_provider_by_global_settings(struct fi_info *prov_list)
{
    struct fi_info *prov, *prov_use = NULL;

    prov = prov_list;
    while (NULL != prov) {
        if (!MPIDI_OFI_match_global_settings(prov)) {
            prov = prov->next;
            continue;
        } else {
            prov_use = prov;
            break;
        }
    }

    return prov_use;
}
