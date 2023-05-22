/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_noinline.h"
#include "mpir_hwtopo.h"
#include "ofi_init.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : CH4_OFI
      description : A category for CH4 OFI netmod variables

cvars:
    - name        : MPIR_CVAR_OFI_SKIP_IPV6
      category    : DEVELOPER
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Skip IPv6 providers.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_DATA
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Enable immediate data fields in OFI to transmit source rank outside of the match bits

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, the OFI addressing information will be stored with an FI_AV_TABLE.
        If false, an FI_AV_MAP will be used.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, use OFI scalable endpoints.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to false (zero), MPICH does not use OFI shared contexts.
        If set to -1, it is determined by the OFI capability sets based on the provider.
        Otherwise, MPICH tries to use OFI shared contexts. If they are unavailable,
        it'll fall back to the mode without shared contexts.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable virtual addressing for OFI memory regions. This variable is only meaningful
        for OFI versions 1.5+. It is equivalent to using FI_MR_BASIC in versions of
        OFI older than 1.5.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_ALLOCATED
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, require all OFI memory regions must be backed by physical memory pages
        at the time the registration call is made. This variable is only meaningful
        for OFI versions 1.5+. It is equivalent to using FI_MR_BASIC in versions of
        OFI older than 1.5.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_REGISTER_NULL
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, memory registration call supports registering with NULL addresses.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable provider supplied key for OFI memory regions. This variable is only
        meaningful for OFI versions 1.5+. It is equivalent to using FI_MR_BASIC in versions of OFI
        older than 1.5.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_TAGGED
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, use tagged message transmission functions in OFI.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_AM
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI active message support.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_RMA
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI RMA support for MPI RMA operations. OFI support for basic RMA is always
        required to implement large messgage transfers in the active message code path.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI Atomics support.

    - name        : MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the maximum number of iovecs that can be used by the OFI provider
        for fetch_atomic operations. The default value is -1, indicating that
        no value is set.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_DATA_AUTO_PROGRESS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable MPI data auto progress.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable MPI control auto progress.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable iovec for pt2pt.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_HMEM
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI HMEM support.

    - name        : MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of bits that will be used for matching the context
        ID. The default value is -1, indicating that no value is set and that
        the default will be defined in the ofi_types.h file.

    - name        : MPIR_CVAR_CH4_OFI_RANK_BITS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of bits that will be used for matching the MPI
        rank. The default value is -1, indicating that no value is set and that
        the default will be defined in the ofi_types.h file.

    - name        : MPIR_CVAR_CH4_OFI_TAG_BITS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of bits that will be used for matching the user
        tag. The default value is -1, indicating that no value is set and that
        the default will be defined in the ofi_types.h file.

    - name        : MPIR_CVAR_CH4_OFI_MAJOR_VERSION
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the major version of the OFI library. The default is the
        major version of the OFI library used with MPICH. If using this CVAR,
        it is recommended that the user also specifies a specific OFI provider.

    - name        : MPIR_CVAR_CH4_OFI_MINOR_VERSION
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the major version of the OFI library. The default is the
        minor version of the OFI library used with MPICH. If using this CVAR,
        it is recommended that the user also specifies a specific OFI provider.

    - name        : MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of transmit
        contexts RMA can utilize in a scalable endpoint.
        This value is effective only when scalable endpoint is available, otherwise
        it will be ignored.

    - name        : MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of retries
        of an ofi operations before returning MPIX_ERR_EAGAIN. This value is
        effective only when the communicator has the MPI_OFI_set_eagain info
        hint set to true.

    - name        : MPIR_CVAR_CH4_OFI_NUM_AM_BUFFERS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of buffers for receiving active messages.

    - name        : MPIR_CVAR_CH4_OFI_NUM_OPTIMIZED_MEMORY_REGIONS
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of optimized memory regions supported by the provider. An optimized
        memory region is used for lower-overhead, unordered RMA operations. It uses a low-overhead
        RX path and additionally, a low-overhead packet format may be used to target an optimized
        memory region.

    - name        : MPIR_CVAR_CH4_OFI_RMA_PROGRESS_INTERVAL
      category    : CH4_OFI
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the interval for manually flushing RMA operations when automatic progress is not
        enabled. It the underlying OFI provider supports auto data progress, this value is ignored.
        If the value is -1, this optimization will be turned off.

    - name        : MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX
      category    : CH4_OFI
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the maximum number of iovecs to allocate for RMA operations
        to/from noncontiguous buffers.

    - name        : MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        This cvar controls the message size at which OFI native path switches from eager to
        rendezvous mode. It does not affect the AM path eager limit. Having this gives a way to
        reliably test native non-path.
        If the number is positive, OFI will init the MPIDI_OFI_global.max_msg_size to the value of
        cvar. If the number is negative, OFI will init the MPIDI_OFI_globa.max_msg_size using
        whatever provider gives (which might be unlimited for socket provider).

    - name        : MPIR_CVAR_CH4_OFI_MAX_NICS
      category    : CH4
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive number, this cvar determines the maximum number of physical nics
        to use (if more than one is available). If the number is -1, underlying netmod or
        shmmod automatically uses an optimal number depending on what is detected on the
        system up to the limit determined by MPIDI_MAX_NICS (in ofi_types.h).

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_STRIPING
      category    : CH4
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, this cvar enables striping of large messages across multiple NICs.

    - name        : MPIR_CVAR_CH4_OFI_MULTI_NIC_STRIPING_THRESHOLD
      category    : CH4
      type        : int
      default     : 1048576
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Striping will happen for message sizes beyond this threshold.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_HASHING
      category    : CH4
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Multi-NIC hashing means to use more than one NIC to send and receive messages above a
        certain size.  If set to positive number, this feature will be turned on. If set to 0, this
        feature will be turned off. If the number is -1, MPICH automatically determines whether to
        use multi-nic hashing depending on what is detected on the system (e.g., number of NICs
        available, number of processes sharing the NICs).

    - name        : MPIR_CVAR_CH4_OFI_MULTIRECV_BUFFER_SIZE
      category    : CH4
      type        : int
      default     : 2097152
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Controls the multirecv am buffer size. It is recommended to match this
        to the hugepage size so that the buffer can be allocated at the page
        boundary.

    - name        : MPIR_CVAR_OFI_USE_MIN_NICS
      category    : DEVELOPER
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true and all nodes do not have the same number of NICs, MPICH will fall back
        to using the fewest number of NICs instead of returning an error.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static int update_global_limits(struct fi_info *prov);
static void dump_global_settings(void);
static void dump_dynamic_settings(void);
static int create_vci_context(int vci, int nic);
static int destroy_vci_context(int vci, int nic);
static int ofi_pvar_init(void);

static int ofi_am_init(void);
static int ofi_am_post_recv(int vci, int nic);

static void *host_alloc(uintptr_t size);
static void host_free(void *ptr);

static int ofi_pvar_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC(MULTINIC,
                                              MPI_UNSIGNED_LONG_LONG,
                                              nic_sent_bytes_count,
                                              MPI_T_VERBOSITY_USER_DETAIL,
                                              MPI_T_BIND_NO_OBJECT,
                                              (MPIR_T_PVAR_FLAG_READONLY |
                                               MPIR_T_PVAR_FLAG_SUM), "CH4",
                                              "number of bytes sent through a particular NIC");

    MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC(MULTINIC,
                                              MPI_UNSIGNED_LONG_LONG,
                                              nic_recvd_bytes_count,
                                              MPI_T_VERBOSITY_USER_DETAIL,
                                              MPI_T_BIND_NO_OBJECT,
                                              (MPIR_T_PVAR_FLAG_READONLY |
                                               MPIR_T_PVAR_FLAG_SUM), "CH4",
                                              "number of bytes received through a particular NIC");

    MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC(MULTINIC,
                                              MPI_UNSIGNED_LONG_LONG,
                                              striped_nic_sent_bytes_count,
                                              MPI_T_VERBOSITY_USER_DETAIL,
                                              MPI_T_BIND_NO_OBJECT,
                                              (MPIR_T_PVAR_FLAG_READONLY |
                                               MPIR_T_PVAR_FLAG_SUM), "CH4",
                                              "number of striped bytes sent through a particular NIC");

    MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC(MULTINIC,
                                              MPI_UNSIGNED_LONG_LONG,
                                              striped_nic_recvd_bytes_count,
                                              MPI_T_VERBOSITY_USER_DETAIL,
                                              MPI_T_BIND_NO_OBJECT,
                                              (MPIR_T_PVAR_FLAG_READONLY |
                                               MPIR_T_PVAR_FLAG_SUM), "CH4",
                                              "number of striped bytes received through a particular NIC");

    MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC(MULTINIC,
                                              MPI_UNSIGNED_LONG_LONG,
                                              rma_pref_phy_nic_put_bytes_count,
                                              MPI_T_VERBOSITY_USER_DETAIL,
                                              MPI_T_BIND_NO_OBJECT,
                                              (MPIR_T_PVAR_FLAG_READONLY |
                                               MPIR_T_PVAR_FLAG_SUM), "CH4",
                                              "number of bytes sent through preferred physical NIC using RMA");

    MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC(MULTINIC,
                                              MPI_UNSIGNED_LONG_LONG,
                                              rma_pref_phy_nic_get_bytes_count,
                                              MPI_T_VERBOSITY_USER_DETAIL,
                                              MPI_T_BIND_NO_OBJECT,
                                              (MPIR_T_PVAR_FLAG_READONLY |
                                               MPIR_T_PVAR_FLAG_SUM), "CH4",
                                              "number of bytes received through preferred physical NIC using RMA");
    return mpi_errno;
}

static void *host_alloc(uintptr_t size)
{
    return MPL_malloc(size, MPL_MEM_BUFFER);
}

static void host_free(void *ptr)
{
    MPL_free(ptr);
}

static void set_sep_counters(int nic)
{
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
        /* Note: currently we request a single tx and rx ctx under MPIDI_OFI_VNI_USE_DOMAIN */
        int num_ctx_per_nic = 1;
#else
        int num_ctx_per_nic = MPIDI_OFI_global.num_vcis;
#endif
        int max_by_prov = MPL_MIN(MPIDI_OFI_global.prov_use[nic]->domain_attr->tx_ctx_cnt,
                                  MPIDI_OFI_global.prov_use[nic]->domain_attr->rx_ctx_cnt);
        num_ctx_per_nic = MPL_MIN(num_ctx_per_nic, max_by_prov);
        MPIDI_OFI_global.prov_use[nic]->ep_attr->tx_ctx_cnt = num_ctx_per_nic;
        MPIDI_OFI_global.prov_use[nic]->ep_attr->rx_ctx_cnt = num_ctx_per_nic;
    }
}

int MPIDI_OFI_init_local(int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_chunk_request, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_read_chunk_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_am_repost_request_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_ssendack_request_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_dynamic_process_request_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.am.netmod_am.ofi.context) ==
                            offsetof(struct MPIR_Request, dev.ch4.netmod.ofi.context));
    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devreq_t) >= sizeof(MPIDI_OFI_request_t));

    int err;
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_UTIL_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_FI_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &err);
    MPIR_Assert(err == 0);

    /* -------------------------------- */
    /* Create the id to object maps     */
    /* -------------------------------- */
    MPIDIU_map_create(&MPIDI_OFI_global.win_map, MPL_MEM_RMA);

    /* Initialize RMA keys allocator */
    MPIDI_OFI_mr_key_allocator_init();

    MPIDI_OFI_global.num_comms_enabled_striping = 0;
    MPIDI_OFI_global.num_comms_enabled_hashing = 0;

    mpi_errno = ofi_pvar_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* -------------------------------- */
    /* Set up the libfabric provider(s) */
    /* -------------------------------- */

    /* WB TODO - I assume that after this function is done, there will be an array of providers in
     * MPIDI_OFI_global.prov_use that will map to the VNI contexts below. We can also use it to
     * generate the addresses in the business card exchange. */
    MPIDI_OFI_global.num_nics = 1;

    struct fi_info *prov = NULL;
    mpi_errno = MPIDI_OFI_find_provider(&prov);
    MPIR_ERR_CHECK(mpi_errno);

    /* init multi-nic and populates MPIDI_OFI_global.prov_use[] */
    mpi_errno = MPIDI_OFI_init_multi_nic(prov);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = update_global_limits(MPIDI_OFI_global.prov_use[0]);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
        dump_global_settings();
    }

    /* Finally open the fabric */
    MPIDI_OFI_CALL(fi_fabric(MPIDI_OFI_global.prov_use[0]->fabric_attr,
                             &MPIDI_OFI_global.fabric, NULL), fabric);

    /* ------------------------------------------------------------------------ */
    /* Create transport level communication contexts.                           */
    /* ------------------------------------------------------------------------ */

    /* TODO: check provider capabilities, such as prov_use->domain_attr->{tx,rx}_ctx_cnt,
     *       abort if we can't support the requested number of vcis.
     */
    int num_vcis = MPIDI_global.n_total_vcis;

    /* Multiple vci without using domain require MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS */
#ifndef MPIDI_OFI_VNI_USE_DOMAIN
    MPIR_Assert(num_vcis == 1 || MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
#endif

    MPIDI_OFI_global.num_vcis = num_vcis;

    /* set rx_ctx_cnt and tx_ctx_cnt for nic 0 */
    set_sep_counters(0);

    /* Creating the context for vci 0 and nic 0.
     * This code maybe moved to a later stage */
    mpi_errno = create_vci_context(0, 0);
    MPIR_ERR_CHECK(mpi_errno);

    /* index datatypes for RMA atomics. */
    MPIDI_OFI_index_datatypes(MPIDI_OFI_global.ctx[0].tx);

    /* make sure ch4 pack buffer pool has sufficient cell size */
    MPIR_Assert(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE <= MPIR_CVAR_CH4_PACK_BUFFER_SIZE);

    ofi_am_init();
    ofi_am_post_recv(0, 0);

  fn_exit:
    *tag_bits = MPIDI_OFI_TAG_BITS;
    MPIDI_OFI_find_provider_cleanup();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_init_world(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_OFI_global.got_named_av) {
        mpi_errno = MPIDI_OFI_addr_exchange_root_ctx();
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
        dump_dynamic_settings();
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    int num_nics = MPIDI_OFI_global.num_nics;
    int tmp_num_vcis = MPIDI_OFI_global.num_vcis;
    int tmp_num_nics = MPIDI_OFI_global.num_nics;

    /* Set the number of NICs and VNIs to 1 temporarily to avoid problems during the collective */
    MPIDI_OFI_global.num_vcis = MPIDI_OFI_global.num_nics = 1;

    /* Confirm that all processes have the same number of NICs */
    mpi_errno = MPIR_Allreduce_allcomm_auto(&tmp_num_nics, &num_nics, 1, MPI_INT,
                                            MPI_MIN, MPIR_Process.comm_world, MPIR_ERR_NONE);
    MPIDI_OFI_global.num_vcis = tmp_num_vcis;
    MPIDI_OFI_global.num_nics = tmp_num_nics;
    MPIR_ERR_CHECK(mpi_errno);

    /* If the user did not ask to fallback to fewer NICs, throw an error if someone is missing a
     * NIC. */
    if (tmp_num_nics != num_nics) {
        if (MPIR_CVAR_OFI_USE_MIN_NICS) {
            MPIDI_OFI_global.num_nics = num_nics;

            /* If we fall down to 1 nic, turn off multi-nic optimizations. */
            if (num_nics == 1) {
                MPIDI_OFI_COMM(MPIR_Process.comm_world).enable_striping = 0;
            }
        } else {
            MPIR_ERR_CHKANDJUMP(num_nics != MPIDI_OFI_global.num_nics, mpi_errno, MPI_ERR_OTHER,
                                "**ofi_num_nics");
        }
    }

    /* FIXME: It would also be helpful to check that all of the NICs can communicate so we can fall
     * back to other options if that is not the case (e.g., verbs are often configured with a
     * different subnet for each "set" of nics). It's unknown how to write a good check for that. */

    /* set rx_ctx_cnt and tx_ctx_cnt for the remaining (non-0) nics */
    for (int nic = 1; nic < MPIDI_OFI_global.num_nics; nic++) {
        set_sep_counters(nic);
    }

    for (int vci = 0; vci < MPIDI_OFI_global.num_vcis; vci++) {
        for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
            /* vci 0 nic 0 already created */
            if (vci > 0 || nic > 0) {
                mpi_errno = create_vci_context(vci, nic);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

    if (MPIDI_OFI_global.num_vcis > 1 || MPIDI_OFI_global.num_nics > 1) {
        mpi_errno = MPIDI_OFI_addr_exchange_all_ctx();
    }

    for (int vci = 1; vci < MPIDI_OFI_global.num_vcis; vci++) {
        ofi_am_post_recv(vci, 0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* static functions needed by finalize */

#define MPIDI_OFI_FLUSH_CONTEXT_ID 0xF000
#define MPIDI_OFI_FLUSH_TAG        1

/* send a dummy message to flush the send queue */
static int flush_send(int dst, int nic, int vci, MPIDI_OFI_dynamic_process_request_t * req)
{
    int mpi_errno = MPI_SUCCESS;

    fi_addr_t addr = MPIDI_OFI_av_to_phys(&MPIDIU_get_av(0, dst), nic, vci);
    static int data = 0;
    uint64_t match_bits = MPIDI_OFI_init_sendtag(MPIDI_OFI_FLUSH_CONTEXT_ID, 0,
                                                 MPIDI_OFI_FLUSH_TAG, MPIDI_OFI_DYNPROC_SEND);

    /* Use the same direct send method as used in establishing dynamic processes */
    req->done = 0;
    req->event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
    if (MPIDI_OFI_ENABLE_DATA) {
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          &data, 4, NULL, 0, addr, match_bits, &req->context),
                             vci, tsenddata);
    } else {
        MPIDI_OFI_CALL_RETRY(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                      &data, 4, NULL, addr, match_bits, &req->context), vci, tsend);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* recv the dummy message the other process sent for the purpose flushing send queue */
static int flush_recv(int src, int nic, int vci, MPIDI_OFI_dynamic_process_request_t * req)
{
    int mpi_errno = MPI_SUCCESS;

    fi_addr_t addr = MPIDI_OFI_av_to_phys(&MPIDIU_get_av(0, src), nic, vci);
    uint64_t mask_bits = 0;
    uint64_t match_bits = MPIDI_OFI_init_sendtag(MPIDI_OFI_FLUSH_CONTEXT_ID, 0,
                                                 MPIDI_OFI_FLUSH_TAG, MPIDI_OFI_DYNPROC_SEND);

    /* Use the same direct recv method as used in establishing dynamic processes */
    req->done = 0;
    req->event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    /* we don't care the data and the tag field is not used */
    void *recvbuf = &(req->tag);
    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(vci, nic)].rx,
                                  recvbuf, 4, NULL, addr, match_bits, mask_bits, &req->context),
                         vci, trecv);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int flush_send_queue(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_dynamic_process_request_t *reqs;
    /* TODO - Iterate over each NIC in addition to each VNI when multi-NIC within the same
     * process is implemented. */
    int num_vcis = (MPIDI_global.is_initialized ? MPIDI_OFI_global.num_vcis : 1);
    int num_reqs = num_vcis * 2;
    reqs = MPL_malloc(sizeof(MPIDI_OFI_dynamic_process_request_t) * num_reqs, MPL_MEM_OTHER);

    /* Apparently by sending self messages can flush the send queue */
    int rank = MPIR_Process.rank;
    for (int vci = 0; vci < num_vcis; vci++) {
        mpi_errno = flush_send(rank, 0, vci, &reqs[vci * 2]);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = flush_recv(rank, 0, vci, &reqs[vci * 2 + 1]);
        MPIR_ERR_CHECK(mpi_errno);
    }

    bool all_done = false;
    while (!all_done) {
        int made_progress;
        for (int vci = 0; vci < num_vcis; vci++) {
            mpi_errno = MPIDI_NM_progress(vci, &made_progress);
            MPIR_ERR_CHECK(mpi_errno);
        }
        all_done = true;
        for (int i = 0; i < num_reqs; i++) {
            if (!reqs[i].done) {
                all_done = false;
                break;
            }
        }
    }
    MPL_free(reqs);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i = 0;

    MPIR_FUNC_ENTER;

    /* Progress until we drain all inflight RMA send long buffers */
    /* NOTE: am currently only use vci 0. Need update once that changes */
    for (int vci = 0; vci < MPIDI_OFI_global.num_vcis; vci++) {
        while (MPIDI_OFI_global.per_vci[vci].am_inflight_rma_send_mrs > 0) {
            MPIDI_OFI_PROGRESS(vci);
        }
    }

    /* Destroy RMA key allocator */
    MPIDI_OFI_mr_key_allocator_destroy();

    if (strcmp("sockets", MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name) == 0) {
        /* sockets provider need flush any last lightweight send. Only do it if we initialized
         * world. Sockets provider can't even send self messages otherwise. */
        if (MPIDI_global.is_initialized) {
            mpi_errno = flush_send_queue();
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else if (MPIR_CVAR_NO_COLLECTIVE_FINALIZE) {
        /* skip collective work arounds */
    } else if (strcmp("verbs;ofi_rxm", MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name) == 0
               || strcmp("psm2", MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name) == 0
               || strcmp("psm3", MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name) == 0) {
        /* verbs;ofi_rxm provider need barrier to prevent message loss */
        mpi_errno = MPIR_pmi_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Progress until we drain all inflight injection emulation requests */
    /* NOTE: am currently only use vci 0. Need update once that changes */
    for (int vci = 0; vci < MPIDI_OFI_global.num_vcis; vci++) {
        while (MPIDI_OFI_global.per_vci[vci].am_inflight_inject_emus > 0) {
            MPIDI_OFI_PROGRESS(vci);
        }
        MPIR_Assert(MPIDI_OFI_global.per_vci[vci].am_inflight_inject_emus == 0);
    }

    /* Tearing down endpoints in reverse order they were created */
    for (int nic = MPIDI_OFI_global.num_nics - 1; nic >= 0; nic--) {
        for (int vci = MPIDI_OFI_global.num_vcis - 1; vci >= 0; vci--) {
            if (MPIDI_global.is_initialized || (vci == 0 && nic == 0)) {
                mpi_errno = destroy_vci_context(vci, nic);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.fabric->fid), fabricclose);

    for (i = 0; i < MPIDI_OFI_global.num_nics; i++) {
        fi_freeinfo(MPIDI_OFI_global.prov_use[i]);
    }

    MPIDIU_map_destroy(MPIDI_OFI_global.win_map);

    if (MPIDI_OFI_ENABLE_AM) {
        for (int vci = 0; vci < MPIDI_OFI_global.num_vcis; vci++) {
            while (MPIDI_OFI_global.per_vci[vci].am_unordered_msgs) {
                MPIDI_OFI_am_unordered_msg_t *uo_msg =
                    MPIDI_OFI_global.per_vci[vci].am_unordered_msgs;
                DL_DELETE(MPIDI_OFI_global.per_vci[vci].am_unordered_msgs, uo_msg);
            }
            MPIDIU_map_destroy(MPIDI_OFI_global.per_vci[vci].am_send_seq_tracker);
            MPIDIU_map_destroy(MPIDI_OFI_global.per_vci[vci].am_recv_seq_tracker);

            MPIDIU_map_destroy(MPIDI_OFI_global.per_vci[vci].req_map);

            MPIDI_OFI_unregister_am_bufs();
            MPL_free(MPIDI_OFI_global.per_vci[vci].am_bufs);

            MPIDU_genq_private_pool_destroy(MPIDI_OFI_global.per_vci[vci].am_hdr_buf_pool);

            MPIR_Assert(MPIDI_OFI_global.per_vci[vci].cq_buffered_static_head ==
                        MPIDI_OFI_global.per_vci[vci].cq_buffered_static_tail);
            MPIR_Assert(NULL == MPIDI_OFI_global.per_vci[vci].cq_buffered_dynamic_head);
        }
    }

    int err;
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_UTIL_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_FI_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &err);
    MPIR_Assert(err == 0);

    memset(&MPIDI_OFI_global, 0, sizeof(MPIDI_OFI_global));

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void *MPIDI_OFI_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}

int MPIDI_OFI_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}

/* ---- static functions for vci contexts ---- */
static int create_vci_domain(struct fid_domain **p_domain, struct fid_av **p_av,
                             struct fid_cntr **p_cntr, int nic);
static int create_cq(struct fid_domain *domain, struct fid_cq **p_cq);
static int create_sep_tx(struct fid_ep *ep, int idx, struct fid_ep **p_tx,
                         struct fid_cq *cq, struct fid_cntr *cntr, int nic);
static int create_sep_rx(struct fid_ep *ep, int idx, struct fid_ep **p_rx, struct fid_cq *cq,
                         int nic);
static int try_open_shared_av(struct fid_domain *domain, struct fid_av **p_av, int nic);
static int open_local_av(struct fid_domain *p_domain, struct fid_av **p_av);

/* This function creates a vci context which includes all of the OFI-level objects needed to
 * initialize OFI (e.g. domain, address vector, endpoint, etc.). This function takes two arguments:
 *
 * vci - The VNI index within a nic to use when assigning the OFI information being created.
 * nic - The NIC that should be used when setting up the OFI interfaces.
 *
 * Each nic will restart its vci indexing. This allows each VNI to use any nic if desired.
 */
static int create_vci_context(int vci, int nic)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    struct fi_info *prov_use = MPIDI_OFI_global.prov_use[nic];
    int ctx_idx;

    /* Each VNI context consists of domain, av, cq, cntr, etc.
     *
     * If MPIDI_OFI_VNI_USE_DOMAIN is true, each context is a separate domain,
     * within which are each separate av, cq, cntr, ..., everything. Within the
     * VNI context, it still can use either simple endpoint or scalable endpoint.
     *
     * If MPIDI_OFI_VNI_USE_DOMAIN is false, then all the VNI contexts will share
     * the same domain and av, and use a single scalable endpoint. Separate VNI
     * context will have its separate cq and separate tx and rx with the SEP. VNIs
     * which are attached to different NICs will have separate scalable endpoints as
     * they require different fid_domains.
     *
     * To accommodate both configurations, each context structure will have all fields
     * including domain, av, cq, ... For "VNI_USE_DOMAIN", they are not shared.
     * When not "VNI_USE_DOMAIN" or "VNI_USE_SEPCTX", domain, av, and ep are shared
     * with the root (or 0th) VNI context.
     */
    struct fid_domain *domain;
    struct fid_av *av;
    struct fid_cntr *rma_cmpl_cntr;
    struct fid_cq *cq;

    struct fid_ep *ep;
    struct fid_ep *tx;
    struct fid_ep *rx;

#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    mpi_errno = create_vci_domain(&domain, &av, &rma_cmpl_cntr, nic);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = create_cq(domain, &cq);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_CALL(fi_scalable_ep(domain, prov_use, &ep, NULL), ep);
        MPIDI_OFI_CALL(fi_scalable_ep_bind(ep, &av->fid, 0), bind);
        MPIDI_OFI_CALL(fi_enable(ep), ep_enable);

        mpi_errno = create_sep_tx(ep, 0, &tx, cq, rma_cmpl_cntr, nic);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = create_sep_rx(ep, 0, &rx, cq, nic);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIDI_OFI_CALL(fi_endpoint(domain, prov_use, &ep, NULL), ep);
        MPIDI_OFI_CALL(fi_ep_bind(ep, &av->fid, 0), bind);
        MPIDI_OFI_CALL(fi_ep_bind(ep, &cq->fid, FI_SEND | FI_RECV | FI_SELECTIVE_COMPLETION), bind);
        MPIDI_OFI_CALL(fi_ep_bind(ep, &rma_cmpl_cntr->fid, FI_READ | FI_WRITE), bind);
        MPIDI_OFI_CALL(fi_enable(ep), ep_enable);
        tx = ep;
        rx = ep;
    }
    ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
    MPIDI_OFI_global.ctx[ctx_idx].domain = domain;
    MPIDI_OFI_global.ctx[ctx_idx].av = av;
    MPIDI_OFI_global.ctx[ctx_idx].rma_cmpl_cntr = rma_cmpl_cntr;
    MPIDI_OFI_cntr_set(ctx_idx, 0);
    MPIDI_OFI_global.ctx[ctx_idx].ep = ep;
    MPIDI_OFI_global.ctx[ctx_idx].cq = cq;
    MPIDI_OFI_global.ctx[ctx_idx].tx = tx;
    MPIDI_OFI_global.ctx[ctx_idx].rx = rx;

#else /* MPIDI_OFI_VNI_USE_SEPCTX */
    /* Endpoints are used to bundle together all VNIs. In addition, we have to duplicate these
     * endpoints such that each NIC gets its own endpoint. So now we have a endpoint per nic and a
     * transmit/receive context per vci. */
    if (vci == 0) {
        mpi_errno = create_vci_domain(&domain, &av, &rma_cmpl_cntr, nic);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);
        domain = MPIDI_OFI_global.ctx[ctx_idx].domain;
        av = MPIDI_OFI_global.ctx[ctx_idx].av;
        rma_cmpl_cntr = MPIDI_OFI_global.ctx[ctx_idx].rma_cmpl_cntr;
    }
    mpi_errno = create_cq(domain, &cq);
    MPIR_ERR_CHECK(mpi_errno);

    /* If this is the first vci in the bundle, we need to create the endpoint. Otherwise, just copy
     * it from the first vci. */
    if (vci == 0) {
        if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
            MPIDI_OFI_CALL(fi_scalable_ep(domain, prov_use, &ep, NULL), ep);
            MPIDI_OFI_CALL(fi_scalable_ep_bind(ep, &av->fid, 0), bind);
            MPIDI_OFI_CALL(fi_enable(ep), ep_enable);
        } else {
            MPIDI_OFI_CALL(fi_endpoint(domain, prov_use, &ep, NULL), ep);
            MPIDI_OFI_CALL(fi_ep_bind(ep, &av->fid, 0), bind);
            MPIDI_OFI_CALL(fi_ep_bind(ep, &cq->fid, FI_SEND | FI_RECV | FI_SELECTIVE_COMPLETION),
                           bind);
            MPIDI_OFI_CALL(fi_ep_bind(ep, &rma_cmpl_cntr->fid, FI_READ | FI_WRITE), bind);
            MPIDI_OFI_CALL(fi_enable(ep), ep_enable);
        }
    } else {
        ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);
        ep = MPIDI_OFI_global.ctx[ctx_idx].ep;
    }

    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        mpi_errno = create_sep_tx(ep, vci, &tx, cq, rma_cmpl_cntr, nic);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = create_sep_rx(ep, vci, &rx, cq, nic);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        tx = ep;
        rx = ep;
    }

    if (vci == 0) {
        ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
        MPIDI_OFI_global.ctx[ctx_idx].domain = domain;
        MPIDI_OFI_global.ctx[ctx_idx].av = av;
        MPIDI_OFI_global.ctx[ctx_idx].rma_cmpl_cntr = rma_cmpl_cntr;
        MPIDI_OFI_global.ctx[ctx_idx].ep = ep;
    } else {
        /* non-zero vci share most fields with vci 0, copy them
         * so we don't have to switch during runtime */
        MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(vci, nic)] =
            MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(0, nic)];
    }
    ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
    MPIDI_OFI_global.ctx[ctx_idx].cq = cq;
    MPIDI_OFI_global.ctx[ctx_idx].tx = tx;
    MPIDI_OFI_global.ctx[ctx_idx].rx = rx;
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int destroy_vci_context(int vci, int nic)
{
    int mpi_errno = MPI_SUCCESS;
    int ctx_num = MPIDI_OFI_get_ctx_index(vci, nic);

#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].tx->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rx->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].cq->fid), cqclose);

        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].ep->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].av->fid), avclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rma_cmpl_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].domain->fid), domainclose);
    } else {    /* normal endpoint */
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].ep->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].cq->fid), cqclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].av->fid), avclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rma_cmpl_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].domain->fid), domainclose);
    }

#else /* MPIDI_OFI_VNI_USE_SEPCTX */
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].tx->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rx->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].cq->fid), cqclose);
        if (vci == 0) {
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].ep->fid), epclose);
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].av->fid), avclose);
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rma_cmpl_cntr->fid), cntrclose);
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].domain->fid), domainclose);
        }
    } else {    /* normal endpoint */
        MPIR_Assert(vci == 0);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].ep->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].cq->fid), cqclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].av->fid), avclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rma_cmpl_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].domain->fid), domainclose);
    }
#endif
    /* Close RMA scalable EP. */
    if (MPIDI_OFI_global.ctx[ctx_num].rma_sep) {
        /* All transmit contexts on RMA must be closed. */
        MPIR_Assert(utarray_len(MPIDI_OFI_global.ctx[ctx_num].rma_sep_idx_array) ==
                    MPIDI_OFI_global.max_rma_sep_tx_cnt);
        utarray_free(MPIDI_OFI_global.ctx[ctx_num].rma_sep_idx_array);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rma_sep->fid), epclose);
    }

    /* Close RMA shared context */
    if (MPIDI_OFI_global.ctx[ctx_num].rma_stx_ctx != NULL) {
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[ctx_num].rma_stx_ctx->fid), stx_ctx_close);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_vci_domain(struct fid_domain **p_domain, struct fid_av **p_av,
                             struct fid_cntr **p_cntr, int nic)
{
    int mpi_errno = MPI_SUCCESS;

    /* ---- domain ---- */
    struct fid_domain *domain;
    MPIDI_OFI_CALL(fi_domain(MPIDI_OFI_global.fabric, MPIDI_OFI_global.prov_use[nic], &domain,
                             NULL), opendomain);
    *p_domain = domain;

    /* ---- av ---- */
    /* ----
     * Attempt to open a shared address vector read-only.
     * The open will fail if the address vector does not exist.
     * Otherwise, set MPIDI_OFI_global.got_named_av and
     * copy the map_addr.
     */
    if (try_open_shared_av(domain, p_av, nic)) {
        MPIDI_OFI_global.got_named_av = 1;
    } else {
        mpi_errno = open_local_av(domain, p_av);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* ---- other shareable objects ---- */
    struct fi_cntr_attr cntr_attr;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    cntr_attr.events = FI_CNTR_EVENTS_COMP;
    if (MPIDI_OFI_COUNTER_WAIT_OBJECTS) {
        cntr_attr.wait_obj = FI_WAIT_UNSPEC;
    } else {
        cntr_attr.wait_obj = FI_WAIT_NONE;
    }
    MPIDI_OFI_CALL(fi_cntr_open(domain, &cntr_attr, p_cntr, NULL), openct);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_cq(struct fid_domain *domain, struct fid_cq **p_cq)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_attr cq_attr;
    memset(&cq_attr, 0, sizeof(cq_attr));
    cq_attr.format = FI_CQ_FORMAT_TAGGED;
    MPIDI_OFI_CALL(fi_cq_open(domain, &cq_attr, p_cq, NULL), opencq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_sep_tx(struct fid_ep *ep, int idx, struct fid_ep **p_tx,
                         struct fid_cq *cq, struct fid_cntr *cntr, int nic)
{
    int mpi_errno = MPI_SUCCESS;

    struct fi_tx_attr tx_attr;
    tx_attr = *(MPIDI_OFI_global.prov_use[nic]->tx_attr);
    tx_attr.op_flags = FI_COMPLETION;
    if (MPIDI_OFI_ENABLE_RMA || MPIDI_OFI_ENABLE_ATOMICS)
        tx_attr.op_flags |= FI_DELIVERY_COMPLETE;
    tx_attr.caps = 0;

    if (MPIDI_OFI_ENABLE_TAGGED)
        tx_attr.caps = FI_TAGGED;

    /* RMA */
    if (MPIDI_OFI_ENABLE_RMA)
        tx_attr.caps |= FI_RMA;
    if (MPIDI_OFI_ENABLE_ATOMICS)
        tx_attr.caps |= FI_ATOMICS;
    /* MSG */
    tx_attr.caps |= FI_MSG;
    tx_attr.caps |= FI_NAMED_RX_CTX;    /* Required for scalable endpoints indexing */

    MPIDI_OFI_CALL(fi_tx_context(ep, idx, &tx_attr, p_tx, NULL), ep);
    MPIDI_OFI_CALL(fi_ep_bind(*p_tx, &cq->fid, FI_SEND | FI_SELECTIVE_COMPLETION), bind);
    MPIDI_OFI_CALL(fi_ep_bind(*p_tx, &cntr->fid, FI_WRITE | FI_READ), bind);
    MPIDI_OFI_CALL(fi_enable(*p_tx), ep_enable);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_sep_rx(struct fid_ep *ep, int idx, struct fid_ep **p_rx, struct fid_cq *cq,
                         int nic)
{
    int mpi_errno = MPI_SUCCESS;

    struct fi_rx_attr rx_attr;
    rx_attr = *(MPIDI_OFI_global.prov_use[nic]->rx_attr);
    rx_attr.caps = 0;

    if (MPIDI_OFI_ENABLE_TAGGED) {
        rx_attr.caps |= FI_TAGGED;
        rx_attr.caps |= FI_DIRECTED_RECV;
    }

    if (MPIDI_OFI_ENABLE_RMA)
        rx_attr.caps |= FI_RMA | FI_REMOTE_READ | FI_REMOTE_WRITE;
    if (MPIDI_OFI_ENABLE_ATOMICS)
        rx_attr.caps |= FI_ATOMICS;
    rx_attr.caps |= FI_MSG;
    rx_attr.caps |= FI_MULTI_RECV;
    rx_attr.caps |= FI_NAMED_RX_CTX;    /* Required for scalable endpoints indexing */

    MPIDI_OFI_CALL(fi_rx_context(ep, idx, &rx_attr, p_rx, NULL), ep);
    MPIDI_OFI_CALL(fi_ep_bind(*p_rx, &cq->fid, FI_RECV), bind);
    MPIDI_OFI_CALL(fi_enable(*p_rx), ep_enable);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int try_open_shared_av(struct fid_domain *domain, struct fid_av **p_av, int nic)
{
    int ret = 0;

    /* It's not possible to use shared address vectors with more than one domain in a single
     * process. If we're trying to do that (for example if we are using MPIDI_OFI_VNI_USE_DOMAIN or
     * we have multiple VNIs because of multi-nic), attempt to open up the shared AV in one VNI and
     * then copy the results to the others later. */
    struct fi_av_attr av_attr;
    memset(&av_attr, 0, sizeof(av_attr));
    if (MPIDI_OFI_ENABLE_AV_TABLE) {
        av_attr.type = FI_AV_TABLE;
    } else {
        av_attr.type = FI_AV_MAP;
    }
    av_attr.rx_ctx_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS;
    av_attr.count = MPIR_Process.size;

    char av_name[128];
    snprintf(av_name, sizeof(av_name), "FI_NAMED_AV_%d\n", MPIR_Process.appnum);
    av_attr.name = av_name;
    av_attr.flags = FI_READ;
    av_attr.map_addr = 0;

    if (0 == fi_av_open(domain, &av_attr, p_av, NULL)) {
        /* TODO - the copy from the pre-existing av map into the 'MPIDI_OFI_AV' */
        /* is wasteful and should be changed so that the 'MPIDI_OFI_AV' object  */
        /* directly references the mapped fi_addr_t array instead               */
        fi_addr_t *mapped_table = (fi_addr_t *) av_attr.map_addr;
        for (int i = 0; i < MPIR_Process.size; i++) {
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest[nic][0] = mapped_table[i];
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " grank mapped to: rank=%d, av=%p, dest=%" PRIu64,
                             i, (void *) &MPIDIU_get_av(0, i), mapped_table[i]));
        }
        ret = 1;
    }

    return ret;
}

static int open_local_av(struct fid_domain *p_domain, struct fid_av **p_av)
{
    struct fi_av_attr av_attr;
    int mpi_errno = MPI_SUCCESS;

    memset(&av_attr, 0, sizeof(av_attr));
    if (MPIDI_OFI_ENABLE_AV_TABLE) {
        av_attr.type = FI_AV_TABLE;
    } else {
        av_attr.type = FI_AV_MAP;
    }
    av_attr.rx_ctx_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS;
    av_attr.count = MPIR_Process.size;

    av_attr.name = NULL;
    av_attr.flags = 0;
    MPIDI_OFI_CALL(fi_av_open(p_domain, &av_attr, p_av, NULL), avopen);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int update_global_limits(struct fi_info *prov)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_global.max_buffered_send = prov->tx_attr->inject_size;
    MPIDI_OFI_global.max_buffered_write = prov->tx_attr->inject_size;
    if (MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE > 0 &&
        MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE <= prov->ep_attr->max_msg_size) {
        /* Truncate max_msg_size to a user-selected value */
        MPIDI_OFI_global.max_msg_size = MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE;
    } else {
        MPIDI_OFI_global.max_msg_size = MPL_MIN(prov->ep_attr->max_msg_size, MPIR_AINT_MAX);
    }
    MPIDI_OFI_global.stripe_threshold = MPIR_CVAR_CH4_OFI_MULTI_NIC_STRIPING_THRESHOLD;
    if (prov->ep_attr->max_order_raw_size > MPIR_AINT_MAX) {
        MPIDI_OFI_global.max_order_raw = -1;
    } else {
        MPIDI_OFI_global.max_order_raw = prov->ep_attr->max_order_raw_size;
    }
    if (prov->ep_attr->max_order_war_size > MPIR_AINT_MAX) {
        MPIDI_OFI_global.max_order_war = -1;
    } else {
        MPIDI_OFI_global.max_order_war = prov->ep_attr->max_order_war_size;
    }
    if (prov->ep_attr->max_order_waw_size > MPIR_AINT_MAX) {
        MPIDI_OFI_global.max_order_waw = -1;
    } else {
        MPIDI_OFI_global.max_order_waw = prov->ep_attr->max_order_waw_size;
    }
    MPIDI_OFI_global.tx_iov_limit = MPL_MIN(prov->tx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.rx_iov_limit = MPL_MIN(prov->rx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.rma_iov_limit = MPL_MIN(prov->tx_attr->rma_iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.max_mr_key_size = prov->domain_attr->mr_key_size;

    /* Ensure that we aren't trying to shove too many bits into the match_bits.
     * Currently, this needs to fit into a uint64_t and we take 4 bits for protocol. */
    MPIR_Assert(MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS <= 60);

    /* if using extended context id, check that selected provider can support it */
    MPIR_Assert(MPIR_CONTEXT_ID_BITS <= MPIDI_OFI_CONTEXT_BITS);
    /* Check that the desired number of ranks is possible and abort if not */
    if (MPIDI_OFI_MAX_RANK_BITS < 32 && MPIR_Process.size > (1 << MPIDI_OFI_MAX_RANK_BITS)) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch4|too_many_ranks");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void dump_global_settings(void)
{
    fprintf(stdout, "==== Capability set configuration ====\n");
    fprintf(stdout, "libfabric provider: %s - %s\n",
            MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name,
            MPIDI_OFI_global.prov_use[0]->fabric_attr->name);
    fprintf(stdout, "MPIDI_OFI_ENABLE_DATA: %d\n", MPIDI_OFI_ENABLE_DATA);
    fprintf(stdout, "MPIDI_OFI_ENABLE_AV_TABLE: %d\n", MPIDI_OFI_ENABLE_AV_TABLE);
    fprintf(stdout, "MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS: %d\n",
            MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_SHARED_CONTEXTS: %d\n", MPIDI_OFI_ENABLE_SHARED_CONTEXTS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS: %d\n", MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_ALLOCATED: %d\n", MPIDI_OFI_ENABLE_MR_ALLOCATED);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_REGISTER_NULL: %d\n", MPIDI_OFI_ENABLE_MR_REGISTER_NULL);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_PROV_KEY: %d\n", MPIDI_OFI_ENABLE_MR_PROV_KEY);
    fprintf(stdout, "MPIDI_OFI_ENABLE_TAGGED: %d\n", MPIDI_OFI_ENABLE_TAGGED);
    fprintf(stdout, "MPIDI_OFI_ENABLE_AM: %d\n", MPIDI_OFI_ENABLE_AM);
    fprintf(stdout, "MPIDI_OFI_ENABLE_RMA: %d\n", MPIDI_OFI_ENABLE_RMA);
    fprintf(stdout, "MPIDI_OFI_ENABLE_ATOMICS: %d\n", MPIDI_OFI_ENABLE_ATOMICS);
    fprintf(stdout, "MPIDI_OFI_FETCH_ATOMIC_IOVECS: %d\n", MPIDI_OFI_FETCH_ATOMIC_IOVECS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS: %d\n",
            MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS: %d\n",
            MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_PT2PT_NOPACK: %d\n", MPIDI_OFI_ENABLE_PT2PT_NOPACK);
    fprintf(stdout, "MPIDI_OFI_ENABLE_HMEM: %d\n", MPIDI_OFI_ENABLE_HMEM);
    fprintf(stdout, "MPIDI_OFI_NUM_AM_BUFFERS: %d\n", MPIDI_OFI_NUM_AM_BUFFERS);
    fprintf(stdout, "MPIDI_OFI_NUM_OPTIMIZED_MEMORY_REGIONS: %d\n",
            MPIDI_OFI_NUM_OPTIMIZED_MEMORY_REGIONS);
    fprintf(stdout, "MPIDI_OFI_CONTEXT_BITS: %d\n", MPIDI_OFI_CONTEXT_BITS);
    fprintf(stdout, "MPIDI_OFI_SOURCE_BITS: %d\n", MPIDI_OFI_SOURCE_BITS);
    fprintf(stdout, "MPIDI_OFI_TAG_BITS: %d\n", MPIDI_OFI_TAG_BITS);
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    fprintf(stdout, "MPIDI_OFI_VNI_USE_DOMAIN: %d\n", 1);
#endif
#ifdef MPIDI_OFI_VNI_USE_SEPCTX
    fprintf(stdout, "MPIDI_OFI_VNI_USE_SEPCTX: %d\n", 1);
#endif
    /* Discover the maximum number of ranks. If the source shift is not
     * defined, there are 32 bits in use due to the uint32_t used in
     * ofi_send.h */
    fprintf(stdout, "MAXIMUM SUPPORTED RANKS: %ld\n", (long int) 1 << MPIDI_OFI_MAX_RANK_BITS);

    /* Discover the tag_ub */
    fprintf(stdout, "MAXIMUM TAG: %lu\n", 1UL << MPIDI_OFI_TAG_BITS);

    /* Print global thresholds */
    fprintf(stdout, "==== Provider global thresholds ====\n");
    fprintf(stdout, "max_buffered_send: %" PRIu64 "\n", MPIDI_OFI_global.max_buffered_write);
    fprintf(stdout, "max_buffered_write: %" PRIu64 "\n", MPIDI_OFI_global.max_buffered_send);
    fprintf(stdout, "max_msg_size: %" PRIu64 "\n", MPIDI_OFI_global.max_msg_size);
    fprintf(stdout, "max_order_raw: " MPI_AINT_FMT_DEC_SPEC "\n", MPIDI_OFI_global.max_order_raw);
    fprintf(stdout, "max_order_war: " MPI_AINT_FMT_DEC_SPEC "\n", MPIDI_OFI_global.max_order_war);
    fprintf(stdout, "max_order_waw: " MPI_AINT_FMT_DEC_SPEC "\n", MPIDI_OFI_global.max_order_waw);
    fprintf(stdout, "tx_iov_limit: " MPI_AINT_FMT_DEC_SPEC "\n", MPIDI_OFI_global.tx_iov_limit);
    fprintf(stdout, "rx_iov_limit: " MPI_AINT_FMT_DEC_SPEC "\n", MPIDI_OFI_global.rx_iov_limit);
    fprintf(stdout, "rma_iov_limit: " MPI_AINT_FMT_DEC_SPEC "\n", MPIDI_OFI_global.rma_iov_limit);
    fprintf(stdout, "max_mr_key_size: %" PRIu64 "\n", MPIDI_OFI_global.max_mr_key_size);
    /* Print various size limits */
    fprintf(stdout, "==== Various sizes and limits ====\n");
    fprintf(stdout, "MPIDI_OFI_AM_MSG_HEADER_SIZE: %d\n", (int) MPIDI_OFI_AM_MSG_HEADER_SIZE);
    fprintf(stdout, "MPIDI_OFI_MAX_AM_HDR_SIZE: %d\n", (int) MPIDI_OFI_MAX_AM_HDR_SIZE);
    fprintf(stdout, "sizeof(MPIDI_OFI_am_request_header_t): %d\n",
            (int) sizeof(MPIDI_OFI_am_request_header_t));
    fprintf(stdout, "sizeof(MPIDI_OFI_per_vci_t): %d\n", (int) sizeof(MPIDI_OFI_per_vci_t));
    fprintf(stdout, "MPIDI_OFI_AM_HDR_POOL_CELL_SIZE: %d\n", (int) MPIDI_OFI_AM_HDR_POOL_CELL_SIZE);
    fprintf(stdout, "MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE: %d\n",
            (int) MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE);
}

static void dump_dynamic_settings(void)
{
    fprintf(stdout, "==== OFI dynamic settings ====\n");
    fprintf(stdout, "num_vcis: %d\n", MPIDI_OFI_global.num_vcis);
    fprintf(stdout, "num_nics: %d\n", MPIDI_OFI_global.num_nics);
    fprintf(stdout, "======================================\n");
}

/* static functions for AM */

int ofi_am_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_OFI_ENABLE_AM) {
        /* Maximum possible message size for short message send (=eager send)
         * See MPIDI_OFI_do_am_isend for short/long switching logic */
        MPIR_Assert(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE <= MPIDI_OFI_global.max_msg_size);
        MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_OFI_am_request_header_t)
                                < MPIDI_OFI_AM_HDR_POOL_CELL_SIZE);
        MPL_COMPILE_TIME_ASSERT(MPIDI_OFI_AM_HDR_POOL_CELL_SIZE
                                >= sizeof(MPIDI_OFI_am_send_pipeline_request_t));
        for (int vci = 0; vci < MPIDI_OFI_global.num_vcis; vci++) {
            mpi_errno = MPIDU_genq_private_pool_create(MPIDI_OFI_AM_HDR_POOL_CELL_SIZE,
                                                       MPIDI_OFI_AM_HDR_POOL_NUM_CELLS_PER_CHUNK,
                                                       0 /* unlimited */ ,
                                                       host_alloc, host_free,
                                                       &MPIDI_OFI_global.
                                                       per_vci[vci].am_hdr_buf_pool);
            MPIR_ERR_CHECK(mpi_errno);

            MPIDI_OFI_global.per_vci[vci].cq_buffered_dynamic_head = NULL;
            MPIDI_OFI_global.per_vci[vci].cq_buffered_dynamic_tail = NULL;
            MPIDI_OFI_global.per_vci[vci].cq_buffered_static_head = 0;
            MPIDI_OFI_global.per_vci[vci].cq_buffered_static_tail = 0;

            MPIDIU_map_create(&MPIDI_OFI_global.per_vci[vci].am_recv_seq_tracker, MPL_MEM_BUFFER);
            MPIDIU_map_create(&MPIDI_OFI_global.per_vci[vci].am_send_seq_tracker, MPL_MEM_BUFFER);
            MPIDI_OFI_global.per_vci[vci].am_unordered_msgs = NULL;

            MPIDIU_map_create(&MPIDI_OFI_global.per_vci[vci].req_map, MPL_MEM_OTHER);

            MPIDI_OFI_global.per_vci[vci].deferred_am_isend_q = NULL;

            MPIDI_OFI_global.per_vci[vci].am_inflight_inject_emus = 0;
            MPIDI_OFI_global.per_vci[vci].am_inflight_rma_send_mrs = 0;
        }
        MPIDIG_am_reg_cb(MPIDI_OFI_INTERNAL_HANDLER_CONTROL, NULL, &MPIDI_OFI_control_handler);
        MPIDIG_am_reg_cb(MPIDI_OFI_AM_RDMA_READ_ACK, NULL, &MPIDI_OFI_am_rdma_read_ack_handler);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int ofi_am_post_recv(int vci, int nic)
{
    int mpi_errno = MPI_SUCCESS;

    /* Only nic 0 for now */
    MPIR_Assert(nic == 0);

    MPIDI_OFI_global.am_bufs_registered = false;

    if (MPIDI_OFI_ENABLE_AM) {
        int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
        size_t optlen = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;

        MPIDI_OFI_CALL(fi_setopt(&(MPIDI_OFI_global.ctx[ctx_idx].rx->fid),
                                 FI_OPT_ENDPOINT,
                                 FI_OPT_MIN_MULTI_RECV, &optlen, sizeof(optlen)), setopt);

        /* we allocate a single buffer and post recvs using an offset */
        MPIDI_OFI_global.per_vci[vci].am_bufs =
            MPL_malloc(MPIDI_OFI_AM_BUFF_SZ * MPIDI_OFI_NUM_AM_BUFFERS, MPL_MEM_BUFFER);
        for (int i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++) {
            MPIDI_OFI_global.per_vci[vci].am_reqs[i].event_id = MPIDI_OFI_EVENT_AM_RECV;
            MPIDI_OFI_global.per_vci[vci].am_reqs[i].index = i;
            MPIR_Assert(MPIDI_OFI_global.per_vci[vci].am_bufs);
            MPIDI_OFI_global.per_vci[vci].am_iov[i].iov_base =
                (char *) MPIDI_OFI_global.per_vci[vci].am_bufs + (MPIDI_OFI_AM_BUFF_SZ * i);
            MPIDI_OFI_global.per_vci[vci].am_iov[i].iov_len = MPIDI_OFI_AM_BUFF_SZ;
            MPIDI_OFI_global.per_vci[vci].am_msg[i].msg_iov =
                &MPIDI_OFI_global.per_vci[vci].am_iov[i];
            MPIDI_OFI_global.per_vci[vci].am_msg[i].desc = NULL;
            MPIDI_OFI_global.per_vci[vci].am_msg[i].addr = FI_ADDR_UNSPEC;
            MPIDI_OFI_global.per_vci[vci].am_msg[i].context =
                &MPIDI_OFI_global.per_vci[vci].am_reqs[i].context;
            MPIDI_OFI_global.per_vci[vci].am_msg[i].iov_count = 1;
            MPIDI_OFI_CALL_RETRY(fi_recvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                            &MPIDI_OFI_global.per_vci[vci].am_msg[i],
                                            FI_MULTI_RECV | FI_COMPLETION), 0, prepost);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* called in MPIDI_OFI_dispatch_function when FI_MULTI_RECV is flagged */
int MPIDI_OFI_am_repost_buffer(int vci, int am_idx)
{
    int mpi_errno = MPI_SUCCESS;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
    MPIDI_OFI_CALL_RETRY_AM(fi_recvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                       &MPIDI_OFI_global.per_vci[vci].am_msg[am_idx],
                                       FI_MULTI_RECV | FI_COMPLETION), prepost);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
