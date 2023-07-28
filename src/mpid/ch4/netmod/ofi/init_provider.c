/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_OFI_USE_PROVIDER
      category    : DEVELOPER
      type        : string
      default     : NULL
      class       : none
      verbosity   : MPI_T_VERBOSITY_MPIDEV_DETAIL
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        This variable is no longer supported. Use FI_PROVIDER instead to
        select libfabric providers.

    - name        : MPIR_CVAR_SINGLE_HOST_ENABLED
      category    : DEVELOPER
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_MPIDEV_DETAIL
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Set this variable to true to indicate that processes are
        launched on a single host. The current implication is to avoid
        the cxi provider to prevent the use of scarce hardware resources.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


/* There are two configurations: with or without RUNTIME_CHECKS.
 *
 * 1. With RUNTIME_CHECKS.
 *    Macros are redirected to fields in MPIDI_OFI_global.settings.
 *    a. First, get a list of providers by fi_getinfo with NULL hints. Environment
 *       variable FI_PROVIDER can be used to filter the list at libfabric layer.
 *    b. Pick providers based on optimal and minimal settings.  Global settings are not
 *       used at this stage and remain uninitialized. The optimal settings are the
 *       default set.
 *    c. The selected provider is used to initialize hints and get final providers.
 *       c.1. Initialize global.settings with preset matching the default set.
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
static struct fi_info *pick_provider_from_list(struct fi_info *prov_list);

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

int MPIDI_OFI_get_required_version(void)
{
    if (MPIDI_OFI_MAJOR_VERSION != -1 && MPIDI_OFI_MINOR_VERSION != -1)
        return FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION);
    else
        return fi_version();
}

/* find best matching provider */
static int find_provider(struct fi_info **prov_out)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;                    /* return from fi_getinfo() */

    if (MPIR_CVAR_OFI_USE_PROVIDER != NULL) {
        fprintf(stderr, "MPIR_CVAR_OFI_USE_PROVIDER is no longer supported in CH4. Use FI_PROVIDER"
                "instead\n");
    }

    int required_version = MPIDI_OFI_get_required_version();
    if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
        printf("Required minimum FI_VERSION: %x, current version: %x\n", required_version,
               fi_version());
    }

    struct fi_info *hints = fi_allocinfo();
    MPIR_Assert(hints != NULL);

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        /* NOTE: prov_list should not be freed until we initialize multi-nic */
        MPIDI_OFI_CALL(fi_getinfo(required_version, NULL, NULL, 0ULL, NULL, &prov_list), getinfo);

        /* Pick a best matching provider and use it to refine hints */
        struct fi_info *prov;
        prov = pick_provider_from_list(prov_list);

        MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        const char *provname = NULL;
        provname = prov->fabric_attr->prov_name;
        /* Initialize MPIDI_OFI_global.settings */
        MPIDI_OFI_init_settings(&MPIDI_OFI_global.settings, provname);
        /* The presets may have non-default minimum version requirement */
        required_version = MPIDI_OFI_get_required_version();
        if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
            printf("Required minimum FI_VERSION: %x, current version: %x\n", required_version,
                   fi_version());
        }
        /* NOTE: we fill the hints with optimal settings. In order to work for providers
         * that support halfway between minimal and optimal, we need try-loop to systematically
         * relax settings. The following code does this for the providers that we have tested.
         */
        mpi_errno = MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = MPL_strdup(provname);
        hints->caps = prov->caps;


        /* Now we have the hints with best matched provider, get the new prov_list */
        struct fi_info *old_prov_list = prov_list;

        /* First try with the initial (optimal) hints */
        ret = fi_getinfo(required_version, NULL, NULL, 0ULL, hints, &prov_list);
        if (ret || prov_list == NULL) {
            /* relax msg_order */
            hints->tx_attr->msg_order = prov->tx_attr->msg_order;
            ret = fi_getinfo(required_version, NULL, NULL, 0ULL, hints, &prov_list);
        }
        if (ret || prov_list == NULL) {
            /* relax mr_mode */
            hints->domain_attr->mr_mode = prov->domain_attr->mr_mode;
            ret = fi_getinfo(required_version, NULL, NULL, 0ULL, hints, &prov_list);
        }
        /* free the old one, the new one will be freed in MPIDI_OFI_find_provider_cleanup */
        fi_freeinfo(old_prov_list);
        MPIR_ERR_CHKANDJUMP(prov_list == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");
    } else {
        /* Make sure that the user-specified provider matches the configure-specified provider. */
        /* Initialize hints based on configure time macros) */
        mpi_errno = MPIDI_OFI_init_hints(hints);
        hints->fabric_attr->prov_name = MPL_strdup(MPIDI_OFI_PROV_NAME);

        ret = fi_getinfo(required_version, NULL, NULL, 0ULL, hints, &prov_list);
        MPIR_ERR_CHKANDJUMP(prov_list == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        int set_number = MPIDI_OFI_get_set_number(prov_list->fabric_attr->prov_name);
        MPIR_ERR_CHKANDJUMP(MPIDI_OFI_SET_NUMBER != set_number,
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");

        /* Some settings should always use runtime settings, ref. ofi_capability_sets.h */
#define UPDATE_SETTING_BY_SET_NUMBER(cap, CVAR) \
    MPIDI_OFI_global.settings.cap = (CVAR != -1) ? CVAR : MPIDI_OFI_caps_list[MPIDI_OFI_SET_NUMBER].cap

        UPDATE_SETTING_BY_SET_NUMBER(enable_hmem, MPIR_CVAR_CH4_OFI_ENABLE_HMEM);
    }

    /* last sanity check */
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE) || (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL || defined(MPIDI_OFI_VNI_USE_DOMAIN))
    MPIR_ERR_CHKANDJUMP(prov_list->domain_attr->threading != FI_THREAD_DOMAIN,
                        mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
#else
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS) {
            MPIR_ERR_CHKANDJUMP(prov_list->domain_attr->threading != FI_THREAD_SAFE,
                                mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
        } else {
            MPIR_ERR_CHKANDJUMP(prov_list->domain_attr->threading != FI_THREAD_COMPLETION,
                                mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
        }
    } else {
        MPIR_ERR_CHKANDJUMP(prov_list->domain_attr->threading != FI_THREAD_DOMAIN,
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
    }
#endif

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

/* return a score of preference. Default is 0.
 * Return < 0 for less favorable providers.
 * Return > 0 for more favorable providers.
 */
static int provider_preference(const char *prov_name)
{
    int n = strlen(prov_name);
    if (n > 8 && strcmp(prov_name + n - 8, ";ofi_rxd") == 0) {
        /* ofi_rxd have more test failures */
        return -2;
    }

    if (strcmp(prov_name, "shm") == 0) {
        /* Obviously shm won't work for internode. User still can force shm by restricting
         * the provider list, e.g. setting FI_PROVIDER=shm */
        return -2;
    }

    if (MPIR_Process.num_nodes == 1 && MPIR_CVAR_SINGLE_HOST_ENABLED &&
        strcmp(prov_name, "cxi") == 0) {
        return -100;
    }

    return 0;
}

static struct fi_info *pick_provider_from_list(struct fi_info *list)
{
    MPIDI_OFI_capabilities_t optimal_settings, minimal_settings;
    MPIDI_OFI_init_settings(&optimal_settings, MPIDI_OFI_SET_NAME_DEFAULT);
    MPIDI_OFI_init_settings(&minimal_settings, MPIDI_OFI_SET_NAME_MINIMAL);

    int best_score = 0;
    struct fi_info *best_prov = NULL;
    for (struct fi_info * prov = list; prov; prov = prov->next) {
        /* Confirm the NIC backed by the provider is actually up. */
        if (!MPIDI_OFI_nic_is_up(prov)) {
            continue;
        }

        int score = MPIDI_OFI_match_provider(prov, &optimal_settings, &minimal_settings);
        int pref_score = provider_preference(prov->fabric_attr->prov_name);
        if (best_score < score + pref_score) {
            best_score = score + pref_score;
            best_prov = prov;
        }
        if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
            printf("provider: %s, score = %d, pref = %d, %s\n", prov->fabric_attr->prov_name,
                   score, pref_score, get_prov_addr(prov));
        }
    }

    return best_prov;
}
