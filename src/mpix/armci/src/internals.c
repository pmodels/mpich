/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include <armci.h>
#include <armcix.h>
#include <armci_internals.h>
#include <debug.h>
#include <gmr.h>

/** ARMCI Internal global state */
global_state_t ARMCII_GLOBAL_STATE = { 0 };

/** Enum strings */
char ARMCII_Strided_methods_str[][10] = { "IOV", "DIRECT" };
char ARMCII_Iov_methods_str[][10]     = { "AUTO", "CONSRV", "BATCHED", "DIRECT" };
char ARMCII_Shr_buf_methods_str[][10] = { "COPY", "NOGUARD" };

/** Raise an internal fatal ARMCI error.
  *
  * @param[in] file Current file name (__FILE__)
  * @param[in] line Current line numeber (__LINE__)
  * @param[in] func Current function name (__func__)
  * @param[in] msg  Message to be printed
  * @param[in] code Exit error code
  */
void ARMCII_Error_impl(const char *file, const int line, const char *func, const char *msg, ...) {
  va_list ap;
  int  disp;
  char string[500];

  disp  = 0;
  va_start(ap, msg);
  disp += vsnprintf(string, 500, msg, ap);
  va_end(ap);

  fprintf(stderr, "[%d] ARMCI Internal error in %s (%s:%d)\n[%d] Messge: %s\n", ARMCI_GROUP_WORLD.rank, 
      func, file, line, ARMCI_GROUP_WORLD.rank, string);
  MPI_Abort(ARMCI_GROUP_WORLD.comm, 100);
}


/** Translate a world process rank to the corresponding process rank in the
  * ARMCI group.
  *
  * @param[in] group      Group to translate to.
  * @param[in] world_rank Rank of the process in the world group.
  * @return               Rank in group or -1 if not in the group.
  */
int ARMCII_Translate_absolute_to_group(MPI_Comm group_comm, int world_rank) {
  int       group_rank;
  MPI_Group world_group, sub_group;

  MPI_Comm_group(ARMCI_GROUP_WORLD.comm, &world_group);
  MPI_Comm_group(group_comm, &sub_group);

  MPI_Group_translate_ranks(world_group, 1, &world_rank, sub_group, &group_rank);

  MPI_Group_free(&world_group);
  MPI_Group_free(&sub_group);

  return group_rank == MPI_UNDEFINED ? -1 : group_rank;
}


/** Translate an ARMCI accumulate data type into an MPI type so we can pass it
  * to mem regions.
  *
  * @param[in]  armci_datatype ARMCI accumulate data type
  * @param[out] mpi_type       MPI data type
  * @param[out] type_size      Size of the MPI data type
  */
void ARMCII_Acc_type_translate(int armci_datatype, MPI_Datatype *mpi_type, int *type_size) {
    // Determine the MPI type for the transfer
    switch (armci_datatype) {
      case ARMCI_ACC_INT:
        *mpi_type = MPI_INT;
        break;
      case ARMCI_ACC_LNG:
        *mpi_type = MPI_LONG;
        break;
      case ARMCI_ACC_FLT:
        *mpi_type = MPI_FLOAT;
        break;
      case ARMCI_ACC_DBL:
        *mpi_type = MPI_DOUBLE;
        break;
      case ARMCI_ACC_CPL:
        *mpi_type = MPI_FLOAT;
        break;
      case ARMCI_ACC_DCP:
        *mpi_type = MPI_DOUBLE;
        break;
      default:
        ARMCII_Error("unknown data type", 100);
        return;
    }

    MPI_Type_size(*mpi_type, type_size);
}


/** Synchronize all public and private windows.
  */
void ARMCII_Flush_local(void) {
  gmr_t *cur_mreg = gmr_list;

  while (cur_mreg) {
    gmr_dla_lock(cur_mreg);
    gmr_dla_unlock(cur_mreg);

    cur_mreg = cur_mreg->next;
  }
}
