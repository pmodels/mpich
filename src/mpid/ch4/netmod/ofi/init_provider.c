/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

/* There are two configurations: with or without RUNTIME_CHECKS.
 *
 * 1. With RUNTIME_CHECKS.
 *    Macros are redirected to fields in MPIDI_OFI_global.settings.
 *    a. First, get a list of providers by fi_getinfo with NULL hints. Environment
 *       variable FI_PROVIDER can be used to filter the list at libfabric layer.
 *    b. Pick providers based on optimal and minimal settings, and provider name if
 *       MPIR_CVAR_OFI_USE_PROVIDER is set. Global settings are not used at
 *       this stage and remain uninitialized. The optimal settings are the
 *       default set or the preset matching MPIR_CVAR_OFI_USE_PROVIDER.
 *    c. The selected provider is used to initialize hints and get final providers.
 *       c.1. Initialize global.settings with preset matching the selected provider name.
 *       c.2. Init hints using global settings.
 *       c.3. Use the hints to get final providers. This may take a
 *            few tries, each time relaxing attributes such as tx_attr and
 *            domain_attr.
 *
 * 2. Without RUNTIME_CHECKS, e.g configure --with-device=ch4:ofi:sockets.
 *    The step a and step b described above is skipped, and step c is simplified,
 *    Just initialize the hints and get the final providers.
 *
 * CVARs such as MPIR_CVAR_CH4_OFI_ENABLE_TAGGED should overwrite both optimal
 * and minimal settings, thus effective in both provider selection and final
 * global settings.
 */

static int find_provider(struct fi_info **prov_out);
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

    mpi_errno = find_provider(prov_out);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        MPIDI_OFI_update_global_settings(*prov_out);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* find best matching provider */
static int find_provider(struct fi_info **prov_out)
{
    int mpi_errno = MPI_SUCCESS;

    const char *provname = MPIR_CVAR_OFI_USE_PROVIDER;
    int ofi_version = MPIDI_OFI_get_ofi_version();

    int ret;                    /* return from fi_getinfo() */

    struct fi_info *hints = fi_allocinfo();
    MPIR_Assert(hints != NULL);

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        /* NOTE: prov_list should not be freed until we initialize multi-nic */
        MPIDI_OFI_CALL(fi_getinfo(ofi_version, NULL, NULL, 0ULL, NULL, &prov_list), getinfo);

        /* Pick a best matching provider and use it to refine hints */
        struct fi_info *prov;
        prov = pick_provider_from_list(provname, prov_list);

        MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        /* OFI provider doesn't expose FI_DIRECTED_RECV by default for performance consideration.
         * MPICH should request this flag to enable it. */
        if (prov->caps & FI_TAGGED) {
            prov->caps |= FI_DIRECTED_RECV;
        }

        /* Initialize MPIDI_OFI_global.settings */
        MPIDI_OFI_init_settings(&MPIDI_OFI_global.settings, provname);
        /* NOTE: we fill the hints with optimal settings. In order to work for providers
         * that support halfway between minimal and optimal, we need try-loop to systematically
         * relax settings. The following code does this for the providers that we have tested.
         */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = MPL_strdup(prov->fabric_attr->prov_name);
        hints->caps = prov->caps;
        hints->addr_format = prov->addr_format;

        /* Now we have the hints with best matched provider, get the new prov_list */
        struct fi_info *old_prov_list = prov_list;

        /* First try with the initial (optimal) hints */
        ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov_list);
        if (ret || prov_list == NULL) {
            /* relax msg_order */
            hints->tx_attr->msg_order = prov->tx_attr->msg_order;
            ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov_list);
        }
        if (ret || prov_list == NULL) {
            if (prov->domain_attr->mr_mode & FI_MR_BASIC) {
                hints->domain_attr->mr_mode = FI_MR_BASIC;
            } else if (prov->domain_attr->mr_mode & FI_MR_SCALABLE) {
                hints->domain_attr->mr_mode = FI_MR_SCALABLE;
            }
            ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov_list);
        }
        /* free the old one, the new one will be freed in MPIDI_OFI_find_provider_cleanup */
        fi_freeinfo(old_prov_list);
        MPIR_ERR_CHKANDJUMP(prov_list == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");
    } else {
        /* Make sure that the user-specified provider matches the configure-specified provider. */
        MPIR_ERR_CHKANDJUMP(provname != NULL &&
                            MPIDI_OFI_SET_NUMBER != MPIDI_OFI_get_set_number(provname),
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
        /* Initialize hints based on configure time macros) */
        MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = provname ? MPL_strdup(provname) : NULL;

        ret = fi_getinfo(ofi_version, NULL, NULL, 0ULL, hints, &prov_list);
        MPIR_ERR_CHKANDJUMP(prov_list == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        int set_number = MPIDI_OFI_get_set_number(prov_list->fabric_attr->prov_name);
        MPIR_ERR_CHKANDJUMP(MPIDI_OFI_SET_NUMBER != set_number,
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
    }

    MPIDI_OFI_set_auto_progress(prov_list);
    *prov_out = prov_list;

  fn_exit:
    /* prov_name is from MPL_strdup, can't let fi_freeinfo to free it */
    MPL_free(hints->fabric_attr->prov_name);
    hints->fabric_attr->prov_name = NULL;
    fi_freeinfo(hints);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */

static const char *get_prov_addr(struct fi_info *prov)
{
    static char addr_str[1024];
    char addr_buf[500];
    switch (prov->addr_format) {
        case FI_SOCKADDR_IN:
            sprintf(addr_str, "FI_SOCKADDR_IN [%zd] %s", prov->src_addrlen,
                    inet_ntoa(((struct sockaddr_in *) prov->src_addr)->sin_addr));
            break;
        case FI_SOCKADDR_IN6:
            sprintf(addr_str, "FI_SOCKADDR_IN6 [%zd] %s", prov->src_addrlen,
                    inet_ntop(AF_INET6, &((struct sockaddr_in6 *) prov->src_addr)->sin6_addr,
                              addr_buf, 500));
            break;
        case FI_SOCKADDR_IB:
            sprintf(addr_str, "FI_SOCKADDR_IB [%zd]", prov->src_addrlen);
            break;
        case FI_ADDR_PSMX:
            sprintf(addr_str, "FI_ADDR_PSMX [%zd]", prov->src_addrlen);
            break;
        case FI_ADDR_GNI:
            sprintf(addr_str, "FI_ADDR_GNI [%zd]", prov->src_addrlen);
            break;
        case FI_ADDR_STR:
            snprintf(addr_str, 1024, "FI_ADDR_STR [%zd] - %s", prov->src_addrlen,
                     (char *) prov->src_addr);
            break;
        default:
            sprintf(addr_str, "FI_FORMAT_UNSPEC [%zd]", prov->src_addrlen);
    }
    return addr_str;
}

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
            printf("provider: %s, score = %d, %s\n", prov->fabric_attr->prov_name, score,
                   get_prov_addr(prov));
        }
    }

    return best_prov;
}
