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
static int              topo_initialized = 0;

HYD_status HYDT_bind_hwloc_init(HYDT_bind_support_level_t * support_level)
{
    int node,sock,core,proc,thread;

    hwloc_obj_t    obj_sys;
    hwloc_obj_t    obj_node;
    hwloc_obj_t    obj_sock;
    hwloc_obj_t    obj_core;
    hwloc_obj_t    obj_proc;

    struct HYDT_topo_obj *node_ptr, *sock_ptr, *core_ptr, *thread_ptr;

    HYD_status status = HYD_SUCCESS;    

    HYDU_FUNC_ENTER();
    
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    topo_initialized = 1;

    /* Get the max number of processing elements */
    HYDT_bind_info.total_proc_units = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_PROC);   

    /* We have qualified for basic binding support level */
    *support_level = HYDT_BIND_BASIC;
   
   /* Setup the machine level */   
   /* get the System object */
   obj_sys = hwloc_get_system_obj(topology);
   /* init Hydra structure */
   HYDT_bind_info.machine.type         = HYDT_TOPO_MACHINE;
   HYDT_bind_info.machine.os_index     = -1; /* This is a set, not a single unit */
   HYDT_bind_info.machine.parent       = NULL;
   HYDT_bind_info.machine.num_children = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_NODE);
   /* There is no real node, consider there is one */
   if (!HYDT_bind_info.machine.num_children)
     HYDT_bind_info.machine.num_children = 1;
   HYDU_MALLOC(HYDT_bind_info.machine.children, struct HYDT_topo_obj *,
	       sizeof(struct HYDT_topo_obj), status);
   HYDT_bind_info.machine.shared_memory_depth = NULL;
   
   /* Setup the nodes levels */
   for(node = 0 ; node < HYDT_bind_info.machine.num_children ; node++)
     {	
	node_ptr         = &HYDT_bind_info.machine.children[node];	
	node_ptr->type   =  HYDT_TOPO_NODE;   
	node_ptr->parent = &HYDT_bind_info.machine;
	obj_node = hwloc_get_obj_inside_cpuset_by_type(topology,obj_sys->cpuset,HWLOC_OBJ_NODE,node);

	if(!obj_node)
	  obj_node = obj_sys;
	node_ptr->os_index     = obj_node->os_index;;
	node_ptr->num_children =  hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_node->cpuset,HWLOC_OBJ_SOCKET);

	/* In case there is no socket! */
	if(!node_ptr->num_children)
	  node_ptr->num_children = 1;
	
	HYDU_MALLOC(node_ptr->children, struct HYDT_topo_obj *,
		    sizeof(struct HYDT_topo_obj) * node_ptr->num_children, status);
	node_ptr->shared_memory_depth = NULL;
		
	/* Setup the socket level */
	for (sock = 0; sock < node_ptr->num_children ; sock++) 
	  {
	     sock_ptr           = &node_ptr->children[sock];
	     sock_ptr->type     = HYDT_TOPO_SOCKET;	     
	     sock_ptr->parent   = node_ptr;
	
	     obj_sock = hwloc_get_obj_inside_cpuset_by_type(topology,obj_node->cpuset,HWLOC_OBJ_SOCKET,sock);
	     if(!obj_sock)
	       obj_sock = obj_node;
	     
	     sock_ptr->os_index     = obj_sock->os_index;	  
	     sock_ptr->num_children = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_sock->cpuset,HWLOC_OBJ_CORE);
	     
	     /* In case there is no core! */
	     if(!sock_ptr->num_children)
		 sock_ptr->num_children = 1;

	     HYDU_MALLOC(sock_ptr->children, struct HYDT_topo_obj *,
			 sizeof(struct HYDT_topo_obj) * sock_ptr->num_children, status);
	     sock_ptr->shared_memory_depth = NULL;
	     
	     /* setup the core level */
	     for (core = 0; core < sock_ptr->num_children; core++) 
	       {	     
		  core_ptr               = &sock_ptr->children[core];
		  core_ptr->type         = HYDT_TOPO_CORE;
		  core_ptr->parent       = sock_ptr;
		  
		  obj_core = hwloc_get_obj_inside_cpuset_by_type(topology,obj_sock->cpuset,HWLOC_OBJ_CORE,core);		  
		  if(!obj_core)
		    obj_core = obj_sock;
		  
		  core_ptr->os_index     = obj_core->os_index;	  
		  core_ptr->num_children = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_core->cpuset,HWLOC_OBJ_PROC);
		  		  
		  HYDU_MALLOC(core_ptr->children, struct HYDT_topo_obj *,
			      sizeof(struct HYDT_topo_obj) * core_ptr->num_children, status);
		  core_ptr->shared_memory_depth = NULL;
		  
		  /* setup the thread level */
		  for (thread = 0; thread < core_ptr->num_children; thread++) 
		    {		       
			obj_proc = hwloc_get_obj_inside_cpuset_by_type(topology,obj_core->cpuset,HWLOC_OBJ_PROC,thread);		  
			thread_ptr                      = &core_ptr->children[thread];
			thread_ptr->type                = HYDT_TOPO_THREAD;
			thread_ptr->os_index            = obj_proc->os_index;
			thread_ptr->parent              = core_ptr;
			thread_ptr->num_children        = 0;
			thread_ptr->children            = NULL;
			thread_ptr->shared_memory_depth = NULL;
		       
		       /*
			fprintf(stdout," thread id %i | Core id %i | Socket id %i | Node id %i \n",
			obj_proc->os_index,obj_core->os_index,obj_sock->os_index,obj_node->os_index);
			*/ 
		    }
		  
	       }
	  }
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
    hwloc_cpuset_t cpuset = hwloc_cpuset_alloc();

    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();
    
    /* If the specified core is negative, we just ignore it */
    if (core < 0)
        goto fn_exit;
       
    hwloc_cpuset_set (cpuset, core);
    if (!topo_initialized)
     {
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
	topo_initialized = 1;
     }
    hwloc_set_cpubind(topology,cpuset,HWLOC_CPUBIND_THREAD);

   
  fn_exit:
    hwloc_cpuset_free(cpuset);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
