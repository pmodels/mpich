/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

/* #define TS_PROFILE 1 */

#if defined(TS_PROFILE)
#include <sys/time.h>
double TS_Wtime( void );
double TS_Wtime( void )
{
    struct timeval  tval;
    gettimeofday( &tval, NULL );
    return ( (double) tval.tv_sec + (double) tval.tv_usec * 0.000001 );
}
#endif

static char* HYDI_pbs_trim_space( char *str )
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

static HYD_status HYDI_pbs_parse_for_nodes(const char *nodefile)
{
    char                       line[HYDT_PBS_STRLEN];
    FILE                      *fp;
    int                        idx;
    int                        num_nodes;
    struct HYDT_bscd_pbs_node *nodes = NULL;
    HYD_status                 status = HYD_SUCCESS;

    if ((fp = fopen(nodefile, "r")) == NULL) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PBS nodefile, %s, can't be opened.\n", nodefile);
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
        strncpy(nodes[idx].name, HYDI_pbs_trim_space(line), HYDT_PBS_STRLEN);
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

/* Comparison function for bsearch() of array of pbs_node[] based on name. */
static int cmp_pbsnode(const void *m1, const void *m2)
{
    struct HYDT_bscd_pbs_node *n1 = (struct HYDT_bscd_pbs_node *) m1;
    struct HYDT_bscd_pbs_node *n2 = (struct HYDT_bscd_pbs_node *) m2;
    return strcmp(n1->name, n2->name);
}

HYD_status HYDT_bscd_pbs_launch_procs(char **args, struct HYD_proxy *proxy_list,
                                      int *control_fd)
{
    int proxy_count, spawned_count, args_count;
    int is_rmk_pbs, idx, ierr;
    struct HYD_proxy *proxy;
    char *targs[HYD_NUM_TMP_STRINGS];
    char *nodefile = NULL;
    HYD_status status = HYD_SUCCESS;

#if defined(TS_PROFILE)
    double stime, etime;
#endif

    HYDU_FUNC_ENTER();

    /* Determine what RMK is being using */
    is_rmk_pbs = !strcmp(HYDT_bsci_info.rmk, "pbs");

    /* Parse PBS_NODEFILE for all the node names */
    nodefile = (char *) getenv("PBS_NODEFILE");
    if (!nodefile)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PBS_NODEFILE is undefined in PBS launcher.\n");

    HYDI_pbs_parse_for_nodes(nodefile);

    if (HYDT_bsci_info.debug) {
        for (idx=0; idx<HYDT_bscd_pbs_sys->num_nodes; idx++) {
            HYDU_dump(stdout, "ID=%d, name=%s.\n",
                      (HYDT_bscd_pbs_sys->nodes[idx]).id,
                      (HYDT_bscd_pbs_sys->nodes[idx]).name);
        }
    }

    proxy_count = 0;
    for (proxy = proxy_list; proxy; proxy = proxy->next)
        proxy_count++;
    HYDT_bscd_pbs_sys->size = proxy_count;

    /* Check if number of proxies > number of processes in this PBS job */
    if (proxy_count > (HYDT_bscd_pbs_sys->tm_root).tm_nnodes)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "Number of proxies(%d) > TM node count(%d)!\n",
                            proxy_count,
                            (HYDT_bscd_pbs_sys->tm_root).tm_nnodes);

    /* Duplicate the args in local copy, targs */
    for (args_count = 0; args[args_count]; args_count++)
        targs[args_count] = HYDU_strdup(args[args_count]);

    /* Allocate memory for taskIDs[] and events[] arrays */
    HYDU_MALLOC(HYDT_bscd_pbs_sys->taskIDs, tm_task_id *,
                HYDT_bscd_pbs_sys->size * sizeof(tm_task_id), status);
    HYDU_MALLOC(HYDT_bscd_pbs_sys->events, tm_event_t *,
                HYDT_bscd_pbs_sys->size * sizeof(tm_event_t), status);

    /* Sort the pbs_node[] in ascending name order for bsearch() when RMK=PBS */
    if (is_rmk_pbs)
        qsort(HYDT_bscd_pbs_sys->nodes, HYDT_bscd_pbs_sys->num_nodes,
              sizeof(struct HYDT_bscd_pbs_node), cmp_pbsnode);

#if defined(TS_PROFILE)
    stime = TS_Wtime();
#endif
    /* Spawn a process on each allocated node through tm_spawn() which
     * returns a taskID for the process + a eventID for the spawning.
     * The returned taskID won't be ready for access until tm_poll()
     * has returned the corresponding eventID. */
    if (is_rmk_pbs) {
        if (HYDT_bsci_info.debug)
            HYDU_dump(stdout,"RMK == PBS\n");
        spawned_count   = 0;
        for (proxy = proxy_list; proxy; proxy = proxy->next) {
            struct HYDT_bscd_pbs_node key, *found;
            strncpy(key.name, proxy->node->hostname, HYDT_PBS_STRLEN);
            found = bsearch(&key,
                            HYDT_bscd_pbs_sys->nodes,
                            HYDT_bscd_pbs_sys->num_nodes,
                            sizeof(struct HYDT_bscd_pbs_node), cmp_pbsnode);
            if (found == NULL) {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "Can't locate proxy, %s, in PBS nodefile\n",
                                    proxy->node->hostname);
            }
            targs[args_count] = HYDU_int_to_str(proxy->proxy_id);
            ierr = tm_spawn(args_count + 1, targs, NULL, found->id,
                            HYDT_bscd_pbs_sys->taskIDs + spawned_count,
                            HYDT_bscd_pbs_sys->events + spawned_count);
            if (HYDT_bsci_info.debug)
                HYDU_dump(stdout, "DEBUG: %d, tm_spawn(TM_nodeID=%d,name=%s)\n",
                          spawned_count, found->id, found->name);
            if (ierr != TM_SUCCESS) {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "tm_spawn() fails with TM err %d!\n", ierr);
            }
            spawned_count++;
        }
        HYDT_bscd_pbs_sys->spawned_count = spawned_count;
    }
    else {
        char *spawned_name;
        spawned_count   = 0;
        spawned_name    = NULL;
        if (HYDT_bsci_info.debug)
            HYDU_dump(stdout,"RMK != PBS\n");
        for (idx=0; idx < HYDT_bscd_pbs_sys->num_nodes; idx++) {
            struct HYDT_bscd_pbs_node *found;
            if (  !spawned_name
               || strcmp(spawned_name, (HYDT_bscd_pbs_sys->nodes[idx]).name) )
                spawned_name = (HYDT_bscd_pbs_sys->nodes[idx]).name;
            else
                continue;
            found = &(HYDT_bscd_pbs_sys->nodes[idx]);
            /* ? Pavan : Not sure what proxyID is, use spawned_count for now */
            targs[args_count] = HYDU_int_to_str(spawned_count);
            ierr = tm_spawn(args_count + 1, targs, NULL, found->id,
                            HYDT_bscd_pbs_sys->taskIDs + spawned_count,
                            HYDT_bscd_pbs_sys->events + spawned_count);
            if (HYDT_bsci_info.debug)
                HYDU_dump(stdout, "DEBUG: %d, tm_spawn(TM_nodeID=%d,name=%s)\n",
                          spawned_count, found->id, found->name);
            if (ierr != TM_SUCCESS) {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "tm_spawn() fails with TM err %d!\n", ierr);
            }
            spawned_count++;
        }
        HYDT_bscd_pbs_sys->spawned_count = spawned_count;
    }
#if defined(TS_PROFILE)
    etime = TS_Wtime();
    HYDU_dump(stdout, "tm_spawn() loop takes %f\n", etime-stime );
#endif

#if defined(COMMENTED_OUT)
    /* Poll the TM for the spawning eventID returned by tm_spawn() to
     * determine if the spawned process has started. */
#if defined(TS_PROFILE)
    stime = TS_Wtime();
#endif
    events_count = 0;
    while (events_count < spawned_count) {
        tm_event_t event = -1;
        int poll_err;
        ierr = tm_poll(TM_NULL_EVENT, &event, 0, &poll_err);
        if (ierr != TM_SUCCESS)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "tm_poll(spawn_event) fails with TM err %d.\n",
                                ierr);
        if (event != TM_NULL_EVENT) {
            for (idx = 0; idx < spawned_count; idx++) {
                if (HYDT_bscd_pbs_sys->events[idx] == event) {
                    if (HYDT_bsci_info.debug) {
                        HYDU_dump(stdout,
                                  "PBS_DEBUG: Event %d received, task %d has started.\n",
                                  event, HYDT_bscd_pbs_sys->taskIDs[idx]);
                    }
                    events_count++;
                    break; /* break from for(idx<spawned_count) loop */
                }
            }
        }
    }
#if defined(TS_PROFILE)
    etime = TS_Wtime();
    HYDU_dump(stdout, "tm_poll(spawn_events) loop takes %f\n", etime-stime );
#endif
#endif

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
