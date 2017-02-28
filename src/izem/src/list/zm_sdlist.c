/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "list/zm_sdlist.h"

void zm_sdlist_init(zm_sdlist_t *list) {
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
}
