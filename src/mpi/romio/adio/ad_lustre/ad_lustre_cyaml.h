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
 * Copyright (c) 2014, Intel Corporation.
 *
 * Author:
 *   Amir Shehata <amir.shehata@intel.com>
 */

#ifdef HAVE_YAML_SUPPORT

#include <stdbool.h>

enum cYAML_object_type {
    CYAML_TYPE_FALSE = 0,
    CYAML_TYPE_TRUE,
    CYAML_TYPE_NULL,
    CYAML_TYPE_NUMBER,
    CYAML_TYPE_STRING,
    CYAML_TYPE_ARRAY,
    CYAML_TYPE_OBJECT
};

struct cYAML {
    /* next/prev allow you to walk array/object chains. */
    struct cYAML *cy_next, *cy_prev;
    /* An array or object item will have a child pointer pointing
     * to a chain of the items in the array/object. */
    struct cYAML *cy_child;
    /* The type of the item, as above. */
    enum cYAML_object_type cy_type;

    /* The item's string, if type==CYAML_TYPE_STRING */
    char *cy_valuestring;
    /* The item's number, if type==CYAML_TYPE_NUMBER */
    int cy_valueint;
    /* The item's number, if type==CYAML_TYPE_NUMBER */
    double cy_valuedouble;
    /* The item's name string, if this item is the child of,
     * or is in the list of subitems of an object. */
    char *cy_string;
    /* user data which might need to be tracked per object */
    void *cy_user_data;
};

typedef void (*cYAML_user_data_free_cb) (void *);

/*
 * cYAML_walk_cb
 *   Callback called when recursing through the tree
 *
 *   cYAML* - pointer to the node currently being visitied
 *   void* - user data passed to the callback.
 *   void** - output value from the callback
 *
 * Returns true to continue recursing.  false to stop recursing
 */
typedef bool(*cYAML_walk_cb) (struct cYAML *, void *, void **);

/*
 * cYAML_build_tree
 *   Build a tree representation of the YAML formatted text passed in.
 */
struct cYAML *cYAML_build_tree(char *yaml_file);

void cYAML_free_tree(struct cYAML *node);


/* list structures and functions */
#define prefetch(a) ((void)a)

struct list_head {
    struct list_head *next, *prev;
};

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/**
 * Insert an entry at the start of a list.
 * \param new  new entry to be inserted
 * \param head list to add it to
 */
static inline void list_add(struct list_head *new, struct list_head *entry)
{
    new->prev = entry;
    new->next = entry->next;
    entry->next = new;
}

/**
 * Remove an entry from the list it is currently in.
 * \param entry the entry to remove
 */
static inline void list_del(struct list_head *entry)
{
    struct list_head *prev = entry->prev;
    struct list_head *next = entry->next;

    next->prev = prev;
    prev->next = next;
}

/**
 * Test whether a list is empty
 * \param head the list to test.
 */
static inline int list_empty(struct list_head *head)
{
    return head->next == head;
}

/**
 * Get the container of a list
 * \param ptr    the embedded list.
 * \param type   the type of the struct this is embedded in.
 * \param member the member name of the list within the struct.
 */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(char *)(&((type *)0)->member)))

/**
 * Iterate over a list
 * \param pos   the iterator
 * \param head  the list to iterate over
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next, prefetch(pos->next); pos != (head); \
		pos = pos->next, prefetch(pos->next))

/**
 * Iterate over a list of given type safe against removal of list entry
 * \param pos        the type * to use as a loop counter.
 * \param n          another type * to use as temporary storage
 * \param head       the head for your list.
 * \param member     the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)			 \
	for (pos = list_entry((head)->next, typeof(*pos), member),	 \
		n = list_entry(pos->member.next, typeof(*pos), member);  \
	     &pos->member != (head);					 \
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#endif /* HAVE_YAML_SUPPORT */
