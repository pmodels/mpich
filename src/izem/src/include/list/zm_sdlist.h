/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_SDLIST_H_
#define _ZM_SDLIST_H_

#include <stdlib.h>

/* Sequential Doubly-linked List (SDList) */

/* forward declarations */
typedef struct zm_sdlist  zm_sdlist_t;
typedef struct zm_sdlnode zm_sdlnode_t;

struct zm_sdlist {
    zm_sdlnode_t *head;
    zm_sdlnode_t *tail;
    int length;
};

struct zm_sdlnode {
    void *data;
    zm_sdlnode_t *next;
    zm_sdlnode_t *prev;
};

void zm_sdlist_init(zm_sdlist_t *);

/* accessors */
static inline zm_sdlnode_t *zm_sdlist_begin(zm_sdlist_t list) {
    return list.head;
}

static inline zm_sdlnode_t *zm_sdlist_end(zm_sdlist_t list) {
    return list.tail;
}

static inline zm_sdlnode_t *zm_sdlist_next(zm_sdlnode_t node) {
    return node.next;
}

static inline int zm_sdlist_length(zm_sdlist_t list) {
    return list.length;
}

/* updating routines */
static inline void zm_sdlist_push_back(zm_sdlist_t *list, void *data) {
    zm_sdlnode_t *tail= list->tail;
    zm_sdlnode_t *node = malloc(sizeof *node);
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    if (tail == NULL) {
        list->head = node;
        list->tail = node;
    }
    else {
        tail->next = node;
        node->prev = tail;
        list->tail = node;
    }
    list->length++;
}

static inline void *zm_sdlist_pop_front(zm_sdlist_t *list) {
    zm_sdlnode_t *head = list->head;
    void *data = NULL;
    if (head != NULL) {
        list->head = head->next;
        if(list->head != NULL)
            list->head->prev = NULL;
        else /* list became empty */
            list->tail = NULL;
        data = head->data;
        free(head);
    }
    list->length--;
    return data;
}

/* remove a known list node */
static inline void zm_sdlist_rmnode(zm_sdlist_t *list, zm_sdlnode_t *node){
    if(node == list->head)
        list->head = node->next;
    if(node == list->tail)
        list->tail = node->prev;
    if (node->prev != NULL)
        node->prev->next = node->next;
    if (node->next != NULL)
        node->next->prev = node->prev;
    free(node);
    list->length--;
}

/* remove a node that contains data */
/* TODO: should we take into account duplicates? */
static inline int zm_sdlist_remove(zm_sdlist_t *list, void *data) {
    zm_sdlnode_t* node = list->head;
    while(node != NULL) {
        if(node->data == data) {
            zm_sdlist_rmnode(list, node);
            break;
        }
        node = zm_sdlist_next(*node);
    }
    if(node != NULL)
        return 1;
    else
        return 0;
}

static inline void zm_sdlist_free(zm_sdlist_t *list) {
    while(zm_sdlist_pop_front(list) != NULL) ;
}
#endif /* _ZM_STLIST_H_ */
