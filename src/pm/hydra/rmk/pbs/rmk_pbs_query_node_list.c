/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"
#include "rmk_pbs.h"

HYD_status HYD_rmkd_pbs_query_node_list(int *num_nodes, struct HYD_proxy **proxy_list)
{
    char *host_file, *hostname, line[HYD_TMP_STRLEN], **arg_list;
    int num_procs;
    FILE *fp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    host_file = getenv("PBS_NODEFILE");

    if (host_file == NULL || host_file == NULL) {
        *proxy_list = NULL;
    }
    else {
        fp = fopen(host_file, "r");
        if (!fp)
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                 "unable to open host file: %s\n", host_file);

        *num_nodes = 0;
        while (fgets(line, HYD_TMP_STRLEN, fp)) {
            char *linep = NULL;

            linep = line;
            strtok(linep, "#");

            while (isspace(*linep))
                linep++;

            /* Ignore blank lines & comments */
            if ((*linep == '#') || (*linep == '\0'))
                continue;

            /* break up the arguments in the line */
            arg_list = HYDU_str_to_strlist(linep);
            if (!arg_list)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "Unable to convert host file entry to strlist\n");

            hostname = arg_list[0];
            num_procs = 1;

            /* Try to find an existing proxy with this name and
             * add this segment in. If there is no existing proxy
             * with this name, we create a new one. */
            status = HYDU_merge_proxy_segment(hostname, *num_nodes, num_procs, proxy_list);
            HYDU_ERR_POP(status, "merge proxy segment failed\n");

            *num_nodes += num_procs;

            HYDU_FREE(arg_list);
        }

        fclose(fp);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
