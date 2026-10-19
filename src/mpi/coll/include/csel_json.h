/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CSEL_JSON_H_INCLUDED
#define CSEL_JSON_H_INCLUDED

/* A minimal parser for the collective selection JSON. It accepts only the
 * subset of JSON that the selection files use:
 *   - The top-level value is an object.
 *   - A value is an object, a string, an integer, true, or false. Floats,
 *     arrays, and null are rejected.
 *   - Strings contain printable ASCII characters only, with no escape
 *     sequences. Spaces inside a string are removed, e.g. "comm_size < 8"
 *     reads as "comm_size<8", the same as the whitespace-stripped built-in
 *     buffer. An empty string is rejected.
 *   - Integers may be negative, have no leading zeros, and fit in an int.
 *   - Keys within an object are unique, and the members keep the file order.
 *   - Objects nest at most MPII_CSEL_JSON_MAX_DEPTH levels deep.
 *   - Whitespace (space, tab, CR, LF) may appear between any tokens.
 * Anything else is reported as an error with its line and column.
 *
 * The parsed tree lives only while a selection JSON is loaded.
 * MPIR_Csel_load_buf (csel.c), called from MPII_Coll_init for the built-in
 * JSON and for MPIR_CVAR_COLL_SELECTION_JSON_FILE, parses the text, converts
 * the tree into the MPIR_Csel_node_s selection trees, and frees it.
 * Collective selection at run time never sees it.
 */

#define MPII_CSEL_JSON_MAX_DEPTH 64

typedef enum {
    MPII_CSEL_JSON_OBJECT,
    MPII_CSEL_JSON_STRING,
    MPII_CSEL_JSON_INT,
    MPII_CSEL_JSON_BOOL,
} MPII_Csel_json_type_e;

/* An object member: a key and its value. The top-level object is a node with
 * key == NULL. The value is the member of u that matches type; check type
 * before reading it, or use the typed getters, which check it. */
typedef struct MPII_Csel_json {
    /* the member */
    char *key;
    int line, col;              /* the position of the key, for error reports */
    struct MPII_Csel_json *next;        /* next member of the enclosing object */
    /* its value */
    MPII_Csel_json_type_e type;
    union {
        struct MPII_Csel_json *child;   /* OBJECT: the first member */
        char *str;              /* STRING */
        int ival;               /* INT */
        bool bval;              /* BOOL */
    } u;
} MPII_Csel_json_s;

/* iterate over the members of obj, which must be an object */
#define MPII_CSEL_JSON_FOREACH(obj, p) \
    for (const MPII_Csel_json_s *p = (obj)->u.child; p; p = p->next)

int MPII_Csel_json_parse(const char *json_str, MPII_Csel_json_s ** root_out);
void MPII_Csel_json_free(MPII_Csel_json_s * root);

/* Return an error that reports errmsg at the position of node */
int MPII_Csel_json_error(const MPII_Csel_json_s * node, const char *errmsg);

/* Get the value of node, or return an error if it has the wrong type.
 * MPII_Csel_json_get_enum takes a string that is one of the n names and
 * sets *idx_out to its index. */
int MPII_Csel_json_get_int(const MPII_Csel_json_s * node, int *val_out);
int MPII_Csel_json_get_bool(const MPII_Csel_json_s * node, bool * val_out);
int MPII_Csel_json_get_enum(const MPII_Csel_json_s * node, const char *const *names, int n,
                            int *idx_out);

#endif /* CSEL_JSON_H_INCLUDED */
