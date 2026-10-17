/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Unit test stand-in for src/mpl/include/mpl.h. The real header includes the
 * configure-generated mplconfig.h; this one defines only what the units use.
 * Add to it as needed, keeping the real signatures. */

#ifndef MPL_H_INCLUDED
#define MPL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

/* MPL_SUCCESS, MPL_ERR_* */
#include "../../../../src/mpl/include/mpl_err.h"

/* memory allocation, ignoring the memory class */
#define MPL_malloc(size, class) malloc(size)
#define MPL_calloc(n, size, class) calloc(n, size)
#define MPL_realloc(p, size, class) realloc(p, size)
#define MPL_strdup(s) strdup(s)
#define MPL_free(p) free(p)

/* Types that headers under test only mention. The layout is a placeholder,
 * NOT the real one, so units must not depend on their contents. */
typedef struct {
    int placeholder;
} MPL_pointer_attr_t;

/* src/mpl/src/mpl_rankmap.c */
int MPL_rankmap_str_to_array(char *mapping, int sz, int *out_nodemap);
int MPL_rankmap_array_to_str(int *nodemap, int sz, char **out_mapping_str);

#endif /* MPL_H_INCLUDED */
