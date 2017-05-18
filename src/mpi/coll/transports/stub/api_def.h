

#define TRANSPORT_NAME STUB_
#define TRANSPORT_NAME_LC stub

/*Interface types declaration: */
#define TSP_sched_t     MPIC_STUB_sched_t
#define TSP_aint_t      MPIC_STUB_aint_t
#define TSP_comm_t      MPIC_STUB_comm_t
#define TSP_global_t    MPIC_STUB_global_t
#define TSP_dt_t        MPIC_STUB_dt_t
#define TSP_op_t        MPIC_STUB_op_t

#define TSP_global      ((MPIC_global_instance.tsp_stub))

#define TSP_init             MPIC_STUB_init
#define TSP_comm_cleanup     MPIC_STUB_comm_cleanup
#define TSP_comm_init        MPIC_STUB_comm_init
#define TSP_fence            MPIC_STUB_fence
#define TSP_wait             MPIC_STUB_wait
#define TSP_opinfo           MPIC_STUB_opinfo
#define TSP_isinplace        MPIC_STUB_isinplace
#define TSP_dtinfo           MPIC_STUB_dtinfo
#define TSP_addref_dt        MPIC_STUB_addref_dt
#define TSP_addref_dt_nb     MPIC_STUB_addref_dt_nb
#define TSP_addref_op        MPIC_STUB_addref_op
#define TSP_addref_op_nb     MPIC_STUB_addref_op_nb
#define TSP_send             MPIC_STUB_send
#define TSP_recv             MPIC_STUB_recv
#define TSP_recv_reduce      MPIC_STUB_recv_reduce
#define TSP_send_accumulate  MPIC_STUB_send_accumulate
#define TSP_test             MPIC_STUB_test
#define TSP_rank             MPIC_STUB_rank
#define TSP_size             MPIC_STUB_size
#define TSP_reduce_local     MPIC_STUB_reduce_local
#define TSP_dtcopy           MPIC_STUB_dtcopy
#define TSP_dtcopy_nb        MPIC_STUB_dtcopy_nb

#define TSP_allocate_mem     MPIC_STUB_allocate_mem
#define TSP_allocate_buffer  MPIC_STUB_allocate_buffer
#define TSP_free_mem         MPIC_STUB_free_mem
#define TSP_free_buffers     MPIC_STUB_free_buffers
#define TSP_free_mem_nb      MPIC_STUB_free_mem_nb

#define TSP_FLAG_REDUCE_L    MPIC_FLAG_REDUCE_L
#define TSP_FLAG_REDUCE_R    MPIC_FLAG_REDUCE_R

/*Cache API*/
#define TSP_sched_get           MPIC_STUB_sched_get
#define TSP_sched_cache_store   MPIC_STUB_sched_cache_store
#define TSP_sched_commit        MPIC_STUB_sched_commit
#define TSP_sched_start         MPIC_STUB_sched_start
#define TSP_sched_finalize      MPIC_STUB_sched_finalize

