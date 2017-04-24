/* Transport Types */
#undef TSP_dt_t
#undef TSP_op_t
#undef TSP_comm_t
#undef TSP_req_t
#undef TSP_sched_t
#undef TSP_aint_t
#undef TSP_global_t
#undef TSP_global

/* NB transport argument types */
#undef TSP_sendrecv_arg_t
#undef TSP_recv_reduce_arg_t
#undef TSP_addref_dt_arg_t
#undef TSP_addref_op_arg_t
#undef TSP_dtcopy_arg_t
#undef TSP_free_mem_arg_t
#undef TSP_KIND_SEND
#undef TSP_KIND_RECV
#undef TSP_KIND_ADDREF_DT
#undef TSP_KIND_ADDREF_OP
#undef TSP_KIND_DTCOPY
#undef TSP_KIND_FREE_MEM
#undef TSP_KIND_RECV_REDUCE
#undef TSP_KIND_REDUCE_LOCAL
#undef TSP_STATE_INIT
#undef TSP_STATE_ISSUED
#undef TSP_STATE_COMPLETE
#undef TSP_FLAG_REDUCE_L
#undef TSP_FLAG_REDUCE_R

/* Transport APIS */
#undef TSP_init
#undef TSP_comm_init
#undef TSP_comm_cleanup
#undef TSP_dt_init
#undef TSP_op_init
#undef TSP_sched_init
#undef TSP_sched_reset
#undef TSP_sched_commit
#undef TSP_sched_start
#undef TSP_sched_finalize
#undef TSP_init_control_dt
#undef TSP_fence
#undef TSP_wait
#undef TSP_wait4
#undef TSP_opinfo
#undef TSP_isinplace
#undef TSP_dtinfo
#undef TSP_addref_dt
#undef TSP_addref_dt_nb
#undef TSP_addref_op
#undef TSP_addref_op_nb
#undef TSP_send
#undef TSP_recv
#undef TSP_recv_reduce
#undef TSP_send_accumulate
#undef TSP_test
#undef TSP_rank
#undef TSP_size
#undef TSP_reduce_local
#undef TSP_dtcopy
#undef TSP_dtcopy_nb
#undef TSP_allocate_mem
#undef TSP_free_mem
#undef TSP_free_mem_nb
#undef TSP_queryfcn
#undef TSP_record_request_completion
#undef TSP_record_request_issue
#undef TSP_issue_request
#undef TSP_decement_num_unfinished_dependencies

#undef TSP_add_vtx_dependencies

#undef TSP_NAMESPACE
