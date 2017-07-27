/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_demux.h"
#include "hydra_demux_internal.h"
#include "hydra_err.h"
#include "hydra_mem.h"

int HYDI_dmx_num_cb_fds = 0;
struct HYDI_dmx_callback *HYDI_dmx_cb_list = NULL;

HYD_status HYD_dmx_register_fd(int fd, HYD_dmx_event_t events, void *userp,
                               HYD_status(*callback) (int fd, HYD_dmx_event_t events, void *userp))
{
    struct HYDI_dmx_callback *cb_element;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_ASSERT(events, status);

#if defined HAVE_ERROR_CHECKING
    if (fd < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "registering bad fd %d\n", fd);

    MPL_HASH_FIND_INT(HYDI_dmx_cb_list, &fd, cb_element);
    if (cb_element)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "registering duplicate fd %d\n", fd);
#endif /* HAVE_ERROR_CHECKING */

    HYD_MALLOC(cb_element, struct HYDI_dmx_callback *, sizeof(struct HYDI_dmx_callback), status);
    cb_element->fd = fd;
    cb_element->events = events;
    cb_element->userp = userp;
    cb_element->callback = callback;

    MPL_HASH_ADD_INT(HYDI_dmx_cb_list, fd, cb_element);
    HYDI_dmx_num_cb_fds++;

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_dmx_deregister_fd(int fd)
{
    struct HYDI_dmx_callback *cb_element;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    MPL_HASH_FIND_INT(HYDI_dmx_cb_list, &fd, cb_element);

    /* FD is not found */
    if (cb_element == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "could not find fd to deregister: %d\n", fd);

    HYDI_dmx_num_cb_fds--;
    MPL_HASH_DEL(HYDI_dmx_cb_list, cb_element);
    MPL_free(cb_element);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_dmx_wait_for_event(int wtime)
{
#if defined HAVE_POLL
    return HYDI_dmx_poll_wait_for_event(wtime);
#elif defined HAVE_SELECT
    return HYDI_dmx_select_wait_for_event(wtime);
#endif
}

int HYD_dmx_query_fd_registration(int fd)
{
    struct HYDI_dmx_callback *run;

    HYD_FUNC_ENTER();

    MPL_HASH_FIND_INT(HYDI_dmx_cb_list, &fd, run);

    HYD_FUNC_EXIT();

    return ! !run;
}
