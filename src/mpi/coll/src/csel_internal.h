/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CSEL_INTERNAL_H_INCLUDED
#define CSEL_INTERNAL_H_INCLUDED

#include "mpiimpl.h"
#include "mpir_csel.h"
#include <stdbool.h>

typedef enum {
    /* global operator types */
    CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED = 0,

    /* comm-specific operator types */
    CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTRA,
    CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTER,

    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE,
    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LT,     /* Deprecated */
    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_NODE_COMM_SIZE,
    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_POW2,

    CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY,
    CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE,

    CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE,
    CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LT,  /* Deprecated */

    /* collective selection operator */
    CSEL_NODE_TYPE__OPERATOR__COLLECTIVE,

    /* message-specific operator types */
    CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE,
    CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LT,  /* Deprecated */
    CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE,
    CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LT,        /* Deprecated */

    CSEL_NODE_TYPE__OPERATOR__COUNT_LE,
    CSEL_NODE_TYPE__OPERATOR__COUNT_LT_POW2,

    CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE,
    CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR,
    CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE,
    CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN,

    /* any - has to be the last branch in an array */
    CSEL_NODE_TYPE__OPERATOR__ANY,

    /* container type */
    CSEL_NODE_TYPE__CONTAINER,
} csel_node_type_e;

typedef struct csel_node {
    csel_node_type_e type;

    union {
        /* global types */
        struct {
            int val;
        } is_multi_threaded;

        /* comm-specific operator types */

        /* collective selection operator */
        struct {
            MPIR_Csel_coll_type_e coll_type;
        } collective;

        /* message-specific operator types */
        struct {
            bool val;
        } is_commutative;
        struct {
            bool val;
        } is_sbuf_inplace;
        struct {
            bool val;
        } is_op_built_in;
        struct {
            bool val;
        } is_block_regular;
        struct {
            bool val;
        } is_node_consecutive;
        struct {
            int val;
        } comm_hierarchy;
        struct {
            void *container;
        } cnt;

        /* value le lt node for comm_size, msg_size, ppn, count */
        struct {
            int val;
        } value_le;
        struct {
            int val;
        } value_lt;
    } u;

    struct csel_node *success;
    struct csel_node *failure;
} csel_node_s;

typedef enum {
    CSEL_TYPE__ROOT,
    CSEL_TYPE__PRUNED,
} csel_type_e;

typedef struct {
    csel_type_e type;

    union {
        struct {
            csel_node_s *tree;
        } root;
        struct {
            /* one tree for each collective */
            csel_node_s *coll_trees[MPIR_CSEL_COLL_TYPE__END];
        } pruned;
    } u;
} csel_s;

extern const char *Csel_coll_type_str[];
extern const char *Csel_comm_hierarchy_str[];
extern const char *Csel_container_type_str[];

void Csel_print_node(csel_node_s * node);
void Csel_print_container(MPII_Csel_container_s * cnt);
void Csel_print_tree(csel_node_s * node);
void Csel_print_rules(csel_node_s * node);

csel_node_s *csel_node_create(csel_node_type_e type);
void csel_node_free(csel_node_s ** node);
void csel_tree_free(csel_node_s ** node);
csel_node_s *csel_node_swap_success(csel_node_s * node, csel_node_s * new_succ);
csel_node_s *csel_node_swap_failure(csel_node_s * node, csel_node_s * new_fail);
void csel_node_update(csel_node_s * node, csel_node_s node_value);

void csel_tree_optimize(csel_node_s ** node);
#endif /* CSEL_INTERNAL_CONTAINER_H_INCLUDED */
