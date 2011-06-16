/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

typedef HRESULT
(*DLLGETCLASSOBJECT)(
    REFCLSID rclsid,
    REFIID rrid,
    LPVOID* ppv
    );

typedef HRESULT
(*DLLCANUNLOADNOW)(void);

static HMODULE MPID_Nem_nd_provider_hnd_g = NULL;
static INDProvider* MPID_Nem_nd_piprovider_g = NULL;
#define MPID_NEM_ND_SERVICE_FLG (XP1_GUARANTEED_DELIVERY | XP1_GUARANTEED_ORDER | \
    XP1_MESSAGE_ORIENTED | XP1_CONNECT_DATA)

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_get_provider_path
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static WCHAR* MPID_Nem_nd_get_provider_path(WSAPROTOCOL_INFOW* pprotocol)
{
    int path_len;
    int ret, err;
    WCHAR* ppath;
    WCHAR* ppath_ex;

    // Get the path length for the provider DLL.
    path_len = 0;
    ret = WSCGetProviderPath(&pprotocol->ProviderId, NULL, &path_len, &err);

    if(err != WSAEFAULT || path_len == 0)
        return NULL;

    ppath = (WCHAR* )HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * path_len);
    if(ppath == NULL)
        return NULL;

    ret = WSCGetProviderPath(&pprotocol->ProviderId, ppath, &path_len, &err);
    if(ret != 0){
        HeapFree(GetProcessHeap(), 0, ppath);
        return NULL;
    }

    path_len = ExpandEnvironmentStringsW(ppath, NULL, 0);
    if(path_len == 0 ){
        HeapFree(GetProcessHeap(), 0, ppath);
        return NULL;
    }

    ppath_ex = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * path_len);
    if(ppath_ex == NULL ){
        HeapFree(GetProcessHeap(), 0, ppath);
        return NULL;
    }

    ret = ExpandEnvironmentStringsW(ppath, ppath_ex, path_len);

    // We don't need the un-expanded path anymore.
    HeapFree(GetProcessHeap(), 0, ppath);

    if( ret != path_len){
        HeapFree(GetProcessHeap(), 0, ppath_ex);
        return NULL;
    }

    return ppath_ex;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_load_provider
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Nem_nd_load_provider(WSAPROTOCOL_INFOW* pprotocol)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    WCHAR* path = NULL;
    DLLGETCLASSOBJECT pfn_DllGetClassObject;
    IClassFactory* pcf = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_LOAD_PROVIDER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_LOAD_PROVIDER);

    MPIU_Assert(pprotocol != NULL);

    path = MPID_Nem_nd_get_provider_path(pprotocol);
    MPIU_ERR_CHKANDJUMP((path == NULL), mpi_errno, MPI_ERR_OTHER, "**nd_prov_init");

    MPID_Nem_nd_provider_hnd_g = LoadLibraryW(path);
    MPIU_ERR_CHKANDJUMP2((MPID_Nem_nd_provider_hnd_g == NULL),
        mpi_errno, MPI_ERR_OTHER, "**nd_prov_init", "**nd_prov_init %s %d",
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()), MPIU_OSW_Get_errno());

    pfn_DllGetClassObject =
        (DLLGETCLASSOBJECT )GetProcAddress(MPID_Nem_nd_provider_hnd_g, "DllGetClassObject");
    MPIU_ERR_CHKANDJUMP2((pfn_DllGetClassObject == NULL),
        mpi_errno, MPI_ERR_OTHER, "**nd_prov_init", "**nd_prov_init %s %d",
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()), MPIU_OSW_Get_errno());

    /*
    MPID_nem_nd_pfn_DllCanUnloadNow_g=
        (DLLCANUNLOADNOW)GetProcAddress(MPID_nem_nd_provider_hnd_g, "DllCanUnloadNow" );
    MPIU_ERR_CHKANDJUMP2((MPID_nem_nd_pfn_DllCanUnloadNow_g == NULL),
        mpi_errno, MPI_ERR_OTHER, "**nd_prov_init", "**nd_prov_init %s %d",
        MPIU_OSW_Strerror(MPIU_OSW_Geterrno()), MPIU_OSW_Geterrno());
    */

    hr = pfn_DllGetClassObject(pprotocol->ProviderId, IID_IClassFactory, (void**)&pcf);
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_prov_init", "**nd_prov_init %s %d",
        _com_error(hr).ErrorMessage(), hr);

    hr = pcf->CreateInstance(NULL, IID_INDProvider, (void**)&MPID_Nem_nd_piprovider_g);
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_prov_init", "**nd_prov_init %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    if(pcf){ pcf->Release(); }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_LOAD_PROVIDER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_provider_hnd_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_provider_hnd_init(void )
{
    int mpi_errno = MPI_SUCCESS;
    int i, rc, len, found=0;
    WSAPROTOCOL_INFOW* pprotocols = NULL;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_PROVIDER_HND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_PROVIDER_HND_INIT);

    /* Find the bytes reqd to store the available protocols */
    if(WSCEnumProtocols(NULL, NULL, (LPDWORD )&len, &rc) == SOCKET_ERROR){
        MPIU_ERR_CHKANDJUMP2((rc != WSAENOBUFS), mpi_errno, MPI_ERR_OTHER, "**nd_unavail", "**nd_unavail %s %d", MPIU_OSW_Strerror(rc), rc);
    }
    pprotocols = (WSAPROTOCOL_INFOW*)HeapAlloc(GetProcessHeap(), 0x0, len);
    MPIU_ERR_CHKANDJUMP((pprotocols == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem");

    if(WSCEnumProtocols(NULL, pprotocols, (LPDWORD )&len, &rc) == SOCKET_ERROR){
        MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**nd_unavail", "**nd_unavail %s %d", MPIU_OSW_Strerror(rc), rc);
    }

    found = 0;

    for(i=0; i<len/sizeof(WSAPROTOCOL_INFOW); i++){
        /* Check if the protocol provides all the service req */
        if((pprotocols[i].dwServiceFlags1 & MPID_NEM_ND_SERVICE_FLG) != MPID_NEM_ND_SERVICE_FLG){
            continue;
        }

        if((pprotocols[i].iAddressFamily != AF_INET) &&
            (pprotocols[i].iAddressFamily != AF_INET6)){
            continue;
        }

        if( pprotocols[i].iSocketType != -1 ){
            continue;
        }

        if( pprotocols[i].iProtocol != 0 ){
            continue;
        }

        if( pprotocols[i].iProtocolMaxOffset != 0 ){
            continue;
        }

        /* Found a provider that meets our req */
        mpi_errno = MPID_Nem_nd_load_provider(&pprotocols[i]);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        found = 1;
        break;
    }

    MPIU_ERR_CHKANDJUMP((found == 0), mpi_errno, MPI_ERR_OTHER, "**nd_unavail");

fn_exit:
    if(pprotocols) HeapFree(GetProcessHeap(), 0x0, pprotocols);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_PROVIDER_HND_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_provider_hnd_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_provider_hnd_finalize(void )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_PROVIDER_HND_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_PROVIDER_HND_FINALIZE);

    if(MPID_Nem_nd_piprovider_g != NULL){
        MPID_Nem_nd_piprovider_g->Release();
        MPID_Nem_nd_piprovider_g = NULL;
    }
    if(MPID_Nem_nd_provider_hnd_g != NULL){
        FreeLibrary(MPID_Nem_nd_provider_hnd_g);
        MPID_Nem_nd_provider_hnd_g = NULL;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_PROVIDER_HND_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* This func is here so that we don't have to declare
 * externs for the global provider handle
 */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_open_ad
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_open_ad(MPID_Nem_nd_dev_hnd_t hnd)
{
    int mpi_errno = MPI_SUCCESS;
    SIZE_T len=0;
    HRESULT hr;
    int i, found = 0;
    SOCKET_ADDRESS_LIST *paddr_list;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_OPEN_AD);
    MPIU_CHKLMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_OPEN_AD);

    MPIU_Assert(MPID_Nem_nd_piprovider_g != NULL);

    /* Find the list of addresses available from the provider */
    hr = MPID_Nem_nd_piprovider_g->QueryAddressList(NULL, &len);
    MPIU_ERR_CHKANDJUMP2((hr != ND_BUFFER_OVERFLOW),
        mpi_errno, MPI_ERR_OTHER, "**nd_ad_open", "**nd_ad_open %s %d",
        _com_error(hr).ErrorMessage(), hr);

    MPIU_CHKLMEM_MALLOC(paddr_list, SOCKET_ADDRESS_LIST *, len, mpi_errno, "Network address list");

    hr = MPID_Nem_nd_piprovider_g->QueryAddressList(paddr_list, &len);
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_ad_open", "**nd_ad_open %s %d",
        _com_error(hr).ErrorMessage(), hr);

    found = 0;
    for(i=0; i<paddr_list->iAddressCount; i++){
        if(paddr_list->Address[i].lpSockaddr->sa_family == AF_INET){
            found = 1;
            /* FIXME: Replace with MPIU_memcpy () ? */
            memcpy((void *)&(hnd->s_addr_in), (void *)paddr_list->Address[i].lpSockaddr, sizeof(struct sockaddr_in));
        }
    }
    MPIU_ERR_CHKANDJUMP((!found), mpi_errno, MPI_ERR_OTHER, "**nd_ad_open");

    hr = MPID_Nem_nd_piprovider_g->OpenAdapter((struct sockaddr *)&(hnd->s_addr_in), sizeof(struct sockaddr_in), &(hnd->p_ad));
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_ad_open", "**nd_ad_open %s %d",
        _com_error(hr).ErrorMessage(), hr);

    /* Get the adapter info */
    len = sizeof(hnd->ad_info);
    hr = hnd->p_ad->Query(1, &(hnd->ad_info), &len);
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_ad_open", "**nd_ad_open %s %d",
        _com_error(hr).ErrorMessage(), hr);

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_OPEN_AD);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_decode_pg_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_decode_pg_info(char *pg_id, int pg_rank, struct MPIDI_VC **pvc, MPIDI_PG_t **ppg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_DECODE_PG_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_DECODE_PG_INFO);

    mpi_errno = MPIDI_PG_Find (pg_id, ppg);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    MPIU_ERR_CHKANDJUMP1 (*ppg == NULL, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "invalid PG");
    MPIU_ERR_CHKANDJUMP1 (pg_rank < 0 || pg_rank > MPIDI_PG_Get_size (*ppg), mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "invalid pg_rank");
        
    MPIDI_PG_Get_vc_set_active (*ppg, pg_rank, pvc);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_DECODE_PG_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_resolve_head_to_head
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_resolve_head_to_head(int remote_rank, MPIDI_PG_t *remote_pg, char *remote_pg_id, int *plocal_won_flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_RESOLVE_HEAD_TO_HEAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_RESOLVE_HEAD_TO_HEAD);

    MPIU_Assert(plocal_won_flag != NULL);
    if(MPIDI_Process.my_pg == remote_pg){
        /* Same process group - compare ranks to determine the winning rank */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Same process group, comparing ranks");
        *plocal_won_flag = (MPIDI_Process.my_pg_rank < remote_rank) ? 1 : 0;
    }
    else{
        /* Different process groups - compare pg ids to determine the winning rank */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Diff process group, comparing ids");
        *plocal_won_flag = (strcmp((char *)MPIDI_Process.my_pg->id, remote_pg_id) < 0) ? 1 : 0;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_RESOLVE_HEAD_TO_HEAD);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
