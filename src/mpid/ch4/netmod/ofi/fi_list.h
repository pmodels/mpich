/*
 * Copyright (c) 2011-2016 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef FI_LIST_H_INCLUDED
#define FI_LIST_H_INCLUDED

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

/*
 * Double-linked list
 */
struct dlist_entry {
    struct dlist_entry *next;
    struct dlist_entry *prev;
};

#define DLIST_INIT(addr) { addr, addr }
#define DEFINE_LIST(name) struct dlist_entry name = DLIST_INIT(&name)

static inline void dlist_init(struct dlist_entry *head)
{
    head->next = head;
    head->prev = head;
}

static inline int dlist_empty(struct dlist_entry *head)
{
    return head->next == head;
}

static inline void dlist_insert_after(struct dlist_entry *item, struct dlist_entry *head)
{
    item->next = head->next;
    item->prev = head;
    head->next->prev = item;
    head->next = item;
}

static inline void dlist_insert_before(struct dlist_entry *item, struct dlist_entry *head)
{
    dlist_insert_after(item, head->prev);
}

#define dlist_insert_head dlist_insert_after
#define dlist_insert_tail dlist_insert_before

static inline void dlist_remove(struct dlist_entry *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
}

#define dlist_foreach(head, item) \
	for (item = (head)->next; item != head; item = item->next)

typedef int dlist_func_t(struct dlist_entry *item, const void *arg);

static inline struct dlist_entry *dlist_find_first_match(struct dlist_entry *head,
                                                         dlist_func_t * match, const void *arg)
{
    struct dlist_entry *item;

    dlist_foreach(head, item) {
        if (match(item, arg))
            return item;
    }

    return NULL;
}

static inline struct dlist_entry *dlist_remove_first_match(struct dlist_entry *head,
                                                           dlist_func_t * match, const void *arg)
{
    struct dlist_entry *item;

    item = dlist_find_first_match(head, match, arg);
    if (item)
        dlist_remove(item);

    return item;
}

/*
 * Single-linked list
 */
struct slist_entry {
    struct slist_entry *next;
};

struct slist {
    struct slist_entry *head;
    struct slist_entry *tail;
};

static inline void slist_init(struct slist *list)
{
    list->head = list->tail = NULL;
}

static inline int slist_empty(struct slist *list)
{
    return !list->head;
}

static inline void slist_insert_head(struct slist_entry *item, struct slist *list)
{
    if (slist_empty(list))
        list->tail = item;
    else
        item->next = list->head;

    list->head = item;
}

static inline void slist_insert_tail(struct slist_entry *item, struct slist *list)
{
    if (slist_empty(list))
        list->head = item;
    else
        list->tail->next = item;

    list->tail = item;
}

static inline struct slist_entry *slist_remove_head(struct slist *list)
{
    struct slist_entry *item;

    item = list->head;
    if (list->head == list->tail)
        slist_init(list);
    else
        list->head = item->next;
    return item;
}

#define slist_foreach(list, item, prev) \
	for (prev = NULL, item = list->head; item; prev = item, item = item->next)

typedef int slist_func_t(struct slist_entry *item, const void *arg);

static inline struct slist_entry *slist_remove_first_match(struct slist *list, slist_func_t * match,
                                                           const void *arg)
{
    struct slist_entry *item, *prev;

    slist_foreach(list, item, prev) {
        if (match(item, arg)) {
            if (prev)
                prev->next = item->next;
            else
                list->head = item->next;

            if (!item->next)
                list->tail = prev;

            return item;
        }
    }

    return NULL;
}

#endif /* FI_LIST_H_INCLUDED */
