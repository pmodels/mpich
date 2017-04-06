/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "recexch_util.h"

#ifndef COLL_NAMESPACE
#error "The tree template must be namespaced with COLL_NAMESPACE"
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLRED_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for allreduce with recursive koupling algorithm

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/*This function calculates the ranks to/from which the
data is sent/recvd in various steps/phases of recursive koupling
algorithm. Recursive Koupling is divided into three steps - Step 1, 2 and 3.
Step 2 is the main step that does the recursive koupling.
Step 1 and Step 3 are done to handle the case of non-power-of-k number of ranks.
Only p_of_k (largest power of k that is less than n) ranks participate in Step 2.
In Step 1, the non partcipating ranks send their data to ranks
participating in Step 2. In Step 3, the ranks participating in Step 2 send
the final data to non-partcipating ranks.

Step 2 is further divided into log_k(p_of_k) phases.

Ideally, this function belongs to recexch_util.h file but because this
function uses TSP_allocate_mem, it had to be put here
*/
static inline
int COLL_get_neighbors_recexch(int rank, int nranks, int *k_, int *step1_sendto, \
                                            int **step1_recvfrom_, int *step1_nrecvs,
                                            int ***step2_nbrs_, int *step2_nphases, int *p_of_k_, int *T_){
    int i,j,k;
    k=*k_;
    if(nranks < k)/*If size of the communicator is less than k, reduce the value of k*/
        k= (nranks > 2) ? nranks : 2;
    *k_=k;
    /*p_of_k is the largest power of k that is less than nranks*/
    int p_of_k = 1,log_p_of_k=0,rem,T,newrank;

    while(p_of_k <= nranks) {
        p_of_k *= k;
        log_p_of_k++;
    }

    p_of_k /= k;
    log_p_of_k--;

    if(0) fprintf(stderr,"allocate memory for storing communication pattern\n");

    *step2_nbrs_ = (int **)TSP_allocate_mem(sizeof(int *)*log_p_of_k);

    for(i=0; i<log_p_of_k; i++) {
        (*step2_nbrs_)[i]=(int *)TSP_allocate_mem(sizeof(int)*(k-1));
    }

    int **step2_nbrs = *step2_nbrs_;
    int *step1_recvfrom = *step1_recvfrom_ = (int *)TSP_allocate_mem(sizeof(int)*(k-1));
    *step2_nphases = log_p_of_k;

    rem = nranks - p_of_k;/*rem is the number of ranks that do not particpate in Step 2*/
    /*We need to identify the non-participating ranks. The first T ranks are divided
    into sets of k consecutive ranks each. From each of these sets, the first k-1 ranks
    are the non-participatig ranks while the last rank participates in Step 2. The non-participating
    ranks send their data to the participating rank in their set.
    */
    T = (rem*k)/(k-1);
    *T_ = T;
    *p_of_k_ = p_of_k;


    if(0) fprintf(stderr,"step 1 nbr calculation started. T is %d \n", T);
    *step1_nrecvs=0;
    *step1_sendto=-1;

    /* Step 1 */
    if(rank < T) {
        if(rank % k != (k-1)) {  /* I am a non-participating rank */
            *step1_sendto = rank + (k-1-rank%k); /* partipating rank to send the data to*/

            if(*step1_sendto > T-1) *step1_sendto=T;/*if the corresponding participating rank is not in T,

                                            then send to the Tth rank to preserve non-commutativity*/
            newrank = -1;/*tag this rank as non-participating*/
        } else { /* receiver rank */
            for(i=0; i<k-1; i++) {
                step1_recvfrom[i]=rank-i-1;
            }

            *step1_nrecvs = k-1;
            /*this is the new rank in the set of participating ranks*/
            newrank = rank / k;
        }
    } else { /* rank >= T */
        newrank = rank - rem;

        if(rank == T && (T-1)%k != k-1 && T>=1) {
            int nsenders = (T-1)%k +1;/*number of ranks sending to me*/

            for(j=nsenders-1; j>=0; j--) {
                step1_recvfrom[nsenders-1-j]=T-nsenders+j;
            }

            *step1_nrecvs = nsenders;
        }
    }

    if(0) fprintf(stderr,"step 1 nbr computation completed\n");

    /*Step 2*/
    if(*step1_sendto==-1){ /* calulate step2_nbrs only for participating ranks */
        int *digit = (int*)TSP_allocate_mem(sizeof(int)*log_p_of_k);
        /*calculate the digits in base k representation of newrank*/
        for(i=0; i<log_p_of_k; i++)
            digit[i]=0;
        int temprank = newrank, index=0, remainder;
        while(temprank!=0) {
            remainder = temprank % k;
            temprank = temprank / k;
            digit[index] = remainder;
            index++;
        }

        int mask=0x1;
        int phase=0, cbit, cnt, nbr, power;
        while(mask<p_of_k){
            cbit = digit[phase];/*phase_th digit changes in this phase, obtain its original value*/
            cnt = 0;
            for(i=0;i<k;i++) {/*there are k-1 neighbors*/
                if(i!=cbit) {/*do not generate yourself as your nieighbor*/
                    digit[phase]=i; /*this gets us the base k representation of the neighbor*/

                    /*calculate the base 10 value of the neighbor rank*/
                    nbr=0;
                    power=1;
                    for(j=0;j<log_p_of_k;j++) {
                        nbr += digit[j]*power;
                        power *= k;
                    }

                    /*calculate its real rank and store it*/
                    step2_nbrs[phase][cnt] = (nbr < rem/(k-1)) ? (nbr*k)+(k-1) : nbr+rem;
                    if(0) printf("step2_nbrs[%d][%d] is %d \n", phase,cnt, step2_nbrs[phase][cnt]);
	            cnt++;
                }
            }
            if(0) fprintf(stderr,"step 2, phase %d nbr calculation completed\n", phase);
            digit[phase]=cbit;
            phase++;
            mask*=k;
        }

        TSP_free_mem(digit);
    }

    return 0;
}

static inline int
COLL_sched_allreduce_recexch(const void         *sendbuf,
                     void               *recvbuf,
                     int                 count,
                     COLL_dt_t          *datatype,
                     COLL_op_t          *op,
                     int                 tag,
                     COLL_comm_t        *comm,
                     COLL_sched_t *s,
                     int finalize)
{

    int k = MPIR_CVAR_ALLRED_RECEXCH_KVAL;
    int is_inplace,is_contig,i;
    int dtcopy_id=-1;
    int nranks = TSP_size(&comm->tsp_comm);
    int rank = TSP_rank(&comm->tsp_comm);

    size_t type_size,extent,lb;
    int is_commutative;
    TSP_opinfo(&op->tsp_op,&is_commutative);
    TSP_dtinfo(&datatype->tsp_dt, &is_contig, &type_size, &extent, &lb);

    is_inplace = TSP_isinplace((void *)sendbuf);
    /*if there is only 1 rank, copy data from sendbuf
      to recvbuf and exit */
    if(nranks == 1){
         if(!is_inplace)
             TSP_dtcopy_nb(recvbuf,count,&datatype->tsp_dt, \
                             sendbuf,count,&datatype->tsp_dt,&s->tsp_sched,0,NULL);
         return 0;
    }

    int step1_sendto=-1;
    int step2_nphases;
    int step1_nrecvs;

    /*COLL_get_neighbors_recexch function allocates memory
    to these pointers */
    int *step1_recvfrom;
    int **step2_nbrs;
    int p_of_k, T;
    /*get the neighbors*/
    COLL_get_neighbors_recexch(rank, nranks, &k, &step1_sendto, &step1_recvfrom, &step1_nrecvs, &step2_nbrs, &step2_nphases,&p_of_k,&T);

    void *tmp_buf = TSP_allocate_mem(count*extent);

    int rrid=-1;
    if(!is_inplace && step1_sendto==-1){/*copy the data to recvbuf but only if you are a rank participating in Step 2*/
         rrid = TSP_dtcopy_nb(recvbuf,count,&datatype->tsp_dt, \
                        sendbuf,count,&datatype->tsp_dt,&s->tsp_sched,0,NULL);
    }

    if(0) fprintf(stderr,"After initial dt copy\n");
    int* id = (int*)TSP_allocate_mem(sizeof(int)*k);
    /*Step 1*/
    if(step1_sendto !=-1){/*non-participating rank sends the data to a partcipating rank*/
        if(!is_inplace)
            TSP_send(sendbuf,count,&datatype->tsp_dt,step1_sendto,
                                tag,&comm->tsp_comm,&s->tsp_sched,0,NULL);
        else
            TSP_send(recvbuf,count,&datatype->tsp_dt,step1_sendto,
                                 tag,&comm->tsp_comm,&s->tsp_sched,0,NULL);
    }
    for(i=0; i<step1_nrecvs; i++){/*partcipating rank gets data from non-partcipating ranks*/
        rrid= TSP_recv_reduce(recvbuf,count,&datatype->tsp_dt,
                              &op->tsp_op,step1_recvfrom[i],tag,&comm->tsp_comm,
                              TSP_FLAG_REDUCE_L,&s->tsp_sched,(i==0 && is_inplace)?0:1,&rrid);
    }
    id[0] = TSP_fence(&s->tsp_sched);
    /*Step 2*/
    int myidx, nbr, phase;
    if(0) fprintf(stderr,"After step1\n");
    for(phase=0; phase<step2_nphases && step1_sendto==-1; phase++){
        if(!is_commutative){/*sort the neighbor list so that receives can be posted in order*/
            qsort(step2_nbrs[phase], k-1, sizeof(int), intcmpfn);
        }

        /*copy the data to a temporary buffer so that sends ands recvs
        can be posted simultaneosly*/
        id[0] = TSP_dtcopy_nb(tmp_buf,count,&datatype->tsp_dt,
                                  recvbuf,count,&datatype->tsp_dt,&s->tsp_sched,(phase==0)?1:k,id);
        /*myidx is the index in the neighbors list such that
        all neighbors before myidx have ranks less than my rank
        and neighbors at and after myidx have rank greater than me.
        This is useful only in the non-commutative case.*/
        myidx = 0;
        /*send data to all the neighbors*/
        for(i=0; i<k-1; i++) {
            nbr = step2_nbrs[phase][i];
            id[i+1] = TSP_send(tmp_buf,count,&datatype->tsp_dt,
                                     nbr,tag,&comm->tsp_comm,&s->tsp_sched,1,id);
            if(rank > nbr){
                 myidx = i+1;
            }
        }

        /*receive from the neighbors to the left*/
        for(i=myidx-1; i>=0; i--) {
            nbr = step2_nbrs[phase][i];
            id[0] = TSP_recv_reduce(recvbuf,count,&datatype->tsp_dt,&op->tsp_op,nbr,
                                        tag,&comm->tsp_comm,TSP_FLAG_REDUCE_L,&s->tsp_sched,1,id);
        }

        /*receive from the neighbors to the right*/
        for(i=myidx; i<k-1; i++) {
            nbr = step2_nbrs[phase][i];
            id[0] = TSP_recv_reduce(recvbuf,count,&datatype->tsp_dt,&op->tsp_op,nbr,
                                        tag,&comm->tsp_comm,TSP_FLAG_REDUCE_R,&s->tsp_sched,1,id);
        }
        if(0) fprintf(stderr,"After step2\n");
    }

    /*Step 3: This is reverse of Step 1. Ranks that participated in Step 2
    send the data to non-partcipating ranks*/
    if(step1_sendto != -1){
        TSP_recv(recvbuf,count,&datatype->tsp_dt,step1_sendto,tag,&comm->tsp_comm,&s->tsp_sched,0,NULL);
        //TSP_free_mem(tmp_buf);
    }
    else{ /* free temporary buffer after it is used in last send */
        //TSP_free_mem_nb(tmp_buf,&s->tsp_sched,k-1,id+1);
    }

    for(i=0; i<step1_nrecvs; i++) {
        TSP_send(recvbuf,count,&datatype->tsp_dt,step1_recvfrom[i],
                    tag,&comm->tsp_comm,&s->tsp_sched,1,id);
    }

    if(0) fprintf(stderr,"After step3\n");
    /*free all allocated memory for storing nbrs*/
    for(i=0;i<step2_nphases;i++)
        TSP_free_mem(step2_nbrs[i]);
    TSP_free_mem(step2_nbrs);
    if(0) fprintf(stderr,"freed step2_nbrs\n");
    TSP_free_mem(step1_recvfrom);

    if(0) fprintf(stderr,"freed step1_recvfrom\n");

    TSP_free_mem(id);
    if(finalize) {
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}
