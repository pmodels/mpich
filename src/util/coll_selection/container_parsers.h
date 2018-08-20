#ifndef CONTAINER_PARSER_H_INCLUDED
#define CONTAINER_PARSER_H_INCLUDED

#include "coll_tree_json.h"

static void bcast_alpha(MPIU_COLL_SELECTION_TREE_NODE json_node, int *cnt_num,
                        MPIDIG_coll_algo_generic_container_t * cnt, int coll_id)
{
    int i = 0;
    int ind = 0;
    int bcast_netmod_algo_id = -1;
    int bcast_shm_algo_id = -1;
    char *leaf_name = NULL;
    int is_known_token = 1;

    cnt[0].id = coll_id;

    json_node = MPIU_COLL_SELECTION_TREE_get_node_json_next(json_node, 0);

    if (json_node != MPIU_COLL_SELECTION_TREE_NULL_TYPE) {
        for (ind = 0; ind < MPIU_COLL_SELECTION_TREE_get_node_json_peer_count(json_node); ind++) {
            leaf_name = strdup(MPIU_COLL_SELECTION_TREE_get_node_json_name(json_node, ind));

            is_known_token = 1;

            char buff[MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE] = { 0 };

            if (leaf_name) {

                MPL_strncpy(buff, leaf_name, MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE);

                char *key = NULL;
                char *value = NULL;
                int algorithm_type = -1;
                int netmod_algo_id = 0;
                int shm_algo_id = 0;

                key = strtok(buff, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
                value = strtok(NULL, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
                algorithm_type = MPIU_COLL_SELECTION_TREE_get_algorithm_type(key);

                if ((algorithm_type != MPIU_COLL_SELECTION_TREE_NETMOD) ||
                    (algorithm_type != MPIU_COLL_SELECTION_TREE_SHM)) {
                    is_known_token = 0;
                }

                if (is_known_token) {
                    if (algorithm_type == MPIU_COLL_SELECTION_TREE_NETMOD) {
                        for (netmod_algo_id = 0;
                             netmod_algo_id <
                             MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                             [algorithm_type]; netmod_algo_id++) {
                            if (strcasecmp
                                (MPIU_COLL_SELECTION_TREE_ALGORITHMS[MPIU_COLL_SELECTION_BCAST]
                                 [algorithm_type][netmod_algo_id], value) == 0) {
                                break;
                            }
                        }
                        if (netmod_algo_id <
                            MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                            [algorithm_type]) {
                            bcast_netmod_algo_id = netmod_algo_id;
                        }
                    }
                    if (algorithm_type == MPIU_COLL_SELECTION_TREE_SHM) {
                        for (shm_algo_id = 0;
                             shm_algo_id <
                             MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                             [algorithm_type]; shm_algo_id++) {
                            if (strcasecmp
                                (MPIU_COLL_SELECTION_TREE_ALGORITHMS[MPIU_COLL_SELECTION_BCAST]
                                 [algorithm_type][shm_algo_id], value) == 0) {
                                break;
                            }
                        }
                        if (shm_algo_id <
                            MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                            [algorithm_type]) {
                            bcast_shm_algo_id = shm_algo_id;
                        }
                    }
                }
                MPL_free(leaf_name);
            }
        }
    }

    cnt[0].params.bcast_params.ch4_bcast_params.ch4_bcast_alpha.roots_bcast = bcast_netmod_algo_id;
    cnt[0].params.bcast_params.ch4_bcast_params.ch4_bcast_alpha.node_bcast = bcast_shm_algo_id;
    cnt[1].id = bcast_netmod_algo_id;
    cnt[2].id = bcast_shm_algo_id;

    for (i = 3; i < MPIU_COLL_SELECTION_TREE_MAX_CNT; i++) {
        cnt[i].id = -1;
    }
    *cnt_num = MPIU_COLL_SELECTION_TREE_MAX_CNT;
}

static void bcast_beta(MPIU_COLL_SELECTION_TREE_NODE json_node, int *cnt_num,
                       MPIDIG_coll_algo_generic_container_t * cnt, int coll_id)
{
    int i = 0;
    int ind = 0;
    int bcast_netmod_algo_id = -1;
    int bcast_shm_algo_id = -1;
    char *leaf_name = NULL;
    int is_known_token = 1;

    cnt[0].id = coll_id;

    json_node = MPIU_COLL_SELECTION_TREE_get_node_json_next(json_node, 0);

    if (json_node != MPIU_COLL_SELECTION_TREE_NULL_TYPE) {
        for (ind = 0; ind < MPIU_COLL_SELECTION_TREE_get_node_json_peer_count(json_node); ind++) {
            leaf_name = strdup(MPIU_COLL_SELECTION_TREE_get_node_json_name(json_node, ind));

            is_known_token = 1;

            char buff[MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE] = { 0 };

            if (leaf_name) {

                MPL_strncpy(buff, leaf_name, MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE);

                char *key = NULL;
                char *value = NULL;
                int algorithm_type = -1;
                int netmod_algo_id = 0;
                int shm_algo_id = 0;

                key = strtok(buff, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
                value = strtok(NULL, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
                algorithm_type = MPIU_COLL_SELECTION_TREE_get_algorithm_type(key);

                if ((algorithm_type != MPIU_COLL_SELECTION_TREE_NETMOD) ||
                    (algorithm_type != MPIU_COLL_SELECTION_TREE_SHM)) {
                    is_known_token = 0;
                }

                if (is_known_token) {
                    if (algorithm_type == MPIU_COLL_SELECTION_TREE_NETMOD) {
                        for (netmod_algo_id = 0;
                             netmod_algo_id <
                             MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                             [algorithm_type]; netmod_algo_id++) {
                            if (strcasecmp
                                (MPIU_COLL_SELECTION_TREE_ALGORITHMS[MPIU_COLL_SELECTION_BCAST]
                                 [algorithm_type][netmod_algo_id], value) == 0) {
                                break;
                            }
                        }
                        if (netmod_algo_id <
                            MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                            [algorithm_type]) {
                            bcast_netmod_algo_id = netmod_algo_id;
                        }
                    }
                    if (algorithm_type == MPIU_COLL_SELECTION_TREE_SHM) {
                        for (shm_algo_id = 0;
                             shm_algo_id <
                             MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                             [algorithm_type]; shm_algo_id++) {
                            if (strcasecmp
                                (MPIU_COLL_SELECTION_TREE_ALGORITHMS[MPIU_COLL_SELECTION_BCAST]
                                 [algorithm_type][shm_algo_id], value) == 0) {
                                break;
                            }
                        }
                        if (shm_algo_id <
                            MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                            [algorithm_type]) {
                            bcast_shm_algo_id = shm_algo_id;
                        }
                    }
                }
                MPL_free(leaf_name);
            }
        }
    }

    cnt[0].params.bcast_params.ch4_bcast_params.ch4_bcast_beta.node_bcast_first = bcast_shm_algo_id;
    cnt[0].params.bcast_params.ch4_bcast_params.ch4_bcast_beta.roots_bcast = bcast_netmod_algo_id;
    cnt[0].params.bcast_params.ch4_bcast_params.ch4_bcast_beta.node_bcast_second =
        bcast_shm_algo_id;
    cnt[1].id = bcast_shm_algo_id;
    cnt[2].id = bcast_netmod_algo_id;
    cnt[3].id = bcast_shm_algo_id;

    for (i = 4; i < MPIU_COLL_SELECTION_TREE_MAX_CNT; i++) {
        cnt[i].id = -1;
    }
    *cnt_num = MPIU_COLL_SELECTION_TREE_MAX_CNT;
}

static void bcast_gamma(MPIU_COLL_SELECTION_TREE_NODE json_node, int *cnt_num,
                        MPIDIG_coll_algo_generic_container_t * cnt, int coll_id)
{
    int i = 0;
    int bcast_netmod_algo_id = -1;
    char *leaf_name = NULL;
    int is_known_token = 1;

    cnt[0].id = coll_id;

    json_node = MPIU_COLL_SELECTION_TREE_get_node_json_next(json_node, 0);

    if (json_node != MPIU_COLL_SELECTION_TREE_NULL_TYPE) {

        leaf_name = strdup(MPIU_COLL_SELECTION_TREE_get_node_json_name(json_node, 0));

        is_known_token = 1;

        char buff[MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE] = { 0 };

        if (leaf_name) {

            MPL_strncpy(buff, leaf_name, MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE);

            char *key = NULL;
            char *value = NULL;
            int algorithm_type = -1;
            int netmod_algo_id = 0;

            key = strtok(buff, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
            value = strtok(NULL, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
            algorithm_type = MPIU_COLL_SELECTION_TREE_get_algorithm_type(key);

            if (algorithm_type != MPIU_COLL_SELECTION_TREE_NETMOD) {
                is_known_token = 0;
            }

            if (is_known_token) {
                for (netmod_algo_id = 0;
                     netmod_algo_id <
                     MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                     [algorithm_type]; netmod_algo_id++) {
                    if (strcasecmp(MPIU_COLL_SELECTION_TREE_ALGORITHMS[MPIU_COLL_SELECTION_BCAST]
                                   [algorithm_type][netmod_algo_id], value) == 0) {
                        break;
                    }
                }
            }
            if (netmod_algo_id <
                MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST][algorithm_type]) {
                bcast_netmod_algo_id = netmod_algo_id;
            }
            MPL_free(leaf_name);
        }
    }

    cnt[0].params.bcast_params.ch4_bcast_params.ch4_bcast_gamma.bcast = bcast_netmod_algo_id;
    cnt[1].id = bcast_netmod_algo_id;

    for (i = 2; i < MPIU_COLL_SELECTION_TREE_MAX_CNT; i++) {
        cnt[i].id = -1;
    }
    *cnt_num = MPIU_COLL_SELECTION_TREE_MAX_CNT;
}


static void bcast_delta(MPIU_COLL_SELECTION_TREE_NODE json_node, int *cnt_num,
                        MPIDIG_coll_algo_generic_container_t * cnt, int coll_id)
{
    int i = 0;
    int bcast_shm_algo_id = -1;
    char *leaf_name = NULL;
    int is_known_token = 1;

    cnt[0].id = coll_id;

    json_node = MPIU_COLL_SELECTION_TREE_get_node_json_next(json_node, 0);

    if (json_node != MPIU_COLL_SELECTION_TREE_NULL_TYPE) {

        leaf_name = strdup(MPIU_COLL_SELECTION_TREE_get_node_json_name(json_node, 0));

        is_known_token = 1;

        char buff[MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE] = { 0 };

        if (leaf_name) {

            MPL_strncpy(buff, leaf_name, MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE);

            char *key = NULL;
            char *value = NULL;
            int algorithm_type = -1;
            int shm_algo_id = 0;

            key = strtok(buff, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
            value = strtok(NULL, MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE);
            algorithm_type = MPIU_COLL_SELECTION_TREE_get_algorithm_type(key);

            if (algorithm_type != MPIU_COLL_SELECTION_TREE_SHM) {
                is_known_token = 0;
            }

            if (is_known_token) {
                for (shm_algo_id = 0;
                     shm_algo_id < MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST]
                     [algorithm_type]; shm_algo_id++) {
                    if (strcasecmp(MPIU_COLL_SELECTION_TREE_ALGORITHMS[MPIU_COLL_SELECTION_BCAST]
                                   [algorithm_type][shm_algo_id], value) == 0) {
                        break;
                    }
                }
            }
            if (shm_algo_id <
                MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_BCAST][algorithm_type]) {
                bcast_shm_algo_id = shm_algo_id;
            }
            MPL_free(leaf_name);
        }
    }

    cnt[0].params.bcast_params.ch4_bcast_params.ch4_bcast_delta.bcast = bcast_shm_algo_id;
    cnt[1].id = bcast_shm_algo_id;

    for (i = 2; i < MPIU_COLL_SELECTION_TREE_MAX_CNT; i++) {
        cnt[i].id = -1;
    }
    *cnt_num = MPIU_COLL_SELECTION_TREE_MAX_CNT;
}

#endif /* CONTAINER_PARSER_H_INCLUDED */
