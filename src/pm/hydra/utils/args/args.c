/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"

static int exists(char *filename)
{
    struct stat file_stat;

    if ((stat(filename, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode))) {
        return 0;       /* no such file, or not a regular file */
    }

    return 1;
}

static HYD_status get_abs_wd(const char *wd, char **abs_wd)
{
    int ret;
    char *cwd;
    HYD_status status = HYD_SUCCESS;

    if (wd == NULL) {
        *abs_wd = NULL;
        goto fn_exit;
    }

    if (wd[0] != '.') {
        *abs_wd = (char *) wd;
        goto fn_exit;
    }

    cwd = HYDU_getcwd();
    ret = chdir(wd);
    if (ret < 0)
        HYDU_ERR_POP(status, "error calling chdir\n");

    *abs_wd = HYDU_getcwd();
    ret = chdir(cwd);
    if (ret < 0)
        HYDU_ERR_POP(status, "error calling chdir\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_find_in_path(const char *execname, char **path)
{
    char *tmp[HYD_NUM_TMP_STRINGS], *path_loc = NULL, *test_loc, *user_path;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* The executable is somewhere in the user's path. Find it. */
    if (MPL_env2str("PATH", (const char **) &user_path))
        user_path = MPL_strdup(user_path);

    if (user_path) {    /* If the PATH environment exists */
        status = get_abs_wd(strtok(user_path, ";:"), &test_loc);
        HYDU_ERR_POP(status, "error getting absolute working dir\n");
        do {
            tmp[0] = MPL_strdup(test_loc);
            tmp[1] = MPL_strdup("/");
            tmp[2] = MPL_strdup(execname);
            tmp[3] = NULL;

            status = HYDU_str_alloc_and_join(tmp, &path_loc);
            HYDU_ERR_POP(status, "unable to join strings\n");
            HYDU_free_strlist(tmp);

            if (exists(path_loc)) {
                tmp[0] = MPL_strdup(test_loc);
                tmp[1] = MPL_strdup("/");
                tmp[2] = NULL;

                status = HYDU_str_alloc_and_join(tmp, path);
                HYDU_ERR_POP(status, "unable to join strings\n");
                HYDU_free_strlist(tmp);

                goto fn_exit;   /* We are done */
            }

            MPL_free(path_loc);
            path_loc = NULL;

            status = get_abs_wd(strtok(NULL, ";:"), &test_loc);
            HYDU_ERR_POP(status, "error getting absolute working dir\n");
        } while (test_loc);
    }

    /* There is either no PATH environment or we could not find the
     * file in the PATH. Just return an empty path */
    *path = MPL_strdup("");

  fn_exit:
    if (user_path)
        MPL_free(user_path);
    if (path_loc)
        MPL_free(path_loc);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status match_arg(char ***argv_p, struct HYD_arg_match_table *match_table)
{
    struct HYD_arg_match_table *m;
    char *arg, *val;
    HYD_status status = HYD_SUCCESS;

    arg = **argv_p;
    while (*arg == '-') /* Remove leading dashes */
        arg++;

    /* If arg is of the form foo=bar, we separate it out as two
     * arguments */
    for (val = arg; *val && *val != '='; val++);
    if (*val == '=') {
        /* Found an '='; use the rest of the argument as a separate
         * argument */
        **argv_p = val + 1;
    }
    else {
        /* Move to the next argument */
        (*argv_p)++;
    }
    *val = 0;   /* close out key */

    m = match_table;
    while (m->handler_fn) {
        if (!strcasecmp(arg, m->arg)) {
            if (**argv_p && (!strcmp(**argv_p, "-h") || !strcmp(**argv_p, "-help") ||
                             !strcmp(**argv_p, "--help"))) {
                if (m->help_fn == NULL) {
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "No help message available\n");
                }
                else {
                    m->help_fn();
                    HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "%s", "");
                }
            }

            status = m->handler_fn(arg, argv_p);
            HYDU_ERR_POP(status, "match handler returned error\n");
            break;
        }
        m++;
    }

    if (m->handler_fn == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized argument %s\n", arg);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_parse_array(char ***argv, struct HYD_arg_match_table *match_table)
{
    HYD_status status = HYD_SUCCESS;

    while (**argv && ***argv == '-') {
        status = match_arg(argv, match_table);
        HYDU_ERR_POP(status, "argument matching returned error\n");
    }

    HYDU_FUNC_ENTER();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_set_str(char *arg, char **var, const char *val)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, *var, HYD_INTERNAL_ERROR, "duplicate setting: %s\n", arg);

    if (val == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cannot assign NULL object\n");

    *var = MPL_strdup(val);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_set_int(char *arg, int *var, int val)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, *var != -1, HYD_INTERNAL_ERROR, "duplicate setting: %s\n", arg);

    *var = val;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

char *HYDU_getcwd(void)
{
    char *cwdval, *retval = NULL;
    const char *pwdval;
    HYD_status status = HYD_SUCCESS;
#if defined HAVE_STAT
    struct stat spwd, scwd;
#endif /* HAVE_STAT */

    if (MPL_env2str("PWD", &pwdval) == 0)
        pwdval = NULL;
    HYDU_MALLOC_OR_JUMP(cwdval, char *, HYDRA_MAX_PATH, status);
    if (getcwd(cwdval, HYDRA_MAX_PATH) == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "allocated space is too small for absolute path\n");

#if defined HAVE_STAT
    if (pwdval && stat(pwdval, &spwd) != -1 && stat(cwdval, &scwd) != -1 &&
        spwd.st_dev == scwd.st_dev && spwd.st_ino == scwd.st_ino) {
        /* PWD and getcwd() match; use the PWD value */
        retval = MPL_strdup(pwdval);
        MPL_free(cwdval);
    }
    else
#endif /* HAVE_STAT */
    {
        /* PWD and getcwd() don't match; use the getcwd value and hope
         * for the best. */
        retval = cwdval;
    }

  fn_exit:
    return retval;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_process_mfile_token(char *token, int newline, struct HYD_node **node_list)
{
    int num_procs;
    char *hostname, *procs, *binding, *tmp, *user, *saveptr;
    struct HYD_node *node;
    HYD_status status = HYD_SUCCESS;

    if (newline) {      /* The first entry gives the hostname and processes */
        hostname = strtok_r(token, ":", &saveptr);
        procs = strtok_r(NULL, ":", &saveptr);
        num_procs = procs ? atoi(procs) : 1;

        status = HYDU_add_to_node_list(hostname, num_procs, node_list);
        HYDU_ERR_POP(status, "unable to add to node list\n");
    }
    else {      /* Not a new line */
        tmp = strtok_r(token, "=", &saveptr);
        if (!strcmp(tmp, "binding")) {
            binding = strtok_r(NULL, "=", &saveptr);

            for (node = *node_list; node->next; node = node->next);
            if (node->local_binding)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "duplicate local binding setting\n");

            node->local_binding = MPL_strdup(binding);
        }
        else if (!strcmp(tmp, "user")) {
            user = strtok_r(NULL, "=", &saveptr);

            for (node = *node_list; node->next; node = node->next);
            if (node->username)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate username setting\n");

            node->username = MPL_strdup(user);
        }
        else {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "token %s not supported at this time\n", token);
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_parse_hostfile(const char *hostfile, struct HYD_node **node_list,
                               HYD_status(*process_token) (char *token, int newline,
                                                           struct HYD_node ** node_list))
{
    char line[HYD_TMP_STRLEN], **tokens;
    FILE *fp;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ((fp = fopen(hostfile, "r")) == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unable to open host file: %s\n", hostfile);

    if (node_list)
        *node_list = NULL;
    while (fgets(line, HYD_TMP_STRLEN, fp)) {
        char *linep = NULL;

        linep = line;

        strtok(linep, "#");
        while (isspace(*linep))
            linep++;

        /* Ignore blank lines & comments */
        if ((*linep == '#') || (*linep == '\0'))
            continue;

        tokens = HYDU_str_to_strlist(linep);
        if (!tokens)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "Unable to convert host file entry to strlist\n");

        for (i = 0; tokens[i]; i++) {
            status = process_token(tokens[i], !i, node_list);
            HYDU_ERR_POP(status, "unable to process token\n");
        }

        HYDU_free_strlist(tokens);
        MPL_free(tokens);
    }

    fclose(fp);


  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

char *HYDU_find_full_path(const char *execname)
{
    char *tmp[HYD_NUM_TMP_STRINGS] = { NULL }, *path = NULL, *test_path = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_find_in_path(execname, &test_path);
    HYDU_ERR_POP(status, "error while searching for executable in user path\n");

    if (test_path) {
        tmp[0] = MPL_strdup(test_path);
        tmp[1] = MPL_strdup(execname);
        tmp[2] = NULL;

        status = HYDU_str_alloc_and_join(tmp, &path);
        HYDU_ERR_POP(status, "error joining strings\n");
    }

  fn_exit:
    HYDU_free_strlist(tmp);
    if (test_path)
        MPL_free(test_path);
    HYDU_FUNC_EXIT();
    return path;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_send_strlist(int fd, char **strlist)
{
    int i, list_len, len;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Check how many arguments we have */
    list_len = HYDU_strlist_lastidx(strlist);
    status = HYDU_sock_write(fd, &list_len, sizeof(int), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to write data to proxy\n");
    HYDU_ASSERT(!closed, status);

    /* Convert the string list to parseable data and send */
    for (i = 0; strlist[i]; i++) {
        len = strlen(strlist[i]) + 1;

        status = HYDU_sock_write(fd, &len, sizeof(int), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to write data to proxy\n");
        HYDU_ASSERT(!closed, status);

        status = HYDU_sock_write(fd, strlist[i], len, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to write data to proxy\n");
        HYDU_ASSERT(!closed, status);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
