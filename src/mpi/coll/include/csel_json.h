/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CSEL_JSON_H_INCLUDED
#define CSEL_JSON_H_INCLUDED

/* A minimal parser for the collective selection JSON. It accepts only the
 * subset of JSON that the selection files use:
 *   - The top-level value is an object.
 *   - A value is either an object or a string. Numbers, arrays, true, false,
 *     and null are rejected.
 *   - Strings contain printable ASCII characters only, with no escape
 *     sequences. Spaces inside a string are removed, e.g. "comm_size < 8"
 *     reads as "comm_size<8", the same as the whitespace-stripped built-in
 *     buffer. An empty string is rejected.
 *   - Keys within an object are unique, and the members keep the file order.
 *   - Objects nest at most MPII_CSEL_JSON_MAX_DEPTH levels deep.
 *   - Whitespace (space, tab, CR, LF) may appear between any tokens.
 * Anything else is reported as an error with its line and column.
 */

#define MPII_CSEL_JSON_MAX_DEPTH 64

/* An object member. The top-level object is a node with key == NULL. */
typedef struct MPII_Csel_json {
    char *key;
    char *str;                  /* the string value, or NULL if the value is an object */
    struct MPII_Csel_json *child;       /* first member of the object value */
    struct MPII_Csel_json *next;        /* next member of the enclosing object */
} MPII_Csel_json_s;

#define MPII_CSEL_JSON_FOREACH(obj, p) \
    for (const MPII_Csel_json_s *p = (obj)->child; p; p = p->next)

int MPII_Csel_json_parse(const char *json_str, MPII_Csel_json_s ** root_out);
void MPII_Csel_json_free(MPII_Csel_json_s * root);

#endif /* CSEL_JSON_H_INCLUDED */
