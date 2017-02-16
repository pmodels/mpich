/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_arg.h"
#include "hydra_err.h"

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
            if (**argv_p &&
                (!strcmp(**argv_p, "-h") || !strcmp(**argv_p, "-help") ||
                 !strcmp(**argv_p, "--help"))) {
                if (m->help_fn == NULL) {
                    HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "No help message available\n");
                }
                else {
                    m->help_fn();
                    exit(0);
                }
            }

            status = m->handler_fn(arg, argv_p);
            HYD_ERR_POP(status, "match handler returned error\n");
            break;
        }
        m++;
    }

    if (m->handler_fn == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unrecognized argument %s\n", arg);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_arg_parse_array(char ***argv, struct HYD_arg_match_table *match_table)
{
    HYD_status status = HYD_SUCCESS;

    while (**argv && ***argv == '-') {
        status = match_arg(argv, match_table);
        HYD_ERR_POP(status, "argument matching returned error\n");
    }

    HYD_FUNC_ENTER();

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_arg_set_str(char *arg, char **var, const char *val)
{
    HYD_status status = HYD_SUCCESS;

    HYD_ERR_CHKANDJUMP(status, *var, HYD_ERR_INTERNAL, "duplicate setting: %s\n", arg);

    if (val == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "cannot assign NULL object\n");

    *var = MPL_strdup(val);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_arg_set_int(char *arg, int *var, int val)
{
    HYD_status status = HYD_SUCCESS;

    HYD_ERR_CHKANDJUMP(status, *var != -1, HYD_ERR_INTERNAL, "duplicate setting: %s\n", arg);

    *var = val;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
