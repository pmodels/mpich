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

    /* some hints need to be reset */
    MPIDI_OFI_set_auto_progress(prov);

    /* Third, update global settings */
    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        MPIDI_OFI_update_global_settings(prov);
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

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        struct fi_info *prov_list, *prov;
        MPIDI_OFI_CALL(fi_getinfo(ofi_version, NULL, NULL, 0ULL, NULL, &prov_list), getinfo);

        prov = pick_provider_from_list(provname, prov_list);

        MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        /* OFI provider doesn't expose FI_DIRECTED_RECV by default for performance consideration.
         * MPICH should request this flag to enable it. */
        if (prov->caps & FI_TAGGED) {
            prov->caps |= FI_DIRECTED_RECV;
        }

        /* Initialize MPIDI_OFI_global.settings */
        MPIDI_OFI_init_settings(&MPIDI_OFI_global.settings, provname);
        /* FIXME: this fills the hints with optimal settings. In order to work for providers
         * that support halfway between minimal and optimal, we need try-loop to systematically
         * relax settings.
         */
        /* Initialize hints based on MPIDI_OFI_global.settings */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = MPL_strdup(prov->fabric_attr->prov_name);
        hints->caps = prov->caps;
        hints->addr_format = prov->addr_format;

        fi_freeinfo(prov_list);
    } else {
        /* Make sure that the user-specified provider matches the configure-specified provider. */
        MPIR_ERR_CHKANDJUMP(provname != NULL &&
                            MPIDI_OFI_SET_NUMBER != MPIDI_OFI_get_set_number(provname),
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
        /* Initialize hints based on configure time macros) */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = provname ? MPL_strdup(provname) : NULL;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */

static struct fi_info *pick_provider_from_list(const char *provname, struct fi_info *list)
{
    bool provname_is_set = (provname &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_DEFAULT) != 0 &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_MINIMAL) != 0);

    MPIDI_OFI_capabilities_t optimal_settings, minimal_settings;
    MPIDI_OFI_init_settings(&optimal_settings, provname);
    MPIDI_OFI_init_settings(&minimal_settings, MPIDI_OFI_SET_NAME_MINIMAL);

    int best_score = 0;
    struct fi_info *best_prov = NULL;
    for (struct fi_info * prov = list; prov; prov = prov->next) {
        if (provname_is_set && 0 != strcmp(provname, prov->fabric_attr->prov_name)) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST, "Skipping provider: name mismatch"));
            continue;
        }

        int score = MPIDI_OFI_match_provider(prov, &optimal_settings, &minimal_settings);
        if (best_score < score) {
            best_score = score;
            best_prov = prov;
        }
        if (MPIR_CVAR_CH4_OFI_CAPABILITY_SETS_DEBUG && MPIR_Process.rank == 0) {
            printf("provider: %s, score = %d\n", prov->fabric_attr->prov_name, score);
        }
    }

    return best_prov;
}
