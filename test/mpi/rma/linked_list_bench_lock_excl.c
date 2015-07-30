/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*            MPI-3 distributed linked list construction example
 *            --------------------------------------------------
 *
 * Construct a distributed shared linked list using proposed MPI-3 dynamic
 * windows.  Initially process 0 creates the head of the list, attaches it to
 * the window, and broadcasts the pointer to all processes.  Each process p then
 * appends N new elements to the list when the tail reaches process p-1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include "mpitest.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define NUM_ELEMS 1000
#define MAX_NPROBE nproc
#define MIN_NPROBE 1
#define ELEM_PER_ROW 16

#define MIN(X,Y) ((X < Y) ? (X) : (Y))
#define MAX(X,Y) ((X > Y) ? (X) : (Y))

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
static const int print_perf = 0;

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
    int procid, nproc, i, j, my_nelem;
    int pollint = 0;
    double time;
    MPI_Win llist_win;
    llist_ptr_t head_ptr, tail_ptr;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &procid);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &llist_win);

    /* Process 0 creates the head node */
    if (procid == 0)
        head_ptr.disp = alloc_elem(procid, llist_win);

    /* Broadcast the head pointer to everyone */
    head_ptr.rank = 0;
    MPI_Bcast(&head_ptr.disp, 1, MPI_AINT, 0, MPI_COMM_WORLD);
    tail_ptr = head_ptr;

    /* All processes append NUM_ELEMS elements to the list; rank 0 has already
     * appended an element. */
    if (procid == 0)
        i = 1;
    else
        i = 0;
    my_nelem = NUM_ELEMS / nproc;
    if (procid < NUM_ELEMS % nproc)
        my_nelem++;

    MPI_Barrier(MPI_COMM_WORLD);
    time = MPI_Wtime();

    for (; i < my_nelem; i++) {
        llist_ptr_t new_elem_ptr;
        int success = 0;

        /* Create a new list element and register it with the window */
        new_elem_ptr.rank = procid;
        new_elem_ptr.disp = alloc_elem(procid, llist_win);

        /* Append the new node to the list.  This might take multiple attempts if
         * others have already appended and our tail pointer is stale. */
        do {
            int flag;

            /* The tail is at my left neighbor, append my element. */
            if (tail_ptr.rank == (procid + nproc - 1) % nproc) {
                if (verbose)
                    printf("%d: Appending to <%d, %p>\n", procid, tail_ptr.rank,
                           (void *) tail_ptr.disp);

                MPI_Win_lock(MPI_LOCK_EXCLUSIVE, tail_ptr.rank, 0, llist_win);
#if USE_ACC
                MPI_Accumulate(&new_elem_ptr, sizeof(llist_ptr_t), MPI_BYTE, tail_ptr.rank,
                               (MPI_Aint) & (((llist_elem_t *) tail_ptr.disp)->next),
                               sizeof(llist_ptr_t), MPI_BYTE, MPI_REPLACE, llist_win);
#else
                MPI_Put(&new_elem_ptr, sizeof(llist_ptr_t), MPI_BYTE, tail_ptr.rank,
                        (MPI_Aint) & (((llist_elem_t *) tail_ptr.disp)->next), sizeof(llist_ptr_t),
                        MPI_BYTE, llist_win);
#endif
                MPI_Win_unlock(tail_ptr.rank, llist_win);

                success = 1;
                tail_ptr = new_elem_ptr;
            }

            /* Otherwise, chase the tail. */
            else {
                llist_ptr_t next_tail_ptr;

                MPI_Win_lock(MPI_LOCK_EXCLUSIVE, tail_ptr.rank, 0, llist_win);
#if USE_ACC
                MPI_Get_accumulate(NULL, 0, MPI_DATATYPE_NULL, &next_tail_ptr,
                                   sizeof(llist_ptr_t), MPI_BYTE, tail_ptr.rank,
                                   (MPI_Aint) & (((llist_elem_t *) tail_ptr.disp)->next),
                                   sizeof(llist_ptr_t), MPI_BYTE, MPI_NO_OP, llist_win);
#else
                MPI_Get(&next_tail_ptr, sizeof(llist_ptr_t), MPI_BYTE, tail_ptr.rank,
                        (MPI_Aint) & (((llist_elem_t *) tail_ptr.disp)->next),
                        sizeof(llist_ptr_t), MPI_BYTE, llist_win);
#endif
                MPI_Win_unlock(tail_ptr.rank, llist_win);

                if (next_tail_ptr.rank != nil.rank) {
                    if (verbose)
                        printf("%d: Chasing to <%d, %p>\n", procid, next_tail_ptr.rank,
                               (void *) next_tail_ptr.disp);
                    tail_ptr = next_tail_ptr;
                    pollint = MAX(MIN_NPROBE, pollint / 2);
                }
                else {
                    for (j = 0; j < pollint; j++)
                        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag,
                                   MPI_STATUS_IGNORE);

                    pollint = MIN(MAX_NPROBE, pollint * 2);
                }
            }
        } while (!success);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    time = MPI_Wtime() - time;

    /* Traverse the list and verify that all processes inserted exactly the correct
     * number of elements. */
    if (procid == 0) {
        int errors = 0;
        int *counts, count = 0;

        counts = (int *) malloc(sizeof(int) * nproc);
        assert(counts != NULL);

        for (i = 0; i < nproc; i++)
            counts[i] = 0;

        tail_ptr = head_ptr;

        MPI_Win_lock_all(0, llist_win);

        /* Walk the list and tally up the number of elements inserted by each rank */
        while (tail_ptr.disp != nil.disp) {
            llist_elem_t elem;

            MPI_Get(&elem, sizeof(llist_elem_t), MPI_BYTE,
                    tail_ptr.rank, tail_ptr.disp, sizeof(llist_elem_t), MPI_BYTE, llist_win);

            MPI_Win_flush(tail_ptr.rank, llist_win);

            tail_ptr = elem.next;

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

        MPI_Win_unlock_all(llist_win);

        if (verbose)
            printf("\n\n");

        /* Verify the counts we collected */
        for (i = 0; i < nproc; i++) {
            int expected;

            expected = NUM_ELEMS / nproc;
            if (i < NUM_ELEMS % nproc)
                expected++;

            if (counts[i] != expected) {
                printf("Error: Rank %d inserted %d elements, expected %d\n", i, counts[i],
                       expected);
                errors++;
            }
        }

        printf("%s\n", errors == 0 ? " No Errors" : "FAIL");
        free(counts);
    }

    if (print_perf) {
        double max_time;

        MPI_Reduce(&time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        if (procid == 0) {
            printf("Total time = %0.2f sec, elem/sec = %0.2f, sec/elem = %0.2f usec\n", max_time,
                   NUM_ELEMS / max_time, max_time / NUM_ELEMS * 1.0e6);
        }
    }

    MPI_Win_free(&llist_win);

    /* Free all the elements in the list */
    for (; my_elems_count > 0; my_elems_count--)
        MPI_Free_mem(my_elems[my_elems_count - 1]);

    MPI_Finalize();
    return 0;
}
