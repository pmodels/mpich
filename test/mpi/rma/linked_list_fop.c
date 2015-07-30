/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*            MPI-3 distributed linked list construction example
 *            --------------------------------------------------
 *
 * Construct a distributed shared linked list using proposed MPI-3 dynamic
 * windows.  Initially process 0 creates the head of the list, attaches it to
 * the window, and broadcasts the pointer to all processes.  All processes then
 * concurrently append N new elements to the list.  When a process attempts to
 * attach its element to the tail of list it may discover that its tail pointer
 * is stale and it must chase ahead to the new tail before the element can be
 * attached.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include "mpitest.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define NUM_ELEMS 32
#define NPROBE    100
#define ELEM_PER_ROW 16

/* Linked list pointer */
typedef struct {
    int rank;
    MPI_Aint disp;
} llist_ptr_t;

/* Linked list element */
typedef struct {
    int value;
    llist_ptr_t next;
} llist_elem_t;

static const llist_ptr_t nil = { -1, (MPI_Aint) MPI_BOTTOM };

static const int verbose = 0;

/* List of locally allocated list elements. */
static llist_elem_t **my_elems = NULL;
static int my_elems_size = 0;
static int my_elems_count = 0;

/* Allocate a new shared linked list element */
MPI_Aint alloc_elem(int value, MPI_Win win)
{
    MPI_Aint disp;
    llist_elem_t *elem_ptr;

    /* Allocate the new element and register it with the window */
    MPI_Alloc_mem(sizeof(llist_elem_t), MPI_INFO_NULL, &elem_ptr);
    elem_ptr->value = value;
    elem_ptr->next = nil;
    MPI_Win_attach(win, elem_ptr, sizeof(llist_elem_t));

    /* Add the element to the list of local elements so we can free it later. */
    if (my_elems_size == my_elems_count) {
        my_elems_size += 100;
        my_elems = realloc(my_elems, my_elems_size * sizeof(void *));
    }
    my_elems[my_elems_count] = elem_ptr;
    my_elems_count++;

    MPI_Get_address(elem_ptr, &disp);
    return disp;
}

int main(int argc, char **argv)
{
    int procid, nproc, i;
    MPI_Win llist_win;
    llist_ptr_t head_ptr, tail_ptr;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &procid);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &llist_win);

    /* Process 0 creates the head node */
    if (procid == 0)
        head_ptr.disp = alloc_elem(-1, llist_win);

    /* Broadcast the head pointer to everyone */
    head_ptr.rank = 0;
    MPI_Bcast(&head_ptr.disp, 1, MPI_AINT, 0, MPI_COMM_WORLD);
    tail_ptr = head_ptr;

    /* All processes concurrently append NUM_ELEMS elements to the list */
    for (i = 0; i < NUM_ELEMS; i++) {
        llist_ptr_t new_elem_ptr;
        int success;

        /* Create a new list element and register it with the window */
        new_elem_ptr.rank = procid;
        new_elem_ptr.disp = alloc_elem(procid, llist_win);

        /* Append the new node to the list.  This might take multiple attempts if
         * others have already appended and our tail pointer is stale. */
        do {
            llist_ptr_t next_tail_ptr = nil;

            MPI_Win_lock(MPI_LOCK_SHARED, tail_ptr.rank, MPI_MODE_NOCHECK, llist_win);

            MPI_Compare_and_swap((void *) &new_elem_ptr.rank, (void *) &nil.rank,
                                 (void *) &next_tail_ptr.rank, MPI_INT, tail_ptr.rank,
                                 (MPI_Aint) & (((llist_elem_t *) tail_ptr.disp)->next.rank),
                                 llist_win);

            MPI_Win_unlock(tail_ptr.rank, llist_win);
            success = (next_tail_ptr.rank == nil.rank);

            if (success) {
                int i, flag;
                MPI_Aint result;

                MPI_Win_lock(MPI_LOCK_SHARED, tail_ptr.rank, MPI_MODE_NOCHECK, llist_win);

                MPI_Fetch_and_op(&new_elem_ptr.disp, &result, MPI_AINT, tail_ptr.rank,
                                 (MPI_Aint) & (((llist_elem_t *) tail_ptr.disp)->next.disp),
                                 MPI_REPLACE, llist_win);

                /* Note: accumulate is faster, since we don't need the result.  Replacing with
                 * Fetch_and_op to create a more complete test case. */
                /*
                 * MPI_Accumulate(&new_elem_ptr.disp, 1, MPI_AINT, tail_ptr.rank,
                 * (MPI_Aint) &(((llist_elem_t*)tail_ptr.disp)->next.disp), 1,
                 * MPI_AINT, MPI_REPLACE, llist_win);
                 */

                MPI_Win_unlock(tail_ptr.rank, llist_win);
                tail_ptr = new_elem_ptr;

                /* For implementations that use pt-to-pt messaging, force progress for other threads'
                 * RMA operations. */
                for (i = 0; i < NPROBE; i++)
                    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag,
                               MPI_STATUS_IGNORE);

            }
            else {
                /* Tail pointer is stale, fetch the displacement.  May take multiple tries
                 * if it is being updated. */
                do {
                    MPI_Win_lock(MPI_LOCK_SHARED, tail_ptr.rank, MPI_MODE_NOCHECK, llist_win);

                    MPI_Fetch_and_op(NULL, &next_tail_ptr.disp, MPI_AINT, tail_ptr.rank,
                                     (MPI_Aint) & (((llist_elem_t *) tail_ptr.disp)->next.disp),
                                     MPI_NO_OP, llist_win);

                    MPI_Win_unlock(tail_ptr.rank, llist_win);
                } while (next_tail_ptr.disp == nil.disp);
                tail_ptr = next_tail_ptr;
            }
        } while (!success);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Traverse the list and verify that all processes inserted exactly the correct
     * number of elements. */
    if (procid == 0) {
        int have_root = 0;
        int errors = 0;
        int *counts, count = 0;

        counts = (int *) malloc(sizeof(int) * nproc);
        assert(counts != NULL);

        for (i = 0; i < nproc; i++)
            counts[i] = 0;

        tail_ptr = head_ptr;

        /* Walk the list and tally up the number of elements inserted by each rank */
        while (tail_ptr.disp != nil.disp) {
            llist_elem_t elem;

            MPI_Win_lock(MPI_LOCK_SHARED, tail_ptr.rank, MPI_MODE_NOCHECK, llist_win);

            MPI_Get(&elem, sizeof(llist_elem_t), MPI_BYTE,
                    tail_ptr.rank, tail_ptr.disp, sizeof(llist_elem_t), MPI_BYTE, llist_win);

            MPI_Win_unlock(tail_ptr.rank, llist_win);

            tail_ptr = elem.next;

            /* This is not the root */
            if (have_root) {
                assert(elem.value >= 0 && elem.value < nproc);
                counts[elem.value]++;
                count++;

                if (verbose) {
                    int last_elem = tail_ptr.disp == nil.disp;
                    printf("%2d%s", elem.value, last_elem ? "" : " -> ");
                    if (count % ELEM_PER_ROW == 0 && !last_elem)
                        printf("\n");
                }
            }

            /* This is the root */
            else {
                assert(elem.value == -1);
                have_root = 1;
            }
        }

        if (verbose)
            printf("\n\n");

        /* Verify the counts we collected */
        for (i = 0; i < nproc; i++) {
            int expected = NUM_ELEMS;

            if (counts[i] != expected) {
                printf("Error: Rank %d inserted %d elements, expected %d\n", i, counts[i],
                       expected);
                errors++;
            }
        }

        printf("%s\n", errors == 0 ? " No Errors" : "FAIL");
        free(counts);
    }

    MPI_Win_free(&llist_win);

    /* Free all the elements in the list */
    for (; my_elems_count > 0; my_elems_count--)
        MPI_Free_mem(my_elems[my_elems_count - 1]);

    MPI_Finalize();
    return 0;
}
