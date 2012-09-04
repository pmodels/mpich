/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>

#define INTERCOMM_TAG 0
#define NITER 1024


void pgroup_create(int grp_size, int *pid_list, MPI_Comm *group_out);
void pgroup_free(MPI_Comm *group);


/** Free a pgroup
  */
void pgroup_free(MPI_Comm *group) {
  /* Note: It's ok to compare predefined handles */
  if (*group == MPI_COMM_NULL || *group == MPI_COMM_SELF)
    return;

  MPI_Comm_free(group);
}


/* Create a processor group containing the processes in pid_list.
 *
 * NOTE: pid_list list must be identical and sorted on all processes
 */
void pgroup_create(int grp_size, int *pid_list, MPI_Comm *group_out) {
  int       i, grp_me, me, nproc, merge_size;
  MPI_Comm  pgroup, inter_pgroup;

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  /* CASE: pgroup size 0 */
  if (grp_size == 0) {
    *group_out = MPI_COMM_NULL;
    return;
  }

  /* CASE: pgroup size 1 */
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
  int me, nproc, i, j, *glist;
  MPI_Comm groups[NITER];
  MPI_Group world_group;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  MPI_Comm_group(MPI_COMM_WORLD, &world_group);

  glist = (int*) malloc(nproc*sizeof(int));

  for (i = 0; i < nproc; i++)
    glist[i] = i;

  if (me == 0)
    printf("Gsize\tPGgroup (sec)\tComm (sec)\n");

  MPI_Barrier(MPI_COMM_WORLD);

  for (i = 1; i <= nproc; i*= 2) {
    double t_start, t_pg, t_comm;
    MPI_Group intracomm_group;

    /** Benchmark pgroup creation cost **/

    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();

    for (j = 0; j < NITER; j++)
      pgroup_create(i, glist, &groups[j]);

    t_pg = MPI_Wtime() - t_start;

    for (j = 0; j < NITER; j++)
      pgroup_free(&groups[j]);

    /** Benchmark intracommunicator creation cost **/

    MPI_Group_incl(world_group, i, glist, &intracomm_group);
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();

    for (j = 0; j < NITER; j++)
      MPI_Comm_create(MPI_COMM_WORLD, intracomm_group, &groups[j]);

    t_comm = MPI_Wtime() - t_start;
    MPI_Group_free(&intracomm_group);

    for (j = 0; j < NITER; j++)
      pgroup_free(&groups[j]);

    if (me == 0)
      printf("%6d\t%0.9f\t%0.9f\n", i, t_pg/NITER, t_comm/NITER);

  }

  free(glist);
  MPI_Group_free(&world_group);

  MPI_Finalize();
  return 0;
}
