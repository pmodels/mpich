/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"

/* There are two configurations: with or without RUNTIME_CHECKS.
 *
 * 1. Without RUNTIME_CHECKS. Global settings are macros defined in ofi_capabilites.h, and
 *    MPIDI_OFI_global.settings are not used.
 *    Global settings are used to init hints, which is used to get the first matching provider.
 *
 * 2. With RUNTIME_CHECKS. Macros are redirected to fields in MPIDI_OFI_global.settings.
 *    a. Find providers based on optimal and minimal settings. Global settings are not used at
 *       this stage and remain uninitialized. The optimal settings are the default set or the set
 *       that matches MPIR_CVAR_OFI_USE_PROVIDER.
 *    b. The selected provider is used to init hints. This step is covoluted since we want to
 *       reuse the init hints routine that is based on global settings.
 *       b.1. Initialize global.settings with default (or MPIR_CVAR_OFI_USE_PROVIDER).
 *       b.2. Init hints using global settings -- this step is the same as without RUNTIME_CHECKS
 *       b.3. Use the hints to get MPIDI_OFI_global.prov_use. This may take a few try, each time
 *            relaxing attributes such as tx_attr and domain_attr.
 *
 *  The hints is used to get the final MPIDI_OFI_global.prov_use, which is used to open the fabric.
 *  Global settings are finally updated again with the final prov_use. Note that in case of without
 *  RUNTIME_CHECKS, global settings are mostly macros so the update had little impact. Some settings
 *  such as "max_buffered_send" are always updated according to fi_info.
 *
 * Another way to understand this, there are three ways of control provider selection.
 * a. Static configuration without RUNTIME_CHECKS, e.g. `--with-device=ch4:ofi:sockets`
 *    Most global settings are macro constants. We try get the provider with initialized hints once,
 *    abort if fails.
 * b. With RUNTIME_CHECKS, use MPIR_CVAR_OFI_USE_PROVIDER. We set the optimal settings based on
 *    matching set. Thus, the static settings in ofi_capabilities.h can be used to control settings.
 * c. With RUNTIME_CHECKS, use FI_PROVIDER. We set the optimal settins based on default set.
 *
 * Since the optimal settings in RUNTIME_CHECKS affect the provider configuration (ref. 2-b.1),
 * there are subtle differences between b and c.
 *
 * CVARs such as MPIR_CVAR_CH4_OFI_ENABLE_TAGGED should overwrite both optimal and minimal settings,
 * thus effective in both provider selection and final global settings.
 */

static int find_provider(void);
static struct fi_info *pick_provider_from_list(const char *provname, struct fi_info *prov_list);

/* pick provider, set MPIDI_OFI_global.prov_use, and update global settings */
int MPIDI_OFI_init_provider(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* First, find the provider and prepare the hints */
    mpi_errno = find_provider();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_update_global_settings(MPIDI_OFI_global.prov_use);

    if (MPIR_CVAR_CH4_OFI_CAPABILITY_SETS_DEBUG && MPIR_Process.rank == 0) {
        MPIDI_OFI_dump_global_settings();
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* find best matching provider and set MPIDI_OFI_global.prov_use */
static int find_provider(void)
{
    int mpi_errno = MPI_SUCCESS;

    const char *provname = MPIR_CVAR_OFI_USE_PROVIDER;
    int ofi_version = MPIDI_OFI_get_fi_version();

    int ret;                    /* return from fi_getinfo() */
    struct fi_info *prov_list = NULL;   /* RUNTIME_CHECK first fi_getinfo() */
    struct fi_info *prov = NULL;        /* final fi_getinfo() with hints */

    struct fi_info *hints = fi_allocinfo();
    MPIR_Assert(hints != NULL);

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        MPIDI_OFI_CALL(fi_getinfo(ofi_version, NULL, NULL, 0ULL, NULL, &prov_list), getinfo);

        struct fi_info *prov_use = pick_provider_from_list(provname, prov_list);
        MPIR_ERR_CHKANDJUMP(prov_use == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        /* OFI provider doesn't expose FI_DIRECTED_RECV by default for performance consideration.
         * MPICH should request this flag to enable it. */
        if (prov_use->caps & FI_TAGGED) {
            prov_use->caps |= FI_DIRECTED_RECV;
        }

        /* Initialize MPIDI_OFI_global.settings */
        MPIDI_OFI_init_settings(&MPIDI_OFI_global.settings, provname);
        /* FIXME: this fills the hints with optimal settings. In order to work for providers
         * that support halfway between minimal and optimal, we need try-loop to systematically
         * relax settings.
         */
        /* Initialize hints based on MPIDI_OFI_global.settings */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = MPL_strdup(prov_use->fabric_attr->prov_name);
        hints->caps = prov_use->caps;
        hints->addr_format = prov_use->addr_format;

        /* try with the initial optimal hints */
        ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov);
        if (ret || prov == NULL) {
            /* relax msg_order */
            hints->tx_attr->msg_order = prov_use->tx_attr->msg_order;
            ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov);
        }
        if (ret || prov == NULL) {
            /* some provider still expects FI_MR_BASIC and FI_MR_SCALABLE */
            if (prov_use->domain_attr->mr_mode & FI_MR_BASIC) {
                hints->domain_attr->mr_mode = FI_MR_BASIC;
            } else if (prov_use->domain_attr->mr_mode & FI_MR_SCALABLE) {
                hints->domain_attr->mr_mode = FI_MR_SCALABLE;
            }
            ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov);
        }

        MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

    } else {
        /* Make sure that the user-specified provider matches the configure-specified provider. */
        MPIR_ERR_CHKANDJUMP(provname != NULL &&
                            MPIDI_OFI_SET_NUMBER != MPIDI_OFI_get_set_number(provname),
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
        /* Initialize hints based on configure time macros) */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = provname ? MPL_strdup(provname) : NULL;

        ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov);
        MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        int set_number = MPIDI_OFI_get_set_number(prov->fabric_attr->prov_name);
        MPIR_ERR_CHKANDJUMP(MPIDI_OFI_SET_NUMBER != set_number,
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
    }

    MPIDI_OFI_global.prov_use = fi_dupinfo(prov);
    MPIR_Assert(MPIDI_OFI_global.prov_use);

    /* stick to the original auto progress setting */
    MPIDI_OFI_global.prov_use->domain_attr->data_progress = hints->domain_attr->data_progress;
    MPIDI_OFI_global.prov_use->domain_attr->control_progress = hints->domain_attr->control_progress;

  fn_exit:
    if (prov_list) {
        fi_freeinfo(prov_list);
    }
    if (prov) {
        fi_freeinfo(prov);
    }

    /* prov_name is from MPL_strdup, can't let fi_freeinfo to free it */
    MPL_free(hints->fabric_attr->prov_name);
    hints->fabric_attr->prov_name = NULL;
    fi_freeinfo(hints);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static struct fi_info *pick_provider_from_list(const char *provname, struct fi_info *prov_list)
{
    bool provname_is_set = (provname &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_DEFAULT) != 0 &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_MINIMAL) != 0);

    MPIDI_OFI_capabilities_t optimal_settings, minimal_settings;
    MPIDI_OFI_init_settings(&optimal_settings, provname);
    MPIDI_OFI_init_settings(&minimal_settings, MPIDI_OFI_SET_NAME_MINIMAL);

    int best_score = 0;
    struct fi_info *best_prov = NULL;
    for (struct fi_info * prov = prov_list; prov; prov = prov->next) {
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
