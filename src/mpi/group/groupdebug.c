/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* style: allow:fprintf:2 sig:0 */
/* style: PMPIuse:PMPI_Abort:2 sig:0 */

/*
 * This file contains routines that are used only to perform testing
 * and debugging of the group routines
 */
void MPITEST_Group_create( int, int, MPI_Group * );
void MPITEST_Group_print( MPI_Group );

/* --BEGIN DEBUG-- */
void MPITEST_Group_create( int nproc, int myrank, MPI_Group *new_group )
{
    MPID_Group *new_group_ptr;
    int i;

    new_group_ptr = (MPID_Group *)MPIU_Handle_obj_alloc( &MPID_Group_mem );
    if (!new_group_ptr) {
	fprintf( stderr, "Could not create a new group\n" );
	PMPI_Abort( MPI_COMM_WORLD, 1 );
    }
    MPIU_Object_set_ref( new_group_ptr, 1 );
    new_group_ptr->lrank_to_lpid = (MPID_Group_pmap_t *)MPIU_Malloc( nproc * sizeof(MPID_Group_pmap_t) );
    if (!new_group_ptr->lrank_to_lpid) {
	fprintf( stderr, "Could not create lrank map for new group\n" );
	PMPI_Abort( MPI_COMM_WORLD, 1 );
    }

    new_group_ptr->rank = MPI_UNDEFINED;
    for (i=0; i<nproc; i++) {
	new_group_ptr->lrank_to_lpid[i].lrank = i;
	new_group_ptr->lrank_to_lpid[i].lpid  = i;
    }
    new_group_ptr->size = nproc;
    new_group_ptr->rank = myrank;
    new_group_ptr->idx_of_first_lpid = -1;

    *new_group = new_group_ptr->handle;
}

void MPITEST_Group_print( MPI_Group g )
{
    MPID_Group *g_ptr;
    int g_idx, size, i;

    MPID_Group_get_ptr( g, g_ptr );

    g_idx = g_ptr->idx_of_first_lpid;
    if (g_idx < 0) { 
	MPIR_Group_setup_lpid_list( g_ptr ); 
	g_idx = g_ptr->idx_of_first_lpid;
    }
    
    /* Loop through these, printing the lpids by rank and in order */
    size = g_ptr->size;
    fprintf( stdout, "Lpids in rank order\n" );
    for (i=0; i<size; i++) {
	fprintf( stdout, "Rank %d has lpid %d\n", 
		 i, g_ptr->lrank_to_lpid[i].lpid );
    }
    
    fprintf( stdout, "Ranks in lpid order\n" );
    while (g_idx >= 0) {
	fprintf( stdout, "Rank %d has lpid %d\n", g_idx, 
		 g_ptr->lrank_to_lpid[g_idx].lpid );
	g_idx = g_ptr->lrank_to_lpid[g_idx].next_lpid;
    }
}
/* --END DEBUG-- */
