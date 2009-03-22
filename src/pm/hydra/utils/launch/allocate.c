/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_alloc_partition(struct HYD_Partition **partition)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*partition, struct HYD_Partition *, sizeof(struct HYD_Partition), status);
    (*partition)->name = NULL;
    (*partition)->segment_list = NULL;
    (*partition)->total_proc_count = 0;

    (*partition)->pid = -1;
    (*partition)->out = -1;
    (*partition)->err = -1;
    (*partition)->exit_status = -1;
    (*partition)->proxy_args[0] = NULL;

    (*partition)->exec_list = NULL;
    (*partition)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_alloc_exec_info(struct HYD_Exec_info **exec_info)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*exec_info, struct HYD_Exec_info *, sizeof(struct HYD_Exec_info), status);
    (*exec_info)->exec_proc_count = 0;
    (*exec_info)->exec[0] = NULL;
    (*exec_info)->user_env = NULL;
    (*exec_info)->prop = HYD_ENV_PROP_UNSET;
    (*exec_info)->prop_env = NULL;
    (*exec_info)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


void HYDU_free_exec_info_list(struct HYD_Exec_info *exec_info_list)
{
    struct HYD_Exec_info *exec_info, *run;

    HYDU_FUNC_ENTER();

    exec_info = exec_info_list;
    while (exec_info) {
        run = exec_info->next;
        HYDU_free_strlist(exec_info->exec);

        HYDU_env_free_list(exec_info->user_env);
        exec_info->user_env = NULL;

        HYDU_env_free_list(exec_info->prop_env);
        exec_info->prop_env = NULL;

        HYDU_FREE(exec_info);
        exec_info = run;
    }

    HYDU_FUNC_EXIT();
}


void HYDU_free_partition_list(struct HYD_Partition *partition_list)
{
    struct HYD_Partition *partition, *tpartition;
    struct HYD_Partition_segment *segment, *tsegment;
    struct HYD_Partition_exec *exec, *texec;

    HYDU_FUNC_ENTER();

    partition = partition_list;
    while (partition) {
        tpartition = partition->next;

        HYDU_FREE(partition->name);

        segment = partition->segment_list;
        while (segment) {
            tsegment = segment->next;
            if (segment->mapping) {
                HYDU_free_strlist(segment->mapping);
                HYDU_FREE(segment->mapping);
            }
            HYDU_FREE(segment);
            segment = tsegment;
        }

        exec = partition->exec_list;
        while (exec) {
            texec = exec->next;
            HYDU_free_strlist(exec->exec);
            if (exec->prop_env)
                HYDU_env_free(exec->prop_env);
            HYDU_FREE(exec);
            exec = texec;
        }

        HYDU_free_strlist(partition->proxy_args);

        HYDU_FREE(partition);
        partition = tpartition;
    }

    HYDU_FUNC_EXIT();
}


HYD_Status HYDU_alloc_partition_segment(struct HYD_Partition_segment **segment)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*segment, struct HYD_Partition_segment *,
                sizeof(struct HYD_Partition_segment), status);
    (*segment)->start_pid = -1;
    (*segment)->proc_count = 0;
    (*segment)->mapping = NULL;
    (*segment)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_merge_partition_segment(char *name, struct HYD_Partition_segment *segment,
                                        struct HYD_Partition **partition_list)
{
    struct HYD_Partition *partition;
    struct HYD_Partition_segment *s;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (partition_list == NULL) {
        HYDU_alloc_partition(partition_list);
        (*partition_list)->segment_list = segment;
    }
    else {
        partition = *partition_list;
        while (partition) {
            if (strcmp(partition->name, name) == 0) {
                if (partition->segment_list == NULL)
                    partition->segment_list = segment;
                else {
                    s = partition->segment_list;
                    while (s->next)
                        s = s->next;
                    s->next = segment;
                }
                break;
            }
            else if (partition->next == NULL) {
                HYDU_alloc_partition(&partition->next);
                partition->next->segment_list = segment;
                break;
            }
            else {
                partition = partition->next;
            }
        }
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_alloc_partition_exec(struct HYD_Partition_exec ** exec)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*exec, struct HYD_Partition_exec *, sizeof(struct HYD_Partition_exec), status);
    (*exec)->exec[0] = NULL;
    (*exec)->proc_count = 0;
    (*exec)->prop = HYD_ENV_PROP_UNSET;
    (*exec)->prop_env = NULL;
    (*exec)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_create_host_list(char *host_file, struct HYD_Partition **partition_list)
{
    FILE *fp = NULL;
    char line[2 * MAX_HOSTNAME_LEN], *hostname, *procs;
    int num_procs, total_count;
    struct HYD_Partition_segment *segment;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(host_file, "HYDRA_USE_LOCALHOST")) {
        HYDU_alloc_partition(&(*partition_list));
        (*partition_list)->name = MPIU_Strdup("localhost");
        (*partition_list)->total_proc_count = 1;

        HYDU_alloc_partition_segment(&((*partition_list)->segment_list));
        (*partition_list)->segment_list->start_pid = 0;
        (*partition_list)->segment_list->proc_count = 1;
    }
    else {
        fp = fopen(host_file, "r");
        if (!fp)
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                 "unable to open host file: %s\n", host_file);

        total_count = 0;
        while (!feof(fp)) {
            if ((fscanf(fp, "%s", line) < 0) && errno)
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                     "unable to read input line (errno: %d)\n", errno);
            if (feof(fp))
                break;

            hostname = strtok(line, ":");
            procs = strtok(NULL, ":");
            num_procs = procs ? atoi(procs) : 1;

            /* Try to find an existing partition with this name and
             * add this segment in. If there is no existing partition
             * with this name, we create a new one. */
            HYDU_alloc_partition_segment(&segment);
            segment->start_pid = total_count;
            segment->proc_count = num_procs;
            HYDU_merge_partition_segment(hostname, segment, partition_list);

            total_count += num_procs;
        }

        fclose(fp);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
