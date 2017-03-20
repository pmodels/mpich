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
    - name        : MPIR_CVAR_KVAL_KARY_BCAST
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for Kary broadcast

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static inline int COLL_init()
{
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t *comm, int * tag, int rank, int size)
{
    
    comm->mpir_comm = container_of(COLL_COMM_BASE(comm), MPIR_Comm, dev.ch4.ch4_coll);
    comm->rank = rank;
    comm->size = size;
    comm->tag = *tag;
  
    int max_k = comm->max_k = 10; 
    int i,k;
    /*allocate space for arrays */
    int *log_size = comm->log_size = (int*)MPL_malloc(sizeof(int)*max_k);
    int *kary_parent = comm->kary_parent = (int*)MPL_malloc(sizeof(int*)*max_k);
    int *kary_nchildren = comm->kary_nchildren = (int*)MPL_malloc(sizeof(int)*max_k);
    int **kary_children = comm->kary_children = (int**)MPL_malloc(sizeof(int*)*max_k);

    int *knomial_parent = comm->knomial_parent = (int*)MPL_malloc(sizeof(int*)*max_k);
    int *knomial_nchildren = comm->knomial_nchildren = (int*)MPL_malloc(sizeof(int)*max_k);
    int **knomial_children = comm->knomial_children = (int**)MPL_malloc(sizeof(int*)*max_k);

    /*pre-calculate log_k(size) values for all possible values of k*/
    memset(log_size, 0, sizeof(int)*max_k);
    for(k=2; k<max_k; k++){
        /*calculate log value*/
        int tmp = size;
        while(tmp){
            tmp/=k;
            log_size[k]++;
        }
        log_size[k]--;
        
        /*calculate kary trees*/
        kary_parent[k] = (rank-1)/k;
        kary_nchildren[k] = max(0, min(k,size-(rank*k+1)));
        kary_children[k] = (int*)MPL_malloc(sizeof(int)*kary_nchildren[k]);
        for(i=0; i<kary_nchildren[k]; i++)
            kary_children[k][i] = rank*k+i+1;

        /*calculate knomial trees*/
    }
    return 0;
}

static inline int COLL_comm_cleanup(COLL_comm_t * comm)
{
    MPL_free(comm->log_size);
    MPL_free(comm->kary_parent);
    MPL_free(comm->kary_nchildren);
    int i;
    for(i=2; i<comm->max_k; i++)
        MPL_free(comm->kary_children[i]);
    MPL_free(comm->kary_children);

    MPL_free(comm->knomial_parent);
    MPL_free(comm->knomial_nchildren);
    /*for(i=2; i<comm->max_k; i++)
        MPL_free(comm->knomial_children[i]);*/
    MPL_free(comm->knomial_children);
    return 0;
}

static inline int COLL_bcast(void *buffer,
                             int count,
                             MPI_Datatype datatype, int root, COLL_comm_t * comm, int* errflag)
{
    int rank = comm->rank;
    int size = comm->size;
    int tag = comm->tag++;
    MPIR_Comm *comm_ptr = comm->mpir_comm;
    MPI_Status status;
    int k=MPIR_CVAR_KVAL_KARY_BCAST;
    if(0) fprintf(stderr, "value of k for kary tree bcast = %d\n", k);

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

    int i;

    /*do the broadcast over the tree*/
    if(comm->rank != 0){/*non-root rank, receive data from parent*/
        if(0) fprintf(stderr, "recv data from %d\n", comm->kary_parent[k]);
        MPIC_Recv(buffer, count, datatype, comm->kary_parent[k], tag, comm_ptr, &status, errflag);
    }
    if(0) fprintf(stderr, "number of children: %d\n", comm->kary_nchildren[k]);
    /*send data to children*/
    for(i=0; i<comm->kary_nchildren[k]; i++){
        if(0) fprintf(stderr, "send data to %d\n", comm->kary_children[k][i]);
        MPIC_Send(buffer, count, datatype, comm->kary_children[k][i], tag, comm_ptr, errflag);
    }

    return 0;
}
