/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_alloc_proxy(struct HYD_Proxy **proxy)
{
    static int proxy_id = 0;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*proxy, struct HYD_Proxy *, sizeof(struct HYD_Proxy), status);

    (*proxy)->hostname = NULL;
    (*proxy)->pid = -1;
    (*proxy)->in = -1;
    (*proxy)->out = -1;
    (*proxy)->err = -1;

    (*proxy)->proxy_id = proxy_id++;
    (*proxy)->active = 0;
    (*proxy)->exec_args = NULL;

    (*proxy)->user_bind_map = NULL;
    (*proxy)->segment_list = NULL;
    (*proxy)->proxy_core_count = 0;

    (*proxy)->exit_status = NULL;
    (*proxy)->control_fd = -1;

    (*proxy)->exec_list = NULL;
    (*proxy)->next = NULL;

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
    (*exec_info)->process_count = 0;
    (*exec_info)->exec[0] = NULL;
    (*exec_info)->user_env = NULL;
    (*exec_info)->env_prop = NULL;
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

        if (exec_info->env_prop)
            HYDU_FREE(exec_info->env_prop);

        HYDU_env_free_list(exec_info->user_env);
        exec_info->user_env = NULL;

        HYDU_FREE(exec_info);
        exec_info = run;
    }

    HYDU_FUNC_EXIT();
}


void HYDU_free_proxy_list(struct HYD_Proxy *proxy_list)
{
    struct HYD_Proxy *proxy, *tproxy;
    struct HYD_Proxy_segment *segment, *tsegment;
    struct HYD_Proxy_exec *exec, *texec;

    HYDU_FUNC_ENTER();

    proxy = proxy_list;
    while (proxy) {
        tproxy = proxy->next;

        if (proxy->hostname)
            HYDU_FREE(proxy->hostname);
        if (proxy->exec_args) {
            HYDU_free_strlist(proxy->exec_args);
            HYDU_FREE(proxy->exec_args);
        }

        if (proxy->user_bind_map)
            HYDU_FREE(proxy->user_bind_map);

        segment = proxy->segment_list;
        while (segment) {
            tsegment = segment->next;
            if (segment->mapping) {
                HYDU_free_strlist(segment->mapping);
                HYDU_FREE(segment->mapping);
            }
            HYDU_FREE(segment);
            segment = tsegment;
        }

        if (proxy->exit_status)
            HYDU_FREE(proxy->exit_status);

        exec = proxy->exec_list;
        while (exec) {
            texec = exec->next;
            HYDU_free_strlist(exec->exec);
            if (exec->user_env)
                HYDU_env_free(exec->user_env);
            if (exec->env_prop)
                HYDU_FREE(exec->env_prop);
            HYDU_FREE(exec);
            exec = texec;
        }

        HYDU_FREE(proxy);
        proxy = tproxy;
    }

    HYDU_FUNC_EXIT();
}


HYD_Status HYDU_alloc_proxy_segment(struct HYD_Proxy_segment **segment)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*segment, struct HYD_Proxy_segment *,
                sizeof(struct HYD_Proxy_segment), status);
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


HYD_Status HYDU_merge_proxy_segment(char *hostname, struct HYD_Proxy_segment *segment,
                                        struct HYD_Proxy **proxy_list)
{
    struct HYD_Proxy *proxy;
    struct HYD_Proxy_segment *s;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*proxy_list == NULL) {
        status = HYDU_alloc_proxy(proxy_list);
        HYDU_ERR_POP(status, "Unable to alloc proxy\n");
        (*proxy_list)->segment_list = segment;
        (*proxy_list)->hostname = HYDU_strdup(hostname);
        (*proxy_list)->proxy_core_count += segment->proc_count;
    }
    else {
        proxy = *proxy_list;
        while (proxy) {
            if (strcmp(proxy->hostname, hostname) == 0) {
                if (proxy->segment_list == NULL)
                    proxy->segment_list = segment;
                else {
                    s = proxy->segment_list;
                    while (s->next)
                        s = s->next;
                    s->next = segment;
                }
                proxy->proxy_core_count += segment->proc_count;
                break;
            }
            else if (proxy->next == NULL) {
                status = HYDU_alloc_proxy(&proxy->next);
                HYDU_ERR_POP(status, "Unable to alloc proxy\n");
                proxy->next->segment_list = segment;
                proxy->next->hostname = HYDU_strdup(hostname);
                proxy->next->proxy_core_count += segment->proc_count;
                break;
            }
            else {
                proxy = proxy->next;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}


static int count_elements(char *str, const char *delim)
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


static char *pad_string(char *str, const char *pad, int count)
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


HYD_Status HYDU_merge_proxy_mapping(char *hostname, char *map, int num_procs,
                                        struct HYD_Proxy **proxy_list)
{
    struct HYD_Proxy *proxy;
    char *tmp[HYD_NUM_TMP_STRINGS], *x;
    int i, count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*proxy_list == NULL) {
        HYDU_alloc_proxy(proxy_list);
        (*proxy_list)->hostname = HYDU_strdup(hostname);

        x = HYDU_strdup(map);
        count = num_procs - count_elements(x, ",");
        HYDU_FREE(x);

        (*proxy_list)->user_bind_map = pad_string(map, ",-1", count);
    }
    else {
        proxy = *proxy_list;
        while (proxy) {
            if (strcmp(proxy->hostname, hostname) == 0) {
                /* Found a proxy with the same hostname; append */
                if (proxy->user_bind_map == NULL) {
                    x = HYDU_strdup(map);
                    count = num_procs - count_elements(x, ",");
                    HYDU_FREE(x);

                    proxy->user_bind_map = pad_string(map, ",-1", count);
                }
                else {
                    x = HYDU_strdup(map);
                    count = num_procs - count_elements(x, ",");
                    HYDU_FREE(x);

                    i = 0;
                    tmp[i++] = HYDU_strdup(proxy->user_bind_map);
                    tmp[i++] = HYDU_strdup(",");
                    tmp[i++] = pad_string(map, ",-1", count);
                    tmp[i++] = NULL;

                    HYDU_FREE(proxy->user_bind_map);
                    status = HYDU_str_alloc_and_join(tmp, &proxy->user_bind_map);
                    HYDU_ERR_POP(status, "unable to join strings\n");

                    HYDU_free_strlist(tmp);
                }
                break;
            }
            else if (proxy->next == NULL) {
                HYDU_alloc_proxy(&proxy->next);
                proxy->next->hostname = HYDU_strdup(hostname);
                proxy->next->user_bind_map = HYDU_strdup(map);
                break;
            }
            else {
                proxy = proxy->next;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_alloc_proxy_exec(struct HYD_Proxy_exec **exec)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*exec, struct HYD_Proxy_exec *, sizeof(struct HYD_Proxy_exec), status);
    (*exec)->exec[0] = NULL;
    (*exec)->proc_count = 0;
    (*exec)->env_prop = NULL;
    (*exec)->user_env = NULL;
    (*exec)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_create_node_list_from_file(char *host_file,
                                           struct HYD_Proxy **proxy_list)
{
    FILE *fp = NULL;
    char line[HYD_TMP_STRLEN], *hostname, *procs, **arg_list;
    char *str[2] = { NULL };
    int num_procs, total_count, arg, i;
    struct HYD_Proxy_segment *segment;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(host_file, "HYDRA_USE_LOCALHOST")) {
        HYDU_alloc_proxy(&(*proxy_list));
        (*proxy_list)->hostname = HYDU_strdup("localhost");
        (*proxy_list)->proxy_core_count = 1;

        HYDU_alloc_proxy_segment(&((*proxy_list)->segment_list));
        (*proxy_list)->segment_list->start_pid = 0;
        (*proxy_list)->segment_list->proc_count = 1;
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

            /* Try to find an existing proxy with this name and
             * add this segment in. If there is no existing proxy
             * with this name, we create a new one. */
            status = HYDU_alloc_proxy_segment(&segment);
            HYDU_ERR_POP(status, "Unable to allocate proxy segment\n");
            segment->start_pid = total_count;
            segment->proc_count = num_procs;
            status = HYDU_merge_proxy_segment(hostname, segment, proxy_list);
            HYDU_ERR_POP(status, "merge proxy segment failed\n");

            total_count += num_procs;

            /* Check for the remaining parameters */
            arg = 1;
            str[0] = str[1] = NULL;
            while (arg_list[arg]) {
                status = HYDU_strsplit(arg_list[arg], &str[0], &str[1], '=');
                HYDU_ERR_POP(status, "unable to split string\n");

                if (!strcmp(str[0], "map")) {
                    status = HYDU_merge_proxy_mapping(hostname, str[1], num_procs,
                                                          proxy_list);
                    HYDU_ERR_POP(status, "merge proxy mapping failed\n");
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
