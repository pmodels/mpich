/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "csel_json.h"

/* See csel_json.h for the accepted format. */

struct json_parser {
    const char *s;
    const char *errmsg;
};

/* parse the top-level object; on failure, report the error position and message */
static bool parse_root(const char *json_str, MPII_Csel_json_s * root,
                       int *line_out, int *col_out, const char **errmsg_out);
/* parse a member's value, either a string or an object, into node */
static bool parse_value(struct json_parser *p, MPII_Csel_json_s * node, int depth);
/* parse the object at p->s (at '{') as the members of node */
static bool parse_object(struct json_parser *p, MPII_Csel_json_s * node, int depth);
/* parse the string at p->s (at '"') into a new buffer, with spaces removed */
static bool parse_string(struct json_parser *p, char **str_out);

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
        MPII_Csel_json_free(node->child);
        MPL_free(node->key);
        MPL_free(node->str);
        MPL_free(node);
        node = next;
    }
}

/* A recursive-descent parser with p->s as the cursor. Each object member
 * becomes a node appended to the parent's child list. The node is linked in
 * before it is parsed, so a partial tree is freed with the root on failure.
 * Each routine returns true on success, or false after setting p->errmsg and
 * leaving p->s at the offending character, which parse_root converts into a
 * line and column for the error report. */

static void skip_spaces(struct json_parser *p);
static void get_line_col(const char *str, const char *pos, int *line_out, int *col_out);

static bool parse_root(const char *json_str, MPII_Csel_json_s * root,
                       int *line_out, int *col_out, const char **errmsg_out)
{
    struct json_parser parser = { json_str, NULL };
    struct json_parser *p = &parser;

    skip_spaces(p);
    if (*p->s != '{') {
        p->errmsg = "expect '{' at the top level";
        goto fn_fail;
    }
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
    get_line_col(json_str, p->s, line_out, col_out);
    *errmsg_out = p->errmsg;
    return false;
}

static bool parse_value(struct json_parser *p, MPII_Csel_json_s * node, int depth)
{
    skip_spaces(p);
    if (*p->s == '"') {
        return parse_string(p, &node->str);
    } else if (*p->s == '{') {
        return parse_object(p, node, depth + 1);
    } else {
        p->errmsg = "expect an object or a string";
        return false;
    }
}

static bool parse_object(struct json_parser *p, MPII_Csel_json_s * node, int depth)
{
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

    MPII_Csel_json_s **tail = &node->child;
    while (1) {
        skip_spaces(p);
        if (*p->s != '"') {
            p->errmsg = "expect a key string";
            return false;
        }

        /* link the member first so that it is freed if parsing fails */
        MPII_Csel_json_s *member = MPL_calloc(1, sizeof(MPII_Csel_json_s), MPL_MEM_COLL);
        *tail = member;
        tail = &member->next;

        const char *key_pos = p->s;
        if (!parse_string(p, &member->key)) {
            return false;
        }
        for (MPII_Csel_json_s * q = node->child; q != member; q = q->next) {
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

static void skip_spaces(struct json_parser *p)
{
    while (*p->s == ' ' || *p->s == '\t' || *p->s == '\n' || *p->s == '\r') {
        p->s++;
    }
}

static void get_line_col(const char *str, const char *pos, int *line_out, int *col_out)
{
    int line = 1, col = 1;
    for (const char *t = str; t < pos; t++) {
        if (*t == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
    }
    *line_out = line;
    *col_out = col;
}
