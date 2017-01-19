/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2016      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <pmix_tool.h>

static volatile bool completed = false;
static volatile pmix_status_t exit_code = PMIX_SUCCESS;

static void notification_fn(size_t evhdlr_registration_id,
                            pmix_status_t status,
                            const pmix_proc_t *source,
                            pmix_info_t info[], size_t ninfo,
                            pmix_info_t results[], size_t nresults,
                            pmix_event_notification_cbfunc_fn_t cbfunc,
                            void *cbdata)
{
    exit_code = status;
    completed = true;
    /* perform our required duty by notifying the library
     * that we are done with the notification */
    if (NULL != cbfunc) {
        cbfunc(PMIX_SUCCESS, NULL, 0, NULL, NULL, cbdata);
    }
}

int main(int argc, char **argv)
{
    pmix_status_t rc;
    pmix_proc_t myproc;
    pmix_info_t *info;
    pmix_app_t *app;
    size_t ninfo, napps;
    bool flag;

    /* we need to attach to a "system" PMIx server so we
     * can ask it to spawn applications for us. There can
     * only be one such connection on a node, so we will
     * instruct the tool library to only look for it */
    ninfo = 1;
    PMIX_INFO_CREATE(info, ninfo);
    flag = true;
    PMIX_INFO_LOAD(&info[0], PMIX_CONNECT_TO_SYSTEM, &flag, PMIX_BOOL);

    /* initialize the library and make the connection */
    if (PMIX_SUCCESS != (rc = PMIx_tool_init(&myproc, info, ninfo))) {
        fprintf(stderr, "PMIx_tool_init failed: %d\n", rc);
        exit(rc);
    }
    if (0 < ninfo) {
        PMIX_INFO_FREE(info, ninfo);
    }

    /* register a default event handler so we can be notified when
     * our spawned job completes, or if it fails (even at launch) */
    PMIx_Register_event_handler(NULL, 0, NULL, 0,
                                notification_fn, NULL, NULL);

    /* parse the cmd line and create our array of app structs
     * describing the application we want launched */
    napps = 1;
    PMIX_APP_CREATE(app, napps);
    /* setup the executable */
    app[0].cmd = strdup("app");
    app[0].argc = 1;
    app[0].argv = (char**)malloc(2*sizeof(char*));
    app[0].argv[0] = strdup("app");
    app[0].argv[1] = NULL;
    /* can also provide environmental params in the app.env field */

    /* provide directives so the apps do what the user requested - just
     * some random examples provided here*/
    ninfo = 3;
    PMIX_INFO_CREATE(app[0].info, ninfo);
    PMIX_INFO_LOAD(&app[0].info[0], PMIX_NP, 128, PMIX_UINT64);
    PMIX_INFO_LOAD(&app[0].info[1], PMIX_MAPBY, "slot", PMIX_STRING);
    /* include a directive that we be notified upon completion of the job */
    PMIX_INFO_LOAD(&app[0].info[2], PMIX_NOTIFY_COMPLETION, &flag, PMIX_BOOL);

    /* spawn the application */
    PMIx_Spawn(NULL, 0, app, napps, appspace);
    /* cleanup */
    PMIX_APP_FREE(app, napps);

    while (!complete) {
        /* just cycle in sleep so we don't eat cpu */
        usleep(100);
    }

    PMIx_tool_finalize(NULL, 0);

    return(exit_code);
}
