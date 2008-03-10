/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_getpname.c
 * \brief Device interface to MPI_Get_processor_name()
 */
#include "mpidimpl.h"

/**
 * \brief Device interface to MPI_Get_processor_name()
 * \param[out] name      Storage for the name as a string
 * \param[in]  namelen   The maximum allowed length
 * \param[out] resultlen The actual length written
 * \returns MPI_SUCCESS
 *
 * All this does is convert the rank to a string and return the data
 */
int MPID_Get_processor_name(char * name, int namelen, int * resultlen)
{
  snprintf(name, namelen, "%s", mpid_hw.name);
  *resultlen = (int)strlen(name);
  return MPI_SUCCESS;
}
