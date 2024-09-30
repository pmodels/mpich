/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE
      category    : CH4
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is less than or equal to MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE (in
        bytes), then enable GPU-basedfast memcpy. The environment variable is valid only when then
        GPU IPC shmmod is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"

#define COPY_BUFFER_SZ 16384

/* localcopy_kind */
enum {
    LOCALCOPY_BLOCKING,
    LOCALCOPY_NONBLOCKING,
    LOCALCOPY_STREAM,
};

static int do_localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                        MPI_Aint sendoffset, void *recvbuf, MPI_Aint recvcount,
                        MPI_Datatype recvtype, MPI_Aint recvoffset, int localcopy_kind,
                        void *extra_param)
{
    int mpi_errno = MPI_SUCCESS;
    int sendtype_iscontig, recvtype_iscontig;
    MPI_Aint sendsize, recvsize, sdata_sz, rdata_sz, copy_sz;
    char *buf = NULL;
    MPL_pointer_attr_t send_attr, recv_attr;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_ENTER;

    MPIR_Datatype_get_size_macro(sendtype, sendsize);
    MPIR_Datatype_get_size_macro(recvtype, recvsize);

    sdata_sz = sendsize * sendcount;
    rdata_sz = recvsize * recvcount;

    send_attr.type = recv_attr.type = MPL_GPU_POINTER_UNREGISTERED_HOST;

    /* if there is no data to copy, bail out */
    if (!sdata_sz || !rdata_sz)
        goto fn_exit;

    copy_sz = sdata_sz;
    if (copy_sz > rdata_sz)
        copy_sz = rdata_sz;

    /* Builtin types is the common case; optimize for it */
    MPIR_Datatype_is_contig(sendtype, &sendtype_iscontig);
    MPIR_Datatype_is_contig(recvtype, &recvtype_iscontig);

    /* NOTE: actual_unpack_bytes is a local variable. It works because yaksa
     *       updates it at issuing time regardless of nonblocking or stream.
     */
    if (sendtype_iscontig) {
        MPI_Aint actual_unpack_bytes;
        MPI_Aint true_extent, sendtype_true_lb;
        MPIR_Type_get_true_extent_impl(sendtype, &sendtype_true_lb, &true_extent);
        const char *bufptr = MPIR_get_contig_ptr(sendbuf, sendtype_true_lb);
        if (localcopy_kind == LOCALCOPY_NONBLOCKING && extra_param) {
            MPIR_Typerep_req *typerep_req = extra_param;
            typerep_req->req = MPIR_TYPEREP_REQ_NULL;
            MPIR_Typerep_iunpack(bufptr + sendoffset, copy_sz, recvbuf, recvcount, recvtype,
                                 recvoffset, &actual_unpack_bytes, typerep_req,
                                 MPIR_TYPEREP_FLAG_NONE);
        } else if (localcopy_kind == LOCALCOPY_STREAM) {
            void *stream = extra_param;
            MPIR_Typerep_unpack_stream(bufptr + sendoffset, copy_sz, recvbuf, recvcount, recvtype,
                                       recvoffset, &actual_unpack_bytes, stream);
        } else {
            /* LOCALCOPY_BLOCKING */
            MPIR_Typerep_unpack(bufptr + sendoffset, copy_sz, recvbuf, recvcount, recvtype,
                                recvoffset, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        }
        MPIR_ERR_CHKANDJUMP(actual_unpack_bytes != copy_sz, mpi_errno, MPI_ERR_TYPE,
                            "**dtypemismatch");
    } else if (recvtype_iscontig) {
        MPI_Aint actual_pack_bytes;
        MPI_Aint true_extent, recvtype_true_lb;
        MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &true_extent);
        char *bufptr = MPIR_get_contig_ptr(recvbuf, recvtype_true_lb);
        if (localcopy_kind == LOCALCOPY_NONBLOCKING && extra_param) {
            MPIR_Typerep_req *typerep_req = extra_param;
            typerep_req->req = MPIR_TYPEREP_REQ_NULL;
            MPIR_Typerep_ipack(sendbuf, sendcount, sendtype, sendoffset,
                               bufptr + recvoffset, copy_sz, &actual_pack_bytes, typerep_req,
                               MPIR_TYPEREP_FLAG_NONE);
        } else if (localcopy_kind == LOCALCOPY_STREAM) {
            void *stream = extra_param;
            MPIR_Typerep_pack_stream(sendbuf, sendcount, sendtype, sendoffset, bufptr + recvoffset,
                                     copy_sz, &actual_pack_bytes, stream);
        } else {
            MPIR_Typerep_pack(sendbuf, sendcount, sendtype, sendoffset,
                              bufptr + recvoffset, copy_sz, &actual_pack_bytes,
                              MPIR_TYPEREP_FLAG_NONE);
        }
        MPIR_ERR_CHKANDJUMP(actual_pack_bytes != copy_sz, mpi_errno, MPI_ERR_TYPE,
                            "**dtypemismatch");
    } else {
        /* Non-contig to non-contig, we allocate a temp buffer of COPY_BUFFER_SZ,
         * unpack to the temp buffer followed with pack to recv buffer.
         *
         * Use blocking version for nonblocking kind, since it is less worth of
         * optimization.
         */
        if (localcopy_kind == LOCALCOPY_NONBLOCKING && extra_param) {
            MPIR_Typerep_req *typerep_req = extra_param;
            typerep_req->req = MPIR_TYPEREP_REQ_NULL;
        }

        /* non-contig to non-contig stream enqueue is not supported. */
        MPIR_Assert(localcopy_kind != LOCALCOPY_STREAM);

        intptr_t sfirst;
        intptr_t rfirst;

        MPIR_GPU_query_pointer_attr(sendbuf, &send_attr);
        MPIR_GPU_query_pointer_attr(recvbuf, &recv_attr);

        if (MPL_gpu_attr_is_strict_dev(&send_attr) && MPL_gpu_attr_is_strict_dev(&recv_attr)) {
            MPL_gpu_malloc((void **) &buf, COPY_BUFFER_SZ, recv_attr.device);
        } else if (MPL_gpu_attr_is_strict_dev(&send_attr) || MPL_gpu_attr_is_strict_dev(&recv_attr)) {
            MPL_gpu_malloc_host((void **) &buf, COPY_BUFFER_SZ);
        } else {
            MPIR_CHKLMEM_MALLOC(buf, char *, COPY_BUFFER_SZ, mpi_errno, "buf", MPL_MEM_BUFFER);
        }

        sfirst = sendoffset;
        rfirst = recvoffset;

        while (1) {
            MPI_Aint max_pack_bytes;
            if (copy_sz - sfirst > COPY_BUFFER_SZ) {
                max_pack_bytes = COPY_BUFFER_SZ;
            } else {
                max_pack_bytes = copy_sz - sfirst;
            }

            MPI_Aint actual_pack_bytes;
            MPIR_Typerep_pack(sendbuf, sendcount, sendtype, sfirst, buf,
                              max_pack_bytes, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(actual_pack_bytes > 0);

            sfirst += actual_pack_bytes;

            MPI_Aint actual_unpack_bytes;
            MPIR_Typerep_unpack(buf, actual_pack_bytes, recvbuf, recvcount, recvtype,
                                rfirst, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(actual_unpack_bytes > 0);

            rfirst += actual_unpack_bytes;

            /* everything that was packed from the source type must be
             * unpacked; otherwise we will lose the remaining data in
             * buf in the next iteration. */
            MPIR_ERR_CHKANDJUMP(actual_pack_bytes != actual_unpack_bytes, mpi_errno,
                                MPI_ERR_TYPE, "**dtypemismatch");

            if (rfirst == copy_sz) {
                /* successful completion */
                break;
            }
        }

        if (MPL_gpu_attr_is_strict_dev(&send_attr) && MPL_gpu_attr_is_strict_dev(&recv_attr)) {
            MPL_gpu_free(buf);
        } else if (MPL_gpu_attr_is_strict_dev(&send_attr) || MPL_gpu_attr_is_strict_dev(&recv_attr)) {
            MPL_gpu_free_host(buf);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (buf) {
        if (MPL_gpu_attr_is_strict_dev(&send_attr) && MPL_gpu_attr_is_strict_dev(&recv_attr)) {
            MPL_gpu_free(buf);
        } else if (MPL_gpu_attr_is_strict_dev(&send_attr) || MPL_gpu_attr_is_strict_dev(&recv_attr)) {
            MPL_gpu_free_host(buf);
        }
    }
    goto fn_exit;
}

#ifdef MPL_HAVE_GPU
static int do_localcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            MPI_Aint sendoffset, MPL_pointer_attr_t * send_attr, void *recvbuf,
                            MPI_Aint recvcount, MPI_Datatype recvtype, MPI_Aint recvoffset,
                            MPL_pointer_attr_t * recv_attr, MPL_gpu_copy_direction_t dir,
                            MPL_gpu_engine_type_t enginetype, bool commit, MPIR_gpu_req * gpu_req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (enginetype == MPL_GPU_ENGINE_TYPE_LAST) {
        goto fn_fallback;
    }

    /* This case is specific for contig datatypes */
    int sendtype_iscontig, recvtype_iscontig;
    MPIR_Datatype_is_contig(sendtype, &sendtype_iscontig);
    MPIR_Datatype_is_contig(recvtype, &recvtype_iscontig);

    if (sendtype_iscontig && recvtype_iscontig) {
        /* Remove guard when other backends implement MPL_gpu_imemcpy and MPL_gpu_fast_memcpy */
#ifdef MPL_HAVE_ZE
        int mpl_errno = MPL_SUCCESS;
        int completed = 0;
        int dev_id = -1;

        MPI_Aint copy_sz;
        {
            MPI_Aint sendsize, recvsize, sdata_sz, rdata_sz;

            MPIR_Datatype_get_size_macro(sendtype, sendsize);
            MPIR_Datatype_get_size_macro(recvtype, recvsize);

            sdata_sz = sendsize * sendcount;
            rdata_sz = recvsize * recvcount;

            /* if there is no data to copy, bail out */
            if (!sdata_sz || !rdata_sz)
                goto fn_exit;

            copy_sz = sdata_sz;
            if (copy_sz > rdata_sz)
                copy_sz = rdata_sz;
        }

        MPI_Aint true_extent, sendtype_true_lb, recvtype_true_lb;
        MPIR_Type_get_true_extent_impl(sendtype, &sendtype_true_lb, &true_extent);
        MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &true_extent);
        void *send_ptr = (char *) MPIR_get_contig_ptr(sendbuf, sendtype_true_lb) + sendoffset;
        void *recv_ptr = (char *) MPIR_get_contig_ptr(recvbuf, recvtype_true_lb) + recvoffset;

        MPL_pointer_attr_t sendattr, recvattr;
        if (send_attr == NULL) {
            MPIR_GPU_query_pointer_attr(sendbuf, &sendattr);
            send_attr = &sendattr;
        }
        if (recv_attr == NULL) {
            MPIR_GPU_query_pointer_attr(recvbuf, &recvattr);
            recv_attr = &recvattr;
        }
        if (send_attr->type == MPL_GPU_POINTER_UNREGISTERED_HOST &&
            recv_attr->type == MPL_GPU_POINTER_UNREGISTERED_HOST) {
            memcpy(recv_ptr, send_ptr, copy_sz);
            if (gpu_req) {
                gpu_req->type = MPIR_NULL_REQUEST;
            }
        } else if (copy_sz <= MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE) {
            mpl_errno = MPL_gpu_fast_memcpy(send_ptr, send_attr, recv_ptr, recv_attr, copy_sz);
            MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**mpl_gpu_fast_memcpy");
            if (gpu_req) {
                gpu_req->type = MPIR_NULL_REQUEST;
            }
        } else {
            if (MPL_gpu_attr_is_strict_dev(send_attr)) {
                dev_id = MPL_gpu_get_dev_id_from_attr(send_attr);
            }

            if (dev_id == -1) {
                if (MPL_gpu_attr_is_strict_dev(recv_attr)) {
                    dev_id = MPL_gpu_get_dev_id_from_attr(recv_attr);
                } else {
                    /* fallback to do_localcopy */
                    goto fn_fallback;
                }
            }
            MPIR_ERR_CHKANDJUMP(dev_id == -1, mpi_errno, MPI_ERR_OTHER,
                                "**mpl_gpu_get_dev_id_from_attr");

            if (gpu_req == NULL) {
                MPL_gpu_request req;
                mpl_errno = MPL_gpu_imemcpy(recv_ptr, send_ptr, copy_sz, dev_id, dir, enginetype,
                                            &req, commit);
                MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                    "**mpl_gpu_imemcpy");

                while (!completed) {
                    mpl_errno = MPL_gpu_test(&req, &completed);
                    MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                        "**mpl_gpu_test");
                }
            } else {
                mpl_errno = MPL_gpu_imemcpy(recv_ptr, send_ptr, copy_sz, dev_id, dir, enginetype,
                                            &gpu_req->u.gpu_req, commit);
                MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                    "**mpl_gpu_imemcpy");
                gpu_req->type = MPIR_GPU_REQUEST;
            }
        }
#else /* !MPL_HAVE_ZE */
        goto fn_fallback;
#endif
    } else {
        /* noncontig datatypes */
        goto fn_fallback;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_fallback:
    /* fallback to do_localcopy */
    if (gpu_req) {
        mpi_errno =
            do_localcopy(sendbuf, sendcount, sendtype, sendoffset, recvbuf, recvcount, recvtype,
                         recvoffset, LOCALCOPY_NONBLOCKING, &gpu_req->u.y_req);
        MPIR_ERR_CHECK(mpi_errno);
        if (gpu_req->u.y_req.req == MPIR_TYPEREP_REQ_NULL) {
            gpu_req->type = MPIR_NULL_REQUEST;
        } else {
            gpu_req->type = MPIR_TYPEREP_REQUEST;
        }
    } else {
        mpi_errno =
            do_localcopy(sendbuf, sendcount, sendtype, sendoffset, recvbuf, recvcount, recvtype,
                         recvoffset, LOCALCOPY_BLOCKING, NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }
    goto fn_exit;
}
#endif

int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno =
        do_localcopy(sendbuf, sendcount, sendtype, 0, recvbuf, recvcount, recvtype, 0,
                     LOCALCOPY_BLOCKING, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ilocalcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                    MPIR_Typerep_req * typerep_req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = do_localcopy(sendbuf, sendcount, sendtype, 0, recvbuf, recvcount, recvtype,
                             0, LOCALCOPY_NONBLOCKING, typerep_req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Only single chunk copy (either sendtype or recvtype is contig) works with do_localcopy.
 * For noncontig to noncontig, the copy consists of pacc/unpack to/from an intermediary
 * temporary buffer. Since MPIR_Typerep_(un)pack_stream cannot handle host-host copy, non-
 * contig local copy needs to be separately handled by caller.
 */
int MPIR_Localcopy_stream(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                          void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, void *stream)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = do_localcopy(sendbuf, sendcount, sendtype, 0, recvbuf, recvcount,
                             recvtype, 0, LOCALCOPY_STREAM, stream);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Localcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       MPI_Aint sendoffset, MPL_pointer_attr_t * sendattr, void *recvbuf,
                       MPI_Aint recvcount, MPI_Datatype recvtype, MPI_Aint recvoffset,
                       MPL_pointer_attr_t * recvattr, MPL_gpu_copy_direction_t dir,
                       MPL_gpu_engine_type_t enginetype, bool commit)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPL_HAVE_GPU
    mpi_errno =
        do_localcopy_gpu(sendbuf, sendcount, sendtype, sendoffset, sendattr, recvbuf, recvcount,
                         recvtype, recvoffset, recvattr, dir, enginetype, commit, NULL);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno =
        do_localcopy(sendbuf, sendcount, sendtype, sendoffset, recvbuf, recvcount, recvtype,
                     recvoffset, LOCALCOPY_BLOCKING, NULL);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ilocalcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                        MPI_Aint sendoffset, MPL_pointer_attr_t * sendattr, void *recvbuf,
                        MPI_Aint recvcount, MPI_Datatype recvtype, MPI_Aint recvoffset,
                        MPL_pointer_attr_t * recvattr, MPL_gpu_copy_direction_t dir,
                        MPL_gpu_engine_type_t enginetype, bool commit, MPIR_gpu_req * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPL_HAVE_GPU
    mpi_errno =
        do_localcopy_gpu(sendbuf, sendcount, sendtype, sendoffset, sendattr, recvbuf, recvcount,
                         recvtype, recvoffset, recvattr, dir, enginetype, commit, req);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno =
        do_localcopy(sendbuf, sendcount, sendtype, sendoffset, recvbuf, recvcount, recvtype,
                     recvoffset, LOCALCOPY_NONBLOCKING, &req->u.y_req);
    MPIR_ERR_CHECK(mpi_errno);
    req->type = MPIR_TYPEREP_REQUEST;
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
