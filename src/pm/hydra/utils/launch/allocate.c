/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

static HYD_Status alloc_partition_base(struct HYD_Partition_base **base)
{
    static partition_id = 0;
    HYD_Status status = HYD_SUCCESS;

    HYDU_MALLOC(*base, struct HYD_Partition_base *, sizeof(struct HYD_Partition_base),
                status);

    (*base)->name = NULL;
    (*base)->pid = -1;
    (*base)->out = -1;
    (*base)->err = -1;

    (*base)->partition_id = partition_id++;
    (*base)->active = 0;
    (*base)->exec_args[0] = NULL;

    (*base)->next = NULL;

fn_exit:
    return status;

fn_fail:
    goto fn_exit;
}

static void free_partition_base(struct HYD_Partition_base *base)
{
    if (base->name)
        HYDU_FREE(base->name);
    HYDU_free_strlist(base->exec_args);

    HYDU_FREE(base);
}

HYD_Status HYDU_alloc_partition(struct HYD_Partition **partition)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*partition, struct HYD_Partition *, sizeof(struct HYD_Partition), status);

    alloc_partition_base(&((*partition)->base));

    (*partition)->user_bind_map = NULL;
    (*partition)->segment_list = NULL;
    (*partition)->one_pass_count = 0;

    (*partition)->exit_status = -1;
    (*partition)->control_fd = -1;

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

        free_partition_base(partition->base);

        if (partition->user_bind_map)
            HYDU_FREE(partition->user_bind_map);

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

    if (*partition_list == NULL) {
        status = HYDU_alloc_partition(partition_list);
        HYDU_ERR_POP(status, "Unable to alloc partition\n");
        (*partition_list)->segment_list = segment;
        (*partition_list)->base->name = HYDU_strdup(name);
        (*partition_list)->one_pass_count += segment->proc_count;
    }
    else {
        partition = *partition_list;
        while (partition) {
            if (strcmp(partition->base->name, name) == 0) {
                if (partition->segment_list == NULL)
                    partition->segment_list = segment;
                else {
                    s = partition->segment_list;
                    while (s->next)
                        s = s->next;
                    s->next = segment;
                }
                partition->one_pass_count += segment->proc_count;
                break;
            }
            else if (partition->next == NULL) {
                status = HYDU_alloc_partition(&partition->next);
                HYDU_ERR_POP(status, "Unable to alloc partition\n");
                partition->next->segment_list = segment;
                partition->next->base->name = HYDU_strdup(name);
                partition->next->one_pass_count += segment->proc_count;
                partition->base->next = partition->next->base;
                break;
            }
            else {
                partition = partition->next;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}


static int count_elements(char *str, char *delim)
{
    int count;

    HYDU_FUNC_ENTER();

    strtok(str, delim);
    count = 1;
    while (strtok(NULL, delim))
        count++;

    HYDU_FUNC_EXIT();

    return count;
}


static char *pad_string(char *str, char *pad, int count)
{
    char *tmp[HYD_NUM_TMP_STRINGS], *out;
    int i, j;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = HYDU_strdup(str);
    for (j = 0; j < count; j++)
        tmp[i++] = HYDU_strdup(pad);
    tmp[i] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &out);
    HYDU_ERR_POP(status, "unable to join strings\n");

    HYDU_free_strlist(tmp);

  fn_exit:
    HYDU_FUNC_EXIT();
    return out;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_merge_partition_mapping(char *name, char *map, int num_procs,
                                        struct HYD_Partition **partition_list)
{
    struct HYD_Partition *partition;
    char *tmp[HYD_NUM_TMP_STRINGS], *x;
    int i, count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*partition_list == NULL) {
        HYDU_alloc_partition(partition_list);
        (*partition_list)->base->name = HYDU_strdup(name);

        x = HYDU_strdup(map);
        count = num_procs - count_elements(x, ",");
        HYDU_FREE(x);

        (*partition_list)->user_bind_map = pad_string(map, ",-1", count);
    }
    else {
        partition = *partition_list;
        while (partition) {
            if (strcmp(partition->base->name, name) == 0) {
                /* Found a partition with the same name; append */
                if (partition->user_bind_map == NULL) {
                    x = HYDU_strdup(map);
                    count = num_procs - count_elements(x, ",");
                    HYDU_FREE(x);

                    partition->user_bind_map = pad_string(map, ",-1", count);
                }
                else {
                    x = HYDU_strdup(map);
                    count = num_procs - count_elements(x, ",");
                    HYDU_FREE(x);

                    i = 0;
                    tmp[i++] = HYDU_strdup(partition->user_bind_map);
                    tmp[i++] = HYDU_strdup(",");
                    tmp[i++] = pad_string(map, ",-1", count);
                    tmp[i++] = NULL;

                    HYDU_FREE(partition->user_bind_map);
                    status = HYDU_str_alloc_and_join(tmp, &partition->user_bind_map);
                    HYDU_ERR_POP(status, "unable to join strings\n");

                    HYDU_free_strlist(tmp);
                }
                break;
            }
            else if (partition->next == NULL) {
                HYDU_alloc_partition(&partition->next);
                partition->base->next = partition->next->base;
                partition->next->base->name = HYDU_strdup(name);
                partition->next->user_bind_map = HYDU_strdup(map);
                break;
            }
            else {
                partition = partition->next;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_alloc_partition_exec(struct HYD_Partition_exec **exec)
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
    char line[HYD_TMP_STRLEN], *hostname, *procs, **arg_list;
    char *str[2] = { NULL };
    int num_procs, total_count, arg, i;
    struct HYD_Partition_segment *segment;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(host_file, "HYDRA_USE_LOCALHOST")) {
        HYDU_alloc_partition(&(*partition_list));
        (*partition_list)->base->name = HYDU_strdup("localhost");
        (*partition_list)->one_pass_count = 1;

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
        while (fgets(line, HYD_TMP_STRLEN, fp)) {
            char *linep = NULL;

            linep = line;

            strtok(linep, "#");

            while (isspace(*linep))
                linep++;

            /* Ignore blank lines & comments */
            if ((*linep == '#') || (*linep == '\0'))
                continue;

            arg_list = HYDU_str_to_strlist(linep);
            if (!arg_list)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "Unable to convert host file entry to strlist\n");

            hostname = strtok(arg_list[0], ":");
            procs = strtok(NULL, ":");
            num_procs = procs ? atoi(procs) : 1;

            /* Try to find an existing partition with this name and
             * add this segment in. If there is no existing partition
             * with this name, we create a new one. */
            status = HYDU_alloc_partition_segment(&segment);
            HYDU_ERR_POP(status, "Unable to allocate partition segment\n");
            segment->start_pid = total_count;
            segment->proc_count = num_procs;
            status = HYDU_merge_partition_segment(hostname, segment, partition_list);
            HYDU_ERR_POP(status, "merge partition segment failed\n");

            total_count += num_procs;

            /* Check for the remaining parameters */
            arg = 1;
            str[0] = str[1] = NULL;
            while (arg_list[arg]) {
                status = HYDU_strsplit(arg_list[arg], &str[0], &str[1], '=');
                HYDU_ERR_POP(status, "unable to split string\n");

                if (!strcmp(str[0], "map")) {
                    status = HYDU_merge_partition_mapping(hostname, str[1], num_procs,
                                                          partition_list);
                    HYDU_ERR_POP(status, "merge partition mapping failed\n");
                }
                if (str[0])
                    HYDU_FREE(str[0]);
                if (str[1])
                    HYDU_FREE(str[1]);
                str[0] = str[1] = NULL;
                arg++;
            }
            HYDU_free_strlist(arg_list);
            HYDU_FREE(arg_list);
        }

        fclose(fp);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    for (i = 0; i < 2; i++)
        if (str[i])
            HYDU_FREE(str[i]);
    goto fn_exit;
}
