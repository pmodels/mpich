#define TRANSPORT_NAME MPICH_
#define TRANSPORT_NAME_LC mpich

/* Transport types*/
#define TSP_sched_t     MPIC_MPICH_sched_t
#define TSP_aint_t      MPIC_MPICH_aint_t
#define TSP_comm_t      MPIC_MPICH_comm_t
#define TSP_global_t    MPIC_MPICH_global_t
#define TSP_dt_t        MPIC_MPICH_dt_t
#define TSP_op_t        MPIC_MPICH_op_t

#define TSP_global      ((MPIC_global_instance.tsp_mpich))
/*General transport API*/
#define TSP_init             MPIC_MPICH_init
#define TSP_comm_cleanup     MPIC_MPICH_comm_cleanup
#define TSP_comm_init        MPIC_MPICH_comm_init
#define TSP_init_control_dt  MPIC_MPICH_init_control_dt
#define TSP_fence            MPIC_MPICH_fence
#define TSP_wait             MPIC_MPICH_wait
#define TSP_opinfo           MPIC_MPICH_opinfo
#define TSP_isinplace        MPIC_MPICH_isinplace
#define TSP_dtinfo           MPIC_MPICH_dtinfo
#define TSP_addref_dt        MPIC_MPICH_addref_dt
#define TSP_addref_dt_nb     MPIC_MPICH_addref_dt_nb
#define TSP_addref_op        MPIC_MPICH_addref_op
#define TSP_addref_op_nb     MPIC_MPICH_addref_op_nb
#define TSP_send             MPIC_MPICH_send
#define TSP_recv             MPIC_MPICH_recv
#define TSP_recv_reduce      MPIC_MPICH_recv_reduce
#define TSP_send_accumulate  MPIC_MPICH_send_accumulate
#define TSP_test             MPIC_MPICH_test
#define TSP_rank             MPIC_MPICH_rank
#define TSP_size             MPIC_MPICH_size
#define TSP_reduce_local     MPIC_MPICH_reduce_local
#define TSP_dtcopy           MPIC_MPICH_dtcopy
#define TSP_dtcopy_nb        MPIC_MPICH_dtcopy_nb
#define TSP_allocate_mem     MPIC_MPICH_allocate_mem
#define TSP_allocate_buffer  MPIC_MPICH_allocate_buffer
#define TSP_free_mem         MPIC_MPICH_free_mem
#define TSP_free_mem_nb      MPIC_MPICH_free_mem_nb
#define TSP_free_buffers     MPIC_MPICH_free_buffers

#define TSP_FLAG_REDUCE_L    MPIC_FLAG_REDUCE_L
#define TSP_FLAG_REDUCE_R    MPIC_FLAG_REDUCE_R

/* Schedule API*/
/* Create/load schedule, reset it in case of load */
#define TSP_get_schedule           MPIC_MPICH_get_schedule
/* Push schedule into cache, in case of implemented cache */
#define TSP_save_schedule   MPIC_MPICH_save_schedule
/*Mark schedule as ready for execution. */
#define TSP_sched_start         MPIC_MPICH_sched_start
#define TSP_sched_start_nb      MPIC_MPICH_sched_start
/* Release schedule (destroy in case of disabled caching) */
#define TSP_sched_finalize      MPIC_MPICH_sched_finalize
#define TSP_sched_commit        MPIC_MPICH_sched_commit
