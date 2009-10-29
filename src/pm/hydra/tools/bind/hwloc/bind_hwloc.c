/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bind.h"
#include "bind_hwloc.h"

struct HYDT_bind_info HYDT_bind_info;

static hwloc_topology_t topology;

HYD_status HYDT_bind_hwloc_init(HYDT_bind_support_level_t * support_level)
{
    hwloc_obj_t    obj_sys;
    hwloc_obj_t    obj_node;
    hwloc_obj_t    obj_sock;
    hwloc_obj_t    obj_core;
    hwloc_obj_t    obj_proc;
    hwloc_obj_t    prev_obj;
    hwloc_cpuset_t cpuset_sys;
    hwloc_cpuset_t cpuset_node;
    hwloc_cpuset_t cpuset_sock;
    hwloc_cpuset_t cpuset_core;

    int node,sock,core,proc,thread;
    int bound,bound2,bound3;

    HYD_status status = HYD_SUCCESS;    

    HYDU_FUNC_ENTER();
    
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    /* Get the max number of processing elements */
    HYDT_bind_info.num_procs = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_PROC);   

    HYDU_MALLOC(HYDT_bind_info.topology, struct HYDT_topology *,
                HYDT_bind_info.num_procs * sizeof(struct HYDT_topology), status);
    for (proc = 0; proc < HYDT_bind_info.num_procs; proc++) {
        HYDT_bind_info.topology[proc].processor_id = -1;
        HYDT_bind_info.topology[proc].socket_rank  = -1;
        HYDT_bind_info.topology[proc].socket_id    = -1;
        HYDT_bind_info.topology[proc].core_rank    = -1;
        HYDT_bind_info.topology[proc].core_id      = -1;
        HYDT_bind_info.topology[proc].thread_rank  = -1;
        HYDT_bind_info.topology[proc].thread_id    = -1;
    }

    proc     = 0;
    prev_obj = NULL;
    while(obj_proc = hwloc_get_next_obj_by_type(topology,HWLOC_OBJ_PROC,prev_obj))
        {
            HYDT_bind_info.topology[proc].processor_id = obj_proc->os_index;
            prev_obj = obj_proc;
            proc++;
        }
    assert(proc == HYDT_bind_info.num_procs);
    
    /* We have qualified for basic binding support level */
    *support_level = HYDT_BIND_BASIC;

    /* get the System object */
    obj_sys = hwloc_get_system_obj(topology);
    cpuset_sys = obj_sys->cpuset;

    /* Compute the number of sockets per NUMA Node */
   /* FIX ME : We assume that the number of sockets is the same for each node (e.g node 0)!*/
   obj_node = hwloc_get_obj_inside_cpuset_by_type(topology,cpuset_sys,HWLOC_OBJ_NODE,0);
   if(obj_node)
     HYDT_bind_info.num_sockets = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_node->cpuset,HWLOC_OBJ_SOCKET);
   else
     HYDT_bind_info.num_sockets = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_SOCKET);
   
   /* Compute the number of cores per socket */
   /* FIX ME : We assume that the number of CORES is the same for each SOCKET (e.g sock 0)!*/
   obj_sock = hwloc_get_obj_by_type(topology,HWLOC_OBJ_SOCKET,0);      
   if(obj_node)
     HYDT_bind_info.num_cores = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_sock->cpuset,HWLOC_OBJ_CORE);    
   else
     HYDT_bind_info.num_cores = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_CORE);
    
    HYDT_bind_info.num_threads = (HYDT_bind_info.num_procs/(hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_CORE))); 

    bound = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_NODE);
    for (proc = 0; proc < HYDT_bind_info.num_procs; proc++)
        {
	   if (bound == 0)
	     {
		for(sock = 0, bound2 = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_SOCKET) ; 
		    sock < bound2 ; 
		    sock++)
		  {
		     obj_sock = obj_sock = hwloc_get_obj_by_type(topology,HWLOC_OBJ_SOCKET,sock);
		     cpuset_sock  = obj_sock->cpuset;
		     
		     if(hwloc_cpuset_isset (cpuset_sock, HYDT_bind_info.topology[proc].processor_id))
		       {
			  for(core = 0, bound3 = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_sock->cpuset,HWLOC_OBJ_CORE) ; 
			      core < bound3; 
			      core++)
			    {
			       obj_core = hwloc_get_obj_inside_cpuset_by_type(topology,cpuset_sock,HWLOC_OBJ_CORE,core);
			       cpuset_core  = obj_core->cpuset;
			       
			       if (hwloc_cpuset_isset (cpuset_core, HYDT_bind_info.topology[proc].processor_id))
				 {
				    int j;
				    /* HYDT_bind_info.topology[proc].node_id   = obj_node->os_index;*/
				    HYDT_bind_info.topology[proc].socket_id = obj_sock->os_index;
				    HYDT_bind_info.topology[proc].core_id   = obj_core->os_index;
				    
				    thread = -1;
				    for (j = 0; j < proc; j++)
				      {
					 /*  if ((HYDT_bind_info.topology[j].node_id   == obj_node->os_index) && 
					  (HYDT_bind_info.topology[j].socket_id == obj_sock->os_index) &&
					  (HYDT_bind_info.topology[j].core_id   == obj_core->os_index))
					  */
					 if ((HYDT_bind_info.topology[j].socket_id == obj_sock->os_index) &&
					     (HYDT_bind_info.topology[j].core_id   == obj_core->os_index))
					   thread = HYDT_bind_info.topology[j].thread_id;
				      }
				    thread++;
				    HYDT_bind_info.topology[proc].thread_id   = thread;
				    HYDT_bind_info.topology[proc].thread_rank = thread;
				    
				    break;
				 }
			    }
		       }
		  }
	     }
	   else
	     {		
		for(node = 0;  node < bound ; node++)
		  {
		     obj_node = hwloc_get_obj_inside_cpuset_by_type(topology,cpuset_sys,HWLOC_OBJ_NODE,node);
		     cpuset_node = obj_node->cpuset;
		     
		     if(hwloc_cpuset_isset (cpuset_node, HYDT_bind_info.topology[proc].processor_id))
		       {
			  for(sock = 0, bound2 = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_node->cpuset,HWLOC_OBJ_SOCKET) ; sock < bound2 ; sock++)
			    {
			       obj_sock = hwloc_get_obj_inside_cpuset_by_type(topology,cpuset_node,HWLOC_OBJ_SOCKET,sock);
			       cpuset_sock  = obj_sock->cpuset;
			       
			       if(hwloc_cpuset_isset (cpuset_sock, HYDT_bind_info.topology[proc].processor_id))
				 {
				    for(core = 0, bound3 = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_sock->cpuset,HWLOC_OBJ_CORE) ; core < bound3; core+\
+)
				      {
					 obj_core = hwloc_get_obj_inside_cpuset_by_type(topology,cpuset_sock,HWLOC_OBJ_CORE,core);
					 cpuset_core  = obj_core->cpuset;
					 
					 if (hwloc_cpuset_isset (cpuset_core, HYDT_bind_info.topology[proc].processor_id))
					   {
					      int j;
					      /* HYDT_bind_info.topology[proc].node_id   = obj_node->os_index;*/
					      HYDT_bind_info.topology[proc].socket_id = obj_sock->os_index;
					      HYDT_bind_info.topology[proc].core_id   = obj_core->os_index;

					      thread = -1;
					      for (j = 0; j < proc; j++)
						{
						   /*  if ((HYDT_bind_info.topology[j].node_id   == obj_node->os_index) && 
						    (HYDT_bind_info.topology[j].socket_id == obj_sock->os_index) &&
						    (HYDT_bind_info.topology[j].core_id   == obj_core->os_index))
						    */
						   if ((HYDT_bind_info.topology[j].socket_id == obj_sock->os_index) &&
						       (HYDT_bind_info.topology[j].core_id   == obj_core->os_index))
						     thread = HYDT_bind_info.topology[j].thread_id;
						}
					      thread++;
					      HYDT_bind_info.topology[proc].thread_id   = thread;
					      HYDT_bind_info.topology[proc].thread_rank = thread;
					      
					      break;
					   }
				      }
				 }
			    }
			  
		       }
		  }
	     }
	}
   
   
   /* Get the rank of each node ID */
   /*
    for(node = 0, bound  = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_NODE) ; node < bound ; node++)
    {
    obj_node  = hwloc_get_obj_by_type(topology,HWLOC_OBJ_NODE,node);
    for (proc = 0; proc < HYDT_bind_info.num_procs; proc++)
    if (HYDT_bind_info.topology[proc].node_id == obj_node->os_index)
                    HYDT_bind_info.topology[proc].node_rank = node;
    }
    */
   
   /* Get the rank of each socket ID */
   for(sock = 0, bound = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_SOCKET) ; sock < bound; sock++)
     {
	obj_sock  = hwloc_get_obj_by_type(topology,HWLOC_OBJ_SOCKET,sock);
	for (proc = 0; proc < HYDT_bind_info.num_procs; proc++) 
	  if (HYDT_bind_info.topology[proc].socket_id == obj_sock->os_index)
	    HYDT_bind_info.topology[proc].socket_rank = sock;
     }
   
   /* Find the rank of each core ID */
   for (core = 0, bound = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_CORE); core < bound ; core++) 
     {
	obj_core  = hwloc_get_obj_by_type(topology,HWLOC_OBJ_CORE,core);
	for (proc = 0; proc < HYDT_bind_info.num_procs; proc++) 
	  if (HYDT_bind_info.topology[proc].core_id == obj_core->os_index)
	    HYDT_bind_info.topology[proc].core_rank = core;
     }
   
   /* We have qualified for topology-aware binding support level */
   *support_level = HYDT_BIND_TOPO;
   
   fn_exit:
   HYDU_FUNC_EXIT();
   return status;
   
   fn_fail:
   goto fn_exit;
}

HYD_status HYDT_bind_hwloc_process(int core)
{
    hwloc_cpuset_t cpuset = 0;

    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();
    
    /* If the specified core is negative, we just ignore it */
    if (core < 0)
        goto fn_exit;
       
    hwloc_cpuset_zero(cpuset); 
    hwloc_cpuset_set (cpuset, core % HYDT_bind_info.num_procs);
    hwloc_set_cpubind(topology,cpuset,HWLOC_CPUBIND_THREAD);
    
  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
