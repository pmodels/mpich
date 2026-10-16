/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "csel_json.h"

/* See csel_json.h for the accepted format. The internal routines return 0 on
 * success, or -1 after recording the error message and leaving p->s at the
 * offending character. */

struct json_parser {
    const char *str;
    const char *s;
    const char *errmsg;
};

static int parse_object(struct json_parser *p, MPII_Csel_json_s * node, int depth);

static int parse_error(struct json_parser *p, const char *errmsg)
{
    p->errmsg = errmsg;
    return -1;
}

static void skip_spaces(struct json_parser *p)
{
    while (*p->s == ' ' || *p->s == '\t' || *p->s == '\n' || *p->s == '\r') {
        p->s++;
    }
}

/* p->s is at the opening quote */
static int parse_string(struct json_parser *p, char **str_out)
{
    const char *start = p->s + 1;
    const char *end = start;
    while (*end != '"') {
        if (*end == '\0') {
            p->s = end;
            return parse_error(p, "unterminated string");
        } else if (*end == '\\') {
            p->s = end;
            return parse_error(p, "escape sequences are not supported");
        } else if (*end < 0x20 || *end > 0x7e) {
            p->s = end;
            return parse_error(p, "only printable ASCII characters are allowed in a string");
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
        return parse_error(p, "empty string");
    }

    *str_out = str;
    p->s = end + 1;
    return 0;
}

static int parse_value(struct json_parser *p, MPII_Csel_json_s * node, int depth)
{
    skip_spaces(p);
    if (*p->s == '"') {
        return parse_string(p, &node->str);
    } else if (*p->s == '{') {
        return parse_object(p, node, depth + 1);
    } else {
        return parse_error(p, "expect an object or a string");
    }
}

/* p->s is at the opening brace */
static int parse_object(struct json_parser *p, MPII_Csel_json_s * node, int depth)
{
    int rc;

    if (depth > MPII_CSEL_JSON_MAX_DEPTH) {
        return parse_error(p, "objects are nested too deep");
    }

    p->s++;
    skip_spaces(p);
    if (*p->s == '}') {
        p->s++;
        return 0;
    }

    MPII_Csel_json_s **tail = &node->child;
    while (1) {
        skip_spaces(p);
        if (*p->s != '"') {
            return parse_error(p, "expect a key string");
        }

        /* link the member first so that it is freed if parsing fails */
        MPII_Csel_json_s *member = MPL_calloc(1, sizeof(MPII_Csel_json_s), MPL_MEM_COLL);
        *tail = member;
        tail = &member->next;

        const char *key_pos = p->s;
        rc = parse_string(p, &member->key);
        if (rc) {
            return rc;
        }
        for (MPII_Csel_json_s * q = node->child; q != member; q = q->next) {
            if (strcmp(q->key, member->key) == 0) {
                p->s = key_pos;
                return parse_error(p, "duplicate key");
            }
        }

        skip_spaces(p);
        if (*p->s != ':') {
            return parse_error(p, "expect ':'");
        }
        p->s++;

        rc = parse_value(p, member, depth);
        if (rc) {
            return rc;
        }

        skip_spaces(p);
        if (*p->s == ',') {
            p->s++;
        } else if (*p->s == '}') {
            p->s++;
            return 0;
        } else {
            return parse_error(p, "expect ',' or '}'");
        }
    }
}

int MPII_Csel_json_parse(const char *json_str, MPII_Csel_json_s ** root_out)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;

    struct json_parser parser = { json_str, json_str, NULL };
    struct json_parser *p = &parser;

    MPII_Csel_json_s *root = MPL_calloc(1, sizeof(MPII_Csel_json_s), MPL_MEM_COLL);

    skip_spaces(p);
    if (*p->s != '{') {
        rc = parse_error(p, "expect '{' at the top level");
    } else {
        rc = parse_object(p, root, 1);
    }
    if (rc == 0) {
        skip_spaces(p);
        if (*p->s != '\0') {
            rc = parse_error(p, "unexpected content after the top-level object");
        }
    }

    if (rc) {
        int line = 1, col = 1;
        for (const char *t = json_str; t < p->s; t++) {
            if (*t == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
        }
        MPII_Csel_json_free(root);
        MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER, "**colljson", "**colljson %d %d %s",
                             line, col, p->errmsg);
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
