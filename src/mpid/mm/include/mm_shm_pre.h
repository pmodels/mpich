/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_SHM_PRE_H
#define MM_SHM_PRE_H

typedef struct MM_Car_data_shm
{
    union mm_car_data_shm_buf
    {
	struct car_shm_simple
	{
	    int num_written;
	} simple;
	struct car_shm_tmp
	{
	    int num_written;
	} tmp;
	struct car_shm_vec
	{
	    MPID_IOV vec[MPID_IOV_LIMIT];
	    int len;
	} vec;
#ifdef WITH_METHOD_SHM
	struct car_shm_shm
	{
	    int num_written;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct car_shm_via
	{
	    int num_written;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct car_shm_via_rdma
	{
	    int num_written;
	} via_rdma;
#endif
    } buf;
} MM_Car_data_shm;

#endif
