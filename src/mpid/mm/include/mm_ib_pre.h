/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_IB_PRE_H
#define MM_IB_PRE_H

#include "ib_types.h"
#include "ib_defs.h" 
#include "blockallocator.h"

typedef struct IB_Info
{
    ib_uint32_t   m_mtu_size;
    ib_uint32_t   m_max_wqes;
    ib_uint32_t   m_dlid;
    ib_uint32_t   m_polling;

    ib_mr_handle_t   m_mr_handle;
    ib_uint32_t      m_lkey;
    ib_qp_handle_t   m_qp_handle;
    BlockAllocator   m_allocator;
    ib_uint32_t      m_dest_qp_num;
    /*
    ib_int64_t       m_snd_work_id;
    ib_int64_t       m_rcv_work_id;
    */
    
    ib_address_handle_t m_address_handle;
    
    /*
    ib_uint32_t      m_snd_completion_counter;
    ib_uint32_t      m_rcv_completion_counter;
    ib_uint32_t      m_snd_posted;
    ib_uint32_t      m_rcv_posted;
    */
} IB_Info;

typedef union ib_work_id_handle_t
{
    ib_uint64_t id;
    struct data
    {
	ib_uint32_t vc, mem;
    } data;
} ib_work_id_handle_t;

typedef struct MM_Car_data_ib
{
    union mm_car_data_ib_buf
    {
	struct car_ib_simple
	{
	    int num_written;
	} simple;
	struct car_ib_tmp
	{
	    int num_written;
	} tmp;
	struct car_ib_vec_read
	{
	    MPID_IOV vec[MPID_IOV_LIMIT];
	    int vec_size;
	    int total_num_read;
	    int cur_num_read;
	    int cur_index;
	} vec_read;
	struct car_ib_vec_write
	{
	    int num_read_copy;
	    MPID_IOV vec[MPID_IOV_LIMIT];
	    int vec_size;
	    int total_num_written;
	    int cur_num_written;
	    int cur_index;
	    int num_written_at_cur_index;
	} vec_write;
	struct car_ib_ib
	{
	    ib_scatter_gather_list_t  sglist;
	} ib;
#ifdef WITH_METHOD_SHM
	struct car_ib_shm
	{
	    int num_written;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct car_ib_via
	{
	    int num_written;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct car_ib_via_rdma
	{
	    int num_written;
	} via_rdma;
#endif
    } buf;
} MM_Car_data_ib;

#endif
