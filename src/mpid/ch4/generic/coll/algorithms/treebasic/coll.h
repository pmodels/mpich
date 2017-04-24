/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <stdio.h>
#include <string.h>

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b;     })

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b;     })

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_MAX_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 3
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum K value that will be used for kary/knomial trees
        Runtime will store the kary/knomial trees upto MPIR_CVAR_MAX_KVAL-1 value of k

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static inline int COLL_init()
{
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t *comm, int id, int * tag, int rank, int size)
{
    comm->id = id;
    comm->mpir_comm = container_of(COLL_COMM_BASE(comm), MPIR_Comm, dev.ch4.ch4_coll);
    comm->rank = rank;
    comm->size = size;
    comm->tag = *tag;
    
    comm->is_subcomm = false;
    int max_k = comm->max_k = MPIR_CVAR_MAX_KVAL; 
    int i,k;
    /*allocate space for arrays */
    comm->kary_tree = (COLL_tree_t*)MPL_malloc(sizeof(COLL_tree_t)*max_k);
    comm->knomial_tree = (COLL_tree_t*)MPL_malloc(sizeof(COLL_tree_t)*max_k);

    /*pre-calculate log_k(size) values for all possible values of k*/
    for(k=2; k<max_k; k++){
        COLL_tree_kary_init(rank,size,k,0,&comm->kary_tree[k]);
        COLL_tree_knomial_init(rank,size,k,0,&comm->knomial_tree[k]);
    }
#if 0
    /*initialize subcommunicator for multileader optimization, only if you have node information*/
    if(comm->mpir_comm->node_comm != NULL && !comm->is_subcomm){
        MPIR_Comm* node_comm = comm->mpir_comm->node_comm;
        int num_subcomms = 2;
        comm->subcomm = (MPIR_Comm*)MPL_malloc(sizeof(MPIR_Comm));
        MPIDI_COLL_COMM(comm->subcomm)->x_treebasic.is_subcomm = true; /*don't break it into further subcomms*/
        int ranks_on_node = node_comm->local_size;
        int block_size = (ranks_on_node+num_subcomms-1)/num_subcomms; /*ceil of ranks_on_node/num_subcomms*/
        int local_rank = node_comm->rank;
        MPIR_comm_split_impl(comm->mpir_comm, local_rank/block_size, rank, comm->subcomm);
    }
#endif
    return 0;
}

static inline int COLL_comm_cleanup(COLL_comm_t * comm)
{
    MPL_free(comm->kary_tree);
    MPL_free(comm->knomial_tree);
    return 0;
}

static inline int COLL_bcast(void *buffer,
                             int count,
                             MPI_Datatype datatype, int root, COLL_comm_t * comm, int* errflag, int tree_type)
{
    int rank = comm->rank;
    int size = comm->size;
    int tag = comm->tag++;
    MPIR_Comm *comm_ptr = comm->mpir_comm;
    MPI_Status status;

    if(root!=0){/*if root is not zero, send the data to rank 0*/
        if(rank==root){/*send data to root of the tree*/
            MPIC_Send(buffer, count, datatype, 0, tag, comm_ptr, errflag);
        }else if(root==0){/*receive data from root of the broadcast*/
            MPIC_Recv(buffer, count, datatype, root, tag, comm_ptr, &status, errflag);
        }
    }

    if(comm_ptr->node_comm != NULL){/*MPICH is shared memory aware*/
        /*Do Internode bcast followed by Intranode bcast*/
        
        /*Internode bcast - performed only by node roots*/
        if(comm_ptr->node_roots_comm!=NULL && comm_ptr->local_size>comm_ptr->node_comm->local_size){
            if(0) fprintf(stderr, "starting internode bcast\n");
            MPID_Bcast(buffer, count, datatype, 0, comm_ptr->node_roots_comm, errflag);
            if(0) fprintf(stderr, "finished internode bcast\n");
        }
        /*Intranode bcast - performed by everyone*/
        if(0) fprintf(stderr, "starting intranode bcast\n");
        MPID_Bcast(buffer, count, datatype, 0, comm_ptr->node_comm, errflag);
        if(0) fprintf(stderr, "finished intranode bcast\n");
        
    }
    int i,j;
    
    COLL_tree_t *tree; int k;
    if(tree_type == 0){ /*Do kary bcast*/
        k=MPIR_CVAR_BCAST_KARY_KVAL;
        tree = &comm->kary_tree[k];
        if(0) fprintf(stderr, "value of k for kary tree bcast = %d\n", k);
    }else if(tree_type == 1){/*Do knomial bcast*/
        k=MPIR_CVAR_BCAST_KNOMIAL_KVAL;
        tree = &comm->knomial_tree[k];
        if(0) fprintf(stderr, "value of k for knomial tree bcast = %d\n", k);
    }else{
        fprintf(stderr, "Invalid Tree Type for Broadcast\n");
        exit(1);
    }
    if(k >= comm->max_k){
        fprintf(stderr, "value of k for broadcast is greater than max_k\n");
        exit(1);
    }
    
    /*do the broadcast over the tree*/
    if(comm->rank != 0){/*non-root rank, receive data from parent*/
        if(0) fprintf(stderr, "recv data from %d\n", tree->parent);
        MPIC_Recv(buffer, count, datatype, tree->parent, tag, comm_ptr, &status, errflag);
    }
    //if(0) fprintf(stderr, "number of children: %d\n", comm->kary_nchildren[k]);
    /*send data to children*/
    for(i=0; i<tree->numRanges; i++){
        for(j=tree->children[i].startRank; j<= tree->children[i].endRank; j++){
            if(0) fprintf(stderr, "send data to %d\n", j);
            MPIC_Send(buffer, count, datatype, j, tag, comm_ptr, errflag);
        }
    }

    return 0;
}
