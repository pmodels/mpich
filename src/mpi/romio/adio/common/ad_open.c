/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "adio_extern.h"
#include "adio_cb_config_list.h"

#include "mpio.h"

#define ROMIO_TOTAL_LOCAL_AGGREGATOR_DEFAULT 512

static int is_aggregator(int rank, ADIO_File fd);
static int uses_generic_read(ADIO_File fd);
static int uses_generic_write(ADIO_File fd);
static int build_cb_config_list(ADIO_File fd,
                                MPI_Comm orig_comm, MPI_Comm comm,
                                int rank, int procs, int *error_code);

static int myCompareString (const void * a, const void * b)
{
    return strcmp (*(char **) a, *(char **) b);
}

void stringSort(char *arr[], int n)
{
    qsort (arr, n, sizeof (char *), myCompareString);
}


/*
  This function reorder the global aggregators in a round robin fashion.  

  Input:
       1. process_node_list: mapping from process to the node index it belongs.
       2. cb_nodes: global aggregator size.
       3. nrecvs: number of physical nodes.
  Output:
       1. ranklist: global aggregators.
       2. info: MPI info in the file descriptor. We modify it just for consistent printout.
*/
int reorder_ranklist(int *process_node_list, int *ranklist, int cb_nodes, int nrecvs, MPI_Info info){
    int i, j, incr, remain;
    int **node_ranks = (int**) ADIOI_Malloc(sizeof(int*)*nrecvs);
    int *node_size = (int*) ADIOI_Calloc(nrecvs,sizeof(int));
    int *node_index = (int*) ADIOI_Calloc(nrecvs,sizeof(int));
    char *value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));
    char *p;

    for ( i = 0; i < cb_nodes; i++ ){
        node_size[process_node_list[ranklist[i]]]++;
    }
    for ( i = 0; i < nrecvs; i++ ){
        node_ranks[i] = (int*) ADIOI_Malloc(sizeof(int)*(node_size[i]+1));
    }
    for ( i = 0; i < cb_nodes; i++ ){
        j = process_node_list[ranklist[i]];
        node_ranks[j][node_index[j]] = ranklist[i];
        node_index[j]++;
    }
    memset( node_index, 0, sizeof(int) * nrecvs );
    j = 0;
    for ( i = 0; i < cb_nodes; i++ ){
        while ( node_index[j] == node_size[j] ) {
            j++;
            if ( j >= nrecvs ) {
                j=0;
            }
        }
        ranklist[i] = node_ranks[j][node_index[j]];
        node_index[j]++;
        j++;
        if ( j >= nrecvs ) {
            j=0;
        }
    }
    p = value;
    /* the (by MPI rank) list of aggregators can be larger than
     * MPI_MAX_INFO_VAL, so we will simply truncate when we reach capacity. I
     * wasn't clever enough to figure out how to rewind and put '...' at the
     * end in the truncate case */
    for (i = 0; i < cb_nodes; i++) {

        remain = (MPI_MAX_INFO_VAL) - (p - value);
        incr = MPL_snprintf(p, remain, "%d ", ranklist[i]);
        if (incr >= remain)
            break;
        p += incr;
    }
    ADIOI_Info_set(info, "romio_aggregator_list", value);
    ADIOI_Free(value);


    for ( i = 0; i < nrecvs; i++ ){
        ADIOI_Free(node_ranks[i]);
    }
    ADIOI_Free(node_ranks);
    ADIOI_Free(node_size);
    ADIOI_Free(node_index);
    return 0;
}

/*
  Input:
       1. rank: process rank with respect to communicator comm.
       2. nprocs: total number of processes in the communicator comm.
       3. comm: a communicator comm that contains this process.
       4. orig_comm: For some reason, ADIOI_cb_gather_name_array requires an extra communicator, we just give it.
  Output:
       1. nrecvs : The number of compute nodes.
       2. process_node_list : An array (size nprocs) that maps a process to a node (the node index correspond to the order of global receivers)
*/
int gather_node_information(int rank, int nprocs,int *nrecvs, int **process_node_list, MPI_Comm comm, MPI_Comm orig_comm){
    ADIO_cb_name_array array;
    char **global_process_names, **unique_nodes;
    int i, j;
    /* cb_config_list should have called this, we can just get the names stored in the communicator easily. */
    ADIOI_cb_gather_name_array(orig_comm, comm, &array);
    if ( rank == 0 ) {
        global_process_names = (char**) ADIOI_Malloc(sizeof(char*) * nprocs);
        memcpy(global_process_names, array->names, sizeof(char*) * nprocs);
        /* Sort string names in order for counting the unique number of strings. */
        stringSort(global_process_names,nprocs);
        /* Need to figure out how many nodes we have.
         * We do subtraction here. */
        nrecvs[0] = nprocs;
        /* Figure out total number of nodes and store into nrecvs */
        for (i = 1; i < nprocs; i++) {
            if (strcmp(global_process_names[i-1], global_process_names[i]) == 0) {
                nrecvs[0]--;
            }
        }
        /* Figure out the name of all nodes (stored in unique nodes referencing the global process name) */
        unique_nodes = (char**) ADIOI_Malloc(sizeof(char*) * nrecvs[0]);
        unique_nodes[0] = global_process_names[0];
        j = 1;
        for (i=1; i<nprocs; i++){
            if (strcmp(global_process_names[i-1],global_process_names[i])!=0){
                unique_nodes[j] = global_process_names[i];
                j++;
            }
        }
        process_node_list[0] = (int*) ADIOI_Malloc(sizeof(int) * (nprocs + 1));
        process_node_list[0][nprocs] = nrecvs[0];
        /* Figure out which process is at with node using their names */
        for ( j = 0; j < nprocs; j++ ) {
            for ( i = 0; i < nrecvs[0]; i++ ) {
                if (strcmp(array->names[j], unique_nodes[i]) == 0) {
                    process_node_list[0][j] = i;
                    break;
                }
            }
        }
        ADIOI_Free(global_process_names);
        ADIOI_Free(unique_nodes);
    }
    if (rank > 0) {
        process_node_list[0] = (int*) ADIOI_Malloc(sizeof(int) * (nprocs + 1));
    }
    MPI_Bcast( process_node_list[0], nprocs + 1, MPI_INT, 0, comm );
    /* The last (and extra) integer from the the process_node_list is the total number of nodes.
     * It is broadcasted along with the process node list from rank 0. */
    nrecvs[0] = process_node_list[0][nprocs];
    return 0;
}

/*
  Input:
       1. rank: process rank with respect to communicator comm.
       2. process_node_list : An array (size nprocs) that maps a process to a node (the node index correspond to the order of global receivers)
       3. nrecvs : The number of compute nodes.
       4. global_aggregator_size: The value of cb_nodes, which is the global aggregator size.
       5. global_aggregators: Original list of global aggregators
  Output:
       1. global_aggregators_new: Reordered global aggregators.
*/
int spreadout_global_aggregators(int rank, int *process_node_list, int nprocs, int nrecvs, int global_aggregator_size, int* global_aggregators, int** global_aggregators_new){
    int i, j, k, local_node_process_size, local_node_aggregator_size, remainder, temp1, temp2;
    int* check_global_aggregators = (int*) ADIOI_Calloc(nprocs,sizeof(int));
    int* temp_local_ranks = (int*) ADIOI_Malloc(sizeof(int)*nprocs);
    *global_aggregators_new = (int*) ADIOI_Malloc(sizeof(int)*global_aggregator_size);
    /* Check if a process is an aggregator*/
    for ( i = 0; i < nprocs; ++i ){
        for ( j = 0; j < global_aggregator_size; ++j ){
            if ( i == global_aggregators[j] ){
                check_global_aggregators[i] = 1;
                break;
            }
        }
    }
    k = 0;
    /* For every node */
    for ( i = 0; i < nrecvs; ++i ){
        local_node_process_size = 0;
        local_node_aggregator_size = 0;
        /* Get process on the ith node and identify aggregators */
        for ( j = 0; j < nprocs; ++j ){
            if ( process_node_list[j] == i ) {
                temp_local_ranks[local_node_process_size] = j;
                local_node_process_size++;
                if ( check_global_aggregators[j] ) {
                    local_node_aggregator_size++;
                }
            }
        }
        /* Spreadout evenly */
        if (local_node_aggregator_size) {
            remainder = local_node_process_size % local_node_aggregator_size;
            temp1 = (local_node_process_size + local_node_aggregator_size - 1) / local_node_aggregator_size;
            temp2 = local_node_process_size / local_node_aggregator_size;
            for ( j = 0; j < local_node_aggregator_size; ++j ){
                if( j < remainder ){
                    global_aggregators_new[0][k] = temp_local_ranks[temp1 * j];
                } else{
                    global_aggregators_new[0][k] = temp_local_ranks[temp1 * remainder + temp2 * ( j - remainder )];
                }
                k++;
            }
        }
    }
    ADIOI_Free(check_global_aggregators);
    ADIOI_Free(temp_local_ranks);
    return 0;
}


/*
  This function output meta information for local aggregator ranks with respect to process assignments at different nodes. The size of local aggregators respects to the co parameter. However, if co is unreasonably large (greater than local process size, it is forced to be local process size.)
  Input:
     1. rank: rank of this process.
     2. process_node_list: a mapping from a process to the node index it belongs to. Node index is determined by the order of its lowest local rank.
     3. nprocs: total number of process.
     4. nrecvs: total number of nodes.
     5. global_aggregator_size: size of global_aggregators.
     6. global_aggregators: an array of all aggregators.
     7. co: number of local aggregators per node.
     8 mode: 0: local aggregators are evenly spreadout on a node. 1: local aggregators try to be a superset/subset of global aggregators
  Output:
     1. is_aggregator_new: if this process is a local aggregator.
     2. local_aggregator_size: length of local_aggregators (the size of local aggergator list)
     3. local_aggregators: an array of local aggregators (the local aggregator list)
     4. nprocs_aggregator: length of aggregator_local_ranks (only significant for local aggregators)
     5. aggregator_local_ranks: an array that tells what ranks this aggregator is responsible for performing proxy communication. (only significant for local aggregators)
     6. local_aggregator_domain: aggregator_local_ranks array for all local aggregators. (only significant for global aggregators)
     7. local_aggregator_domain_size: an array for the size of individual array of local_aggregator_domain. (only significant for local aggregators)
     8. my_local_aggregator: which local aggregator I will send to.
*/
int aggregator_meta_information(int rank, int *process_node_list, int nprocs, int nrecvs, int global_aggregator_size, int *global_aggregators, int co, int* is_aggregator_new, int* local_aggregator_size, int **local_aggregators, int* nprocs_aggregator, int **aggregator_local_ranks, int ***local_aggregator_domain, int **local_aggregator_domain_size, int *my_local_aggregator, int mode){
    int i, j, k, local_node_aggregator_size, local_node_process_size, *local_node_aggregators, *temp_local_ranks, *check_local_aggregators, *check_global_aggregators, *process_aggregator_list, temp1, temp2, temp3, base, remainder, co2, test, *ptr;
    process_aggregator_list = (int*) ADIOI_Malloc(sizeof(int)*nprocs);
    local_node_aggregators = (int*) ADIOI_Malloc(sizeof(int)*nprocs);
    temp_local_ranks = (int*) ADIOI_Malloc(sizeof(int)*nprocs);
    check_global_aggregators = (int*) ADIOI_Calloc(nprocs,sizeof(int));
    check_local_aggregators = (int*) ADIOI_Calloc(nprocs,sizeof(int));
    nprocs_aggregator[0] = 0;
    local_aggregator_size[0] = 0;
    is_aggregator_new[0] = 0;
    /* Check if a process is an aggregator*/
    for ( i = 0; i < nprocs; ++i ){
        for ( j = 0; j < global_aggregator_size; ++j ){
            if ( i == global_aggregators[j] ){
                check_global_aggregators[i] = 1;
                break;
            }
        }
    }
    for ( i = 0; i < nrecvs; ++i ){
        local_node_process_size = 0;
        /* Get process on the ith node and identify aggregators*/
        for ( j = 0; j < nprocs; ++j ){
            if ( process_node_list[j] == i ) {
                local_node_process_size++;
            }
        }
        if ( co > local_node_process_size ){
            local_aggregator_size[0] += local_node_process_size;
        }else{
            local_aggregator_size[0] += co;
        }
    }
    local_aggregators[0] = (int*) ADIOI_Malloc(local_aggregator_size[0]*sizeof(int));
    ptr = local_aggregators[0];
    /* For every node*/
    for ( i = 0; i < nrecvs; ++i ){
        /* How many processes per node? */
        local_node_process_size = 0;
        /* This variable is the number of local aggregators. */
        local_node_aggregator_size = 0;
        /* Get process on the ith node and identify aggregators*/
        for ( j = 0; j < nprocs; ++j ){
            if ( process_node_list[j] == i ) {
                temp_local_ranks[local_node_process_size] = j;
                local_node_process_size++;
                /* This condition is not necessary if we are using mode 0*/
                if ( check_global_aggregators[j] ) {
                    local_node_aggregators[local_node_aggregator_size] = j;
                    local_node_aggregator_size++;
                }
            }
        }
        /* Work out maximum number of intranode aggregator per node*/
        if( co > local_node_process_size){
            co2 = local_node_process_size;
        } else{
            co2 = co;
        }
        if (mode){
            /*Mode 1: Build local aggregators on top of global aggregators*/
            if ( co2 > local_node_aggregator_size ){
                temp2 = local_node_aggregator_size;
                /* Go through all local processes. fill the local aggregator array up to the number of element co2 */
                for ( j = 0; j < local_node_process_size; j++ ){
                    /* Make sure that the added ranks do not repeat with the existing aggregator ranks*/
                    test = 1;
                    for ( k = 0; k < temp2; k++ ){
                        if( temp_local_ranks[j] == local_node_aggregators[k]){
                            test = 0;
                            break;
                        }
                    }
                    if (test){
                        local_node_aggregators[local_node_aggregator_size] = temp_local_ranks[j];
                        local_node_aggregator_size++;
                    }
                    if (local_node_aggregator_size == co2){
                        break;
                    }
                }
            } else{
                local_node_aggregator_size = co2;
            }
        } else{
            /* Mode 0: Spread out local aggregators across the local node process list. */
            local_node_aggregator_size = co2;
            remainder = local_node_process_size % local_node_aggregator_size;
            temp1 = (local_node_process_size + local_node_aggregator_size - 1) / local_node_aggregator_size;
            temp2 = local_node_process_size / local_node_aggregator_size;
            for ( j = 0; j < local_node_aggregator_size; ++j ){
                if( j < remainder ){
                    local_node_aggregators[j] = temp_local_ranks[temp1 * j];
                } else{
                    local_node_aggregators[j] = temp_local_ranks[temp1 * remainder + temp2 * ( j - remainder )];
                }
            }
        }

        memcpy(ptr, local_node_aggregators, sizeof(int) * local_node_aggregator_size);
        ptr += local_node_aggregator_size;
        for ( j = 0; j < local_node_aggregator_size; ++j ){
            check_local_aggregators[local_node_aggregators[j]] = 1;
            if (rank == local_node_aggregators[j]){
                is_aggregator_new[0] = 1;
            }
        }
        if ( local_node_aggregator_size ){
            /* We bind non-aggregators to local aggregators with the following rules. 
               1. Every local aggregator is responsible for either temp1 or temp2 number of non-aggregators.
               2. A local aggregator is responsible for itself.
            */
            remainder = local_node_process_size % local_node_aggregator_size;
            temp1 = (local_node_process_size + local_node_aggregator_size - 1) / local_node_aggregator_size;
            temp2 = local_node_process_size / local_node_aggregator_size;
            /* Search index*/
            base = 0;
            for ( j = 0; j < local_node_aggregator_size; ++j ){
                test = 1;
                /* Determine comm group size. */
                if ( j < remainder ){
                    temp3 = temp1;
                } else{
                    temp3 = temp2;
                }
                /*
                   The algorithm is as the following.
                   Scan the local process list, if local process is a local aggregator and it is not currently iterated (local_node_aggregators[j]), simply jump. This local aggregator is handled in another j loop (by searching or passing)
                   If it is current local aggregator, just add to the list. For non-aggregators, just keep adding them into the list until the entire group is filled.
                   When the group is one element away from being filled and the current local aggregator has not been added. We directly add it.
                */
                for ( k = 0; k < temp3; ++k ){
                    if ( k == temp3 -1 && test ){
                        process_aggregator_list[ local_node_aggregators[j] ] = local_node_aggregators[j];
                        break;
                    }
                    while ( (check_local_aggregators[temp_local_ranks[base]] && temp_local_ranks[base] != local_node_aggregators[j]) ){
                        base++;
                    }
                    if (check_local_aggregators[temp_local_ranks[base]]){
                        test = 0;
                    }
                    process_aggregator_list[ temp_local_ranks[base] ] = local_node_aggregators[j];
                    base++;
                }
            }
        }
    }
    *my_local_aggregator = process_aggregator_list[rank];
    /* if this process is a local aggregator, work out the process ranks that it is responsible for.*/
    if (is_aggregator_new[0]){
        for ( i = 0; i < nprocs; ++i ){
            if (process_aggregator_list[i] == rank ){
                nprocs_aggregator[0]++;
            }
        }
        aggregator_local_ranks[0] = (int*) ADIOI_Malloc(nprocs_aggregator[0]*sizeof(int));
        j = 0;
        for ( i = 0; i < nprocs; ++i ){
            if (process_aggregator_list[i] == rank ){
                aggregator_local_ranks[0][j] = i;
                j++;
            }
        }
    }

    /* Get something ready for global aggregators*/
    if (check_global_aggregators[rank]) {
        /* Global aggregators must know which process is handled by which local aggregator in order to unpack the data */
        *local_aggregator_domain = (int**) ADIOI_Malloc(local_aggregator_size[0]*sizeof(int*));
        *local_aggregator_domain_size = (int*) ADIOI_Malloc(local_aggregator_size[0]*sizeof(int));
        memset(temp_local_ranks, 0, sizeof(int) * nprocs);
        local_aggregator_domain[0][0] = (int*) ADIOI_Malloc(nprocs * sizeof(int));
        for ( i = 0; i < nprocs; ++i) {
            temp_local_ranks[process_aggregator_list[i]]++;
        }
        for ( i = 0; i < local_aggregator_size[0]; ++i ) {
            if (i) {
                local_aggregator_domain[0][i] = local_aggregator_domain[0][i - 1] + local_aggregator_domain_size[0][i - 1];
            }
            local_aggregator_domain_size[0][i] = temp_local_ranks[local_aggregators[0][i]];
            j = 0;
            for ( k = 0; k < nprocs; ++k ) {
                if (process_aggregator_list[k] == local_aggregators[0][i] ){
                    local_aggregator_domain[0][i][j] = k;
                    j++;
                }
            }
        }
    }
    ADIOI_Free(process_aggregator_list);
    ADIOI_Free(local_node_aggregators);
    ADIOI_Free(temp_local_ranks);
    ADIOI_Free(check_local_aggregators);
    ADIOI_Free(check_global_aggregators);
    return 0;
}

int set_tam_hints(ADIO_File fd, int rank, int *process_node_list, int nrecvs, int nprocs){
    char *p;
    int *global_aggregators_new, i;
    int co;

    /*TAM rank list preparation*/
    p=getenv("ROMIO_LOCAL_AGGREGATOR_CO");
    if (p==NULL){
        co = (ROMIO_TOTAL_LOCAL_AGGREGATOR_DEFAULT + nrecvs - 1) / nrecvs;
    } else{
        co = atoi(p);
    }
    /* We spreadout the global aggregators within a node (keeping the same number of global aggregators per node)*/

    spreadout_global_aggregators(rank, process_node_list, nprocs, nrecvs, fd->hints->cb_nodes, fd->hints->ranklist, &global_aggregators_new);
    if (fd->hints->cb_nodes > 0){
        ADIOI_Free(fd->hints->ranklist);
    }

    /* Need to update some global variables.*/

    fd->hints->ranklist = global_aggregators_new;
    fd->my_cb_nodes_index = -2;
    fd->is_agg = is_aggregator(rank, fd);

    /* Reorder the array of global aggregators according to node robin style*/
    reorder_ranklist(process_node_list, fd->hints->ranklist, fd->hints->cb_nodes, nrecvs, fd->info);

    /* Construct arrays required by TAM */
    aggregator_meta_information(rank, process_node_list, nprocs, nrecvs, fd->hints->cb_nodes, fd->hints->ranklist, co, &(fd->is_local_aggregator), &(fd->local_aggregator_size), &(fd->local_aggregators), &(fd->nprocs_aggregator), &(fd->aggregator_local_ranks), &(fd->local_aggregator_domain), &(fd->local_aggregator_domain_size), &(fd->my_local_aggregator), 0);

    /* Prepare TAM temporary variables. (so they are used later without repeated mallocs.) */
    fd->local_buf = NULL;
    fd->local_buf_size = 0;
    fd->cb_send_size = (int*) ADIOI_Malloc(sizeof(int) * fd->hints->cb_nodes);

    if (fd->is_local_aggregator||fd->is_agg){
        i = 0;
        if (fd->is_agg){
            /* Workout my global aggregator index, works the same as cb_node_index */
            for ( i = 0; i < fd->hints->cb_nodes; ++i ) {
                if (fd->hints->ranklist[i] == rank) {
                    fd->global_aggregator_index = i;
                    break;
                }
            }
            fd->global_recv_size = (MPI_Aint*) ADIOI_Malloc(sizeof(MPI_Aint) * fd->local_aggregator_size);
            i += fd->local_aggregator_size;
        }
        if (fd->is_local_aggregator){
            /* Used when local aggregators send data to global aggregators 
             * Wrap data to he same destination all at once. */
            fd->array_of_displacements = (MPI_Aint*) ADIOI_Malloc(fd->nprocs_aggregator * sizeof(MPI_Aint));
            fd->array_of_blocklengths = (int*) ADIOI_Malloc(fd->nprocs_aggregator * sizeof(int));
            fd->new_types = (MPI_Datatype*) ADIOI_Malloc(fd->hints->cb_nodes * sizeof(MPI_Datatype));
            /* Used to store prefix-sum (inclusive) of local_send_size, we want to be extra careful to not to overflow integers. */
            fd->local_lens = (MPI_Aint*) ADIOI_Malloc(sizeof(MPI_Aint) * nprocs * fd->nprocs_aggregator);
            fd->local_send_size = (int*) ADIOI_Malloc(sizeof(int) * fd->hints->cb_nodes * fd->nprocs_aggregator);
            i += fd->hints->cb_nodes;
        }
        /* status and request variables used by TAM, we malloc once.
         * The size is either maximum inter-node or intra-node communication size, depending on which is larger. */
        if (fd->nprocs_aggregator > i && fd->is_local_aggregator){
            fd->req = (MPI_Request *) ADIOI_Malloc((fd->nprocs_aggregator + 1) * sizeof(MPI_Request));
            fd->sts = (MPI_Status *) ADIOI_Malloc((fd->nprocs_aggregator + 1) * sizeof(MPI_Status));
        } else{
            fd->req = (MPI_Request *) ADIOI_Malloc((i + 1) * sizeof(MPI_Request));
            fd->sts = (MPI_Status *) ADIOI_Malloc((i + 1) * sizeof(MPI_Status));
        }
    } else{
        fd->req = (MPI_Request *) ADIOI_Malloc(sizeof(MPI_Request));
        fd->sts = (MPI_Status *) ADIOI_Malloc(sizeof(MPI_Status));
    }
    return 0;
}

int additional_hint_print(ADIO_File fd, int nrecvs, int nprocs){
    char *p;
    int i, co;
    /*TAM hints print out, we print a full list of global aggregators again here.*/

    p=getenv("ROMIO_LOCAL_AGGREGATOR_CO");
    if (p==NULL){
        co = (ROMIO_TOTAL_LOCAL_AGGREGATOR_DEFAULT + nrecvs - 1) / nrecvs;
    } else{
        co = atoi(p);
    }
    printf("key = %-25s value = ", "global aggregators");
    for ( i = 0; i < fd->hints->cb_nodes; i++ ){
        printf("%d ",fd->hints->ranklist[i]);
    }
    printf("\n");
    printf("key = %-25s value = %-10d\n", "number of nodes", nrecvs);
    printf("key = %-25s value = %-10d\n", "ROMIO_LOCAL_AGGREGATOR_CO", co);
    printf("key = %-25s value = ", "local aggregators");
    for ( i = 0; i < fd->local_aggregator_size; i++ ){
        printf("%d ",fd->local_aggregators[i]);
    }
    printf("\n");
    return 0;
}

ADIO_File ADIO_Open(MPI_Comm orig_comm,
                    MPI_Comm comm, const char *filename, int file_system,
                    ADIOI_Fns * ops,
                    int access_mode, ADIO_Offset disp, MPI_Datatype etype,
                    MPI_Datatype filetype, MPI_Info info, int perm, int *error_code)
{
    MPI_File mpi_fh;
    ADIO_File fd;
    int err, rank, procs;
    static char myname[] = "ADIO_OPEN";
    int max_error_code;
    MPI_Info dupinfo;
    int syshints_processed, can_skip;
    char *p;
    int *process_node_list, i, nrecvs;
    char key_val[MPI_MAX_INFO_VAL + 1];

    *error_code = MPI_SUCCESS;

    /* obtain MPI_File handle */
    mpi_fh = MPIO_File_create(sizeof(struct ADIOI_FileD));
    if (mpi_fh == MPI_FILE_NULL) {
        fd = ADIO_FILE_NULL;
        *error_code = MPIO_Err_create_code(*error_code,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__, MPI_ERR_OTHER, "**nomem2", 0);
        goto fn_exit;
    }
    fd = MPIO_File_resolve(mpi_fh);

    fd->cookie = ADIOI_FILE_COOKIE;
    fd->fp_ind = disp;
    fd->fp_sys_posn = 0;
    fd->comm = comm;    /* dup'ed in MPI_File_open */
    fd->filename = ADIOI_Strdup(filename);
    fd->file_system = file_system;
    fd->fs_ptr = NULL;

    fd->fns = ops;

    fd->disp = disp;
    fd->split_coll_count = 0;
    fd->shared_fp_fd = ADIO_FILE_NULL;
    fd->atomicity = 0;
    fd->etype = etype;  /* MPI_BYTE by default */
    fd->filetype = filetype;    /* MPI_BYTE by default */
    fd->etype_size = 1; /* default etype is MPI_BYTE */

    fd->file_realm_st_offs = NULL;
    fd->file_realm_types = NULL;

    fd->perm = perm;

    fd->async_count = 0;

    fd->fortran_handle = -1;

    fd->err_handler = ADIOI_DFLT_ERR_HANDLER;

    fd->io_buf_window = MPI_WIN_NULL;
    fd->io_buf_put_amounts_window = MPI_WIN_NULL;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);
/* create and initialize info object */
    fd->hints = (ADIOI_Hints *) ADIOI_Calloc(1, sizeof(struct ADIOI_Hints_struct));
    if (fd->hints == NULL) {
        *error_code = MPIO_Err_create_code(*error_code,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__, MPI_ERR_OTHER, "**nomem2", 0);
        goto fn_exit;
    }
    fd->hints->cb_config_list = NULL;
    fd->hints->ranklist = NULL;
    fd->hints->initialized = 0;
    fd->info = MPI_INFO_NULL;

    /* move system-wide hint processing *back* into open, but this time the
     * hintfile reader will do a scalable read-and-broadcast.  The global
     * ADIOI_syshints will get initialized at first open.  subsequent open
     * calls will just use result from first open.
     *
     * We have two goals here:
     * 1: avoid processing the hintfile multiple times
     * 2: have all processes participate in hintfile processing (so we can read-and-broadcast)
     *
     * a code might do an "initialize from 0", so we can only skip hint
     * processing once everyone has particpiated in hint processing */
    if (ADIOI_syshints == MPI_INFO_NULL)
        syshints_processed = 0;
    else
        syshints_processed = 1;

    gather_node_information(rank, procs, &nrecvs, &process_node_list, fd->comm, orig_comm);

    MPI_Allreduce(&syshints_processed, &can_skip, 1, MPI_INT, MPI_MIN, fd->comm);
    if (!can_skip) {
        if (ADIOI_syshints == MPI_INFO_NULL)
            MPI_Info_create(&ADIOI_syshints);
        ADIOI_process_system_hints(fd, ADIOI_syshints);
    }
    /* The number of physical node is used to determine cb_nodes for lustre case */
    MPL_snprintf(key_val, MPI_MAX_INFO_VAL + 1, "%d", nrecvs);
    ADIOI_Info_set(ADIOI_syshints, "number_of_nodes", key_val);
    /* Environmental setting for testing purpose with highest priority.
     * This is convenient for usage.n*/
    p=getenv("ROMIO_cb_nodes");
    if (p!=NULL){
        ADIOI_Info_set(ADIOI_syshints, "cb_nodes", p);
    }

    p=getenv("ROMIO_cb_config_list");
    if (p!=NULL){
        ADIOI_Info_set(ADIOI_syshints, "cb_config_list", p);
    }
    /* For Lustre hints related to global aggregators, we need to actually open the file before the colletive open function.
     * We use the original access mode. If it does not work, we just forget about the hints. */
    fd->access_mode = access_mode;

    ADIOI_incorporate_system_hints(info, ADIOI_syshints, &dupinfo);
    ADIO_SetInfo(fd, dupinfo, &err);
    if (dupinfo != MPI_INFO_NULL) {
        *error_code = MPI_Info_free(&dupinfo);
        if (*error_code != MPI_SUCCESS)
            goto fn_exit;
    }
    ADIOI_Info_set(fd->info, "romio_filesystem_type", fd->fns->fsname);

    /* Instead of repeatedly allocating this buffer in collective read/write,
     * allocating up-front might make memory management on small platforms
     * (e.g. Blue Gene) more efficent */

    fd->io_buf = ADIOI_Malloc(fd->hints->cb_buffer_size);
    /* deferred open:
     * we can only do this optimization if 'fd->hints->deferred_open' is set
     * (which means the user hinted 'no_indep_rw' and collective buffering).
     * Furthermore, we only do this if our collective read/write routines use
     * our generic function, and not an fs-specific routine (we can defer opens
     * only if we use our aggreagation code). */
    if (fd->hints->deferred_open && !(uses_generic_read(fd)
                                      && uses_generic_write(fd))) {
        fd->hints->deferred_open = 0;
    }
    if (ADIO_Feature(fd, ADIO_SCALABLE_OPEN))
        /* disable deferred open on these fs so that scalable broadcast
         * will always use the propper communicator */
        fd->hints->deferred_open = 0;


    /* on BlueGene, the cb_config_list is built when hints are processed. No
     * one else does that right now */
    if (fd->hints->ranklist == NULL) {
        build_cb_config_list(fd, orig_comm, comm, rank, procs, error_code);
        if (*error_code != MPI_SUCCESS)
            goto fn_exit;
    }
    fd->is_open = 0;
    fd->my_cb_nodes_index = -2;
    fd->is_agg = is_aggregator(rank, fd);

    /* Setup global aggregators and local aggregators.
     * A lot of things is done to global aggregator list, but the number should still be the same as current one.
     */
    set_tam_hints(fd, rank, process_node_list, nrecvs, procs);

    /* Now we do not need the process_node_list anymore, let us free it. */
    ADIOI_Free(process_node_list);

    /* deferred open used to split the communicator to create an "aggregator
     * communicator", but we only used it as a way to indicate that deferred
     * open happened.  fd->is_open and fd->is_agg are sufficient */

    /* actual opens start here */
    /* generic open: one process opens to create the file, all others open */
    /* nfs open: everybody opens or else you'll end up with "file not found"
     * due to stupid nfs consistency semantics */
    /* scalable open: one process opens and broadcasts results to everyone */

    ADIOI_OpenColl(fd, rank, access_mode, error_code);

    /* deferred open consideration: if an independent process lied about
     * "no_indep_rw" and opens the file later (example: HDF5 uses independent
     * i/o for metadata), that deferred open will use the access_mode provided
     * by the user.  CREATE|EXCL only makes sense here -- exclusive access in
     * the deferred open case is going to fail and surprise the user.  Turn off
     * the excl amode bit. Save user's ammode for MPI_FILE_GET_AMODE */
    fd->orig_access_mode = access_mode;
    if (fd->access_mode & ADIO_EXCL)
        fd->access_mode ^= ADIO_EXCL;


    /* for debugging, it can be helpful to see the hints selected. Some file
     * systes set up the hints in the open call (e.g. lustre) */
    p = getenv("ROMIO_PRINT_HINTS");
    if (rank == 0 && p != NULL) {
        ADIOI_Info_print_keyvals(fd->info);
        additional_hint_print(fd, nrecvs, procs);
    }

  fn_exit:
    MPI_Allreduce(error_code, &max_error_code, 1, MPI_INT, MPI_MAX, comm);
    if (max_error_code != MPI_SUCCESS) {

        /* If the file was successfully opened, close it */
        if (*error_code == MPI_SUCCESS) {

            /* in the deferred open case, only those who have actually
             * opened the file should close it */
            if (fd->hints->deferred_open) {
                if (fd->is_agg) {
                    (*(fd->fns->ADIOI_xxx_Close)) (fd, error_code);
                }
            } else {
                (*(fd->fns->ADIOI_xxx_Close)) (fd, error_code);
            }
        }
        ADIOI_Free(fd->filename);
        ADIOI_Free(fd->hints->ranklist);
        if (fd->hints->cb_config_list != NULL)
            ADIOI_Free(fd->hints->cb_config_list);
        ADIOI_Free(fd->hints);
        if (fd->info != MPI_INFO_NULL)
            MPI_Info_free(&(fd->info));
        ADIOI_Free(fd->io_buf);
        ADIOI_Free(fd);
        fd = ADIO_FILE_NULL;
        if (*error_code == MPI_SUCCESS) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE, myname,
                                               __LINE__, MPI_ERR_IO, "**oremote_fail", 0);
        }
    }

    return fd;
}

/* a simple linear search. possible enancement: add a my_cb_nodes_index member
 * (index into cb_nodes, else -1 if not aggregator) for faster lookups
 *
 * fd->hints->cb_nodes is the number of aggregators
 * fd->hints->ranklist[] is an array of the ranks of aggregators
 *
 * might want to move this to adio/common/cb_config_list.c
 */
int is_aggregator(int rank, ADIO_File fd)
{
    int i;

    if (fd->my_cb_nodes_index == -2) {
        for (i = 0; i < fd->hints->cb_nodes; i++) {
            if (rank == fd->hints->ranklist[i]) {
                fd->my_cb_nodes_index = i;
                return 1;
            }
        }
        fd->my_cb_nodes_index = -1;
    } else if (fd->my_cb_nodes_index != -1)
        return 1;

    return 0;
}

/*
 * If file system implements some version of two-phase -- doesn't have to be
 * generic -- we can still carry out the defered open optimization
 */
static int uses_generic_read(ADIO_File fd)
{
    if (ADIO_Feature(fd, ADIO_TWO_PHASE))
        return 1;
    return 0;
}

static int uses_generic_write(ADIO_File fd)
{
    if (ADIO_Feature(fd, ADIO_TWO_PHASE))
        return 1;
    return 0;
}

static int build_cb_config_list(ADIO_File fd,
                                MPI_Comm orig_comm, MPI_Comm comm,
                                int rank, int procs, int *error_code)
{
    ADIO_cb_name_array array;
    int *tmp_ranklist;
    int rank_ct;
    char *value;
    static char myname[] = "ADIO_OPEN cb_config_list";

    /* gather the processor name array if we don't already have it */
    /* this has to be done early in ADIO_Open so that we can cache the name
     * array in both the dup'd communicator (in case we want it later) and the
     * original communicator */
    ADIOI_cb_gather_name_array(orig_comm, comm, &array);

/* parse the cb_config_list and create a rank map on rank 0 */
    if (rank == 0) {
        tmp_ranklist = (int *) ADIOI_Malloc(sizeof(int) * procs);
        if (tmp_ranklist == NULL) {
            *error_code = MPIO_Err_create_code(*error_code,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__, MPI_ERR_OTHER, "**nomem2", 0);
            return 0;
        }

        rank_ct = ADIOI_cb_config_list_parse(fd->hints->cb_config_list,
                                             array, tmp_ranklist, fd->hints->cb_nodes);

        /* store the ranklist using the minimum amount of memory */
        if (rank_ct > 0) {
            fd->hints->ranklist = (int *) ADIOI_Malloc(sizeof(int) * rank_ct);
            memcpy(fd->hints->ranklist, tmp_ranklist, sizeof(int) * rank_ct);
        }
        ADIOI_Free(tmp_ranklist);
        fd->hints->cb_nodes = rank_ct;
        /* TEMPORARY -- REMOVE WHEN NO LONGER UPDATING INFO FOR FS-INDEP. */
        value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));
        MPL_snprintf(value, MPI_MAX_INFO_VAL + 1, "%d", rank_ct);
        ADIOI_Info_set(fd->info, "cb_nodes", value);
        ADIOI_Free(value);
    }

    ADIOI_cb_bcast_rank_map(fd);
    if (fd->hints->cb_nodes <= 0) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__, MPI_ERR_IO, "**ioagnomatch", 0);
        fd = ADIO_FILE_NULL;
    }
    return 0;
}

/*
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
