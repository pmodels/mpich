/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bind.h"

#if defined HAVE_PLPA
#include "plpa/bind_plpa.h"
#endif /* HAVE_PLPA */

struct HYDU_bind_info HYDU_bind_info;

HYD_Status HYDU_bind_init(HYD_Bindlib_t bindlib, char *user_bind_map)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_bind_info.supported = 0;

#if defined HAVE_PLPA
    if (bindlib == HYD_BINDLIB_PLPA) {
        status = HYDU_bind_plpa_init(user_bind_map, &HYDU_bind_info.supported);
        HYDU_ERR_POP(status, "unable to initialize plpa\n");
    }
#endif /* HAVE_PLPA */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_bind_process(int core)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_PLPA
    if (HYDU_bind_info.supported) {
        status = HYDU_bind_plpa_process(core);
        HYDU_ERR_POP(status, "PLPA failure binding process to core\n");
    }
#endif /* HAVE_PLPA */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


int HYDU_bind_get_core_id(int id, HYD_Binding_t binding)
{
    int socket = 0, core = 0, curid, realid;

    HYDU_FUNC_ENTER();

    if (HYDU_bind_info.supported) {
        realid = (id % (HYDU_bind_info.num_cores * HYDU_bind_info.num_sockets));

        if (binding == HYD_BIND_RR) {
            return realid;
        }
        else if (binding == HYD_BIND_BUDDY) {
            /* If we reached the maximum, loop around */
            curid = 0;
            for (core = 0; core < HYDU_bind_info.num_cores; core++) {
                for (socket = 0; socket < HYDU_bind_info.num_sockets; socket++) {
                    if (curid == realid)
                        return HYDU_bind_info.bind_map[socket][core];
                    curid++;
                }
            }
        }
        else if (binding == HYD_BIND_PACK) {
            curid = 0;
            for (socket = 0; socket < HYDU_bind_info.num_sockets; socket++) {
                for (core = 0; core < HYDU_bind_info.num_cores; core++) {
                    if (curid == realid)
                        return HYDU_bind_info.bind_map[socket][core];
                    curid++;
                }
            }
        }
        else if (binding == HYD_BIND_NONE) {
            return -1;
        }
        else if (binding == HYD_BIND_USER) {
            if (!HYDU_bind_info.user_bind_valid)
                return -1;
            else
                return HYDU_bind_info.user_bind_map[realid];
        }
    }
    else {
        HYDU_Error_printf("Process-core binding is not supported on this platform\n");
    }

    HYDU_FUNC_EXIT();
    return -1;
}
