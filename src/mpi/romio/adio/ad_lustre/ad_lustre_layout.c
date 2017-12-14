/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 2014, 2015, Intel Corporation
 */

#include "ad_lustre.h"
#include <unistd.h>
#include <getopt.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <string.h>

#ifdef HAVE_YAML_SUPPORT
#include "ad_lustre_cyaml.h"
#endif

#undef LAYOUT_DEBUG

#ifdef HAVE_LUSTRE_COMP_LAYOUT_SUPPORT
struct layout_setstripe_args {
    unsigned long long lsa_comp_end;
    unsigned long long lsa_stripe_size;
    int lsa_stripe_count;
    int lsa_stripe_off;
    __u32 lsa_comp_flags;
    int lsa_nr_osts;
    __u32 *lsa_osts;
    char *lsa_pool_name;
};

static int comp_args_to_layout(struct llapi_layout **composite, struct layout_setstripe_args *lsa)
{
    struct llapi_layout *layout = *composite;
    uint64_t prev_end = 0;
    int i = 0, rc;

    if (layout == NULL) {
        layout = llapi_layout_alloc();
        if (layout == NULL) {
            LDEBUG("Alloc llapi_layout failed. %s\n", strerror(errno));
            return -ENOMEM;
        }
        *composite = layout;
    } else {
        uint64_t start;

        /* Get current component extent, current component
         * must be the tail component. */
        rc = llapi_layout_comp_extent_get(layout, &start, &prev_end);
        if (rc) {
            LDEBUG("Get comp extent failed. %s\n", strerror(errno));
            return rc;
        }

        rc = llapi_layout_comp_add(layout);
        if (rc) {
            LDEBUG("Add component failed. %s\n", strerror(errno));
            return rc;
        }
    }

    rc = llapi_layout_comp_extent_set(layout, prev_end, lsa->lsa_comp_end);
    if (rc) {
        LDEBUG("Set extent [%lu, %llu) failed. %s\n", prev_end, lsa->lsa_comp_end, strerror(errno));
        return rc;
    }

    if (lsa->lsa_stripe_size != 0) {
        rc = llapi_layout_stripe_size_set(layout, lsa->lsa_stripe_size);
        if (rc) {
            LDEBUG("Set stripe size %llu failed. %s\n", lsa->lsa_stripe_size, strerror(errno));
            return rc;
        }
    }

    if (lsa->lsa_stripe_count != 0) {
        rc = llapi_layout_stripe_count_set(layout,
                                           lsa->lsa_stripe_count == -1 ?
                                           LLAPI_LAYOUT_WIDE : lsa->lsa_stripe_count);
        if (rc) {
            LDEBUG("Set stripe count %d failed. %s\n", lsa->lsa_stripe_count, strerror(errno));
            return rc;
        }
    }

    if (lsa->lsa_nr_osts > 0) {
        if (lsa->lsa_stripe_count > 0 && lsa->lsa_nr_osts != lsa->lsa_stripe_count) {
            LDEBUG("stripe_count(%d) != nr_osts(%d)\n", lsa->lsa_stripe_count, lsa->lsa_nr_osts);
            return -EINVAL;
        }
        for (i = 0; i < lsa->lsa_nr_osts; i++) {
            rc = llapi_layout_ost_index_set(layout, i, lsa->lsa_osts[i]);
            if (rc)
                break;
        }
    } else if (lsa->lsa_stripe_off != -1) {
        rc = llapi_layout_ost_index_set(layout, 0, lsa->lsa_stripe_off);
    }
    if (rc) {
        LDEBUG("Set ost index %d failed. %s\n", i, strerror(errno));
        return rc;
    }

    return 0;
}

/**
 * Parse a string containing an OST index list into an array of integers.
 *
 * The input string contains a comma delimited list of individual
 * indices and ranges, for example "1,2-4,7". Add the indices into the
 * \a osts array and remove duplicates.
 *
 * \param[out] osts    array to store indices in
 * \param[in] size     size of \a osts array
 * \param[in] offset   starting index in \a osts
 * \param[in] arg      string containing OST index list
 *
 * \retval positive    number of indices in \a osts
 * \retval -EINVAL     unable to parse \a arg
 */
static int parse_targets(__u32 * osts, int size, int offset, char *arg)
{
    int rc;
    int nr = offset;
    int slots = size - offset;
    char *ptr = NULL;
    bool end_of_loop;

    if (arg == NULL)
        return -EINVAL;

    end_of_loop = false;
    while (!end_of_loop) {
        int start_index;
        int end_index;
        int i;
        char *endptr = NULL;

        rc = -EINVAL;

        ptr = strchrnul(arg, ',');

        end_of_loop = *ptr == '\0';
        *ptr = '\0';

        start_index = strtol(arg, &endptr, 0);
        if (endptr == arg)      /* no data at all */
            break;
        if (*endptr != '-' && *endptr != '\0')  /* has invalid data */
            break;
        if (start_index < 0)
            break;

        end_index = start_index;
        if (*endptr == '-') {
            end_index = strtol(endptr + 1, &endptr, 0);
            if (*endptr != '\0')
                break;
            if (end_index < start_index)
                break;
        }

        for (i = start_index; i <= end_index && slots > 0; i++) {
            int j;

            /* remove duplicate */
            for (j = 0; j < offset; j++) {
                if (osts[j] == i)
                    break;
            }
            if (j == offset) {  /* no duplicate */
                osts[nr++] = i;
                --slots;
            }
        }
        if (slots == 0 && i < end_index)
            break;

        *ptr = ',';
        arg = ++ptr;
        offset = nr;
        rc = 0;
    }
    if (!end_of_loop && ptr != NULL)
        *ptr = ',';

    return rc < 0 ? rc : nr;
}

static inline bool arg_is_eof(char *arg)
{
    return !strncmp(arg, "-1", strlen("-1")) ||
        !strncmp(arg, "EOF", strlen("EOF")) || !strncmp(arg, "eof", strlen("eof"));
}

static inline void setstripe_args_init(struct layout_setstripe_args *lsa)
{
    memset(lsa, 0, sizeof(*lsa));
    lsa->lsa_stripe_off = -1;
}

int ADIOI_LUSTRE_Parse_comp_layout_opt(char *opt, struct llapi_layout **layout_ptr)
{
    int c;
    struct layout_setstripe_args lsa = { 0 };
    __u32 osts[LOV_MAX_STRIPE_COUNT] = { 0 };
    unsigned long long size_units = 1;
    int start = 0, end = 0, len = 0;
    int optargc = 1;
    char *optargv[MPI_MAX_INFO_VAL] = { "comp_layout" };
    int rc = 0;
    char *endptr;
    char *temp = opt;
    struct option long_opts[] = {
        {.val = 'c',.name = "stripe-count",.has_arg = required_argument},
        {.val = 'c',.name = "stripe_count",.has_arg = required_argument},
        {.val = 'E',.name = "comp-end",.has_arg = required_argument},
        {.val = 'E',.name = "component-end",
         .has_arg = required_argument},
        {.val = 'i',.name = "stripe-index",.has_arg = required_argument},
        {.val = 'i',.name = "stripe_index",.has_arg = required_argument},
        {.val = 'o',.name = "ost-list",.has_arg = required_argument},
        {.val = 'o',.name = "ost_list",.has_arg = required_argument},
        {.val = 'S',.name = "stripe-size",.has_arg = required_argument},
        {.val = 'S',.name = "stripe_size",.has_arg = required_argument},
        {.name = NULL}
    };

    /* parse the opt string to generate optargc and optargv */
    if (*opt != '-') {
        LDEBUG("'%s' is not a valid layout option string.\n", opt);
        rc = -EINVAL;
        goto error;
    }
    do {
        end++;
        if (*(temp + end) == ' ' || *(temp + end) == '\0') {
            len = end - start;
            optargv[optargc] = (char *) ADIOI_Malloc(len + 1);
            memset(optargv[optargc], 0, len + 1);
            ADIOI_Strncpy(optargv[optargc], temp + start, len);
#ifdef LAYOUT_DEBUG
            LDEBUG("argv[%d]=%s\n", optargc, optargv[optargc]);
#endif
            optargc++;
            start = end + 1;
        }
    } while (*(temp + end) != '\0');

    optind = 0;
    while ((c = getopt_long(optargc, optargv, "c:E:i:o:S:", long_opts, NULL)) >= 0) {
        switch (c) {
            case 'c':
                lsa.lsa_stripe_count = strtoul(optarg, &endptr, 0);
                if (*endptr != '\0') {
                    LDEBUG("error: bad stripe count '%s'\n", optarg);
                    goto error;
                }
                break;
            case 'E':
                if (lsa.lsa_comp_end != 0) {
                    rc = comp_args_to_layout(layout_ptr, &lsa);
                    if (rc)
                        goto error;
                    setstripe_args_init(&lsa);
                }

                if (arg_is_eof(optarg)) {
                    lsa.lsa_comp_end = LUSTRE_EOF;
                } else {
                    rc = llapi_parse_size(optarg, &lsa.lsa_comp_end, &size_units, 0);
                    if (rc) {
                        LDEBUG("error: bad component end '%s'\n", optarg);
                        goto error;
                    }
                }
                break;
            case 'i':
                if (strcmp(optargv[optind - 1], "--index") == 0)
                    LDEBUG("warning: '--index' deprecated, " "use '--stripe-index' instead\n");
                lsa.lsa_stripe_off = strtol(optarg, &endptr, 0);
                if (*endptr != '\0') {
                    LDEBUG("error: bad stripe offset '%s'\n", optarg);
                    goto error;
                }
                break;
            case 'o':
                lsa.lsa_nr_osts = parse_targets(osts,
                                                sizeof(osts) / sizeof(__u32),
                                                lsa.lsa_nr_osts, optarg);
                if (lsa.lsa_nr_osts < 0) {
                    LDEBUG("error: bad OST indices '%s'\n", optarg);
                    goto error;
                }

                lsa.lsa_osts = osts;
                if (lsa.lsa_stripe_off == -1)
                    lsa.lsa_stripe_off = osts[0];
                break;
            case 'S':
                rc = llapi_parse_size(optarg, &lsa.lsa_stripe_size, &size_units, 0);
                if (rc) {
                    LDEBUG("error: bad stripe size '%s'\n", optarg);
                    goto error;
                }
                break;
            default:
                LDEBUG("option '%s' unrecognized\n", optargv[optind - 1]);
                rc = -EINVAL;
                goto error;
        }
    }

    if (lsa.lsa_comp_end != 0) {
        rc = comp_args_to_layout(layout_ptr, &lsa);
        if (rc)
            goto error;
    }

    return 0;
  error:
    llapi_layout_free(*layout_ptr);
    return rc;
}

#ifdef HAVE_YAML_SUPPORT
static int build_layout_from_yaml_node(struct cYAML *node,
                                       struct llapi_layout **layout,
                                       struct layout_setstripe_args *lsa, __u32 * osts)
{
    char *string;

    while (node) {
        string = node->cy_string;
        /* skip leading lmm_ if present, to simplify parsing */
        if (string != NULL && strncmp(string, "lmm_", 4) == 0)
            string += 4;

        if (node->cy_type == CYAML_TYPE_STRING) {
            if (!strcmp(string, "lcme_extent.e_end")) {
                if (!strcmp(node->cy_valuestring, "EOF") || !strcmp(node->cy_valuestring, "eof"))
                    lsa->lsa_comp_end = LUSTRE_EOF;
            } else if (!strcmp(string, "pool")) {
                lsa->lsa_pool_name = node->cy_valuestring;
            }
        } else if (node->cy_type == CYAML_TYPE_NUMBER) {
            if (!strcmp(string, "lcme_extent.e_start")) {
                if (node->cy_valueint != 0)
                    /* build a component */
                    comp_args_to_layout(layout, lsa);

                /* initialize lsa */
                setstripe_args_init(lsa);
                lsa->lsa_osts = osts;
            } else if (!strcmp(string, "lcme_extent.e_end")) {
                if (node->cy_valueint == -1)
                    lsa->lsa_comp_end = LUSTRE_EOF;
                else
                    lsa->lsa_comp_end = node->cy_valueint;
            } else if (!strcmp(string, "stripe_count")) {
                lsa->lsa_stripe_count = node->cy_valueint;
            } else if (!strcmp(string, "stripe_size")) {
                lsa->lsa_stripe_size = node->cy_valueint;
            } else if (!strcmp(string, "stripe_offset")) {
                lsa->lsa_stripe_off = node->cy_valueint;
            } else if (!strcmp(string, "l_ost_idx")) {
                osts[lsa->lsa_nr_osts] = node->cy_valueint;
                lsa->lsa_nr_osts++;
            }
        } else if (node->cy_type == CYAML_TYPE_OBJECT) {
            /* go deep to sub blocks */
            build_layout_from_yaml_node(node->cy_child, layout, lsa, osts);
        }
        node = node->cy_next;
    }

    return 0;
}

static int lfs_comp_create_from_yaml(char *tempfile,
                                     struct llapi_layout **layout,
                                     struct layout_setstripe_args *lsa, __u32 * osts)
{
    struct cYAML *tree = NULL;
    int rc = 0;

    tree = cYAML_build_tree(tempfile);
    if (!tree) {
        LDEBUG("'%s' is not a valid YAML template file.\n", tempfile);
        rc = -EINVAL;
        goto err;
    }

    /* initialize lsa for plain file */
    setstripe_args_init(lsa);
    lsa->lsa_osts = osts;

    rc = build_layout_from_yaml_node(tree, layout, lsa, osts);
    if (rc) {
        LDEBUG("Cannot build layout from YAML file %s.\n", tempfile);
        goto err;
    } else {
        comp_args_to_layout(layout, lsa);
    }
    /* clean clean lsa */
    setstripe_args_init(lsa);

  err:
    if (tree)
        cYAML_free_tree(tree);
    return rc;
}

int ADIOI_LUSTRE_Parse_yaml_temp(char *tempfile, struct llapi_layout **layout_ptr)
{
    struct layout_setstripe_args lsa = { 0 };
    __u32 osts[LOV_MAX_STRIPE_COUNT] = { 0 };
    int rc;

    /* generate a layout from a YAML template file */
    return lfs_comp_create_from_yaml(tempfile, layout_ptr, &lsa, osts);
}
#endif /* HAVE_YAML_SUPPORT */
#endif /* HAVE_LUSTRE_COMP_LAYOUT_SUPPORT */
