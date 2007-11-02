/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ibu.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "mpidi_ch3_impl.h"

#ifdef USE_IB_IBAL

#include "ibuimpl.ibal.h"

/* structures to manage cached pinned memory buffers */
typedef struct ibu_mem_node_t
{
    int ref_count;
    void *buf;
    int len;
    ibu_mem_t mem;
    struct ibu_mem_node_t *left, *right;
} ibu_mem_node_t;

ibu_mem_node_t *ibu_pin_cache = NULL;
static int g_total_mem_pinned = 0;

int g_onoff = 1;
#define EVENODD(a) (a = !a)

#undef FUNCNAME
#define FUNCNAME ib_malloc_register
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *ib_malloc_register(size_t size, ib_mr_handle_t *mhp, uint32_t *lp, uint32_t *rp)
{
    ib_api_status_t status;
    void *ptr;
    ib_mr_create_t mem;
    MPIDI_STATE_DECL(MPID_STATE_IB_MALLOC_REGISTER);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MALLOC_REGISTER);

    MPIU_DBG_PRINTF(("entering ib_malloc_register\n"));

    MPIU_DBG_PRINTF(("ib_malloc_register(%d) called\n", size));

    ptr = MPIU_Malloc(size);
    if (ptr == NULL)
    {
	MPIU_Internal_error_printf("ib_malloc_register: MPIU_Malloc(%d) failed.\n", size);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
	return NULL;
    }
    memset(&mem, 0, sizeof(ib_mr_create_t)); /* Added by Mellanox, dafna April 11th */
    mem.vaddr = ptr;
    mem.length = (uint64_t)size;

    mem.access_ctrl = IB_AC_LOCAL_WRITE | IB_AC_RDMA_WRITE | IB_AC_RDMA_READ;
    status = ib_reg_mem(IBU_Process.pd_handle, &mem, lp, rp, mhp);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ib_malloc_register: ib_reg_mem failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
	return NULL;
    }

    MPIU_DBG_PRINTF(("exiting ib_malloc_register\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
    return ptr;
}

#undef FUNCNAME
#define FUNCNAME ib_free_deregister
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void ib_free_deregister(void *p/*, ib_mr_handle_t mem_handle*/)
{
    /*ib_api_status_t status;*/
    MPIDI_STATE_DECL(MPID_STATE_IB_FREE_DEREGISTER);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_FREE_DEREGISTER);
    MPIU_DBG_PRINTF(("entering ib_free_derigister\n"));
    /*
    status = ib_dereg_mr(mem_handle);
    if (status != IBU_SUCCESS)
    {
    MPIU_Internal_error_printf("ibu_deregister_memory: ib_dereg_mr failed, error %s\n", ib_get_err_str(status));
    MPIDI_FUNC_EXIT(MPID_STATE_IB_FREE_DEREGISTER);
    return IBU_FAIL;
    }
    */
    MPIU_Free(p);
    MPIU_DBG_PRINTF(("exiting ib_free_derigster\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IB_FREE_DEREGISTER);
}

/*
* Test code to track registering/deregistering memory
*
*/
#ifdef TRACK_MEMORY_REGISTRATION
typedef struct regmem_t
{
    void *ptr;
    struct regmem_t *next;
} regmem_t;
regmem_t *g_list = NULL;
int find_mem(void *p)
{
    regmem_t *iter = g_list;
    while (iter)
    {
	if (iter->ptr == p)
	    return 1;
	iter = iter->next;
    }
    return 0;
}
void add_mem(void *p)
{
    regmem_t *r = (regmem_t*)MPIU_Malloc(sizeof(regmem_t));
    if (find_mem(p))
    {
	MPIU_Msg_printf("registering mem pointer twice: %p\n", p);
	fflush(stdout);
    }
    else
    {
	MPIU_Msg_printf("registered: %p\n", p);fflush(stdout);
    }
    r->ptr = p;
    r->next = g_list;
    g_list = r;
}
void remove_mem(void *p)
{
    regmem_t *iter, *trailer;
    iter = trailer = g_list;
    while (iter != NULL)
    {
	if (iter->ptr == p)
	{
	    if (iter == g_list)
	    {
		g_list = g_list->next;
	    }
	    else
	    {
		trailer->next = iter->next;
	    }
	    MPIU_Free(iter);
	    MPIU_Msg_printf("deregistered: %p\n", p);fflush(stdout);
	    return;
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }
    MPIU_Msg_printf("removing pointer that was not registered: %p\n", p);fflush(stdout);
}
/*
* End test code
*/
#endif

static int ibui_free_pin_tree(ibu_mem_node_t *node)
{
    ib_api_status_t status;
    if (node == NULL)
	return IBU_SUCCESS;
    ibui_free_pin_tree(node->left);
    ibui_free_pin_tree(node->right);

    if (node->ref_count == 0)
    {
	status = ib_dereg_mr(node->mem.handle);
	if (status != IBU_SUCCESS)
	{
	    MPIU_Internal_error_printf("ibu_deregister_memory: ib_dereg_mr failed, error %s\n", ib_get_err_str(status));
	    return IBU_FAIL;
	}
	g_total_mem_pinned -= node->len;
    }
    return IBU_SUCCESS;
}

static int ibui_add_nodes_to_cache(ibu_mem_node_t *node)
{
    ibu_mem_node_t *left, *right, *iter;
    if (node == NULL)
	return IBU_SUCCESS;
    left = node->left;
    right = node->right;

    /* insert node in the global tree */
    if (node->ref_count == 0)
    {
	MPIU_Free(node);
    }
    else
    {
	node->left = NULL;
	node->right = NULL;
	iter = ibu_pin_cache;
	while (iter != NULL)
	{
	    if (node->buf < iter->buf)
	    {
		if (iter->left == NULL)
		{
		    iter->left = node;
		    break;
		}
		else
		{
		    iter = iter->left;
		}
	    }
	    else
	    {
		if (iter->right == NULL)
		{
		    iter->right = node;
		    break;
		}
		else
		{
		    iter = iter->right;
		}
	    }
	}
	if (iter == NULL)
	{
	    ibu_pin_cache = node;
	}
    }

    /* recursively add the left and right nodes */
    ibui_add_nodes_to_cache(left);
    ibui_add_nodes_to_cache(right);

    return IBU_SUCCESS;
}

int ibu_clean_pin_cache()
{
    ibu_mem_node_t *iter;
    ibui_free_pin_tree(ibu_pin_cache);
    iter = ibu_pin_cache;
    ibu_pin_cache = NULL;
    ibui_add_nodes_to_cache(iter);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_register_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_register_memory(void *buf, int len, ibu_mem_t *state)
{
    ib_api_status_t status;
    ib_mr_create_t mem;

#ifndef USE_NO_PIN_CACHE    /* Added by Mellanox, dafna April 11th */
    ibu_mem_node_t *iter;
#endif    
    MPIDI_STATE_DECL(MPID_STATE_IBU_REGISTER_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_REGISTER_MEMORY);

    MPIU_DBG_PRINTF(("entering ibu_register_memory\n"));

#ifdef USE_NO_PIN_CACHE /* Added by Mellanox, dafna April 11th */

    /* caching turned off */
    memset(&mem, 0, sizeof(ib_mr_create_t));
    mem.vaddr = buf;
    mem.length = (uint64_t)len;
    mem.access_ctrl = IB_AC_LOCAL_WRITE | IB_AC_RDMA_WRITE | IB_AC_RDMA_READ;
    status = ib_reg_mem(
	IBU_Process.pd_handle,
	&mem,
	&state->lkey,
	&state->rkey,
	&state->handle);

    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_register_memory: ib_reg_mem failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_REGISTER_MEMORY);
	return IBU_FAIL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_REGISTER_MEMORY);
    return IBU_SUCCESS;

#else


#ifdef TRACK_MEMORY_REGISTRATION
    add_mem(buf);
#endif
start_over:
    iter = ibu_pin_cache;
    while (iter != NULL)
    {
	if (buf == iter->buf && len == iter->len)
	{
	    iter->ref_count++;
	    *state = iter->mem;
	    return IBU_SUCCESS;
	}
	if ((buf >= iter->buf) && ((char*)buf + len < ((char*)iter->buf + iter->len)))
	{
	    iter->ref_count++;
	    *state = iter->mem;
	    return IBU_SUCCESS;
	}
	if (buf < iter->buf)
	{
	    if (iter->left == NULL)
	    {
		if (g_total_mem_pinned > IBU_MAX_PINNED && EVENODD(g_onoff))
		{
		    ibu_clean_pin_cache();
		    goto start_over;
		}

		iter->left = (ibu_mem_node_t*)MPIU_Malloc(sizeof(ibu_mem_node_t));
		iter = iter->left;
		iter->left = NULL;
		iter->right = NULL;
		iter->buf = buf;
		iter->len = len;
		iter->ref_count = 1;
		break;
	    }
	    iter = iter->left;
	}
	else
	{
	    if (iter->right == NULL)
	    {
		if (g_total_mem_pinned > IBU_MAX_PINNED && EVENODD(g_onoff))
		{
		    ibu_clean_pin_cache();
		    goto start_over;
		}

		iter->right = (ibu_mem_node_t*)MPIU_Malloc(sizeof(ibu_mem_node_t));
		iter = iter->right;
		iter->left = NULL;
		iter->right = NULL;
		iter->buf = buf;
		iter->len = len;
		iter->ref_count = 1;
		break;
	    }
	    iter = iter->right;
	}
    }
    if (iter == NULL)
    {
	iter = (ibu_mem_node_t*)MPIU_Malloc(sizeof(ibu_mem_node_t));
	iter->buf = buf;
	iter->len = len;
	iter->left = NULL;
	iter->right = NULL;
	iter->ref_count = 1;
	ibu_pin_cache = iter;
    }

    memset(&mem, 0, sizeof(ib_mr_create_t)); /* Added by Mellanox, dafna April 11th */
    mem.vaddr = buf;
    mem.length = (uint64_t)len;
    mem.access_ctrl = IB_AC_LOCAL_WRITE | IB_AC_RDMA_WRITE | IB_AC_RDMA_READ;
    status = ib_reg_mem(
	IBU_Process.pd_handle,
	&mem,
	&state->lkey,
	&state->rkey,
	&state->handle);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_register_memory: ib_reg_mem failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_REGISTER_MEMORY);
	return IBU_FAIL;
    }

    iter->mem.handle = state->handle;
    iter->mem.lkey = state->lkey;
    iter->mem.rkey = state->rkey;
    g_total_mem_pinned += len;

    MPIU_DBG_PRINTF(("exiting ibu_register_memory\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_REGISTER_MEMORY);
    return IBU_SUCCESS;
#endif /* USE_NO_PIN_CACHE */    
}

#undef FUNCNAME
#define FUNCNAME ibu_deregister_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_deregister_memory(void *buf, int len, ibu_mem_t *state)
{
#ifdef USE_NO_PIN_CACHE	
    ib_api_status_t status;
#else
    ibu_mem_node_t *iter;    
#endif    
    MPIDI_STATE_DECL(MPID_STATE_IBU_DEREGISTER_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_DEREGISTER_MEMORY);

#ifdef USE_NO_PIN_CACHE /* Added by Mellanox, dafna April 11th */ 

    /* caching turned off */
    status =  ib_dereg_mr(state->handle);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_deregister_memory: ib_dereg_mr failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_DEREGISTER_MEMORY);
	return IBU_FAIL;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DEREGISTER_MEMORY);
    return IBU_SUCCESS;

#else

#ifdef TRACK_MEMORY_REGISTRATION
    remove_mem(buf);
#endif
    iter = ibu_pin_cache;
    while (iter != NULL)
    {
	if (buf == iter->buf && len == iter->len)
	{
	    iter->ref_count--;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DEREGISTER_MEMORY);
	    return IBU_SUCCESS;
	}
	if ((buf >= iter->buf) && ((char*)buf + len < ((char*)iter->buf + iter->len)))
	{
	    iter->ref_count--;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DEREGISTER_MEMORY);
	    return IBU_SUCCESS;
	}
	if (buf < iter->buf)
	{
	    if (iter->left == NULL)
	    {
		/* error, node not found */
		/*MPIU_Internal_error_printf("deregister memory not in cache buf = %p len = %d", buf, len);*/
		break;
	    }
	    iter = iter->left;
	}
	else
	{
	    if (iter->right == NULL)
	    {
		/* error, node not found */
		/*MPIU_Internal_error_printf("deregister memory not in cache buf = %p len = %d", buf, len);*/
		break;
	    }
	    iter = iter->right;
	}	
    }
    /* error, node not found */
    /*MPIU_Internal_error_printf("deregister memory not in cache buf = %p len = %d", buf, len);*/
    /*
    status = ib_dereg_mr(state->handle);
    if (status != IBU_SUCCESS)
    {
    MPIU_Internal_error_printf("ibu_deregister_memory: ib_dereg_mr failed, error %s\n", ib_get_err_str(status));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DEREGISTER_MEMORY);
    return IBU_FAIL;
    }
    */
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DEREGISTER_MEMORY);
    return IBU_SUCCESS;

#endif    
}

#undef FUNCNAME
#define FUNCNAME ibu_invalidate_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_invalidate_memory(void *buf, int len)
{
#ifndef USE_NO_PIN_CACHE
    ibu_mem_node_t *iter;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_INVALIDATE_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_INVALIDATE_MEMORY);

#ifdef USE_NO_PIN_CACHE

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INVALIDATE_MEMORY);
    return IBU_SUCCESS;

#else

    iter = ibu_pin_cache;
    while (iter != NULL)
    {
	if (buf == iter->buf && len == iter->len)
	{
	    /* destroy this node */
	    iter->ref_count = 0;
	    ibu_clean_pin_cache();
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INVALIDATE_MEMORY);
	    return IBU_SUCCESS;
	}
	if ((buf >= iter->buf) && ((char*)buf + len < ((char*)iter->buf + iter->len)))
	{
	    /* destroy this node */
	    iter->ref_count = 0;
	    ibu_clean_pin_cache();
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INVALIDATE_MEMORY);
	    return IBU_SUCCESS;
	}
	if (buf < iter->buf)
	{
	    if (iter->left == NULL)
	    {
		/* node not found */
		break;
	    }
	    iter = iter->left;
	}
	else
	{
	    if (iter->right == NULL)
	    {
		/* node not found */
		break;
	    }
	    iter = iter->right;
	}	
    }

    /* memory not found */

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INVALIDATE_MEMORY);
    return IBU_SUCCESS;

#endif
}

#undef FUNCNAME
#define FUNCNAME ibu_nocache_register_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_nocache_register_memory(void *buf, int len, ibu_mem_t *state)
{
    ib_api_status_t status;
    ib_mr_create_t mem;
    MPIDI_STATE_DECL(MPID_STATE_IBU_NOCACHE_REGISTER_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_NOCACHE_REGISTER_MEMORY);

    MPIU_DBG_PRINTF(("entering ibu_nocache_register_memory\n"));

    memset(&mem, 0, sizeof(ib_mr_create_t));
    mem.vaddr = buf;
    mem.length = (uint64_t)len;
    mem.access_ctrl = IB_AC_LOCAL_WRITE | IB_AC_RDMA_WRITE | IB_AC_RDMA_READ;
    status = ib_reg_mem(
	IBU_Process.pd_handle,
	&mem,
	&state->lkey,
	&state->rkey,
	&state->handle);

    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_nocache_register_memory: ib_reg_mem failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_NOCACHE_REGISTER_MEMORY);
	return IBU_FAIL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_NOCACHE_REGISTER_MEMORY);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_nocache_deregister_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_nocache_deregister_memory(void *buf, int len, ibu_mem_t *state)
{
    ib_api_status_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_NOCACHE_DEREGISTER_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_NOCACHE_DEREGISTER_MEMORY);

    status =  ib_dereg_mr(state->handle);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_nocache_deregister_memory: ib_dereg_mr failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_NOCACHE_DEREGISTER_MEMORY);
	return IBU_FAIL;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_NOCACHE_DEREGISTER_MEMORY);
    return IBU_SUCCESS;
}

#endif
