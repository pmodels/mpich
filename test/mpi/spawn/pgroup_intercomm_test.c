/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/** PGROUP Creation (Intercommunicator Method)
  * James Dinan <dinan@mcs.anl.gov>
  * May, 2011
  *
  * In this test, processes create an intracommunicator and creation is
  * collective only on the members of the new communicator, not on the parent
  * communicator.  This is accomplished by building up and merging
  * intercommunicators starting with MPI_COMM_SELF for each process involved.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>

#define INTERCOMM_TAG 0

const int verbose = 0;

void pgroup_create(int grp_size, int *pid_list, MPI_Comm *group_out);
void pgroup_free(MPI_Comm *group);

/** Free an ARMCI group
  */
void pgroup_free(MPI_Comm *group) {
  /* Note: It's ok to compare predefined handles */
  if (*group == MPI_COMM_NULL || *group == MPI_COMM_SELF)
    return;

  MPI_Comm_free((MPI_Comm*) group);
}


/* Create an ARMCI processor group containing the processes in pid_list.
 *
 * NOTE: pid_list list must be identical and sorted on all processes
 */
void pgroup_create(int grp_size, int *pid_list, MPI_Comm *group_out) {
  int       i, grp_me, me, nproc, merge_size;
  MPI_Comm  pgroup, inter_pgroup;

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  /* CASE: Group size 0 */
  if (grp_size == 0) {
    *group_out = MPI_COMM_NULL;
    return;
  }

  /* CASE: Group size 1 */
  else if (grp_size == 1 && pid_list[0] == me) {
    *group_out = MPI_COMM_SELF;
    return;
  }

  /* CHECK: If I'm not a member, return COMM_NULL */
  grp_me = -1;
  for (i = 0; i < grp_size; i++) {
    if (pid_list[i] == me) {
      grp_me = i;
      break;
    }
  }

  if (grp_me < 0) {
    *group_out = MPI_COMM_NULL;
    return;
  }

  pgroup = MPI_COMM_SELF;

  for (merge_size = 1; merge_size < grp_size; merge_size *= 2) {
    int      gid        = grp_me / merge_size;
    MPI_Comm pgroup_old = pgroup;

    if (gid % 2 == 0) {
      /* Check if right partner doesn't exist */
      if ((gid+1)*merge_size >= grp_size)
        continue;

      MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, pid_list[(gid+1)*merge_size], INTERCOMM_TAG, &inter_pgroup);
      MPI_Intercomm_merge(inter_pgroup, 0 /* LOW */, &pgroup);
    } else {
      MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, pid_list[(gid-1)*merge_size], INTERCOMM_TAG, &inter_pgroup);
      MPI_Intercomm_merge(inter_pgroup, 1 /* HIGH */, &pgroup);
    }

    MPI_Comm_free(&inter_pgroup);
    if (pgroup_old != MPI_COMM_SELF) MPI_Comm_free(&pgroup_old);
  }

  *group_out = pgroup;
}


int main(int argc, char **argv) {
  int me, nproc, i;
  int gsize, *glist;
  MPI_Comm group;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  gsize = nproc/2 + (nproc % 2);
  glist = malloc(gsize*sizeof(int));

  for (i = 0; i < nproc; i += 2)
    glist[i/2] = i;

  MPI_Barrier(MPI_COMM_WORLD);

  if (me % 2 == 0) {
    int    gme, gnproc;
    double t1, t2;

    t1 = MPI_Wtime();
    pgroup_create(gsize, glist, &group);
    t2 = MPI_Wtime();

    MPI_Barrier(group);

    MPI_Comm_rank(group, &gme);
    MPI_Comm_size(group, &gnproc);
    if (verbose) printf("[%d] Group rank = %d, size = %d time = %f sec\n", me, gme, gnproc, t2-t1);

    pgroup_free(&group);
  }

  free(glist);

  if (me == 0)
    printf(" No Errors\n");

  MPI_Finalize();
  return 0;
}
