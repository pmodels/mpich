/* Transport Types */
#undef TSP_dt_t
#undef TSP_op_t
#undef TSP_comm_t
#undef TSP_req_t
#undef TSP_sched_t
#undef TSP_aint_t
#undef TSP_global_t

/* NB transport argument types */
#undef TSP_sendrecv_arg_t
#undef TSP_recv_reduce_arg_t
#undef TSP_send_accumulate_arg_t
#undef TSP_addref_dt_arg_t
#undef TSP_addref_op_arg_t
#undef TSP_dtcopy_arg_t
#undef TSP_free_mem_arg_t
#undef TSP_reduce_localarg_t
#undef TSP_KIND_SEND
#undef TSP_KIND_RECV
#undef TSP_KIND_ADDREF_DT
#undef TSP_KIND_ADDREF_OP
#undef TSP_KIND_DTCOPY
#undef TSP_KIND_FREE_MEM
#undef TSP_KIND_RECV_REDUCE
#undef TSP_KIND_SEND_ACCUMULATE
#undef TSP_KIND_REDUCE_LOCAL
#undef TSP_STATE_INIT
#undef TSP_STATE_ISSUED
#undef TSP_STATE_COMPLETE
#undef TSP_FLAG_REDUCE_L
#undef TSP_FLAG_REDUCE_R


/* Transport global variables */
#undef TSP_global

/* Transport APIS */
#undef TSP_init
#undef TSP_sched_init
#undef TSP_sched_commit
#undef TSP_sched_start
#undef TSP_sched_finalize
#undef TSP_init_control_dt
#undef TSP_fence
#undef TSP_wait
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
#undef TSP_set_trigger
#undef TSP_rank
#undef TSP_size
#undef TSP_reduce_local
#undef TSP_dtcopy
#undef TSP_dtcopy_nb
#undef TSP_allocate_mem
#undef TSP_free_mem
#undef TSP_free_mem_nb

#undef TSP_NAMESPACE
#undef GLOBAL_NAME
#undef TRANSPORT_NAME
