/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

#if defined(HAVE_TM_H)
struct HYDT_bscd_pbs_sys *HYDT_bscd_pbs_sys;

static char* HYD_pbs_trim_space( char *str )
{
     char *newstr = NULL;
     int   len, idx;

     len = strlen( str );
     /* Locate the Last non-white space character and pad it with NULL */
     for (idx=len-1; idx>=0; idx--) {
         if ( !isspace(str[idx]) ) {
             str[idx+1] = 0;
             len = idx;
             break;
         }
     }
     /* Locate the First non-white space character */
     for (idx=0; idx < len; idx++) {
         if ( !isspace(str[idx]) ) {
             newstr = &(str[idx]);
             break;
         }
     }
     return newstr;
}

static HYD_status HYPU_pbs_parse_for_nodes(const char *nodefile)
{
    char  line[HYDT_PBS_STRLEN];
    FILE *fp;
    int   idx;
    int   num_nodes;
    struct HYDT_bscd_pbs_node *nodes = NULL;
    HYD_status status = HYD_SUCCESS;

    if ((fp = fopen(nodefile, "r")) == NULL) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "unable to open host file: %s\n", nodefile);
    }

    /* Go over once to find the number of lines */
    for (num_nodes = 0; fgets(line, HYDT_PBS_STRLEN, fp); num_nodes++) ;

    /* Allocate the memory for the array of nodes */
    HYDU_MALLOC(nodes, struct HYDT_bscd_pbs_node *,
                num_nodes * sizeof(struct HYDT_bscd_pbs_node), status);

    /* Allocate the memory for each of member in the array of nodes */
    rewind(fp);
    for (idx = 0; fgets(line, HYDT_PBS_STRLEN, fp); idx++) {
        nodes[idx].id  = idx;
        strncpy(nodes[idx].name, HYD_pbs_trim_space(line), HYDT_PBS_STRLEN); 
    }
    fclose(fp);

    /* Update global PBS data structure */
    HYDT_bscd_pbs_sys->num_nodes = num_nodes;
    HYDT_bscd_pbs_sys->nodes     = nodes;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bsci_launcher_pbs_init(void)
{
    char *nodefile = NULL;
    int ierr;
    HYD_status status = HYD_SUCCESS;
    int idx;

    HYDU_FUNC_ENTER();

    HYDT_bsci_fns.launch_procs = HYDT_bscd_pbs_launch_procs;
    HYDT_bsci_fns.query_env_inherit = HYDT_bscd_pbs_query_env_inherit;
    HYDT_bsci_fns.wait_for_completion = HYDT_bscd_pbs_wait_for_completion;
    HYDT_bsci_fns.launcher_finalize = HYDT_bscd_pbs_launcher_finalize;

    HYDU_MALLOC(HYDT_bscd_pbs_sys, struct HYDT_bscd_pbs_sys *,
                sizeof(struct HYDT_bscd_pbs_sys), status);

    /* Initialize TM and Hydra's PBS data structure: Nothing in the
     * returned tm_root is useful except tm_root.tm_nnodes which is
     * the number of processes allocated in this PBS job. */
    ierr = tm_init(NULL, &(HYDT_bscd_pbs_sys->tm_root));
    if (ierr != TM_SUCCESS)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "tm_init() fails with TM err=%d.\n",
                            ierr);
    HYDT_bscd_pbs_sys->spawned_count = 0;
    HYDT_bscd_pbs_sys->size = 0;
    HYDT_bscd_pbs_sys->taskIDs = NULL;
    HYDT_bscd_pbs_sys->events = NULL;
    HYDT_bscd_pbs_sys->taskobits = NULL;
    HYDT_bscd_pbs_sys->num_nodes = 0;
    HYDT_bscd_pbs_sys->nodes = NULL;

    /* Parse PBS_NODEFILE for all the node names */
    nodefile = (char *) getenv("PBS_NODEFILE");
    if (!nodefile)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PBS_NODEFILE is undefined in PBS launcher.\n");

    HYPU_pbs_parse_for_nodes(nodefile);

    if (HYDT_bsci_info.debug) {
        for (idx=0; idx<HYDT_bscd_pbs_sys->num_nodes; idx++) {
            HYDU_dump(stdout, "ID=%d, name=%s.\n",
                      (HYDT_bscd_pbs_sys->nodes[idx]).id,
                      (HYDT_bscd_pbs_sys->nodes[idx]).name);
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
#endif /* if defined(HAVE_TM_H) */

HYD_status HYDT_bsci_rmk_pbs_init(void)
{
    HYDT_bsci_fns.query_node_list = HYDT_bscd_pbs_query_node_list;
    HYDT_bsci_fns.query_native_int = HYDT_bscd_pbs_query_native_int;
    HYDT_bsci_fns.query_jobid = HYDT_bscd_pbs_query_jobid;

    return HYD_SUCCESS;
}
