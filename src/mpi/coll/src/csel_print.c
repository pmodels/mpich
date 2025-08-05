/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "csel_internal.h"
#include "utlist.h"

static int nesting = -1;

#define printf_indent(indent)                                \
    do {                                                     \
        for (int nindent = 0; nindent < (indent); nindent++) \
            printf("  ");                                    \
    } while (0)

#define printf_newline()                        \
    do {                                        \
        printf("\n");                           \
    } while (0)

struct Csel_decision_rule;
typedef struct Csel_decision_rule {
    int size;
    int algorithm;
    csel_node_s **path;
    struct Csel_decision_rule *prev, *next;
} Csel_decision_rule;

void Csel_print_tree(csel_node_s * node)
{
    nesting++;

    if (node == NULL) {
        return;
    }

    printf_indent(nesting);
    Csel_print_node(node);
    printf_newline();

    if (node->type != CSEL_NODE_TYPE__CONTAINER) {
        Csel_print_tree(node->success);
        if (node->failure) {
            nesting--;
            Csel_print_tree(node->failure);
            nesting++;
        }
    }

    nesting--;
}

void Csel_print_container(MPII_Csel_container_s * cnt)
{
    printf("Algorithm: %s", Csel_container_type_str[cnt->id]);
}

void Csel_print_node(csel_node_s * node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED:
            printf("MPI library is multithreaded");
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTRA:
            printf("intra_comm");
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTER:
            printf("inter_comm");
            break;
        case CSEL_NODE_TYPE__OPERATOR__COLLECTIVE:
            printf("collective: %s", Csel_coll_type_str[node->u.collective.coll_type]);
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE:
            printf("comm_size <= %d", node->u.value_le.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LT:
            printf("comm_size < %d", node->u.value_lt.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_POW2:
            printf("comm_size is power-of-two");
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_NODE_COMM_SIZE:
            printf("comm_size == node_comm_size");
            break;
        case CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE:
            printf("avg_msg_size <= %d", node->u.value_le.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LT:
            printf("avg_msg_size < %d", node->u.value_lt.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE:
            printf("total_msg_size <= %d", node->u.value_le.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LT:
            printf("total_msg_size < %d", node->u.value_lt.val);
            break;
        case CSEL_NODE_TYPE__CONTAINER:
            Csel_print_container(node->u.cnt.container);
            break;
        case CSEL_NODE_TYPE__OPERATOR__COUNT_LE:
            printf("count <= %d", node->u.value_le.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__COUNT_LT_POW2:
            printf("count < nearest power-of-two less than comm size");
            break;
        case CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE:
            printf("source buffer is MPI_IN_PLACE");
            break;
        case CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR:
            printf("all blocks have the same count");
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY:
            printf("comm hierarchy is: %s", Csel_comm_hierarchy_str[node->u.comm_hierarchy.val]);
            break;
        case CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE:
            printf("process ranks are consecutive on the node");
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE:
            printf("comm avg ppn <= %d", node->u.value_le.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LT:
            printf("comm avg ppn < %d", node->u.value_lt.val);
            break;
        case CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE:
            if (node->u.is_commutative.val == true)
                printf("commutative OP");
            else
                printf("not commutative OP");
            break;
        case CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN:
            printf("built-in OP");
            break;
        case CSEL_NODE_TYPE__OPERATOR__ANY:
            printf("any");
            break;
        default:
            printf("unknown operator");
            MPIR_Assert(0);
    }
}

static csel_node_s *decision_path[128] = { 0 };
static Csel_decision_rule *algorithms[MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count] = { 0 };

static void collect_rules(csel_node_s * node);

void collect_rules(csel_node_s * node)
{
    nesting++;

    if (node == NULL) {
        return;
    }

    if (node->type == CSEL_NODE_TYPE__CONTAINER) {
        int alg_id = ((MPII_Csel_container_s *) node->u.cnt.container)->id;
        /* save decision path */
        // for (int i = 0; i < nesting; i++) {
        //     Csel_print_node(decision_path[i]);
        // }
        // printf("--> %s\n", Csel_container_type_str[alg_id]);
        Csel_decision_rule *new_rule =
            (Csel_decision_rule *) MPL_malloc(sizeof(Csel_decision_rule), MPL_MEM_OTHER);
        new_rule->size = nesting;
        new_rule->algorithm = alg_id;
        new_rule->path =
            (csel_node_s **) MPL_malloc(sizeof(csel_node_s *) * nesting, MPL_MEM_OTHER);
        memcpy(new_rule->path, decision_path, sizeof(csel_node_s *) * new_rule->size);
        DL_APPEND(algorithms[alg_id], new_rule);
    } else {
        decision_path[nesting] = node;
        collect_rules(node->success);
        if (node->failure) {
            nesting--;
            collect_rules(node->failure);
            nesting++;
        }
    }
    nesting--;
}

void Csel_print_rules(csel_node_s * node)
{
    memset(decision_path, 0, sizeof(csel_node_s *) * 128);
    memset(algorithms, 0,
           sizeof(Csel_decision_rule *) * MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count);

    collect_rules(node);

    for (int i = 0; i < MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count; i++) {
        if (algorithms[i] != NULL) {
            printf("%s\n", Csel_container_type_str[i]);
            if (algorithms[i] == NULL) {
                continue;
            } else {
                Csel_decision_rule *iter = NULL, *temp = NULL;
                if (algorithms[i]->size == 1) {
                    printf_indent(1);
                    printf(" >>> any\n");
                    MPL_free(algorithms[i]->path);
                    MPL_free(algorithms[i]);
                } else {
                    DL_FOREACH_SAFE(algorithms[i], iter, temp) {
                        printf_indent(1);
                        for (int j = 1; j < iter->size; j++) {
                            printf(" >>> ");
                            Csel_print_node(iter->path[j]);
                        }
                        printf_newline();
                        DL_DELETE(algorithms[i], iter);
                        MPL_free(iter->path);
                        MPL_free(iter);
                    }
                }
            }
        }
    }
}
