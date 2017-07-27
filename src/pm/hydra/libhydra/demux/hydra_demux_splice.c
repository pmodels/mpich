/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_demux.h"
#include "hydra_demux_internal.h"
#include "hydra_err.h"
#include "hydra_sock.h"
#include "hydra_mem.h"
#include "mpl_utlist.h"

#define SPLICE_BUF_SIZE  (16384)

struct splice_context {
    int in;
    int out;
    char buf[SPLICE_BUF_SIZE];
    int buf_start;
    int buf_length;
    struct splice_context *next;
};

static struct splice_context *splice_context_list_head = NULL;
static struct splice_context *splice_context_list_tail = NULL;

static HYD_status flush_cb(int fd, HYD_dmx_event_t events, void *userp);
static HYD_status splice_cb(int fd, HYD_dmx_event_t events, void *userp);

static HYD_status flush_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    int sent, closed;
    struct splice_context *splice_context, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    splice_context = (struct splice_context *) userp;
    HYD_ASSERT(splice_context, status);

    /* if we did not have data in the buffer, the 'out' fd should not
     * be registered with the demux engine */
    HYD_ASSERT(splice_context->buf_length, status);

    status =
        HYD_sock_write(splice_context->out, splice_context->buf + splice_context->buf_start,
                       splice_context->buf_length, &sent, &closed, HYD_SOCK_COMM_TYPE__NONBLOCKING);
    HYD_ERR_POP(status, "error writing to fd\n");

    if (closed) {
        status = HYD_dmx_deregister_fd(splice_context->out);
        HYD_ERR_POP(status, "error deregistering fd\n");

        /* multiple 'in' sockets could be splicing into a single 'out'
         * socket.  search for all of them and stop forwarding.  */
        MPL_LL_FOREACH(splice_context_list_head, tmp) {
            if (tmp->out == splice_context->out) {
                status = HYD_dmx_deregister_fd(tmp->in);
                HYD_ERR_POP(status, "error deregistering fd\n");
            }
        }

        goto fn_exit;
    }

    splice_context->buf_length -= sent;
    if (splice_context->buf_length == 0) {
        splice_context->buf_start = 0;

        /* deregister the 'out' fd and register the 'in' fd */
        status = HYD_dmx_deregister_fd(splice_context->out);
        HYD_ERR_POP(status, "error deregistering fd\n");

        status =
            HYD_dmx_register_fd(splice_context->in, HYD_DMX_POLLOUT, splice_context, splice_cb);
        HYD_ERR_POP(status, "error registering fd\n");
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status splice_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    int sent, recvd, closed;
    struct splice_context *splice_context;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    splice_context = (struct splice_context *) userp;
    HYD_ASSERT(splice_context, status);

    /* if we had data in the buffer, the 'in' fd should not be
     * registered with the demux engine */
    HYD_ASSERT(splice_context->buf_length == 0, status);

    status =
        HYD_sock_read(fd, splice_context->buf, SPLICE_BUF_SIZE, &recvd, &closed,
                      HYD_SOCK_COMM_TYPE__NONBLOCKING);
    HYD_ERR_POP(status, "error reading from fd\n");

    if (closed) {
        status = HYD_dmx_deregister_fd(splice_context->in);
        HYD_ERR_POP(status, "error deregistering fd\n");

        goto fn_exit;
    }

    status =
        HYD_sock_write(splice_context->out, splice_context->buf, recvd, &sent, &closed,
                       HYD_SOCK_COMM_TYPE__NONBLOCKING);
    HYD_ERR_POP(status, "error writing to fd\n");
    HYD_ASSERT(!closed, status);

    /* if we could not send some of the data, deregister the 'in' fd
     * and register the 'out' fd instead */
    if (sent < recvd) {
        splice_context->buf_start = sent;
        splice_context->buf_length = recvd - sent;

        status = HYD_dmx_deregister_fd(splice_context->in);
        HYD_ERR_POP(status, "error deregistering fd\n");

        status =
            HYD_dmx_register_fd(splice_context->out, HYD_DMX_POLLOUT, splice_context, flush_cb);
        HYD_ERR_POP(status, "error registering fd\n");
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_dmx_splice(int in, int out)
{
    struct splice_context *splice_context;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(splice_context, struct splice_context *, sizeof(struct splice_context), status);

    splice_context->in = in;
    splice_context->out = out;
    splice_context->buf_start = 0;
    splice_context->buf_length = 0;
    MPL_LL_APPEND(splice_context_list_head, splice_context_list_tail, splice_context);

    status = HYD_dmx_register_fd(in, HYD_DMX_POLLIN, (void *) splice_context, splice_cb);
    HYD_ERR_POP(status, "error registering fd\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_dmx_unsplice(int fd)
{
    struct splice_context *splice_context = NULL;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    MPL_LL_FOREACH(splice_context_list_head, splice_context) {
        if (splice_context->in == fd) {
            MPL_LL_DELETE(splice_context_list_head, splice_context_list_tail, splice_context);

            if (HYD_dmx_query_fd_registration(fd)) {
                status = HYD_dmx_deregister_fd(fd);
                HYD_ERR_POP(status, "error deregistering spliced fd\n");
            }

            break;
        }
    }

    HYD_ASSERT(splice_context, status);
    MPL_free(splice_context);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
