/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/** PGROUP Creation (Connect/Accept Method)
  * James Dinan <dinan@mcs.anl.gov>
  * May, 2011
  *
  * In this test, processes create an intracommunicator and creation is
  * collective only on the members of the new communicator, not on the parent
  * communicator.  This is accomplished by building up and merging
  * intercommunicators using Connect/Accept to merge with a master/controller
  * process.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>

const int verbose = 0;

void PGroup_create(int count, int members[], MPI_Comm *group);
void PGroup_create(int count, int members[], MPI_Comm *group)
{
  int       i, me, nproc, is_member;
  char     *port;
  MPI_Comm  pgroup;

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (count == 0) {
    *group = MPI_COMM_NULL;
    return;
  }
  else if (count == 1 && members[0] == me) {
    *group = MPI_COMM_SELF;
    return;
  }

  for (i = 0, is_member = 0; i < count; i++) {
    if (members[i] == me) {
      is_member = 1;
      break;
    }
  }

  if (!is_member) {
    *group = MPI_COMM_NULL;
    return;
  }

  port = malloc(MPI_MAX_PORT_NAME);

  /* NOTE: Assume members list is identical and sorted on all processes */

  /* I am the leader */
  if (me == members[0]) {
    MPI_Comm pgroup_new;

    pgroup = MPI_COMM_SELF;

    MPI_Open_port(MPI_INFO_NULL, port);

    for (i = 1; i < count; i++) {
      MPI_Comm pgroup_old = pgroup;

      MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, members[i], 0, MPI_COMM_WORLD);
      MPI_Comm_accept(port, MPI_INFO_NULL, 0, pgroup, &pgroup_new);
      MPI_Intercomm_merge(pgroup_new, 0 /* LOW */, &pgroup);

      MPI_Comm_free(&pgroup_new);
      if (pgroup_old != MPI_COMM_SELF)
        MPI_Comm_free(&pgroup_old);
    }

    MPI_Close_port(port);
  }

  /* I am not the leader */
  else {
    int merged=0;
    MPI_Comm pgroup_new;

    for (i = 0; i < count; i ++) {
      if (members[i] == me) {
        MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, members[0], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &pgroup_new);
        MPI_Intercomm_merge(pgroup_new, 1 /* HIGH */, &pgroup);
        MPI_Comm_free(&pgroup_new);
        merged = 1;
      }
      else if (merged) {
        MPI_Comm pgroup_old = pgroup;

        MPI_Comm_connect(port, MPI_INFO_NULL, 0, pgroup, &pgroup_new);
        MPI_Intercomm_merge(pgroup_new, 0 /* HIGH */, &pgroup);

        MPI_Comm_free(&pgroup_new);
        MPI_Comm_free(&pgroup_old);
      }
      /* else => !merged */
    }
  }

  free(port);
  *group = pgroup;
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

  if (me % 2 == 0) {
    int gme, gnproc;

    PGroup_create(gsize, glist, &group);
    MPI_Barrier(group);

    MPI_Comm_rank(group, &gme);
    MPI_Comm_size(group, &gnproc);
    if (verbose) 
      printf("[%d] Group rank = %d, size = %d\n", me, gme, gnproc);

    if (group != MPI_COMM_SELF)
      MPI_Comm_free(&group);
  }

  free(glist);

  MPI_Finalize();

  if (me == 0)
    printf(" No Errors\n");

  return 0;
}
