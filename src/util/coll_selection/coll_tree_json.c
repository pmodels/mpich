#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include "mpl.h"
#include "mpiimpl.h"
#include "coll_tree_json.h"

static inline MPIU_COLL_SELECTION_TREE_NODE MPIU_COLL_SELECTION_TREE_text_to_json(char *contents)
{
    MPIU_COLL_SELECTION_TREE_NODE root = MPIU_COLL_SELECTION_TREE_NULL_TYPE;
    root = json_tokener_parse(contents);
    return root;
}

static inline char *MPIU_COLL_SELECTION_TREE_get_node_json_name(MPIU_COLL_SELECTION_TREE_NODE
                                                                json_node, int index)
{
    struct json_object_iter iter;
    int count = 0;

    json_object_object_foreachC(json_node, iter) {
        if (count == index)
            return (char *) lh_entry_k(iter.entry);
        count++;
    }

    return NULL;
}

static inline MPIU_COLL_SELECTION_TREE_NODE
MPIU_COLL_SELECTION_TREE_get_node_json_next(MPIU_COLL_SELECTION_TREE_NODE json_node, int index)
{
    struct json_object_iter iter;
    int count = 0;

    json_object_object_foreachC(json_node, iter) {
        if (count == index)
            return (struct json_object *) lh_entry_v(iter.entry);
        count++;
    }

    return MPIU_COLL_SELECTION_TREE_NULL_TYPE;
}

static inline int MPIU_COLL_SELECTION_TREE_get_node_json_type(MPIU_COLL_SELECTION_TREE_NODE
                                                              json_node)
{
    return json_node->o_type;
}

static inline int MPIU_COLL_SELECTION_TREE_get_node_json_peer_count(MPIU_COLL_SELECTION_TREE_NODE
                                                                    json_node)
{
    if (json_node == MPIU_COLL_SELECTION_TREE_NULL_TYPE) {
        return 0;
    } else {
        return json_node->o.c_object->count;
    }
}

MPIU_COLL_SELECTION_TREE_NODE MPIU_COLL_SELECTION_TREE_read(char *file)
{
    MPIU_COLL_SELECTION_TREE_NODE root = MPIU_COLL_SELECTION_TREE_NULL_TYPE;
    FILE *fp = NULL;
    struct stat filestat;
    int size = 0;
    char *contents = NULL;

    if (file == NULL) {
        goto fn_exit;
    }

    if (stat(file, &filestat) != 0) {
        MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "File %s not found\n", file));
        goto fn_exit;
    }

    size = filestat.st_size;
    contents = (char *) MPL_malloc(filestat.st_size, MPL_MEM_OTHER);
    if (contents == NULL) {
        MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "Unable to allocate %d bytes\n", size));
        goto fn_exit;
    }

    fp = fopen(file, "rt");
    if (fp == NULL) {
        MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Unable to open %s\n", file));
        goto fn_exit;
    }

    size_t left_to_read = size;
    size_t completed = 0;

    while (left_to_read) {
        completed = fread((char *) contents + (size - left_to_read), 1, left_to_read, fp);

        if (completed == 0) {
            break;
        }

        left_to_read -= completed;
    }

    if (left_to_read != 0) {
        MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "Unable to read content of %s\n", file));
        fclose(fp);
        goto fn_exit;
    }
    fclose(fp);

    root = MPIU_COLL_SELECTION_TREE_text_to_json(contents);
    if (root == MPIU_COLL_SELECTION_TREE_NULL_TYPE) {
        MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "Unable to load tuning settings\n"));
        goto fn_exit;
    }

  fn_exit:
    if (contents)
        MPL_free(contents);

    return root;
}
