/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 Mellanox Technologies, Inc.
 *
 */


#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

#include "mpid_nem_impl.h"
#include "mxm_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_NEMESIS_MXM_BULK_CONNECT
      category    : CH3
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, force mxm to connect all processes at initialization
        time.

    - name        : MPIR_CVAR_NEMESIS_MXM_BULK_DISCONNECT
      category    : CH3
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, force mxm to disconnect all processes at
        finalization time.

    - name        : MPIR_CVAR_NEMESIS_MXM_HUGEPAGE
      category    : CH3
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, mxm tries detecting hugepage support.  On HPC-X 2.3
        and earlier, this might cause problems on Ubuntu and other
        platforms even if the system provides hugepage support.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


MPID_nem_netmod_funcs_t MPIDI_nem_mxm_funcs = {
    MPID_nem_mxm_init,
    MPID_nem_mxm_finalize,
#ifdef ENABLE_CHECKPOINTING
    NULL,
    NULL,
    NULL,
#endif
    MPID_nem_mxm_poll,
    MPID_nem_mxm_get_business_card,
    MPID_nem_mxm_connect_to_root,
    MPID_nem_mxm_vc_init,
    MPID_nem_mxm_vc_destroy,
    MPID_nem_mxm_vc_terminate,
    MPID_nem_mxm_anysource_iprobe,
    MPID_nem_mxm_anysource_improbe,
    MPID_nem_mxm_get_ordering
};

static MPIDI_Comm_ops_t comm_ops = {
    MPID_nem_mxm_recv,  /* recv_posted */

    MPID_nem_mxm_send,  /* send */
    MPID_nem_mxm_send,  /* rsend */
    MPID_nem_mxm_ssend, /* ssend */
    MPID_nem_mxm_isend, /* isend */
    MPID_nem_mxm_isend, /* irsend */
    MPID_nem_mxm_issend,        /* issend */

    NULL,       /* send_init */
    NULL,       /* bsend_init */
    NULL,       /* rsend_init */
    NULL,       /* ssend_init */
    NULL,       /* startall */

    MPID_nem_mxm_cancel_send,   /* cancel_send */
    MPID_nem_mxm_cancel_recv,   /* cancel_recv */

    MPID_nem_mxm_probe, /* probe */
    MPID_nem_mxm_iprobe,        /* iprobe */
    MPID_nem_mxm_improbe        /* improbe */
};


static MPID_nem_mxm_module_t _mxm_obj;
MPID_nem_mxm_module_t *mxm_obj;

static int _mxm_init(int rank, int size);
static int _mxm_fini(void);
static int _mxm_post_init(void);
static int _mxm_connect(MPID_nem_mxm_ep_t * ep, const char *business_card,
                        MPID_nem_mxm_vc_area * vc_area);
static int _mxm_disconnect(MPID_nem_mxm_ep_t * ep);
static int _mxm_add_comm(MPID_Comm * comm, void *param);
static int _mxm_del_comm(MPID_Comm * comm, void *param);
static int _mxm_conf(void);


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int r;
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MXM_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_INIT);

    /* first make sure that our private fields in the vc and req fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_mxm_vc_area) <= MPIDI_NEM_VC_NETMOD_AREA_LEN);
    MPIU_Assert(sizeof(MPID_nem_mxm_req_area) <= MPIDI_NEM_REQ_NETMOD_AREA_LEN);


    /* mpich-specific initialization of mxm */
    /* check if the user is not trying to override the tls setting
     * before resetting it */
    if (getenv("MXM_TLS") == NULL) {
        r = MPL_putenv("MXM_TLS=rc,dc,ud");
        MPIR_ERR_CHKANDJUMP(r, mpi_errno, MPI_ERR_OTHER, "**putenv");
    }

    /* [PB @ 2014-10-06] If hugepage support is not enabled, we force
     * memory allocation to go through mmap.  This is mainly to
     * workaround issues in MXM with Ubuntu where the detection has
     * some issues (either because of bugs on the platform or within
     * MXM) causing errors.  This can probably be deleted eventually
     * when this issue is resolved.  */
    if (MPIR_CVAR_NEMESIS_MXM_HUGEPAGE == 0) {
        if (getenv("MXM_MEM_ALLOC") == NULL) {
            r = MPL_putenv("MXM_MEM_ALLOC=mmap,libc,sysv");
            MPIR_ERR_CHKANDJUMP(r, mpi_errno, MPI_ERR_OTHER, "**putenv");
        }
    }


    mpi_errno = _mxm_init(pg_rank, pg_p->size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_mxm_get_business_card(pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIDI_CH3I_Register_anysource_notification(MPID_nem_mxm_anysource_posted,
                                                   MPID_nem_mxm_anysource_matched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_register_initcomp_cb(_mxm_post_init);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH3U_Comm_register_create_hook(_mxm_add_comm, NULL);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH3U_Comm_register_destroy_hook(_mxm_del_comm, NULL);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDI_Anysource_improbe_fn = MPID_nem_mxm_anysource_improbe;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MXM_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_FINALIZE);

    _mxm_barrier();

    mpi_errno = _mxm_fini();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_get_business_card
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MXM_GET_BUSINESS_CARD);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_GET_BUSINESS_CARD);

    str_errno = MPIU_Str_add_binary_arg(bc_val_p, val_max_sz_p, MXM_MPICH_ENDPOINT_KEY,
                                        _mxm_obj.mxm_ep_addr, _mxm_obj.mxm_ep_addr_size);
    if (str_errno) {
        MPIR_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_GET_BUSINESS_CARD);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_connect_to_root
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MXM_CONNECT_TO_ROOT);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_CONNECT_TO_ROOT);

    MPIR_ERR_SETFATAL(mpi_errno, MPI_ERR_OTHER, "**notimpl");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_CONNECT_TO_ROOT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_vc_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_vc_init(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPID_nem_mxm_vc_area *vc_area = VC_BASE(vc);

    MPIDI_STATE_DECL(MPID_STATE_MXM_VC_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_VC_INIT);

    /* local connection is used for any source communication */
    MPIU_Assert(MPID_nem_mem_region.rank != vc->lpid);
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE,
                     (MPIU_DBG_FDEST,
                      "[%i]=== connecting  to  %i  \n", MPID_nem_mem_region.rank, vc->lpid));
    {
        char *business_card;
        int val_max_sz;
#ifdef USE_PMI2_API
        val_max_sz = PMI2_MAX_VALLEN;
#else
        mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif

        business_card = (char *) MPIU_Malloc(val_max_sz);
        mpi_errno = vc->pg->getConnInfo(vc->pg_rank, business_card, val_max_sz, vc->pg);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        vc_area->ctx = vc;
        vc_area->mxm_ep = &_mxm_obj.endpoint[vc->pg_rank];
        mpi_errno = _mxm_connect(&_mxm_obj.endpoint[vc->pg_rank], business_card, vc_area);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPIU_Free(business_card);
    }

    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    vc_area->pending_sends = 0;

    /* Use default rendezvous functions */
    vc->eager_max_msg_sz = _mxm_eager_threshold();
    vc->ready_eager_max_msg_sz = vc->eager_max_msg_sz;
    vc->rndvSend_fn = MPID_nem_lmt_RndvSend;
    vc->rndvRecv_fn = MPID_nem_lmt_RndvRecv;

    vc->sendNoncontig_fn = MPID_nem_mxm_SendNoncontig;
    vc->comm_ops = &comm_ops;

    vc_ch->iStartContigMsg = MPID_nem_mxm_iStartContigMsg;
    vc_ch->iSendContig = MPID_nem_mxm_iSendContig;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_VC_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_vc_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_vc_destroy(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MXM_VC_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_VC_DESTROY);

    /* Do nothing because
     * finalize is called before vc destroy as result it is not possible
     * to destroy endpoint here
     */
#if 0
    MPID_nem_mxm_vc_area *vc_area = VC_BASE(vc);
    if (vc_area->ctx == vc) {
        mpi_errno = _mxm_disconnect(vc_area->mxm_ep);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_VC_DESTROY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_vc_terminate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_vc_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_mxm_vc_area *vc_area = VC_BASE(vc);

    MPIDI_STATE_DECL(MPID_STATE_MXM_VC_TERMINATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_VC_TERMINATE);

    if (vc->state != MPIDI_VC_STATE_CLOSED) {
        /* VC is terminated as a result of a fault.  Complete
         * outstanding sends with an error and terminate connection
         * immediately. */
        MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
    }
    else {
        while (vc_area->pending_sends > 0)
            MPID_nem_mxm_poll(FALSE);
    }

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_VC_TERMINATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_get_ordering
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_get_ordering(int *ordering)
{
    (*ordering) = 1;
    return MPI_SUCCESS;
}

static int _mxm_conf(void)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned long cur_ver;

    cur_ver = mxm_get_version();
    if (cur_ver != MXM_API) {
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE,
                         (MPIU_DBG_FDEST,
                          "WARNING: MPICH was compiled with MXM version %d.%d but version %ld.%ld detected.",
                          MXM_VERNO_MAJOR,
                          MXM_VERNO_MINOR,
                          (cur_ver >> MXM_MAJOR_BIT) & 0xff, (cur_ver >> MXM_MINOR_BIT) & 0xff));
    }

    _mxm_obj.compiletime_version = MXM_VERNO_STRING;
#if MXM_API >= MXM_VERSION(3,0)
    _mxm_obj.runtime_version = MPIU_Strdup(mxm_get_version_string());
#else
    _mxm_obj.runtime_version = MPIU_Malloc(sizeof(MXM_VERNO_STRING) + 10);
    snprintf(_mxm_obj.runtime_version, (sizeof(MXM_VERNO_STRING) + 9),
             "%ld.%ld", (cur_ver >> MXM_MAJOR_BIT) & 0xff, (cur_ver >> MXM_MINOR_BIT) & 0xff);
#endif

    _mxm_obj.conf.bulk_connect =
        cur_ver < MXM_VERSION(3, 2) ? 0 : MPIR_CVAR_NEMESIS_MXM_BULK_CONNECT;
    _mxm_obj.conf.bulk_disconnect =
        cur_ver < MXM_VERSION(3, 2) ? 0 : MPIR_CVAR_NEMESIS_MXM_BULK_DISCONNECT;

    if (cur_ver < MXM_VERSION(3, 2) &&
        (_mxm_obj.conf.bulk_connect || _mxm_obj.conf.bulk_disconnect)) {
        _mxm_obj.conf.bulk_connect = 0;
        _mxm_obj.conf.bulk_disconnect = 0;
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE,
                         (MPIU_DBG_FDEST,
                          "WARNING: MPICH runs with %s version of MXM that is less than 3.2, "
                          "so bulk connect/disconnect cannot work properly and will be turn off.",
                          _mxm_obj.runtime_version));
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int _mxm_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;

    ret = _mxm_conf();

    ret = mxm_config_read_opts(&_mxm_obj.mxm_ctx_opts, &_mxm_obj.mxm_ep_opts, "MPICH2", NULL, 0);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                         mpi_errno, MPI_ERR_OTHER,
                         "**mxm_config_read_opts",
                         "**mxm_config_read_opts %s", mxm_error_string(ret));

    ret = mxm_init(_mxm_obj.mxm_ctx_opts, &_mxm_obj.mxm_context);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                         mpi_errno, MPI_ERR_OTHER,
                         "**mxm_init", "**mxm_init %s", mxm_error_string(ret));

    ret =
        mxm_set_am_handler(_mxm_obj.mxm_context, MXM_MPICH_HID_ADI_MSG, MPID_nem_mxm_get_adi_msg,
                           0);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK, mpi_errno, MPI_ERR_OTHER, "**mxm_set_am_handler",
                         "**mxm_set_am_handler %s", mxm_error_string(ret));

    ret = mxm_mq_create(_mxm_obj.mxm_context, MXM_MPICH_MQ_ID, &_mxm_obj.mxm_mq);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                         mpi_errno, MPI_ERR_OTHER,
                         "**mxm_mq_create", "**mxm_mq_create %s", mxm_error_string(ret));

    ret = mxm_ep_create(_mxm_obj.mxm_context, _mxm_obj.mxm_ep_opts, &_mxm_obj.mxm_ep);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                         mpi_errno, MPI_ERR_OTHER,
                         "**mxm_ep_create", "**mxm_ep_create %s", mxm_error_string(ret));

    _mxm_obj.mxm_ep_addr_size = MXM_MPICH_MAX_ADDR_SIZE;
    ret = mxm_ep_get_address(_mxm_obj.mxm_ep, &_mxm_obj.mxm_ep_addr, &_mxm_obj.mxm_ep_addr_size);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                         mpi_errno, MPI_ERR_OTHER,
                         "**mxm_ep_get_address", "**mxm_ep_get_address %s", mxm_error_string(ret));

    _mxm_obj.mxm_rank = rank;
    _mxm_obj.mxm_np = size;
    _mxm_obj.endpoint =
        (MPID_nem_mxm_ep_t *) MPIU_Malloc(_mxm_obj.mxm_np * sizeof(MPID_nem_mxm_ep_t));
    memset(_mxm_obj.endpoint, 0, _mxm_obj.mxm_np * sizeof(MPID_nem_mxm_ep_t));

    list_init(&_mxm_obj.free_queue);
    list_grow_mxm_req(&_mxm_obj.free_queue);
    MPIU_Assert(list_length(&_mxm_obj.free_queue) == MXM_MPICH_MAX_REQ);

    _mxm_obj.sreq_queue.head = _mxm_obj.sreq_queue.tail = NULL;

    mxm_obj = &_mxm_obj;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int _mxm_fini(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (_mxm_obj.mxm_context) {

        while (!list_is_empty(&_mxm_obj.free_queue)) {
            MPIU_Free(list_dequeue(&_mxm_obj.free_queue));
        }

#if MXM_API >= MXM_VERSION(3,1)
        if (_mxm_obj.conf.bulk_disconnect) {
            mxm_ep_powerdown(_mxm_obj.mxm_ep);
        }
#endif

        while (_mxm_obj.mxm_np) {
            _mxm_disconnect(&(_mxm_obj.endpoint[--_mxm_obj.mxm_np]));
        }

        if (_mxm_obj.endpoint)
            MPIU_Free(_mxm_obj.endpoint);

        _mxm_barrier();

        if (_mxm_obj.mxm_ep)
            mxm_ep_destroy(_mxm_obj.mxm_ep);

        if (_mxm_obj.mxm_mq)
            mxm_mq_destroy(_mxm_obj.mxm_mq);

        mxm_cleanup(_mxm_obj.mxm_context);
        _mxm_obj.mxm_context = NULL;

        mxm_config_free_ep_opts(_mxm_obj.mxm_ep_opts);
        mxm_config_free_context_opts(_mxm_obj.mxm_ctx_opts);

        MPIU_Free(_mxm_obj.runtime_version);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int _mxm_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    _mxm_barrier();

#if MXM_API >= MXM_VERSION(3,1)
    /* Current logic guarantees that all VCs have been initialized before post init call */
    if (_mxm_obj.conf.bulk_connect) {
        mxm_ep_wireup(_mxm_obj.mxm_ep);
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int _mxm_connect(MPID_nem_mxm_ep_t * ep, const char *business_card,
                        MPID_nem_mxm_vc_area * vc_area)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
    mxm_error_t ret = MXM_OK;
    char mxm_ep_addr[MXM_MPICH_MAX_ADDR_SIZE];
    int len = 0;

    str_errno =
        MPIU_Str_get_binary_arg(business_card, MXM_MPICH_ENDPOINT_KEY, mxm_ep_addr,
                                sizeof(mxm_ep_addr), &len);
    MPIR_ERR_CHKANDJUMP(str_errno, mpi_errno, MPI_ERR_OTHER, "**buscard");

    ret = mxm_ep_connect(_mxm_obj.mxm_ep, mxm_ep_addr, &ep->mxm_conn);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                         mpi_errno, MPI_ERR_OTHER,
                         "**mxm_ep_connect", "**mxm_ep_connect %s", mxm_error_string(ret));

    mxm_conn_ctx_set(ep->mxm_conn, vc_area->ctx);

    list_init(&ep->free_queue);
    list_grow_mxm_req(&ep->free_queue);
    MPIU_Assert(list_length(&ep->free_queue) == MXM_MPICH_MAX_REQ);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int _mxm_disconnect(MPID_nem_mxm_ep_t * ep)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;

    MPIU_Assert(ep);

    if (ep->mxm_conn) {
        ret = mxm_ep_disconnect(ep->mxm_conn);
        MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                             mpi_errno, MPI_ERR_OTHER,
                             "**mxm_ep_disconnect",
                             "**mxm_ep_disconnect %s", mxm_error_string(ret));

        while (!list_is_empty(&ep->free_queue)) {
            MPIU_Free(list_dequeue(&ep->free_queue));
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int _mxm_add_comm(MPID_Comm * comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;
    mxm_mq_h *mq_h_v;
    MPIU_CHKPMEM_DECL(1);

    MPIU_CHKPMEM_MALLOC(mq_h_v, mxm_mq_h *, sizeof(mxm_mq_h) * 2, mpi_errno,
                        "mxm_mq_h_context_ptr");

    _dbg_mxm_output(6, "Add COMM comm %p (rank %d type %d context %d | %d size %d | %d) \n",
                    comm, comm->rank, comm->comm_kind,
                    comm->context_id, comm->recvcontext_id, comm->local_size, comm->remote_size);

    ret = mxm_mq_create(_mxm_obj.mxm_context, comm->recvcontext_id, &mq_h_v[0]);
    MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                         mpi_errno, MPI_ERR_OTHER,
                         "**mxm_mq_create", "**mxm_mq_create %s", mxm_error_string(ret));

    if (comm->recvcontext_id != comm->context_id) {
        ret = mxm_mq_create(_mxm_obj.mxm_context, comm->context_id, &mq_h_v[1]);
        MPIR_ERR_CHKANDJUMP1(ret != MXM_OK,
                             mpi_errno, MPI_ERR_OTHER,
                             "**mxm_mq_create", "**mxm_mq_create %s", mxm_error_string(ret));
    }
    else
        memcpy(&mq_h_v[1], &mq_h_v[0], sizeof(mxm_mq_h));

    comm->dev.ch.netmod_priv = (void *) mq_h_v;

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

static int _mxm_del_comm(MPID_Comm * comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_mq_h *mxm_mq = (mxm_mq_h *) comm->dev.ch.netmod_priv;

    _dbg_mxm_output(6, "Del COMM comm %p (rank %d type %d) \n", comm, comm->rank, comm->comm_kind);

    if (mxm_mq[1] != mxm_mq[0])
        mxm_mq_destroy(mxm_mq[1]);
    if (mxm_mq[0])
        mxm_mq_destroy(mxm_mq[0]);

    MPIU_Free(mxm_mq);

    comm->dev.ch.netmod_priv = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
