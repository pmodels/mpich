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
    int proc,sock,core,thread,i,j;
    int num_cores = 0;
    int my_num_cores; 
    int my_num_threads; 
    
    hwloc_obj_t    obj_sock,obj_core,obj_proc;
    hwloc_obj_t    prev_obj = NULL;
    hwloc_cpuset_t cpuset_sock,cpuset_core;

    HYD_status status = HYD_SUCCESS;    

    HYDU_FUNC_ENTER();
    
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    /* Get the max number of processing elements */
    HYDT_bind_info.num_procs = hwloc_get_nbobjs_by_type(topology,HWLOC_OBJ_PROC);   
    HYDU_MALLOC(HYDT_bind_info.topology, struct HYDT_topology *,
                HYDT_bind_info.num_procs * sizeof(struct HYDT_topology), status);
    for (i = 0; i < HYDT_bind_info.num_procs; i++) {
        HYDT_bind_info.topology[i].processor_id = -1;
        HYDT_bind_info.topology[i].socket_rank = -1;
        HYDT_bind_info.topology[i].socket_id = -1;
        HYDT_bind_info.topology[i].core_rank = -1;
        HYDT_bind_info.topology[i].core_id = -1;
        HYDT_bind_info.topology[i].thread_rank = -1;
        HYDT_bind_info.topology[i].thread_id = -1;
    }

    do{
	obj_proc = hwloc_get_next_obj_by_type(topology,HWLOC_OBJ_PROC,prev_obj);
	if (!obj_proc) {
            /* Unable to get processor ID */
            HYDU_warn_printf("hwloc get processor id failed\n");
            if (HYDT_bind_info.topology)
                HYDU_FREE(HYDT_bind_info.topology);
            goto fn_fail;
        }	
	HYDT_bind_info.topology[i].processor_id = obj_proc->os_index;		
	prev_obj = obj_proc;
    }while(prev_obj);
	
    /* We have qualified for basic binding support level */
    *support_level = HYDT_BIND_BASIC;

    /* Compute the total number of sockets */
    HYDT_bind_info.num_sockets = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_SOCKET);   
    assert(HYDT_bind_info.num_sockets);

    /* Compute the total number of cores */
    my_num_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);   
    assert(my_num_cores);

    /* Compute again the total number of cores */
    for(i=0; i< HYDT_bind_info.num_sockets ; i++)
	{	
	    obj_sock = hwloc_get_obj_by_type(topology,HWLOC_OBJ_SOCKET,i);      
	    num_cores += hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_sock->cpuset,HWLOC_OBJ_CORE);    
	}
    
    /* if this fails, it means that the number of cores */
    /* per socket is not always the same !*/
    assert(my_num_cores == num_cores);

    HYDT_bind_info.num_cores = my_num_cores/HYDT_bind_info.num_sockets;
    
    /* Alternate version : We take the amount on socket 0 as the reference */
    /*
    obj_sock = hwloc_get_obj_by_type(topology,HWLOC_OBJ_SOCKET,0);
    HYDT_bind_info.num_cores = hwloc_get_nbobjs_inside_cpuset_by_type(topology,&(obj_sock->cpuset),HWLOC_OBJ_CORE);
    */

    my_num_threads = HYDT_bind_info.num_procs/my_num_cores;
    
    /* Alternate version */
    HYDT_bind_info.num_threads = HYDT_bind_info.num_procs /
        (HYDT_bind_info.num_sockets * HYDT_bind_info.num_cores);

    /* these should be the same */
    assert(HYDT_bind_info.num_threads == my_num_threads);

    /* Find the socket and core IDs for all processor IDs */        
    for(sock = 0 ; sock < HYDT_bind_info.num_sockets; sock++)
	{
	    obj_sock  = hwloc_get_obj_by_type(topology,HWLOC_OBJ_SOCKET,sock);
	    cpuset_sock  = obj_sock->cpuset;
	    num_cores = hwloc_get_nbobjs_inside_cpuset_by_type(topology,obj_sock->cpuset,HWLOC_OBJ_CORE);

	    for(core = 0 ; core < num_cores; core++)
		{       
		    obj_core = hwloc_get_obj_inside_cpuset_by_type(topology,cpuset_sock,HWLOC_OBJ_CORE,core);
		    cpuset_core  = obj_core->cpuset;
		    for (proc = 0; proc < HYDT_bind_info.num_procs; proc++) 
			{
			    if((hwloc_cpuset_isset (cpuset_sock, proc)) && 
			       (hwloc_cpuset_isset (cpuset_core, proc)))
				{
				    HYDT_bind_info.topology[proc].socket_id = sock;
				    HYDT_bind_info.topology[proc].core_id   = core;
				 
				    thread = -1;
				    for (j = 0; j < proc; j++)
					if ((HYDT_bind_info.topology[j].socket_id == sock) &&
					    (HYDT_bind_info.topology[j].core_id   == core))
					    thread = HYDT_bind_info.topology[j].thread_id;
				    thread++;
				    
				    HYDT_bind_info.topology[proc].thread_id = thread;
				    HYDT_bind_info.topology[proc].thread_rank = thread;	
				    
				    break;
				}
			}
		}
	}

    /* Get the rank of each socket ID */
    for(sock = 0 ; sock < HYDT_bind_info.num_sockets; sock++)
	{
	    obj_sock  = hwloc_get_obj_by_type(topology,HWLOC_OBJ_SOCKET,sock);
	    for (proc = 0; proc < HYDT_bind_info.num_procs; proc++) 
		if (HYDT_bind_info.topology[proc].socket_id == obj_sock->os_index)
		    HYDT_bind_info.topology[proc].socket_rank = sock;
	}

    /* Find the rank of each core ID */
    for (core = 0; core < my_num_cores ; core++) 
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
