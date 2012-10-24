/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/***********************************************************************
 * Information associated with a specific executable image
 */
typedef struct 
{
    const struct mqs_image_callbacks * image_callbacks; /* Functions needed here */

    /* in the embedded MPI_Status object */
    int count_offs;
    int MPI_SOURCE_offs;
    int MPI_TAG_offs;
    
    /* Fields in MPIR_Comm_list */
    int sequence_number_offs;
    int comm_head_offs;
    
    /* Fields in MPID_Comm */
    int comm_rsize_offs;
    int lrank_to_grank_offs;
    int comm_rank_offs;
    int comm_context_id_offs;
    int comm_recvcontext_id_offs;
    int comm_next_offs;
    int comm_name_offs;
    
    /* Fields in MPID_Request (including structures within the request) */
    int req_status_offs;
    int req_cc_offs;
    int req_dev_offs;
    int req_next_offs;
    int req_tag_offs;
    int req_rank_offs;
    int req_context_id_offs;
    int req_user_buf_offs;
    int req_user_count_offs;
    int req_datatype_offs;
    
    /* Fields in MPIR_Sendq */
    int sendq_next_offs;
    int sendq_tag_offs;
    int sendq_rank_offs;
    int sendq_context_id_offs;
    int sendq_req_offs;
} mpich_image_info; 

/***********************************************************************
 * Information associated with a specific process
 */

typedef struct group_t
{
  mqs_taddr_t table_base;			/* Where was it in the process  */
  int     ref_count;				/* How many references to us */
  int     entries;				/* How many entries */
  int     *local_to_global;			/* The translation table */
} group_t;

/* Information for a single process, a list of communicators, some
 * useful addresses, and the state of the iterators.
 */
typedef struct 
{
  const struct mqs_process_callbacks * process_callbacks; /* Functions needed here */

  struct communicator_t *communicator_list;	/* List of communicators in the process */
  mqs_target_type_sizes sizes;			/* Process architecture information */

  /* Addresses in the target process */
  mqs_taddr_t posted_base;		/* Where to find the message queues */
  mqs_taddr_t unexpected_base;
  mqs_taddr_t sendq_base;		/* Where to find the send queue */
  mqs_taddr_t commlist_base;		/* Where to find the list of communicators */

  /* Other info we need to remember about it */
  mqs_tword_t communicator_sequence;		
  int has_sendq;

  /* State for the iterators */
  struct communicator_t *current_communicator;	/* Easy, we're walking a simple list */
    
  mqs_taddr_t   next_msg;			/* And state for the message iterator */
  mqs_op_class  what;				/* What queue are we looking on */
} mpich_process_info;





