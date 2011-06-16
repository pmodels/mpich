/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <armci_internals.h>
#include <debug.h>

/** NOTE: Domains are not implemented.  These dummy wrappers assume that all
  * domains are of size 1. */

/** Query the size of a given domain.
  *
  * @param[in] domain    Desired domain.
  * @param[in] domain_id Domain id or -1 for my domain.
  */
int armci_domain_nprocs(armci_domain_t domain, int domain_id) {
  return 1;
}

/** Query which domain a process belongs to.
  */
int armci_domain_id(armci_domain_t domain, int glob_proc_id) {
  return glob_proc_id;
}

/** Translate a domain process ID to a global process ID.
  */
int armci_domain_glob_proc_id(armci_domain_t domain, int domain_id, int loc_proc_id) {
  ARMCII_Assert(loc_proc_id == 0); // Groups must be size 1
  return domain_id;
}

/** Query the ID of my domain.
  */
int armci_domain_my_id(armci_domain_t domain) {
  return ARMCI_GROUP_WORLD.rank;
}

/** Query the number of domains.
  */
int armci_domain_count(armci_domain_t domain) {
  return ARMCI_GROUP_WORLD.size;
}

/** Query if the given process shared a domain with me.
  */
int armci_domain_same_id(armci_domain_t domain, int glob_proc_id) {
  return glob_proc_id == ARMCI_GROUP_WORLD.rank;
}


/** Query if a process is on the same node as the caller.
  *
  * @param[in] proc Process id in question
  */
int ARMCI_Same_node(int proc) {
  return proc == ARMCI_GROUP_WORLD.rank;
}
