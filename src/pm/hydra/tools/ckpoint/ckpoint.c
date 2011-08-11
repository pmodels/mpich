/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "ckpoint.h"
#include "hydt_ftb.h"
#ifdef HAVE_PTHREAD_H
#include "pthread.h"
#endif

#if defined HAVE_BLCR
#include "blcr/ckpoint_blcr.h"
#endif /* HAVE_BLCR */

#ifdef HAVE_PTHREADS
static pthread_t thread;
#endif
static enum { HYDT_CKPOINT_NONE, HYDT_CKPOINT_RUNNING, HYDT_CKPOINT_FINISHED } in_ckpt;
struct HYDT_ckpoint_info HYDT_ckpoint_info;

HYD_status HYDT_ckpoint_init(const char *user_ckpointlib, int user_ckpoint_num)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (user_ckpointlib)
        HYDT_ckpoint_info.ckpointlib = user_ckpointlib;
    else if (MPL_env2str("HYDRA_CKPOINTLIB", (const char **) &HYDT_ckpoint_info.ckpointlib) ==
             0)
        HYDT_ckpoint_info.ckpointlib = HYDRA_DEFAULT_CKPOINTLIB;

    /* If there is no default checkpointlib, we bail out */
    if (HYDRA_DEFAULT_CKPOINTLIB == NULL)
        goto fn_exit;

    HYDT_ckpoint_info.ckpoint_num = (user_ckpoint_num == -1) ? 0 : user_ckpoint_num;
    in_ckpt = HYDT_CKPOINT_NONE;

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDT_ckpoint_blcr_init();
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
        goto fn_exit;
    }
#endif /* HAVE_BLCR */

    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized ckpoint library\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

#ifdef HAVE_PTHREADS
static void *ckpoint_thread(void *arg)
{
    HYD_status status = HYD_SUCCESS;
    char ftb_event_payload[HYDT_FTB_MAX_PAYLOAD_DATA];

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status =
            HYDT_ckpoint_blcr_checkpoint(HYDT_ckpoint_info.prefix, HYDT_ckpoint_info.pgid,
                                         HYDT_ckpoint_info.id, HYDT_ckpoint_info.ckpoint_num);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

    HYDT_ftb_publish("FTB_MPI_PROCS_CKPTED", ftb_event_payload);

    ++HYDT_ckpoint_info.ckpoint_num;

  fn_exit:
    in_ckpt = HYDT_CKPOINT_FINISHED;
    return (void *) status;

  fn_fail:
    HYDT_ftb_publish("FTB_MPI_PROCS_CKPT_FAIL", ftb_event_payload);
    goto fn_exit;

}
#endif


HYD_status HYDT_ckpoint_checkpoint(int pgid, int id, const char *user_ckpoint_prefix)
{
#ifdef HAVE_PTHREADS
    HYD_status status = HYD_SUCCESS;
    struct stat st;
    int ret;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(user_ckpoint_prefix, status);

    ret = stat(user_ckpoint_prefix, &st);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_FAILURE,
                        "Failed to stat checkpoint prefix \"%s\": %s\n",
                        user_ckpoint_prefix, strerror(errno));
    HYDU_ERR_CHKANDJUMP(status, !S_ISDIR(st.st_mode), HYD_FAILURE,
                        "checkpoint prefix \"%s\" is not a directory.\n", user_ckpoint_prefix);

    HYDU_ERR_CHKANDJUMP(status, in_ckpt == HYDT_CKPOINT_RUNNING, HYD_FAILURE,
                        "Previous checkpoint has not completed.");

    /* if another ckpoint thread had started and finished, we need to
     * join with it to free resources */
    if (in_ckpt == HYDT_CKPOINT_FINISHED) {
        ret = pthread_join(thread, NULL);
        HYDU_ERR_CHKANDJUMP(status, ret, HYD_FAILURE, "pthread_join failed: %s.",
                            strerror(ret));
    }

    /* set state, and start the thread to do the checkpoint */
    in_ckpt = HYDT_CKPOINT_RUNNING;
    HYDT_ckpoint_info.prefix = user_ckpoint_prefix;
    HYDT_ckpoint_info.pgid = pgid;
    HYDT_ckpoint_info.id = id;
    ret = pthread_create(&thread, NULL, ckpoint_thread, NULL);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_FAILURE, "pthread_create failed: %s.", strerror(ret));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
#else
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();
    HYDU_ERR_SETANDJUMP(status, HYD_FAILURE, "pthreads required for checkpointing");
  fn_exit:
  fn_fail:
    HYDU_FUNC_EXIT();
    return status;
#endif
}

HYD_status HYDT_ckpoint_restart(int pgid, int id, struct HYD_env * envlist, int num_ranks,
                                int ranks[], int *in, int *out, int *err, int *pid,
                                const char *user_ckpoint_prefix)
{
    HYD_status status = HYD_SUCCESS;
    struct stat st;
    int ret;
    char ftb_event_payload[HYDT_FTB_MAX_PAYLOAD_DATA];

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(user_ckpoint_prefix, status);

    ret = stat(user_ckpoint_prefix, &st);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_FAILURE,
                        "Failed to stat checkpoint prefix \"%s\": %s\n",
                        user_ckpoint_prefix, strerror(errno));
    HYDU_ERR_CHKANDJUMP(status, !S_ISDIR(st.st_mode), HYD_FAILURE,
                        "checkpoint prefix \"%s\" is not a directory.\n", user_ckpoint_prefix);

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status =
            HYDT_ckpoint_blcr_restart(user_ckpoint_prefix, pgid, id,
                                      HYDT_ckpoint_info.ckpoint_num, envlist, num_ranks, ranks,
                                      in, out, err, pid);
        HYDU_ERR_POP(status, "blcr restart returned error\n");
    }
#endif /* HAVE_BLCR */

    HYDT_ftb_publish("FTB_MPI_PROCS_RESTARTED", ftb_event_payload);

    /* next checkpoint number should be the one after the one we restarted from */
    ++HYDT_ckpoint_info.ckpoint_num;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    HYDT_ftb_publish("FTB_MPI_PROCS_RESTART_FAIL", ftb_event_payload);
    goto fn_exit;
}
