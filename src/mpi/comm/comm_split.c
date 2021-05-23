/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

typedef struct splittype {
    int color, key;
} splittype;

/* Same as splittype but with an additional field to stabilize the qsort.  We
 * could just use one combined type, but using separate types simplifies the
 * allgather step. */
typedef struct sorttype {
    int color, key;
    int orig_idx;
} sorttype;

#if defined(HAVE_QSORT)
static int sorttype_compare(const void *v1, const void *v2)
{
    const sorttype *s1 = v1;
    const sorttype *s2 = v2;

    if (s1->key > s2->key)
        return 1;
    if (s1->key < s2->key)
        return -1;

    /* (s1->key == s2->key), maintain original order */
    if (s1->orig_idx > s2->orig_idx)
        return 1;
    else if (s1->orig_idx < s2->orig_idx)
        return -1;

    /* --BEGIN ERROR HANDLING-- */
    return 0;   /* should never happen */
    /* --END ERROR HANDLING-- */
}
#endif

/* Sort the entries in keytable into increasing order by key.  A stable
   sort should be used in case the key values are not unique. */
static void MPIU_Sort_inttable(sorttype * keytable, int size)
{
    sorttype tmp;
    int i, j;

#if defined(HAVE_QSORT)
    /* temporary switch for profiling performance differences */
    if (MPIR_CVAR_COMM_SPLIT_USE_QSORT) {
        /* qsort isn't a stable sort, so we have to enforce stability by keeping
         * track of the original indices */
        for (i = 0; i < size; ++i)
            keytable[i].orig_idx = i;
        qsort(keytable, size, sizeof(sorttype), &sorttype_compare);
    } else
#endif
    {
        /* --BEGIN USEREXTENSION-- */
        /* fall through to insertion sort if qsort is unavailable/disabled */
        for (i = 1; i < size; ++i) {
            tmp = keytable[i];
            j = i - 1;
            while (1) {
                if (keytable[j].key > tmp.key) {
                    keytable[j + 1] = keytable[j];
                    j = j - 1;
                    if (j < 0)
                        break;
                } else {
                    break;
                }
            }
            keytable[j + 1] = tmp;
        }
        /* --END USEREXTENSION-- */
    }
}

int MPIR_Comm_split_impl(MPIR_Comm * comm_ptr, int color, int key, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *local_comm_ptr;
    splittype *table, *remotetable = 0;
    sorttype *keytable, *remotekeytable = 0;
    int rank, size, remote_size, i, new_size, new_remote_size,
        first_entry = 0, first_remote_entry = 0, *last_ptr;
    int in_newcomm;             /* TRUE iff *newcomm should be populated */
    MPIR_Context_id_t new_context_id, remote_context_id;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm_map_t *mapper;
    MPIR_CHKLMEM_DECL(4);

    rank = comm_ptr->rank;
    size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;

    /* Step 1: Find out what color and keys all of the processes have */
    MPIR_CHKLMEM_MALLOC(table, splittype *, size * sizeof(splittype), mpi_errno,
                        "table", MPL_MEM_COMM);
    table[rank].color = color;
    table[rank].key = key;

    /* Get the communicator to use in collectives on the local group of
     * processes */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        if (!comm_ptr->local_comm) {
            MPII_Setup_intercomm_localcomm(comm_ptr);
        }
        local_comm_ptr = comm_ptr->local_comm;
    } else {
        local_comm_ptr = comm_ptr;
    }
    /* Gather information on the local group of processes */
    mpi_errno =
        MPIR_Allgather(MPI_IN_PLACE, 2, MPI_INT, table, 2, MPI_INT, local_comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    /* Step 2: How many processes have our same color? */
    new_size = 0;
    if (color != MPI_UNDEFINED) {
        /* Also replace the color value with the index of the *next* value
         * in this set.  The integer first_entry is the index of the
         * first element */
        last_ptr = &first_entry;
        for (i = 0; i < size; i++) {
            /* Replace color with the index in table of the next item
             * of the same color.  We use this to efficiently populate
             * the keyval table */
            if (table[i].color == color) {
                new_size++;
                *last_ptr = i;
                last_ptr = &table[i].color;
            }
        }
    }
    /* We don't need to set the last value to -1 because we loop through
     * the list for only the known size of the group */

    /* If we're an intercomm, we need to do the same thing for the remote
     * table, as we need to know the size of the remote group of the
     * same color before deciding to create the communicator */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        splittype mypair;
        /* For the remote group, the situation is more complicated.
         * We need to find the size of our "partner" group in the
         * remote comm.  The easiest way (in terms of code) is for
         * every process to essentially repeat the operation for the
         * local group - perform an (intercommunicator) all gather
         * of the color and rank information for the remote group.
         */
        MPIR_CHKLMEM_MALLOC(remotetable, splittype *,
                            remote_size * sizeof(splittype), mpi_errno,
                            "remotetable", MPL_MEM_COMM);
        /* This is an intercommunicator allgather */

        /* We must use a local splittype because we've already modified the
         * entries in table to indicate the location of the next rank of the
         * same color */
        mypair.color = color;
        mypair.key = key;
        mpi_errno = MPIR_Allgather(&mypair, 2, MPI_INT, remotetable, 2, MPI_INT,
                                   comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* Each process can now match its color with the entries in the table */
        new_remote_size = 0;
        last_ptr = &first_remote_entry;
        for (i = 0; i < remote_size; i++) {
            /* Replace color with the index in table of the next item
             * of the same color.  We use this to efficiently populate
             * the keyval table */
            if (remotetable[i].color == color) {
                new_remote_size++;
                *last_ptr = i;
                last_ptr = &remotetable[i].color;
            }
        }
        /* Note that it might find that there a now processes in the remote
         * group with the same color.  In that case, COMM_SPLIT will
         * return a null communicator */
    } else {
        /* Set the size of the remote group to the size of our group.
         * This simplifies the test below for intercomms with an empty remote
         * group (must create comm_null) */
        new_remote_size = new_size;
    }

    in_newcomm = (color != MPI_UNDEFINED && new_remote_size > 0);

    /* Step 3: Create the communicator */
    /* Collectively create a new context id.  The same context id will
     * be used by each (disjoint) collections of processes.  The
     * processes whose color is MPI_UNDEFINED will not influence the
     * resulting context id (by passing ignore_id==TRUE). */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single critical section */
    mpi_errno = MPIR_Get_contextid_sparse(local_comm_ptr, &new_context_id, !in_newcomm);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(new_context_id != 0);

    /* In the intercomm case, we need to exchange the context ids */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        if (comm_ptr->rank == 0) {
            mpi_errno = MPIC_Sendrecv(&new_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, 0,
                                      &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE,
                                      0, 0, comm_ptr, MPI_STATUS_IGNORE, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno =
                MPIR_Bcast(&remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, local_comm_ptr,
                           &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        } else {
            /* Broadcast to the other members of the local group */
            mpi_errno =
                MPIR_Bcast(&remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, local_comm_ptr,
                           &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        }
    }

    *newcomm_ptr = NULL;

    /* Now, create the new communicator structure if necessary */
    if (in_newcomm) {

        mpi_errno = MPIR_Comm_create(newcomm_ptr);
        if (mpi_errno)
            goto fn_fail;

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->local_size = new_size;
        (*newcomm_ptr)->comm_kind = comm_ptr->comm_kind;
        /* Other fields depend on whether this is an intercomm or intracomm */

        /* Step 4: Order the processes by their key values.  Sort the
         * list that is stored in table.  To simplify the sort, we
         * extract the table into a smaller array and sort that.
         * Also, store in the "color" entry the rank in the input communicator
         * of the entry. */
        MPIR_CHKLMEM_MALLOC(keytable, sorttype *, new_size * sizeof(sorttype),
                            mpi_errno, "keytable", MPL_MEM_COMM);
        for (i = 0; i < new_size; i++) {
            keytable[i].key = table[first_entry].key;
            keytable[i].color = first_entry;
            first_entry = table[first_entry].color;
        }

        /* sort key table.  The "color" entry is the rank of the corresponding
         * process in the input communicator */
        MPIU_Sort_inttable(keytable, new_size);

        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
            MPIR_CHKLMEM_MALLOC(remotekeytable, sorttype *,
                                new_remote_size * sizeof(sorttype),
                                mpi_errno, "remote keytable", MPL_MEM_COMM);
            for (i = 0; i < new_remote_size; i++) {
                remotekeytable[i].key = remotetable[first_remote_entry].key;
                remotekeytable[i].color = first_remote_entry;
                first_remote_entry = remotetable[first_remote_entry].color;
            }

            /* sort key table.  The "color" entry is the rank of the
             * corresponding process in the input communicator */
            MPIU_Sort_inttable(remotekeytable, new_remote_size);

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_size, MPIR_COMM_MAP_DIR__L2L, &mapper);

            for (i = 0; i < new_size; i++) {
                mapper->src_mapping[i] = keytable[i].color;
                if (keytable[i].color == comm_ptr->rank)
                    (*newcomm_ptr)->rank = i;
            }

            /* For the remote group, the situation is more complicated.
             * We need to find the size of our "partner" group in the
             * remote comm.  The easiest way (in terms of code) is for
             * every process to essentially repeat the operation for the
             * local group - perform an (intercommunicator) all gather
             * of the color and rank information for the remote group.
             */
            /* We apply the same sorting algorithm to the entries that we've
             * found to get the correct order of the entries.
             *
             * Note that if new_remote_size is 0 (no matching processes with
             * the same color in the remote group), then MPI_COMM_SPLIT
             * is required to return MPI_COMM_NULL instead of an intercomm
             * with an empty remote group. */

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_remote_size, MPIR_COMM_MAP_DIR__R2R, &mapper);

            for (i = 0; i < new_remote_size; i++)
                mapper->src_mapping[i] = remotekeytable[i].color;

            (*newcomm_ptr)->context_id = remote_context_id;
            (*newcomm_ptr)->remote_size = new_remote_size;
            (*newcomm_ptr)->local_comm = 0;
            (*newcomm_ptr)->is_low_group = comm_ptr->is_low_group;

        } else {
            /* INTRA Communicator */
            (*newcomm_ptr)->context_id = (*newcomm_ptr)->recvcontext_id;
            (*newcomm_ptr)->remote_size = new_size;

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_size, MPIR_COMM_MAP_DIR__L2L, &mapper);

            for (i = 0; i < new_size; i++) {
                mapper->src_mapping[i] = keytable[i].color;
                if (keytable[i].color == comm_ptr->rank)
                    (*newcomm_ptr)->rank = i;
            }
        }

        /* Inherit the error handler (if any) */
        MPID_THREAD_CS_ENTER(POBJ, comm_ptr->mutex);
        MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);
        (*newcomm_ptr)->errhandler = comm_ptr->errhandler;
        if (comm_ptr->errhandler) {
            MPIR_Errhandler_add_ref(comm_ptr->errhandler);
        }
        MPID_THREAD_CS_EXIT(POBJ, comm_ptr->mutex);
        MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);

        (*newcomm_ptr)->tainted = comm_ptr->tainted;
        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
