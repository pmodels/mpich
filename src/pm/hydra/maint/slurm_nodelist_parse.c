/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>      /* for POSIX regular expressions */

#define MAX_GMATCH 5    /* max number of atoms in group matches + 1 */
#define MAX_RMATCH 5    /* max number of atoms in range matches + 1 */
#define MAX_EMATCH 2    /* max number of atoms in element matches + 1 */
#define MAX_NNODES_STRLEN 8     /* max string len for node # incl. '\0' */
#define MAX_HOSTNAME_LEN 64
#define MAX_NODELIST_LEN 8192

struct list {
    char *hostname;
    struct list *next;
};

void add_to_node_list(char *string, struct list **node_list)
{
    if (*node_list) {
        add_to_node_list(string, &(*node_list)->next);
    } else {
        (*node_list) = malloc(sizeof(struct list));
        (*node_list)->hostname = strdup(string);
        (*node_list)->next = NULL;
    }
}

void cleanup_node_list(struct list **node_list)
{
    if (!(*node_list))
        return;

    struct list *tmp = *node_list;
    struct list *next = tmp->next;

    *node_list = NULL;
    free(tmp->hostname);
    free(tmp);

    cleanup_node_list(&next);
}

void list_to_nodes(char *str, struct list **node_list)
{
    regex_t gmatch_old[2];
    regex_t gmatch_new[2];
    regex_t rmatch_old;
    regex_t rmatch_new;
    regex_t ematch_old;
    regex_t ematch_new;
    regmatch_t gmatch[2][MAX_GMATCH];
    regmatch_t rmatch[MAX_RMATCH];
    regmatch_t ematch[MAX_EMATCH];
    char hostname[MAX_HOSTNAME_LEN];
    char basename[MAX_HOSTNAME_LEN];
    char rbegin[MAX_NNODES_STRLEN];
    char rend[MAX_NNODES_STRLEN];
    char *gpattern[2];
    char *rpattern;
    char *epattern;
    char *string;
    char tmp[2];
    int j, begin, end, k = 0;

    string = strdup(str);

    /* compile regex patterns for nodelist matching */

    /* compile group-0 regex for old format: "[h00-h12,h14] | h00-h12 | h14" */
    regcomp(&gmatch_old[0],
            "(,|^)(\\[[-,a-z0-9]+\\]|[a-z]+[0-9]+-[a-z]+[0-9]+|[a-z]+[0-9]+)(,|$)",
            REG_EXTENDED | REG_ICASE);

    /* compile group-1 regex for old format: "h00-h12 | h14" */
    regcomp(&gmatch_old[1],
            "([[,]|^)([a-z]+[0-9]+-[a-z]+[0-9]+|[a-z]+[0-9]+)([],]|$)", REG_EXTENDED | REG_ICASE);

    /* compile range regex for old format: "h00-h12" */
    regcomp(&rmatch_old, "([a-z]+)([0-9]+)-([a-z]+)([0-9]+)", REG_EXTENDED | REG_ICASE);

    /* compile element regex for old format: "h14" */
    regcomp(&ematch_old, "([a-z]+[0-9]+)", REG_EXTENDED | REG_ICASE);

    /* compile group-0 regex for new format: "h00-[00-12,14] | h00[00-12,14] | h00-14 | h0014" */
    regcomp(&gmatch_new[0], "(,|^)([a-z0-9][\\.a-z0-9-]+)(\\[[-,0-9]+\\])?(,|$)",
            REG_EXTENDED | REG_ICASE);

    /* compile group-1 regex for new format: "00-12 | 14" */
    regcomp(&gmatch_new[1], "([[,]|^)([0-9]+-[0-9]+|[0-9]+)([],]|$)", REG_EXTENDED | REG_ICASE);

    /* compile range regex for new format: "00-12" */
    regcomp(&rmatch_new, "([0-9]+)-([0-9]+)", REG_EXTENDED | REG_ICASE);

    /* compile element regex for new format: "14" */
    regcomp(&ematch_new, "([0-9]+)", REG_EXTENDED | REG_ICASE);

    gpattern[0] = string;

    /* match old group-0 pattern: (,|^)([h00-h12,h14] | h00-h12 | h14)(,|$) */
    while (*gpattern[0] && regexec(&gmatch_old[0], gpattern[0], MAX_GMATCH, gmatch[0], 0) == 0) {
        /* bound group-0 for group-1 matching: [h00-h12,h14],... -> [h00-h12,h14]\0... */
        tmp[0] = *(gpattern[0] + gmatch[0][0].rm_eo);
        *(gpattern[0] + gmatch[0][0].rm_eo) = 0;

        /* select second atom in group-0 */
        gpattern[1] = gpattern[0] + gmatch[0][2].rm_so;

        /* match group-1 pattern: ([|,|^)(h00-h12 | h14)(]|,|$) */
        while (*gpattern[1] && regexec(&gmatch_old[1], gpattern[1], MAX_GMATCH, gmatch[1], 0) == 0) {
            /* bound group-1 for range/element matching: h00-h12, -> h00-h12\0 | h14] -> h14\0 */
            tmp[1] = *(gpattern[1] + gmatch[1][0].rm_eo);
            *(gpattern[1] + gmatch[1][0].rm_eo) = 0;

            /* select second atom in group-1 */
            rpattern = gpattern[1] + gmatch[1][2].rm_so;
            epattern = rpattern;

            if (regexec(&rmatch_old, rpattern, MAX_RMATCH, rmatch, 0) == 0) {
                /* matched range: (h)(00)-(h)(12) */
                snprintf(basename, MAX_HOSTNAME_LEN, "%.*s",
                         (int) (rmatch[1].rm_eo - rmatch[1].rm_so), rpattern + rmatch[1].rm_so);
                snprintf(rbegin, MAX_NNODES_STRLEN, "%.*s",
                         (int) (rmatch[2].rm_eo - rmatch[2].rm_so), rpattern + rmatch[2].rm_so);
                snprintf(rend, MAX_NNODES_STRLEN, "%.*s",
                         (int) (rmatch[4].rm_eo - rmatch[4].rm_so), rpattern + rmatch[4].rm_so);
                begin = atoi(rbegin);
                end = atoi(rend);

                /* expand range and add nodes to global node list */
                for (j = begin; j <= end; j++) {
                    snprintf(hostname, MAX_HOSTNAME_LEN, "%s%.*d",
                             basename, (int) (rmatch[2].rm_eo - rmatch[2].rm_so), j);
                    add_to_node_list(hostname, node_list);
                }
            } else if (regexec(&ematch_old, epattern, MAX_EMATCH, ematch, 0) == 0) {
                /* matched element: (h14) */
                snprintf(hostname, MAX_HOSTNAME_LEN, "%.*s",
                         (int) (ematch[1].rm_eo - ematch[1].rm_so), epattern + ematch[1].rm_so);
                add_to_node_list(hostname, node_list);
            }

            /* unbound group-1 and move to next group-1: h00-h12\0 -> h00-h12, | h14\0 -> h14] */
            *(gpattern[1] + gmatch[1][0].rm_eo) = tmp[1];
            gpattern[1] += gmatch[1][0].rm_eo;
        }

        /* unbound group-0 and move to next group-0: [h00-h12,h14]\0... -> [h00-h12,h14],... */
        *(gpattern[0] + gmatch[0][0].rm_eo) = tmp[0];
        gpattern[0] += gmatch[0][0].rm_eo;
    }

    /* match new group-0 pattern: (,|^)(h-)([00-12,14] | [00-12] | 14)(,|$) */
    while (*gpattern[0] && regexec(&gmatch_new[0], gpattern[0], MAX_GMATCH, gmatch[0], 0) == 0) {
        /* bound group-0 for group-1 matching: h-[00-h12,14],... -> h-[00-12,14]\0... */
        tmp[0] = *(gpattern[0] + gmatch[0][0].rm_eo);
        *(gpattern[0] + gmatch[0][0].rm_eo) = 0;

        /* extranct basename from atom 2 in group-0 */
        snprintf(basename, MAX_HOSTNAME_LEN, "%.*s",
                 (int) (gmatch[0][2].rm_eo - gmatch[0][2].rm_so), gpattern[0] + gmatch[0][2].rm_so);

        /*
         * name is matched entirely by second atom of group-0 pattern;
         * this happens when there is no numeric range ([00-12]), e.g.,
         * h00, h-00, etc ...
         */
        if (gmatch[0][3].rm_so == gmatch[0][3].rm_eo) {
            char trail = *(gpattern[0] + gmatch[0][2].rm_eo - 1);
            /* hostname must end with a letter or a digit */
            if ((trail >= 'a' && trail <= 'z') ||
                (trail >= 'A' && trail <= 'Z') || (trail >= '0' && trail <= '9')) {
                add_to_node_list(basename, node_list);
                *(gpattern[0] + gmatch[0][0].rm_eo) = tmp[0];
                gpattern[0] += gmatch[0][0].rm_eo;
                continue;
            } else {
                fprintf(stderr, "Error: hostname format not recognized, %s not added to nodelist\n",
                        basename);
                break;
            }
        }

        /* select third atom in group-0 */
        gpattern[1] = gpattern[0] + gmatch[0][3].rm_so;

        /* match new group-1 pattern: ([|,|^)(00-12 | 14)(]|,|$) */
        while (*gpattern[1] && regexec(&gmatch_new[1], gpattern[1], MAX_GMATCH, gmatch[1], 0) == 0) {
            /* bound group-1 for range/element matching: 00-12, -> 00-12\0 | 14] -> 14\0 */
            tmp[1] = *(gpattern[1] + gmatch[1][0].rm_eo);
            *(gpattern[1] + gmatch[1][0].rm_eo) = 0;

            /* select second atom in group-1 */
            rpattern = gpattern[1] + gmatch[1][2].rm_so;
            epattern = rpattern;

            if (regexec(&rmatch_new, rpattern, MAX_RMATCH, rmatch, 0) == 0) {
                /* matched range: (00)-(10) */
                snprintf(rbegin, MAX_NNODES_STRLEN, "%.*s",
                         (int) (rmatch[1].rm_eo - rmatch[1].rm_so), rpattern + rmatch[1].rm_so);
                snprintf(rend, MAX_NNODES_STRLEN, "%.*s",
                         (int) (rmatch[2].rm_eo - rmatch[2].rm_so), rpattern + rmatch[2].rm_so);
                begin = atoi(rbegin);
                end = atoi(rend);

                /* expand range and add nodes to global node list */
                for (j = begin; j <= end; j++) {
                    snprintf(hostname, MAX_HOSTNAME_LEN, "%s%.*d",
                             basename, (int) (rmatch[1].rm_eo - rmatch[1].rm_so), j);
                    add_to_node_list(hostname, node_list);
                }
            } else if (regexec(&ematch_new, epattern, MAX_EMATCH, ematch, 0) == 0) {
                /* matched element: (14) */
                snprintf(rbegin, MAX_NNODES_STRLEN, "%.*s",
                         (int) (ematch[1].rm_eo - ematch[1].rm_so), epattern + ematch[1].rm_so);
                snprintf(hostname, MAX_HOSTNAME_LEN, "%s%s", basename, rbegin);
                add_to_node_list(hostname, node_list);
            }

            /* unbound group-1 and move to next group-1: 00-10\0 -> 00-10, | 14\0 -> 14] */
            *(gpattern[1] + gmatch[1][0].rm_eo) = tmp[1];
            gpattern[1] += gmatch[1][0].rm_eo;
        }

        /* unbound group-0 and move to next group-0: h-[00-12,14]\0... -> h-[00-12,14],... */
        *(gpattern[0] + gmatch[0][0].rm_eo) = tmp[0];
        gpattern[0] += gmatch[0][0].rm_eo;
    }

    /* if nodelist format not recognized throw an error message and abort */
    if (*node_list == NULL) {
        fprintf(stdout, "Error: node list format not recognized.\n");
        fflush(stdout);
        abort();
    }

    /* clean up match patterns */
    regfree(&gmatch_old[0]);
    regfree(&gmatch_new[0]);
    regfree(&gmatch_old[1]);
    regfree(&gmatch_new[1]);
    regfree(&rmatch_old);
    regfree(&rmatch_new);
    regfree(&ematch_old);
    regfree(&ematch_new);

    /* free local nodelist */
    free(string);

  fn_exit:
    return;

  fn_fail:
    goto fn_exit;
}

int main(int argc, char *argv[])
{
    int i, errs = 0;
    char comp[MAX_NODELIST_LEN];
    char expn[MAX_NODELIST_LEN];
    char *token;
    struct list *node_list = NULL;
    struct list *node_list_ptr;

    fprintf(stdout, "input compressed nodelist: ");
    fscanf(stdin, "%s", comp);

    token = strdup(comp);

    /* parse nodelist and extract hostname(s) into nodelist */
    list_to_nodes(token, &node_list);

    /* parse node_list and put hostname(s) into expanded nodelist */
    i = 0;
    node_list_ptr = node_list;
    while (node_list_ptr != NULL) {
        /* get hostname */
        char *index = node_list_ptr->hostname;

        /* copy hostname into expanded list */
        while (*index != '\0')
            expn[i++] = *(index++);

        /* terminate every hostname in expanded list with ',' */
        expn[i++] = ',';

        /* move to next hostname in node_list */
        node_list_ptr = node_list_ptr->next;
    }

    expn[i - 1] = '\n';

    /* print expanded nodelist */
    fprintf(stdout, "expanded nodelist: %s\n", expn);

    cleanup_node_list(&node_list);

    free(token);

    return EXIT_SUCCESS;
}
