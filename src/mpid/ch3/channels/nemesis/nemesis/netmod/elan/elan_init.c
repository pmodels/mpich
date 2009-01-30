
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <elan/elan.h>
#include <elan/capability.h>
#include <elan/elanctrl.h>
#include "mpidimpl.h"
#include "mpid_nem_impl.h"
#include "elan_impl.h"

MPID_nem_netmod_funcs_t MPIDI_nem_elan_funcs = {
    MPID_nem_elan_init,
    MPID_nem_elan_finalize,
    MPID_nem_elan_ckpt_shutdown,
    MPID_nem_elan_poll,
    MPID_nem_elan_send,
    MPID_nem_elan_get_business_card,
    MPID_nem_elan_connect_to_root,
    MPID_nem_elan_vc_init,
    MPID_nem_elan_vc_destroy,
    MPID_nem_elan_vc_terminate
};


#define MPID_NEM_ELAN_ALLOC_SIZE         16 
#define MPIDI_CH3I_QUEUE_PTR_KEY         "q_ptr_val"
#define MPIDI_CH3I_ELAN_VPID_KEY         "elan_vpid"
#define MPID_NEM_ELAN_CONTEXT_ID_OFFSET  2

ELAN_QUEUE_TX     **rxq_ptr_array;
ELAN_QUEUE_TX      *mpid_nem_elan_recv_queue_ptr;
static ELAN_QUEUE  *localq_ptr; 
static ELAN_QUEUE **localq_ptr_val; 
static int         *node_ids;  
static int          my_node_id;
static int          min_node_id;
static int          max_node_id;
static int          my_ctxt_id;

int  MPID_nem_elan_freq = 0;
int  MPID_nem_module_elan_pendings_sends = 0;
int *MPID_nem_elan_vpids = NULL;

static MPID_nem_elan_event_queue_t _elan_free_event_q;
static MPID_nem_elan_event_queue_t _elan_pending_event_q;
static MPID_nem_queue_t            _free_queue;

MPID_nem_elan_event_queue_ptr_t MPID_nem_module_elan_free_event_queue    = &_elan_free_event_q ;
MPID_nem_elan_event_queue_ptr_t MPID_nem_module_elan_pending_event_queue = &_elan_pending_event_q ;
MPID_nem_elan_cell_ptr_t        MPID_nem_module_elan_cells       = 0;
MPID_nem_queue_ptr_t            MPID_nem_module_elan_free_queue  = 0;
MPID_nem_queue_ptr_t            MPID_nem_process_recv_queue      = 0;
MPID_nem_queue_ptr_t            MPID_nem_process_free_queue      = 0;

static 
int my_compar(const void *a, const void *b)
{
   int _a = *(int *)a;
   int _b = *(int *)b;
   
   if ( _a <= _b ) 
     return -1;
   else
     return 1;
}

#undef FUNCNAME
#define FUNCNAME init_elan
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int init_elan( MPIDI_PG_t *pg_p )
{
   char            capability_str[MPID_NEM_ELAN_ALLOC_SIZE];
   int             mpi_errno = MPI_SUCCESS;
   char            file_name[256];
   char            line[255]; 
   int             numprocs  = MPID_nem_mem_region.ext_procs;
   char            * key;
   char            * val;
   int             key_max_sz;
   int             val_max_sz;
   char           *kvs_name;
   FILE           *myfile;
   int             ncells;
   int             grank;
   int             index; 
   int             pmi_errno;
   int             ret;
   ELAN_BASE      *base = NULL;
   ELAN_FLAGS      flags;
   MPIU_CHKLMEM_DECL(2);

   /* Allocate space for pmi keys and values */
   pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
   MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
   MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

   pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
   MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
   MPIU_CHKLMEM_MALLOC(val, char *, val_max_sz, mpi_errno, "val");
   
   if ( !getenv("ELAN_AUTO") && !getenv("RMS_NPROCS") ) {
       /* Get My Node Id from relevant file */
       myfile = fopen("/proc/qsnet/elan3/device0/position","r");
       if (myfile == NULL) 
       {
	   myfile = fopen("/proc/qsnet/elan4/device0/position","r");
       }
   
       if (myfile != NULL)
       {	
	   ret = fscanf(myfile,"%s%i",&line,&my_node_id);
       }
       else
       {
	   /* Error */
       }
       
       mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);      
       
       /* Put My Node Id */
       for (index = 0 ; index < numprocs ; index++)
       {	
	   grank = MPID_nem_mem_region.ext_ranks[index];
	   MPIU_Snprintf (val, key_max_sz, "%i",my_node_id);
	   MPIU_Snprintf (key, key_max_sz, "QsNetkey[%d:%d]", MPID_nem_mem_region.rank, grank);
	   
	   pmi_errno = PMI_KVS_Put (kvs_name, key, val);
	   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
	   
	   pmi_errno = PMI_KVS_Commit (kvs_name);
	   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
       }   
       pmi_errno = PMI_Barrier();
       MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
       
       /* Get Node Ids from others */
       node_ids = (int *)MPIU_Malloc(numprocs * sizeof(int));
       for (index = 0 ; index < numprocs ; index++)
       {
	   grank = MPID_nem_mem_region.ext_ranks[index];
	   memset(val, 0, key_max_sz);
	   MPIU_Snprintf (key, key_max_sz,"QsNetkey[%d:%d]", grank, MPID_nem_mem_region.rank);
	   
	   pmi_errno = PMI_KVS_Get (kvs_name, key, val, key_max_sz);
	   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
	   
	   ret = sscanf (val, "%i", &(node_ids[index]));
	   MPIU_ERR_CHKANDJUMP1 (ret != 1, mpi_errno, MPI_ERR_OTHER, "**business_card", "**business_card %s", val);	
       }
       pmi_errno = PMI_Barrier();
       MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
       
       /* Compute Min and Max  Ids*/
       qsort(node_ids, numprocs, sizeof(int), my_compar);   
       
       if (node_ids[0] < my_node_id)
	   min_node_id = node_ids[0] ;
       else
	   min_node_id = my_node_id ;
       
       if (node_ids[numprocs - 1] > my_node_id)
	   max_node_id = node_ids[numprocs - 1] ;
       else
	   max_node_id = my_node_id;
       
       /* Generate capability string */
       MPIU_Snprintf(capability_str, MPID_NEM_ELAN_ALLOC_SIZE, "N%dC%d-%d-%dN%d-%dR1b",
		     my_node_id,
		     MPID_NEM_ELAN_CONTEXT_ID_OFFSET,
		     MPID_NEM_ELAN_CONTEXT_ID_OFFSET+MPID_nem_mem_region.local_rank,
		     MPID_NEM_ELAN_CONTEXT_ID_OFFSET+(MPID_nem_mem_region.num_local - 1),
		     min_node_id,max_node_id);      
       elan_generateCapability (capability_str);    
   }
   
   /* Init Elan */
   base = elan_baseInit(0);
   /* From this point, we can use elan_base pointer, which is not declared anywhere */
   
   MPID_nem_elan_vpids = (int *)MPIU_Malloc(MPID_nem_mem_region.num_procs*sizeof(int));
   for (index = 0 ; index < MPID_nem_mem_region.num_procs ; index++)
     MPID_nem_elan_vpids[index] = -1 ;
   MPID_nem_elan_vpids[MPID_nem_mem_region.rank] = elan_base->state->vp ;
   
   /* Enable the network */
   elan_enable_network(elan_base->state);

   /* Allocate more than needed */
   rxq_ptr_array  = (ELAN_QUEUE_TX **)MPIU_Malloc(MPID_nem_mem_region.num_procs*sizeof(ELAN_QUEUE_TX *));   
   localq_ptr     = elan_allocQueue(elan_base->state);      
   localq_ptr_val = (ELAN_QUEUE **)MPIU_Malloc(sizeof(ELAN_QUEUE *));   
  *localq_ptr_val = localq_ptr ;
	   
   /* For now, one Quadrics'cell equals to one Nemesis'cell */
   MPIU_Assert( (MPID_NEM_ELAN_SLOT_SIZE) <= (elan_queueMaxSlotSize(elan_base->state)));
   
   for (index = 0 ; index < MPID_nem_mem_region.num_procs ; index++) 
     rxq_ptr_array[index] = NULL ; 
   
   ncells = MPID_NEM_ELAN_NUM_SLOTS*numprocs;
   if(ncells > MPID_NEM_ELAN_MAX_NUM_SLOTS)
     ncells = MPID_NEM_ELAN_MAX_NUM_SLOTS;
   
   rxq_ptr_array[MPID_nem_mem_region.rank] = elan_queueRxInit(elan_base->state,
							      localq_ptr,
							      ncells,
							      MPID_NEM_ELAN_SLOT_SIZE,
							      MPID_NEM_ELAN_RAIL_NUM,
							      flags);   
   mpid_nem_elan_recv_queue_ptr = rxq_ptr_array[MPID_nem_mem_region.rank] ;     
   MPID_nem_elan_freq           = 1 ;
   MPID_nem_module_elan_cells   = (MPID_nem_elan_cell_ptr_t)MPIU_Calloc( MPID_NEM_ELAN_NUM_SLOTS, sizeof(MPID_nem_elan_cell_t));
   MPID_nem_module_elan_free_event_queue->head    = NULL;
   MPID_nem_module_elan_free_event_queue->tail    = NULL;   
   MPID_nem_module_elan_pending_event_queue->head = NULL;
   MPID_nem_module_elan_pending_event_queue->tail = NULL;   
   for (index = 0; index < MPID_NEM_ELAN_NUM_SLOTS ; ++index)
     {
	MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,&MPID_nem_module_elan_cells[index]);
     }
   
   fn_exit:
     MPIU_CHKLMEM_FREEALL();
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

/*
 int  
   MPID_nem_elan_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
	          MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		  MPID_nem_queue_ptr_t *module_free_queue)

   IN
       proc_recv_queue -- main recv queue for the process
       proc_free_queue -- main free queueu for the process
       proc_elements -- pointer to the process' queue elements
       num_proc_elements -- number of process' queue elements
       module_elements -- pointer to queue elements to be used by this module
       num_module_elements -- number of queue elements for this module
       ckpt_restart -- true if this is a restart from a checkpoint.  In a restart, the network needs to be brought up again, but
                       we want to keep things like sequence numbers.
   OUT
       free_queue -- pointer to the free queue for this module.  The process will return elements to
                     this queue
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		MPID_nem_queue_ptr_t proc_free_queue, 
		MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		MPIDI_PG_t *pg_p, int pg_rank,
		char **bc_val_p, int *val_max_sz_p)
{   
   int mpi_errno = MPI_SUCCESS ;
   int index;
   
   /* first make sure that our private fields in the vc fit into the area provided  */
   MPIU_Assert(sizeof(MPID_nem_elan_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

   if( MPID_nem_mem_region.ext_procs > 0)
     {
	init_elan(pg_p);
	mpi_errno = MPID_nem_elan_get_business_card (pg_rank, bc_val_p, val_max_sz_p);
	if (mpi_errno) MPIU_ERR_POP (mpi_errno);		
     }   
   
   MPID_nem_process_recv_queue = proc_recv_queue;
   MPID_nem_process_free_queue = proc_free_queue;   
   
   MPID_nem_module_elan_free_queue = &_free_queue;
   MPID_nem_queue_init (MPID_nem_module_elan_free_queue);
   for (index = 0; index < num_module_elements; ++index)
     {
	MPID_nem_queue_enqueue (MPID_nem_module_elan_free_queue, &module_elements[index]);
     }
   
   *module_free_queue = MPID_nem_module_elan_free_queue;

   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
  MPID_nem_elan_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
   int mpi_errno = MPI_SUCCESS;

   mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ELAN_VPID_KEY, elan_base->state->vp);
   if (mpi_errno != MPIU_STR_SUCCESS)
     {	
	if (mpi_errno == MPIU_STR_NOMEM)
	  {	     
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	  }       
	else
	  {	     
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	  }	
	goto fn_exit;
     }   
   
   mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_QUEUE_PTR_KEY, (char *)&(*localq_ptr_val), sizeof(ELAN_QUEUE *));
   
   if (mpi_errno != MPIU_STR_SUCCESS)
     {	
	if (mpi_errno == MPIU_STR_NOMEM)
	  {	     
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	  }	
	else
	  {	     
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	  }	
	goto fn_exit;
     }

   MPIU_Free(localq_ptr_val);
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_get_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_get_from_bc (const char *business_card,ELAN_QUEUE **remoteq_ptr, int *vpid)
{
   int mpi_errno = MPI_SUCCESS;
   int tmp_vpid;
   int len;

   mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_ELAN_VPID_KEY, &tmp_vpid);
   if (mpi_errno != MPIU_STR_SUCCESS)
     {	
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
     }
   *vpid = tmp_vpid;
   
   mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_QUEUE_PTR_KEY,(char *)remoteq_ptr, sizeof(ELAN_QUEUE *), &len);
   if ((mpi_errno != MPIU_STR_SUCCESS) || len != sizeof(ELAN_QUEUE *))
     {	
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
     }
   
   fn_exit:
     return mpi_errno;
  fn_fail:  
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
   int mpi_errno = MPI_SUCCESS;   
   if( MPID_nem_mem_region.ext_procs > 0)
     {
	ELAN_QUEUE *remoteq_ptr ; 
	ELAN_FLAGS  flags;
        int         vpid;
	  
	mpi_errno = MPID_nem_elan_get_from_bc (business_card, &remoteq_ptr, &vpid);
	/* --BEGIN ERROR HANDLING-- */   
	if (mpi_errno) 
	  {	
	     MPIU_ERR_POP (mpi_errno);
	  }
	/* --END ERROR HANDLING-- */
	
	rxq_ptr_array[vc->pg_rank]       = elan_queueTxInit(elan_base->state,remoteq_ptr,MPID_NEM_ELAN_RAIL_NUM,flags);
	MPID_nem_elan_vpids[vc->pg_rank] = vpid;

	VC_FIELD(vc, rxq_ptr_array) = rxq_ptr_array;   
	VC_FIELD(vc, vpid)          = vpid;
     }   
   fn_exit:   
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_elan_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   
   fn_exit:   
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_elan_vc_terminate (MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}
