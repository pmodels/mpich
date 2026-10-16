/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_coll_types.h"


#define MPIDI_OFI_COLL_MIN_SIZE 2

MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                        offsetof(MPIDI_OFI_coll_request_t, context));

MPIDI_OFI_coll_global_t MPIDI_OFI_coll_global = { 0 };

static void mpidi_ofi_coll_log(const char *operation, MPIR_Comm * comm, int fi_errno)
{
    if (MPIDI_OFI_coll_global.config.verbosity_level > 0) {
        fprintf(stderr, "OFI collective %s on comm %p (context %d) failed: %s\n", operation,
                (void *) comm, comm ? comm->context_id : -1, fi_strerror(-fi_errno));
    }
}

int MPIDI_OFI_coll_init(const MPIDI_OFI_coll_config_t * config)
{
    MPIR_Assert(config);
    MPIR_Assert(config->fabric);
    MPIR_Assert(config->domain);
    MPIR_Assert(config->av);
    MPIR_Assert(config->ep);
    MPIR_Assert(config->cq);

    if (MPIDI_OFI_coll_global.initialized) {
        return MPI_SUCCESS;
    }

    int mpi_errno = MPI_SUCCESS;
    static const MPIDI_OFI_coll_config_t empty_config = { 0 };

    /* The caller asked for collective offload, so a missing EQ or missing provider
     * collective ops is an error rather than a silent fallback. */
    MPIR_ERR_CHKANDJUMP(!MPIDI_OFI_coll_global.eq ||
                        MPIDI_OFI_coll_global.eq_bound_ep != config->ep,
                        mpi_errno, MPI_ERR_OTHER, "**ofi_coll_eq");

    MPIDI_OFI_coll_global.config = *config;
    if (!config->domain->ops || !config->domain->ops->query_collective ||
        !config->ep->collective || !config->av->ops || !config->av->ops->av_set) {
        MPIDI_OFI_coll_global.config = empty_config;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofi_coll_nosupport");
    }
    MPIDI_OFI_coll_global.initialized = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_coll_pre_enable_bind(struct fid_ep *ep)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_eq_attr eq_attr = { 0 };
    int ret;

    MPIR_FUNC_ENTER;
    if (MPIDI_OFI_coll_global.eq) {
        goto fn_exit;
    }
    if (!ep) {
        goto fn_exit;
    }

    eq_attr.wait_obj = FI_WAIT_NONE;
    ret = fi_eq_open(MPIDI_OFI_global.fabric, &eq_attr, &MPIDI_OFI_coll_global.eq, NULL);
    MPIR_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ofi_coll_eq");

    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        ret = fi_scalable_ep_bind(ep, &MPIDI_OFI_coll_global.eq->fid, 0);
    } else {
        ret = fi_ep_bind(ep, &MPIDI_OFI_coll_global.eq->fid, 0);
    }
    MPIR_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ofi_coll_eq");
    MPIDI_OFI_coll_global.eq_bound_ep = ep;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (MPIDI_OFI_coll_global.eq && !MPIDI_OFI_coll_global.eq_bound_ep) {
        fi_close(&MPIDI_OFI_coll_global.eq->fid);
        MPIDI_OFI_coll_global.eq = NULL;
    }
    goto fn_exit;
}

int MPIDI_OFI_coll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_OFI_coll_global.eq) {
        MPIR_ERR_CHKANDJUMP(fi_close(&MPIDI_OFI_coll_global.eq->fid), mpi_errno,
                            MPI_ERR_OTHER, "**ofi_collop_failed");
    }
    memset(&MPIDI_OFI_coll_global, 0, sizeof(MPIDI_OFI_coll_global));

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_coll_progress(int *made_progress)
{
    uint32_t event;
    struct fi_eq_entry entry;
    struct fi_eq_err_entry err_entry = { 0 };
    ssize_t ret;

    if (!MPIDI_OFI_coll_global.initialized || !MPIDI_OFI_coll_global.eq) {
        return MPI_SUCCESS;
    }

    while (1) {
        ret = fi_eq_read(MPIDI_OFI_coll_global.eq, &event, &entry, sizeof(entry), 0);
        if (ret > 0) {
            if (event == FI_JOIN_COMPLETE && entry.context) {
                MPIDI_OFI_coll_join_request_t *request = entry.context;
                MPL_atomic_release_store_int(&request->completed, 1);
            }
        } else if (ret == -FI_EAVAIL) {
            /* A failed join is reported as an EQ error. Complete the join request with the
             * error so the communicator falls back to the MPIR algorithms. */
            ret = fi_eq_readerr(MPIDI_OFI_coll_global.eq, &err_entry, 0);
            if (ret <= 0) {
                /* Could not consume the error; stop so we do not spin on it. */
                ret = -FI_EIO;
                break;
            }
            if (err_entry.context) {
                MPIDI_OFI_coll_join_request_t *request = err_entry.context;
                request->fi_error = err_entry.err ? err_entry.err : FI_EIO;
                MPL_atomic_release_store_int(&request->completed, 1);
            }
        } else {
            break;
        }
        if (ret > 0 && made_progress) {
            *made_progress = 1;
        }
    }

    /* -FI_EAGAIN means the queue is empty */
    return ret == -FI_EAGAIN ? MPI_SUCCESS : MPI_ERR_OTHER;
}

int MPIDI_OFI_coll_cq_event(void *context)
{
    MPIDI_OFI_coll_request_t *request =
        MPL_container_of(context, MPIDI_OFI_coll_request_t, context);

    MPL_atomic_release_store_int(&request->completed, 1);
    return MPI_SUCCESS;
}

int MPIDI_OFI_coll_cq_error(void *context, int fi_error)
{
    MPIDI_OFI_coll_request_t *request =
        MPL_container_of(context, MPIDI_OFI_coll_request_t, context);

    request->fi_error = fi_error;
    MPL_atomic_release_store_int(&request->completed, 1);
    return MPI_SUCCESS;
}

int MPIDI_OFI_coll_wait(MPIDI_OFI_coll_request_t * request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    while (!MPL_atomic_acquire_load_int(&request->completed)) {
        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPIR_ERR_CHKANDJUMP(request->fi_error, mpi_errno, MPI_ERR_OTHER, "**ofi_collop_failed");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_coll_query(MPIR_Comm * comm, enum fi_collective_op collective,
                         enum fi_datatype datatype, enum fi_op op)
{
    struct fi_collective_attr attr = {
        .op = op,
        .datatype = datatype,
        .mode = 0,
    };
    int ret;

    if (!MPIDI_OFI_coll_global.config.domain->ops ||
        !MPIDI_OFI_coll_global.config.domain->ops->query_collective) {
        return MPIDI_OFI_COLL_RETVAL_FALLBACK;
    }
    ret = fi_query_collective(MPIDI_OFI_coll_global.config.domain, collective, &attr, 0);
    if (ret || attr.max_members < comm->local_size) {
        return MPIDI_OFI_COLL_RETVAL_FALLBACK;
    }
    return MPIDI_OFI_COLL_RETVAL_SUCCESS;
}

int MPIDI_OFI_coll_comm_create_hook(MPIR_Comm * comm)
{
    struct fi_av_set_attr attr = { 0 };
    fi_addr_t set_addr;
    MPIDI_OFI_coll_join_request_t joined;
    int ret;

    MPIR_FUNC_ENTER;
    MPL_atomic_relaxed_store_int(&joined.completed, 0);
    joined.fi_error = 0;
    memset(&MPIDI_OFI_COMM(comm).coll, 0, sizeof(MPIDI_OFI_COMM(comm).coll));

    if (!MPIDI_OFI_coll_global.initialized ||
        comm->comm_kind != MPIR_COMM_KIND__INTRACOMM ||
        comm->local_size < MPIDI_OFI_COLL_MIN_SIZE) {
        goto fn_exit;
    }
    if (!MPIDI_OFI_coll_global.config.domain->ops ||
        !MPIDI_OFI_coll_global.config.domain->ops->query_collective ||
        !MPIDI_OFI_coll_global.config.ep->collective ||
        !MPIDI_OFI_coll_global.config.av->ops || !MPIDI_OFI_coll_global.config.av->ops->av_set) {
        mpidi_ofi_coll_log("communicator setup", comm, FI_ENOSYS);
        goto fn_exit;
    }

    attr.count = comm->local_size;
    attr.start_addr = FI_ADDR_NOTAVAIL;
    attr.end_addr = FI_ADDR_NOTAVAIL;
    attr.stride = 0;
    attr.flags = FI_BROADCAST_SET | FI_ALLREDUCE_SET;
    ret = fi_av_set(MPIDI_OFI_coll_global.config.av, &attr,
                    &MPIDI_OFI_COMM(comm).coll.av_set, NULL);
    if (ret) {
        mpidi_ofi_coll_log("communicator setup", comm, ret);
        MPIDI_OFI_COMM(comm).coll.av_set = NULL;
        goto fn_exit;
    }

    for (int rank = 0; rank < comm->local_size; rank++) {
        fi_addr_t addr = MPIDI_OFI_av_to_phys_root(MPIDIU_comm_rank_to_av(comm, rank));
        ret = fi_av_set_insert(MPIDI_OFI_COMM(comm).coll.av_set, addr);
        if (ret) {
            mpidi_ofi_coll_log("communicator setup", comm, ret);
            goto failed;
        }
    }

    ret = fi_av_set_addr(MPIDI_OFI_COMM(comm).coll.av_set, &set_addr);
    if (ret) {
        mpidi_ofi_coll_log("communicator setup", comm, ret);
        goto failed;
    }

    ret = fi_join_collective(MPIDI_OFI_coll_global.config.ep, set_addr,
                             MPIDI_OFI_COMM(comm).coll.av_set, 0, &MPIDI_OFI_COMM(comm).coll.mc,
                             &joined);
    if (ret) {
        mpidi_ofi_coll_log("communicator setup", comm, ret);
        goto failed;
    }

    while (!MPL_atomic_acquire_load_int(&joined.completed)) {
        int mpi_errno = MPID_Progress_test(NULL);
        if (mpi_errno != MPI_SUCCESS) {
            goto failed;
        }
    }
    if (joined.fi_error) {
        goto failed;
    }

    MPIDI_OFI_COMM(comm).coll.coll_addr = fi_mc_addr(MPIDI_OFI_COMM(comm).coll.mc);
    MPIDI_OFI_COMM(comm).coll.initialized = 1;
    goto fn_exit;

  failed:
    if (MPIDI_OFI_COMM(comm).coll.mc) {
        fi_close(&MPIDI_OFI_COMM(comm).coll.mc->fid);
        MPIDI_OFI_COMM(comm).coll.mc = NULL;
    }
    if (MPIDI_OFI_COMM(comm).coll.av_set) {
        fi_close(&MPIDI_OFI_COMM(comm).coll.av_set->fid);
        MPIDI_OFI_COMM(comm).coll.av_set = NULL;
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDI_OFI_coll_comm_destroy_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    if (!comm || !MPIDI_OFI_COMM(comm).coll.initialized) {
        goto fn_exit;
    }
    if (MPIDI_OFI_COMM(comm).coll.mc) {
        MPIR_ERR_CHKANDJUMP(fi_close(&MPIDI_OFI_COMM(comm).coll.mc->fid), mpi_errno,
                            MPI_ERR_OTHER, "**ofi_collop_failed");
    }
    if (MPIDI_OFI_COMM(comm).coll.av_set) {
        MPIR_ERR_CHKANDJUMP(fi_close(&MPIDI_OFI_COMM(comm).coll.av_set->fid), mpi_errno,
                            MPI_ERR_OTHER, "**ofi_collop_failed");
    }
    memset(&MPIDI_OFI_COMM(comm).coll, 0, sizeof(MPIDI_OFI_COMM(comm).coll));

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
