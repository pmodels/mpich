/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#ifndef _ARMCIX_H_
#define _ARMCIX_H_

#include <armci.h>
#include <armciconf.h>

#if   HAVE_STDINT_H
#  include <stdint.h>
#elif HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

/** Access mode extensions: set the access mode for an ARMCI allocation,
  * enabling runtime layer optimizations.
  */

enum armcix_access_mode_e {
  ARMCIX_MODE_ALL           = 0x1,  /* All access types permitted          */
  ARMCIX_MODE_CONFLICT_FREE = 0x2,  /* Operations do not conflict          */
  ARMCIX_MODE_NO_LOAD_STORE = 0x4   /* Load/store operations not permitted */
};

int ARMCIX_Mode_set(int mode, void *ptr, ARMCI_Group *group);
int ARMCIX_Mode_get(void *ptr);

/** Processor group extensions.
  */

int ARMCIX_Group_split(ARMCI_Group *parent, int color, int key, ARMCI_Group *new_group);

/** Mutex handles: These improve on basic ARMCI mutexes by allowing you to
  * create multiple batches of mutexes.  This is needed to allow libraries access to
  * mutexes.
  */

struct armcix_mutex_hdl_s {
  int        my_count;
  int        max_count;
  MPI_Comm   comm;
  MPI_Win   *windows;
  uint8_t  **bases;
};

typedef struct armcix_mutex_hdl_s * armcix_mutex_hdl_t;

armcix_mutex_hdl_t ARMCIX_Create_mutexes_hdl(int count, ARMCI_Group *pgroup);
int  ARMCIX_Destroy_mutexes_hdl(armcix_mutex_hdl_t hdl);
void ARMCIX_Lock_hdl(armcix_mutex_hdl_t hdl, int mutex, int proc);
int  ARMCIX_Trylock_hdl(armcix_mutex_hdl_t hdl, int mutex, int proc);
void ARMCIX_Unlock_hdl(armcix_mutex_hdl_t hdl, int mutex, int proc);

#endif /* _ARMCIX_H_ */
