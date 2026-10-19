/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "csel_json.h"
#include <limits.h>

/* See csel_json.h for the accepted format. */

struct json_parser {
    const char *s;
    int line;                   /* the line of p->s */
    const char *line_start;     /* the first character of that line */
    const char *errmsg;
};

/* parse the top-level object; on failure, report the error position and message */
static bool parse_root(const char *json_str, MPII_Csel_json_s * root,
                       int *line_out, int *col_out, const char **errmsg_out);
/* parse a member's value into node */
static bool parse_value(struct json_parser *p, MPII_Csel_json_s * node, int depth);
/* parse the object at p->s (at '{') as the members of node */
static bool parse_object(struct json_parser *p, MPII_Csel_json_s * node, int depth);
/* parse the string at p->s (at '"') into a new buffer, with spaces removed */
static bool parse_string(struct json_parser *p, char **str_out);
/* parse the integer at p->s (at '-' or a digit) */
static bool parse_int(struct json_parser *p, int *val_out);

int MPII_Csel_json_parse(const char *json_str, MPII_Csel_json_s ** root_out)
{
    int mpi_errno = MPI_SUCCESS;

    MPII_Csel_json_s *root = MPL_calloc(1, sizeof(MPII_Csel_json_s), MPL_MEM_COLL);

    int line, col;
    const char *errmsg;
    if (!parse_root(json_str, root, &line, &col, &errmsg)) {
        MPII_Csel_json_free(root);
        MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER, "**colljson", "**colljson %d %d %s",
                             line, col, errmsg);
    }

    *root_out = root;

  fn_exit:
    return mpi_errno;
  fn_fail:
    *root_out = NULL;
    goto fn_exit;
}

void MPII_Csel_json_free(MPII_Csel_json_s * root)
{
    MPII_Csel_json_s *node = root;
    while (node) {
        MPII_Csel_json_s *next = node->next;
        if (node->type == MPII_CSEL_JSON_OBJECT) {
            MPII_Csel_json_free(node->u.child);
        } else if (node->type == MPII_CSEL_JSON_STRING) {
            MPL_free(node->u.str);
        }
        MPL_free(node->key);
        MPL_free(node);
        node = next;
    }
}

int MPII_Csel_json_error(const MPII_Csel_json_s * node, const char *errmsg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER, "**colljson", "**colljson %d %d %s",
                         node->line, node->col, errmsg);
  fn_fail:
    return mpi_errno;
}

int MPII_Csel_json_get_int(const MPII_Csel_json_s * node, int *val_out)
{
    if (node->type != MPII_CSEL_JSON_INT) {
        return MPII_Csel_json_error(node, "expect an integer");
    }
    *val_out = node->u.ival;
    return MPI_SUCCESS;
}

int MPII_Csel_json_get_bool(const MPII_Csel_json_s * node, bool * val_out)
{
    if (node->type != MPII_CSEL_JSON_BOOL) {
        return MPII_Csel_json_error(node, "expect true or false");
    }
    *val_out = node->u.bval;
    return MPI_SUCCESS;
}

int MPII_Csel_json_get_enum(const MPII_Csel_json_s * node, const char *const *names, int n,
                            int *idx_out)
{
    if (node->type != MPII_CSEL_JSON_STRING) {
        return MPII_Csel_json_error(node, "expect a string");
    }
    for (int i = 0; i < n; i++) {
        if (strcmp(node->u.str, names[i]) == 0) {
            *idx_out = i;
            return MPI_SUCCESS;
        }
    }
    return MPII_Csel_json_error(node, "unknown name");
}

/* A recursive-descent parser with p->s as the cursor. Each object member
 * becomes a node appended to the parent's child list. The node is linked in
 * before it is parsed, so a partial tree is freed with the root on failure.
 * Each routine returns true on success, or false after setting p->errmsg and
 * leaving p->s at the offending character, which parse_root reports with its
 * line and column. Line breaks only appear in whitespace, so skip_spaces keeps
 * track of the line. */

static void skip_spaces(struct json_parser *p);

static bool parse_root(const char *json_str, MPII_Csel_json_s * root,
                       int *line_out, int *col_out, const char **errmsg_out)
{
    struct json_parser parser = { json_str, 1, json_str, NULL };
    struct json_parser *p = &parser;

    skip_spaces(p);
    if (*p->s != '{') {
        p->errmsg = "expect '{' at the top level";
        goto fn_fail;
    }
    root->type = MPII_CSEL_JSON_OBJECT;
    root->line = p->line;
    root->col = p->s - p->line_start + 1;
    if (!parse_object(p, root, 1)) {
        goto fn_fail;
    }
    skip_spaces(p);
    if (*p->s != '\0') {
        p->errmsg = "unexpected content after the top-level object";
        goto fn_fail;
    }
    return true;

  fn_fail:
    *line_out = p->line;
    *col_out = p->s - p->line_start + 1;
    *errmsg_out = p->errmsg;
    return false;
}

static bool parse_value(struct json_parser *p, MPII_Csel_json_s * node, int depth)
{
    skip_spaces(p);
    if (*p->s == '{') {
        node->type = MPII_CSEL_JSON_OBJECT;
        return parse_object(p, node, depth + 1);
    } else if (*p->s == '"') {
        node->type = MPII_CSEL_JSON_STRING;
        return parse_string(p, &node->u.str);
    } else if (*p->s == '-' || (*p->s >= '0' && *p->s <= '9')) {
        node->type = MPII_CSEL_JSON_INT;
        return parse_int(p, &node->u.ival);
    } else if (strncmp(p->s, "true", 4) == 0) {
        node->type = MPII_CSEL_JSON_BOOL;
        node->u.bval = true;
        p->s += 4;
        return true;
    } else if (strncmp(p->s, "false", 5) == 0) {
        node->type = MPII_CSEL_JSON_BOOL;
        node->u.bval = false;
        p->s += 5;
        return true;
    } else {
        p->errmsg = "expect an object, a string, an integer, true, or false";
        return false;
    }
}

static bool parse_object(struct json_parser *p, MPII_Csel_json_s * node, int depth)
{
    MPIR_Assert(node->type == MPII_CSEL_JSON_OBJECT);
    if (depth > MPII_CSEL_JSON_MAX_DEPTH) {
        p->errmsg = "objects are nested too deep";
        return false;
    }

    p->s++;
    skip_spaces(p);
    if (*p->s == '}') {
        p->s++;
        return true;
    }

    MPII_Csel_json_s **tail = &node->u.child;
    while (1) {
        skip_spaces(p);
        if (*p->s != '"') {
            p->errmsg = "expect a key string";
            return false;
        }

        /* link the member first so that it is freed if parsing fails */
        MPII_Csel_json_s *member = MPL_calloc(1, sizeof(MPII_Csel_json_s), MPL_MEM_COLL);
        member->type = MPII_CSEL_JSON_OBJECT;   /* empty until parse_value sets it */
        *tail = member;
        tail = &member->next;

        const char *key_pos = p->s;
        member->line = p->line;
        member->col = p->s - p->line_start + 1;
        if (!parse_string(p, &member->key)) {
            return false;
        }
        for (MPII_Csel_json_s * q = node->u.child; q != member; q = q->next) {
            if (strcmp(q->key, member->key) == 0) {
                p->s = key_pos;
                p->errmsg = "duplicate key";
                return false;
            }
        }

        skip_spaces(p);
        if (*p->s != ':') {
            p->errmsg = "expect ':'";
            return false;
        }
        p->s++;

        if (!parse_value(p, member, depth)) {
            return false;
        }

        skip_spaces(p);
        if (*p->s == ',') {
            p->s++;
        } else if (*p->s == '}') {
            p->s++;
            return true;
        } else {
            p->errmsg = "expect ',' or '}'";
            return false;
        }
    }
}

static bool parse_string(struct json_parser *p, char **str_out)
{
    const char *start = p->s + 1;
    const char *end = start;
    while (*end != '"') {
        if (*end == '\0') {
            p->s = end;
            p->errmsg = "unterminated string";
            return false;
        } else if (*end == '\\') {
            p->s = end;
            p->errmsg = "escape sequences are not supported";
            return false;
        } else if (*end < 0x20 || *end > 0x7e) {
            p->s = end;
            p->errmsg = "only printable ASCII characters are allowed in a string";
            return false;
        }
        end++;
    }

    char *str = MPL_malloc(end - start + 1, MPL_MEM_COLL);
    int n = 0;
    for (const char *t = start; t < end; t++) {
        if (*t != ' ') {
            str[n++] = *t;
        }
    }
    str[n] = '\0';

    if (n == 0) {
        MPL_free(str);
        p->errmsg = "empty string";
        return false;
    }

    *str_out = str;
    p->s = end + 1;
    return true;
}

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

static bool parse_int(struct json_parser *p, int *val_out)
{
    const char *t = p->s;
    bool negative = false;
    if (*t == '-') {
        negative = true;
        t++;
    }
    if (!IS_DIGIT(*t)) {
        p->s = t;
        p->errmsg = "expect a digit";
        return false;
    }
    if (*t == '0' && IS_DIGIT(t[1])) {
        p->s = t;
        p->errmsg = "leading zeros are not allowed";
        return false;
    }

    /* accumulate the magnitude, which is at most INT_MAX + 1 for INT_MIN */
    long long val = 0;
    while (IS_DIGIT(*t)) {
        val = val * 10 + (*t - '0');
        if (val > (long long) INT_MAX + 1) {
            p->errmsg = "integer out of range";
            return false;
        }
        t++;
    }
    if (*t == '.' || *t == 'e' || *t == 'E') {
        p->s = t;
        p->errmsg = "only integers are supported";
        return false;
    }
    if (negative) {
        val = -val;
    } else if (val > INT_MAX) {
        p->errmsg = "integer out of range";
        return false;
    }

    *val_out = (int) val;
    p->s = t;
    return true;
}

static void skip_spaces(struct json_parser *p)
{
    while (*p->s == ' ' || *p->s == '\t' || *p->s == '\n' || *p->s == '\r') {
        if (*p->s == '\n') {
            p->line++;
            p->line_start = p->s + 1;
        }
        p->s++;
    }
}
