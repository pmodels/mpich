/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This file is to be included in run_mpitests.c to provide utility routines
 * for running multi-tests.
 */

extern struct mpitest alltests[];
int num_tests;

FILE *test_in;
FILE *test_out;

/* routines used in run_mpitests.c */
static void init_multi_tests(void);
static bool get_test(const char **name, const char **args, run_fn * fn);
static void load_cvars(void);
static void cleanup_cvars(void);

/* internal routines */
static run_fn find_test(const char *name);
static void split_cmd(char *cmd, const char **name_out, const char **args_out);

static void load_cvars(void);
static void cleanup_cvars(void);
static int get_cvar_idx(const char *name);
static void set_cvar(char *cmd);
static void set_cvar_value(int cvar_idx, const char *val);
static int get_cvar_enum_value(int cvar_idx, const char *str);

/* parsing utilities */
#define skip_space(s)        while (isspace(*s)) s++
#define skip_nonspace(s)     while (*s && !isspace(*s)) s++
#define skip_word(s)         while (isalnum(*s) || *s == '_') s++

#define skip_line(s) \
    do { \
        while (*s && *s != '\n') s++; \
        while (*s && *s == '\n') s++; \
    } while (0)

#define check_null_return(s) \
    do { \
        if (*s == '\0') return; \
    } while (0)


/* for qsort */
static int cmp_testname(const void *a, const void *b)
{
    return strcmp(((struct mpitest *) a)->name, ((struct mpitest *) b)->name);
}

static void init_multi_tests(void)
{
    int n = 0;
    while (alltests[n].name) {
        n++;
    }

    num_tests = n;

    qsort(alltests, num_tests, sizeof(struct mpitest), cmp_testname);

    int thread_level = MPI_THREAD_SINGLE;
    int provided_thread_level;
    MPI_T_init_thread(thread_level, &provided_thread_level);

    load_cvars();

    test_in = stdin;
    test_out = stdout;
}

static void finalize_multi_tests(void)
{
    cleanup_cvars();
    MPI_T_finalize();
}

static run_fn find_test(const char *name)
{
    if (num_tests == 0) {
        return NULL;
    } else if (strcmp(name, alltests[0].name) == 0) {
        return alltests[0].run;
    } else if (num_tests > 1 && strcmp(name, alltests[num_tests - 1].name) == 0) {
        return alltests[num_tests - 1].run;
    }

    /* binary search */
    int i1 = 0;
    int i2 = num_tests - 1;
    while (i1 <= i2) {
        int i3 = (i1 + i2) / 2;
        int ret = strcmp(name, alltests[i3].name);
        if (ret == 0) {
            return alltests[i3].run;
        } else if (ret < 0) {
            i2 = i3 - 1;
        } else {
            i1 = i3 + 1;
        }
    }

    return NULL;
}

static void split_cmd(char *cmd, const char **name_out, const char **args_out)
{
    char *s = cmd;

    *name_out = NULL;
    *args_out = NULL;

    skip_space(s);
    check_null_return(s);
    *name_out = s;

    skip_nonspace(s);
    check_null_return(s);
    *s++ = '\0';

    skip_space(s);
    check_null_return(s);
    *args_out = s;
}

static bool get_test(const char **name, const char **args, run_fn * fn)
{
    static char cmd[ARGS_MAX];
    while (1) {
        /* rank 0 reads the command */
        if (wrank == 0) {
            if (!fgets(cmd, ARGS_MAX, test_in)) {
                cmd[0] = '\0';
            }
        }

        /* bcast */
        MPI_Bcast(cmd, ARGS_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);

        /* all processes process the same command */
        if (cmd[0] == '\0') {
            /* all done */
            return false;
        } else if (strncmp(cmd, "MPIR_CVAR_", 10) == 0) {
            /* set or reset cvar */
            set_cvar(cmd);
            continue;
        } else if (strncmp(cmd, "TIMEOUT ", 8) == 0) {
            /* set timeout */
            int timeout = atoi(cmd + 8);
            if (timeout > 0) {
                alarm(timeout);
            }
            continue;
        } else {
            /* a testlist item */
            split_cmd(cmd, name, args);
            if (*name) {
                *fn = find_test(*name);
            } else {
                *fn = NULL;
            }
            return true;
        }
    }
}

/* CVARs */
#define CVAR_NAME_LEN 256
#define CVAR_VAL_LEN 400        /* ref: MPICH internally at 384 */
#define CVAR_MAX_ENUMS 20
struct cvar {
    char name[CVAR_NAME_LEN];
    MPI_Datatype datatype;
    int has_value;
    char value[CVAR_VAL_LEN];
    int num_enums;
    char *enum_list[CVAR_MAX_ENUMS];
};

static int num_cvars;
static struct cvar *cvar_list;

static void load_cvars(void)
{
    MPI_T_cvar_get_num(&num_cvars);
    cvar_list = malloc(num_cvars * sizeof(struct cvar));

    for (int i = 0; i < num_cvars; i++) {
        int namelen = CVAR_NAME_LEN;
        MPI_T_cvar_get_info(i, cvar_list[i].name, &namelen, NULL, &cvar_list[i].datatype,
                            NULL, NULL, NULL, NULL, NULL);
        cvar_list[i].has_value = 0;
        cvar_list[i].num_enums = 0;
    }
}

static void cleanup_cvars(void)
{
    for (int i = 0; i < num_cvars; i++) {
        if (cvar_list[i].num_enums > 0) {
            for (int j = 0; j < num_cvars; j++) {
                free(cvar_list[i].enum_list[j]);
            }
        }
    }
    free(cvar_list);
}

static int get_cvar_idx(const char *name)
{
    int idx = -1;
    for (int i = 0; i < num_cvars; i++) {
        if (strcmp(cvar_list[i].name, name) == 0) {
            idx = i;
            break;
        }
    }
    return idx;
}

/* cmd to set cvar:
 *     MPIR_CVAR_XXX=xxx  - set cvar with value
 *     MPIR_CVAR_XXX=     - reset cvar
 */
static void set_cvar(char *cmd)
{
    char *s = cmd;
    skip_word(s);
    if (*s == '=') {
        *s = '\0';
        s++;
        int cvar_idx = get_cvar_idx(cmd);
        if (cvar_idx != -1) {
            if (isalnum(*s)) {
                char *val = s;
                skip_word(s);
                *s = '\0';
                set_cvar_value(cvar_idx, val);
            } else {
                /* reset CVAR */
                set_cvar_value(cvar_idx, NULL);
            }
        }
    }
}

static void set_cvar_value(int cvar_idx, const char *val)
{
    int count;
    MPI_T_cvar_handle handle;

    struct cvar *p = &cvar_list[cvar_idx];

    /* assume MPI_T_BIND_NO_OBJECT and scalar */
    MPI_T_cvar_handle_alloc(cvar_idx, NULL, &handle, &count);
    assert(count == 1 || (p->datatype == MPI_CHAR && count < CVAR_VAL_LEN));

    if (!p->has_value) {
        MPI_T_cvar_read(handle, p->value);
        p->has_value = 1;
    }

    int intval;
    if (val == NULL) {
        /* restore initial value */
        val = p->value;
        if (p->datatype == MPI_INT) {
            intval = *(int *) val;
        }
    } else if (p->datatype == MPI_CHAR) {
        /* val is a C string */
    } else if (p->datatype == MPI_INT) {
        if ((val[0] == '-' && isdigit(val[1])) || isdigit(val[0])) {
            intval = atoi(val);
        } else {
            /* must be an enum */
            intval = get_cvar_enum_value(cvar_idx, val);
        }

        val = (void *) &intval;
    }
    MPI_T_cvar_write(handle, val);
    MPI_T_cvar_handle_free(&handle);
}

static int get_cvar_enum_value(int cvar_idx, const char *str)
{
    struct cvar *p = &cvar_list[cvar_idx];

    /* grab enum_list if not already */
    if (p->num_enums == 0) {
        char desc[1024];
        int desclen = 1024;
        MPI_T_cvar_get_info(cvar_idx, NULL, NULL, NULL, NULL, NULL, desc, &desclen, NULL, NULL);
        const char *s = desc;
        int i = 0;
        while (*s) {
            /* enum values has pattern: /^\w+\s+-/ */
            const char *val = s;
            skip_word(s);
            int val_len = s - val;

            if (*s == ' ') {
                skip_space(s);
                if (s[0] == '-' && s[1] == ' ') {
                    p->enum_list[i++] = strndup(val, val_len);
                    p->num_enums = i;
                }
            }
            skip_line(s);
        }
    }

    for (int i = 0; i < p->num_enums; i++) {
        if (strcmp(p->enum_list[i], str) == 0) {
            return i;
        }
    }
    return -1;
}
