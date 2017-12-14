/*
 * LGPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * LGPL HEADER END
 *
 * Copyright (c) 2014, 2015, Intel Corporation.
 *
 * Author:
 *   Amir Shehata <amir.shehata@intel.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "ad_lustre.h"

#ifdef HAVE_YAML_SUPPORT
#include <yaml.h>
#include "ad_lustre_cyaml.h"

#define INDENT		4
#define EXTRA_IND	2

#undef YAML_DEBUG

/*
 *  cYAML_ll
 *  Linked list of different trees representing YAML
 *  documents.
 */
struct cYAML_ll {
    struct list_head list;
    struct cYAML *obj;
};

enum cYAML_handler_error {
    CYAML_ERROR_NONE = 0,
    CYAML_ERROR_UNEXPECTED_STATE = -1,
    CYAML_ERROR_NOT_SUPPORTED = -2,
    CYAML_ERROR_OUT_OF_MEM = -3,
    CYAML_ERROR_BAD_VALUE = -4,
    CYAML_ERROR_PARSE = -5,
};

enum cYAML_tree_state {
    TREE_STATE_COMPLETE = 0,
    TREE_STATE_INITED,
    TREE_STATE_TREE_STARTED,
    TREE_STATE_BLK_STARTED,
    TREE_STATE_KEY,
    TREE_STATE_KEY_FILLED,
    TREE_STATE_VALUE,
    TREE_STATE_SEQ_START,
};

struct cYAML_tree_node {
    struct cYAML *root;
    /* cur is the current node we're operating on */
    struct cYAML *cur;
    enum cYAML_tree_state state;
    int from_blk_map_start;
    /* represents the tree depth */
    struct list_head ll;
};

typedef enum cYAML_handler_error (*yaml_token_handler) (yaml_token_t * token,
                                                        struct cYAML_tree_node *);

static enum cYAML_handler_error yaml_parse_error(yaml_token_t * token,
                                                 struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_stream_start(yaml_token_t * token,
                                                  struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_stream_end(yaml_token_t * token, struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_not_supported(yaml_token_t * token,
                                                   struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_blk_mapping_start(yaml_token_t * token,
                                                       struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_block_end(yaml_token_t * token, struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_key(yaml_token_t * token, struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_value(yaml_token_t * token, struct cYAML_tree_node *tree);
static enum cYAML_handler_error yaml_scalar(yaml_token_t * token, struct cYAML_tree_node *tree);

/* dispatch table for token types */
static yaml_token_handler dispatch_tbl[] = {
    [YAML_NO_TOKEN] = yaml_parse_error,
    [YAML_STREAM_START_TOKEN] = yaml_stream_start,
    [YAML_STREAM_END_TOKEN] = yaml_stream_end,
    [YAML_BLOCK_MAPPING_START_TOKEN] = yaml_blk_mapping_start,
    [YAML_BLOCK_END_TOKEN] = yaml_block_end,
    [YAML_KEY_TOKEN] = yaml_key,
    [YAML_VALUE_TOKEN] = yaml_value,
    [YAML_SCALAR_TOKEN] = yaml_scalar,
};

/* dispatch table */
static char *token_type_string[] = {
    [YAML_NO_TOKEN] = "YAML_NO_TOKEN",
    [YAML_STREAM_START_TOKEN] = "YAML_STREAM_START_TOKEN",
    [YAML_STREAM_END_TOKEN] = "YAML_STREAM_END_TOKEN",
    [YAML_BLOCK_MAPPING_START_TOKEN] = "YAML_BLOCK_MAPPING_START_TOKEN",
    [YAML_BLOCK_END_TOKEN] = "YAML_BLOCK_END_TOKEN",
    [YAML_KEY_TOKEN] = "YAML_KEY_TOKEN",
    [YAML_VALUE_TOKEN] = "YAML_VALUE_TOKEN",
    [YAML_SCALAR_TOKEN] = "YAML_SCALAR_TOKEN",
};

static void cYAML_ll_free(struct list_head *ll)
{
    struct cYAML_ll *node, *tmp;

    list_for_each_entry_safe(node, tmp, ll, list) {
        free(node);
    }
}

static int cYAML_ll_push(struct cYAML *obj, struct list_head *list)
{
    struct cYAML_ll *node = calloc(1, sizeof(*node));
    if (node == NULL)
        return -1;

    INIT_LIST_HEAD(&node->list);
    node->obj = obj;
    list_add(&node->list, list);

    return 0;
}

static struct cYAML *cYAML_ll_pop(struct list_head *list)
{
    struct cYAML_ll *pop;
    struct cYAML *obj = NULL;

    if (!list_empty(list)) {
        pop = list_entry(list->next, struct cYAML_ll, list);
        obj = pop->obj;
        list_del(&pop->list);
        free(pop);
    }

    return obj;
}

static int cYAML_ll_count(struct list_head *ll)
{
    int i = 0;
    struct list_head *node;

    list_for_each(node, ll)
        i++;

    return i;
}

static int cYAML_tree_init(struct cYAML_tree_node *tree)
{
    struct cYAML *obj = NULL, *cur = NULL;

    if (tree == NULL)
        return -1;

    obj = calloc(1, sizeof(*obj));
    if (obj == NULL)
        return -1;

    if (tree->root) {
        /* append the node */
        cur = tree->root;
        while (cur->cy_next != NULL)
            cur = cur->cy_next;
        cur->cy_next = obj;
    } else {
        tree->root = obj;
    }

    obj->cy_type = CYAML_TYPE_OBJECT;

    tree->cur = obj;
    tree->state = TREE_STATE_COMPLETE;

    /* free it and start anew */
    if (!list_empty(&tree->ll))
        cYAML_ll_free(&tree->ll);

    return 0;
}

static struct cYAML *create_child(struct cYAML *parent)
{
    struct cYAML *obj;

    if (parent == NULL)
        return NULL;

    obj = calloc(1, sizeof(*obj));
    if (obj == NULL)
        return NULL;

    /* set the type to OBJECT and let the value change that */
    obj->cy_type = CYAML_TYPE_OBJECT;

    parent->cy_child = obj;

    return obj;
}

static struct cYAML *create_sibling(struct cYAML *sibling)
{
    struct cYAML *obj;

    if (sibling == NULL)
        return NULL;

    obj = calloc(1, sizeof(*obj));
    if (obj == NULL)
        return NULL;

    /* set the type to OBJECT and let the value change that */
    obj->cy_type = CYAML_TYPE_OBJECT;

    sibling->cy_next = obj;
    obj->cy_prev = sibling;

    return obj;
}

/* Parse the input text to generate a number,
 * and populate the result into item. */
static bool parse_number(struct cYAML *item, const char *input)
{
    double n = 0, sign = 1, scale = 0;
    int subscale = 0, signsubscale = 1;
    const char *num = input;

    if (*num == '-') {
        sign = -1;
        num++;
    }

    if (*num == '0')
        num++;

    if (*num >= '1' && *num <= '9') {
        do {
            n = (n * 10.0) + (*num++ - '0');
        } while (*num >= '0' && *num <= '9');
    }

    if (*num == '.' && num[1] >= '0' && num[1] <= '9') {
        num++;
        do {
            n = (n * 10.0) + (*num++ - '0');
            scale--;
        } while (*num >= '0' && *num <= '9');
    }

    if (*num == 'e' || *num == 'E') {
        num++;
        if (*num == '+') {
            num++;
        } else if (*num == '-') {
            signsubscale = -1;
            num++;
        }
        while (*num >= '0' && *num <= '9')
            subscale = (subscale * 10) + (*num++ - '0');
    }

    /* check to see if the entire string is consumed.  If not then
     * that means this is a string with a number in it */
    if (num != (input + strlen(input)))
        return false;

    /* number = +/- number.fraction * 10^+/- exponent */
    n = sign * n * pow(10.0, (scale + subscale * signsubscale));

    item->cy_valuedouble = n;
    item->cy_valueint = (int) n;
    item->cy_type = CYAML_TYPE_NUMBER;

    return true;
}

static int assign_type_value(struct cYAML *obj, const char *value)
{
    if (value == NULL)
        return -1;

    if (strcmp(value, "null") == 0)
        obj->cy_type = CYAML_TYPE_NULL;
    else if (strcmp(value, "false") == 0) {
        obj->cy_type = CYAML_TYPE_FALSE;
        obj->cy_valueint = 0;
    } else if (strcmp(value, "true") == 0) {
        obj->cy_type = CYAML_TYPE_TRUE;
        obj->cy_valueint = 1;
    } else if (*value == '-' || (*value >= '0' && *value <= '9')) {
        if (parse_number(obj, value) == 0) {
            obj->cy_valuestring = strdup(value);
            obj->cy_type = CYAML_TYPE_STRING;
        }
    } else {
        obj->cy_valuestring = strdup(value);
        obj->cy_type = CYAML_TYPE_STRING;
    }

    return 0;
}

/*
 * yaml_handle_token
 *  Builds the YAML tree rpresentation as the tokens are passed in
 *
 *  if token == STREAM_START && tree_state != COMPLETE
 *    something wrong. fail.
 *  else tree_state = INITIED
 *  if token == STREAM_END && tree_state != INITED
 *    something wrong fail.
 *  else tree_state = COMPLETED
 *  if token == YAML_KEY_TOKEN && state != TREE_STARTED
 *    something wrong, fail.
 *  if token == YAML_SCALAR_TOKEN && state != KEY || VALUE
 *    fail.
 *  else if tree_state == KEY
 *     create a new sibling under the current head of the ll (if ll is
 *     empty insert the new node there and it becomes the root.)
 *    add the scalar value in the "string"
 *    tree_state = KEY_FILLED
 *  else if tree_state == VALUE
 *    try and figure out whether this is a double, int or string and store
 *    it appropriately
 *    state = TREE_STARTED
 * else if token == YAML_BLOCK_MAPPING_START_TOKEN && tree_state != VALUE
 *   fail
 * else push the current node on the ll && state = TREE_STARTED
 * if token == YAML_BLOCK_END_TOKEN && state != TREE_STARTED
 *   fail.
 * else pop the current token off the ll and make it the cur
 * if token == YAML_VALUE_TOKEN && state != KEY_FILLED
 *   fail.
 * else state = VALUE
 *
 */
static enum cYAML_handler_error yaml_parse_error(yaml_token_t * token, struct cYAML_tree_node *tree)
{
    return CYAML_ERROR_PARSE;
}

static enum cYAML_handler_error yaml_stream_start(yaml_token_t * token,
                                                  struct cYAML_tree_node *tree)
{
    enum cYAML_handler_error rc;

    /* with each new stream initialize a new tree */
    rc = cYAML_tree_init(tree);

    if (rc != CYAML_ERROR_NONE)
        return rc;

    tree->state = TREE_STATE_INITED;

    return CYAML_ERROR_NONE;
}

static enum cYAML_handler_error yaml_stream_end(yaml_token_t * token, struct cYAML_tree_node *tree)
{
    if (tree->state != TREE_STATE_TREE_STARTED && tree->state != TREE_STATE_COMPLETE)
        return CYAML_ERROR_UNEXPECTED_STATE;

    tree->state = TREE_STATE_INITED;

    return CYAML_ERROR_NONE;
}

static enum cYAML_handler_error yaml_key(yaml_token_t * token, struct cYAML_tree_node *tree)
{
    if (tree->state != TREE_STATE_BLK_STARTED && tree->state != TREE_STATE_VALUE)
        return CYAML_ERROR_UNEXPECTED_STATE;

    if (tree->from_blk_map_start == 0 || tree->state == TREE_STATE_VALUE)
        tree->cur = create_sibling(tree->cur);

    tree->from_blk_map_start = 0;

    tree->state = TREE_STATE_KEY;

    return CYAML_ERROR_NONE;
}

static enum cYAML_handler_error yaml_scalar(yaml_token_t * token, struct cYAML_tree_node *tree)
{
    if (tree->state == TREE_STATE_KEY) {
        /* assign the scalar value to the key that was created */
        tree->cur->cy_string = strdup((const char *) token->data.scalar.value);

        tree->state = TREE_STATE_KEY_FILLED;
    } else if (tree->state == TREE_STATE_VALUE) {
        if (assign_type_value(tree->cur, (char *) token->data.scalar.value))
            /* failed to assign a value */
            return CYAML_ERROR_BAD_VALUE;
        tree->state = TREE_STATE_BLK_STARTED;
    } else {
        return CYAML_ERROR_UNEXPECTED_STATE;
    }

    return CYAML_ERROR_NONE;
}

static enum cYAML_handler_error yaml_value(yaml_token_t * token, struct cYAML_tree_node *tree)
{
    if (tree->state != TREE_STATE_KEY_FILLED)
        return CYAML_ERROR_UNEXPECTED_STATE;

    tree->state = TREE_STATE_VALUE;

    return CYAML_ERROR_NONE;
}

static enum cYAML_handler_error
yaml_blk_mapping_start(yaml_token_t * token, struct cYAML_tree_node *tree)
{
    struct cYAML *obj;

    if (tree->state != TREE_STATE_VALUE &&
        tree->state != TREE_STATE_INITED &&
        tree->state != TREE_STATE_SEQ_START && tree->state != TREE_STATE_TREE_STARTED)
        return CYAML_ERROR_UNEXPECTED_STATE;

    /* block_mapping_start means we're entering another block
     * indentation, so we need to go one level deeper
     * create a child of cur */
    obj = create_child(tree->cur);

    /* push cur on the stack */
    if (cYAML_ll_push(tree->cur, &tree->ll))
        return CYAML_ERROR_OUT_OF_MEM;

    /* adding the new child to cur */
    tree->cur = obj;

    tree->state = TREE_STATE_BLK_STARTED;

    tree->from_blk_map_start = 1;

    return CYAML_ERROR_NONE;
}

static enum cYAML_handler_error yaml_block_end(yaml_token_t * token, struct cYAML_tree_node *tree)
{
    if (tree->state != TREE_STATE_BLK_STARTED && tree->state != TREE_STATE_VALUE)
        return CYAML_ERROR_UNEXPECTED_STATE;

    tree->cur = cYAML_ll_pop(&tree->ll);

    /* if you have popped all the way to the top level, then move to
     * the complete state. */
    if (cYAML_ll_count(&tree->ll) == 0)
        tree->state = TREE_STATE_COMPLETE;
    else if (tree->state == TREE_STATE_VALUE)
        tree->state = TREE_STATE_BLK_STARTED;

    return CYAML_ERROR_NONE;
}

static enum cYAML_handler_error yaml_not_supported(yaml_token_t * token,
                                                   struct cYAML_tree_node *tree)
{
    return CYAML_ERROR_NOT_SUPPORTED;
}

static bool free_node(struct cYAML *node, void *user_data, void **out)
{
    if (node->cy_type == CYAML_TYPE_STRING)
        free(node->cy_valuestring);
    if (node->cy_string)
        free(node->cy_string);
    if (node)
        free(node);

    return true;
}

void cYAML_tree_recursive_walk(struct cYAML *node, cYAML_walk_cb cb,
                               bool cb_first, void *usr_data, void **out)
{
    if (node == NULL)
        return;

    if (cb_first) {
        if (!cb(node, usr_data, out))
            return;
    }
    if (node->cy_child)
        cYAML_tree_recursive_walk(node->cy_child, cb, cb_first, usr_data, out);
    if (node->cy_next)
        cYAML_tree_recursive_walk(node->cy_next, cb, cb_first, usr_data, out);
    if (!cb_first) {
        if (!cb(node, usr_data, out))
            return;
    }
}

void cYAML_free_tree(struct cYAML *node)
{
    cYAML_tree_recursive_walk(node, free_node, false, NULL, NULL);
}

struct cYAML *cYAML_build_tree(char *yaml_file)
{
    yaml_parser_t parser;
    yaml_token_t token;
    struct cYAML_tree_node tree;
    enum cYAML_handler_error rc;
    yaml_token_type_t token_type;
    char err_str[256];
    FILE *input = NULL;
    int done = 0;

    memset(&tree, 0, sizeof(struct cYAML_tree_node));

    INIT_LIST_HEAD(&tree.ll);

    /* Create the Parser object. */
    yaml_parser_initialize(&parser);

    /* file always takes precedence */
    if (yaml_file != NULL) {
        /* Set a file input. */
        input = fopen(yaml_file, "rb");
        if (input == NULL) {
            LDEBUG("Failed to open file: %s\n", yaml_file);
            return NULL;
        }
        yaml_parser_set_input_file(&parser, input);
    }

    /* Read the event sequence. */
    while (!done) {
        /*
         * Go through the parser and build a cYAML representation
         * of the passed in YAML text
         */
        yaml_parser_scan(&parser, &token);
#ifdef YAML_DEBUG
        LDEBUG("token.type = %s: %s: tree.state=%d\n",
               token_type_string[token.type],
               (token.type == YAML_SCALAR_TOKEN) ?
               (char *) token.data.scalar.value : "", tree.state);
#endif
        rc = dispatch_tbl[token.type] (&token, &tree);

        /* Are we finished? */
        done = (rc != CYAML_ERROR_NONE ||
                token.type == YAML_STREAM_END_TOKEN || token.type == YAML_NO_TOKEN);

        token_type = token.type;
        yaml_token_delete(&token);
    }

    /* Destroy the Parser object. */
    yaml_parser_delete(&parser);

    if (input != NULL)
        fclose(input);

    if (token_type == YAML_STREAM_END_TOKEN && rc == CYAML_ERROR_NONE)
        return tree.root;
    cYAML_free_tree(tree.root);

    return NULL;
}

#endif /* HAVE_YAML_SUPPORT */
