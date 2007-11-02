/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_TCP_PRE_H
#define MM_TCP_PRE_H

typedef struct MM_Car_data_tcp
{
    union mm_car_data_tcp_buf
    {
	struct car_tcp_simple
	{
	    int num_written;
	} simple;
	struct car_tcp_tmp
	{
	    int num_written;
	} tmp;
	struct car_tcp_vec_read
	{
	    MPID_IOV vec[MPID_IOV_LIMIT];
	    int vec_size;
	    int total_num_read;
	    int cur_num_read;
	    int cur_index;
	} vec_read;
	struct car_tcp_vec_write
	{
	    int num_read_copy;
	    MPID_IOV vec[MPID_IOV_LIMIT];
	    int vec_size;
	    int total_num_written;
	    int cur_num_written;
	    int cur_index;
	    int num_written_at_cur_index;
	} vec_write;
#ifdef WITH_METHOD_SHM
	struct car_tcp_shm
	{
	    int num_written;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct car_tcp_via
	{
	    int num_written;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct car_tcp_via_rdma
	{
	    int num_written;
	} via_rdma;
#endif
    } buf;
} MM_Car_data_tcp;

#endif
