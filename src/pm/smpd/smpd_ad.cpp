/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#include <iads.h>
#include "smpd_service.h"
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lm.h>
#define SECURITY_WIN32
#include <security.h>
#include <adshlp.h>
#include <atlbase.h>
#include <adserr.h>
#endif

static HRESULT GetGCSearch(IDirectorySearch **ppDS);
static int smpd_build_spn_list();

/* FIXME: Remove a static spn list - use spn list handles */
static smpd_host_spn_node_t *spn_list = NULL;

#undef FCNAME
#define FCNAME "smpd_register_spn"
int smpd_register_spn(const char *dc, const char *dn, const char *dh)
{
    DWORD len;
    char err_msg[256];
    LPSTR *spns;
    HANDLE ds;
    DWORD result;
    char domain_controller[SMPD_MAX_HOST_LENGTH] = "";
    char domain_name[SMPD_MAX_HOST_LENGTH] = "";
    char domain_host[SMPD_MAX_HOST_LENGTH] = "";
    char host[SMPD_MAX_HOST_LENGTH] = "";
    int really = 0;
    char *really_env;
    PDOMAIN_CONTROLLER_INFO pInfo;

    result = DsGetDcName(NULL/*local computer*/, NULL, NULL, NULL,
	/*DS_IS_FLAT_NAME | DS_RETURN_DNS_NAME | DS_DIRECTORY_SERVICE_REQUIRED, */
	DS_DIRECTORY_SERVICE_REQUIRED | DS_KDC_REQUIRED,
	&pInfo);
    if (result == ERROR_SUCCESS)
    {
	strcpy(domain_controller, pInfo->DomainControllerName);
	strcpy(domain_name, pInfo->DomainName);
	NetApiBufferFree(pInfo);
    }

    if (dc && *dc != '\0')
    {
	strcpy(domain_controller, dc);
    }
    if (dn && *dn != '\0')
    {
	strcpy(domain_name, dn);
    }
    if (dh && *dh != '\0')
    {
	strcpy(domain_host, dh);
    }
    if (domain_host[0] == '\0')
    {
	smpd_get_hostname(host, SMPD_MAX_HOST_LENGTH);
	if (domain_name[0] != '\0')
	{
	    sprintf(domain_host, "%s\\%s", domain_name, host);
	}
	else
	{
	    strcpy(domain_host, host);
	}
    }

    printf("DsBind(%s, %s, ...)\n", domain_controller[0] == '\0' ? NULL : domain_controller, domain_name[0] == '\0' ? NULL : domain_name);
    result = DsBind(
	domain_controller[0] == '\0' ? NULL : domain_controller,
	domain_name[0] == '\0' ? NULL : domain_name, &ds);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 256, NULL);
	smpd_err_printf("DsBind failed: %s\n", err_msg);
	return SMPD_FAIL;
    }

    really_env = getenv("really");
    if (really_env)
	really = 1;

#if 1
    len = 1;
    /*result = DsGetSpn(DS_SPN_SERVICE, SMPD_SERVICE_NAME, SMPD_SERVICE_NAME, 0, 0, NULL, NULL, &len, &spns);*/
    result = DsGetSpn(DS_SPN_DNS_HOST, SMPD_SERVICE_NAME, NULL, SMPD_LISTENER_PORT, 0, NULL, NULL, &len, &spns);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 256, NULL);
	smpd_err_printf("DsGetSpn failed: %s\n", err_msg);
	return SMPD_FAIL;
    }
    if (really)
    {
	printf("registering: %s\n", spns[0]);
	len = SMPD_MAX_HOST_LENGTH;
	GetComputerObjectName(NameFullyQualifiedDN, domain_host, &len);
	printf("on account: %s\n", domain_host);
	result = DsWriteAccountSpn(ds, DS_SPN_ADD_SPN_OP, domain_host, 1, (LPCSTR*)spns);
	if (result != ERROR_SUCCESS)
	{
	    DsFreeSpnArray(1, spns);
	    smpd_translate_win_error(result, err_msg, 256, NULL);
	    smpd_err_printf("DsWriteAccountSpn failed: %s\n", err_msg);
	    return SMPD_FAIL;
	}
    }
    else
    {
	printf("would register '%s' on %s\n", spns[0], domain_host);
    }
    DsFreeSpnArray(1, spns);
#else
    if (really)
    {
	result = DsServerRegisterSpn(DS_SPN_ADD_SPN_OP, SMPD_SERVICE_NAME, domain_host);
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, 256, NULL);
	    smpd_err_printf("DsServerRegisterSpn failed: %s\n", err_msg);
	    return SMPD_FAIL;
	}
    }
    else
    {
	printf("would register '%s' on %s\n", SMPD_SERVICE_NAME, domain_host);
    }
#endif
    result = DsUnBind(&ds);
    return SMPD_SUCCESS;
}

#define SMPD_INIT_SPN(spn, spn_len, service_class, service_dns_name, fq_hostname)   \
    MPIU_Snprintf(spn, spn_len, "%s/%s/%s", service_class, service_dns_name, fq_hostname)

#undef FCNAME
#define FCNAME "smpd_spn_list_dbg_print"
int smpd_spn_list_dbg_print(smpd_spn_list_hnd_t hnd){
    smpd_host_spn_node_t *iter=NULL;
    smpd_enter_fn(FCNAME);

    if(!SMPD_SPN_LIST_HND_IS_INIT(hnd)){
        smpd_dbg_printf("Invalid handle to spn list\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    iter = *hnd;
    while(iter){
        smpd_dbg_printf("FQ Service name = %s\n", iter->fq_service_name);
        iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
#undef FCNAME
#define FCNAME "smpd_spn_list_init"
int smpd_spn_list_init(smpd_spn_list_hnd_t *spn_list_hnd_p)
{
    HRESULT hr;
    int result = SMPD_SUCCESS;
    IDirectoryObject *pSCP = NULL;
    ADS_ATTR_INFO *pPropEntries = NULL;
    IDirectorySearch *pSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    LPWSTR pszDN;                  /* distinguished name of SCP. */
    LPWSTR pszServiceDNSName;      /* service DNS name. */
    LPWSTR pszClass;               /* name of service class. */
    USHORT usPort;                 /* service port. */
    WCHAR pszSearchString[SMPD_MAX_NAME_LENGTH];
    char service_class[SMPD_MAX_NAME_LENGTH];
    smpd_host_spn_node_t *iter;
    smpd_host_spn_node_t *spn_list_head=NULL;

    smpd_enter_fn(FCNAME);
    if(spn_list_hnd_p == NULL){
        smpd_err_printf("Invalid pointer to SPN list handle\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    spn_list_head = NULL;

    /* Allocate memory for spn list handle contents. Currently the handle just has the head
     * ptr to the list 
     */
    *spn_list_hnd_p = (smpd_host_spn_node_t **) MPIU_Malloc(sizeof(smpd_host_spn_node_t *));
    if(*spn_list_hnd_p == NULL){
        smpd_err_printf("Unable to allocate memory for list handle\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    **spn_list_hnd_p = NULL;

    CoInitialize(NULL);

    /* Get an IDirectorySearch pointer for the Global Catalog.  */
    hr = GetGCSearch(&pSearch);
    if (FAILED(hr) || pSearch == NULL) {
	    smpd_err_printf("GetGC failed 0x%x\n", hr);
        result = SMPD_FAIL;
	    goto Cleanup;
    }

    /* Set up a deep search.
      Thousands of objects are not expected, therefore
      query for 1000 rows per page.*/
    ADS_SEARCHPREF_INFO SearchPref[2];
    DWORD dwPref = sizeof(SearchPref)/sizeof(ADS_SEARCHPREF_INFO);
    SearchPref[0].dwSearchPref =    ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref[0].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[0].vValue.Integer =  ADS_SCOPE_SUBTREE;

    SearchPref[1].dwSearchPref =    ADS_SEARCHPREF_PAGESIZE;
    SearchPref[1].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[1].vValue.Integer =  1000;

    hr = pSearch->SetSearchPreference(SearchPref, dwPref);
    if (FAILED(hr)){
	    smpd_err_printf("Failed to set search prefs: hr:0x%x\n", hr);
        result = SMPD_FAIL;
	    goto Cleanup;
    }

    /* Execute the search. From the GC get the distinguished name 
      of the SCP. Use the DN to bind to the SCP and get the other 
      properties. */
    LPWSTR rgszDN[] = {L"distinguishedName"};

    /* Search for a match of the product GUID. */
    swprintf(pszSearchString, L"(keywords=%s)", SMPD_SERVICE_VENDOR_GUIDW);
    hr = pSearch->ExecuteSearch(pszSearchString, rgszDN, 1, &hSearch);
    /*hr = pSearch->ExecuteSearch(L"keywords=5722fe5f-cf46-4594-af7c-0997ca2e9d72", rgszDN, 1, &hSearch);*/
    if (FAILED(hr)){
	    smpd_err_printf("ExecuteSearch failed: hr:0x%x\n", hr);
        result = SMPD_FAIL;
	    goto Cleanup;
    }

    /* Loop through the results. Each row should be an instance of the 
      service identified by the product GUID.
      Add logic to select from multiple service instances. */
    while (SUCCEEDED(hr = pSearch->GetNextRow(hSearch))){
        DWORD dwError = ERROR_SUCCESS;
        WCHAR szError[512];
        WCHAR szProvider[512];

        if (hr == S_ADS_NOMORE_ROWS){
            ADsGetLastError(&dwError, szError, 512, szProvider, 512);
            if (ERROR_MORE_DATA == dwError){
                continue;
            }
            smpd_dbg_printf("No more result rows from GC\n");
            result = SMPD_SUCCESS;
            goto Cleanup;
	    }

        ADS_SEARCH_COLUMN Col;

        hr = pSearch->GetColumn(hSearch, L"distinguishedName", &Col);
        if(FAILED(hr)){
            smpd_err_printf("Failed to get Distinguished Name for SPN\n");
            result = SMPD_FAIL;
            goto Cleanup;
        }
        pszDN = AllocADsStr(Col.pADsValues->CaseIgnoreString);
        if(pszDN == NULL){
            ADsGetLastError(&dwError, szError, 512, szProvider, 512);
            smpd_err_printf("Failed to allocate memory for ADs string, 0x%x\n", dwError);
            result = SMPD_FAIL;
            goto Cleanup;
        }
        pSearch->FreeColumn(&Col);

        /* Bind to the DN to get the other properties. */
        LPWSTR lpszLDAPPrefix = L"LDAP://";
        DWORD dwSCPPathLength = (DWORD)(wcslen(lpszLDAPPrefix) + wcslen(pszDN) + 1);
        LPWSTR pwszSCPPath = (LPWSTR)malloc(sizeof(WCHAR) * dwSCPPathLength);
        if (pwszSCPPath){
            wcscpy(pwszSCPPath, lpszLDAPPrefix);
            wcscat(pwszSCPPath, pszDN);
        }
        else{
	        smpd_err_printf("Failed to allocate a buffer\n");
            result = SMPD_FAIL;
	        goto Cleanup;
	    }               
        /*wprintf(L"pszDN = %s\n", pszDN);*/

        hr = ADsGetObject(pwszSCPPath, IID_IDirectoryObject, (void**)&pSCP);
        free(pwszSCPPath);

        if (SUCCEEDED(hr)) {
            /* Properties to retrieve from the SCP object. */
            LPWSTR rgszAttribs[]=
                {
                    {L"serviceClassName"},
                    {L"serviceDNSName"},
                    /*{L"serviceDNSNameType"},*/
                    {L"serviceBindingInformation"}
                };

            DWORD dwAttrs = sizeof(rgszAttribs)/sizeof(LPWSTR);
            DWORD dwNumAttrGot;
            hr = pSCP->GetObjectAttributes(rgszAttribs, dwAttrs, &pPropEntries, &dwNumAttrGot);
            if (FAILED(hr)){
                smpd_err_printf("GetObjectAttributes Failed. hr:0x%x\n", hr);
                result = SMPD_FAIL;
                goto Cleanup;
            }

            pszServiceDNSName = NULL;
            pszClass = NULL;
            iter = (smpd_host_spn_node_t*)malloc(sizeof(smpd_host_spn_node_t));
            if (iter == NULL){
                smpd_err_printf("Unable to allocate memory to store an SPN entry.\n");
                result = SMPD_FAIL;
                goto Cleanup;
            }
            iter->next = NULL;
            iter->host[0] = '\0';
            iter->dnshost[0] = '\0';
            iter->spn[0] = '\0';

            /* Loop through the entries returned by GetObjectAttributes 
             * and save the values in the appropriate buffers.
             */
            for (int i = 0; i < (LONG)dwAttrs; i++){
                if ((wcscmp(L"serviceDNSName", pPropEntries[i].pszAttrName) == 0) &&
                        (pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)){
                    pszServiceDNSName = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
                    /*wprintf(L"pszServiceDNSName = %s\n", pszServiceDNSName);*/
                }
                if ((wcscmp(L"serviceClassName", pPropEntries[i].pszAttrName) == 0) &&
                        (pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)){
                    pszClass = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
                    /*wprintf(L"pszClass = %s\n", pszClass);*/
                }
                if ((wcscmp(L"serviceBindingInformation", pPropEntries[i].pszAttrName) == 0) &&
                    (pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)){
                    usPort=(USHORT)_wtoi(pPropEntries[i].pADsValues->CaseIgnoreString);
                    /*wprintf(L"usPort = %d\n", usPort);*/
                }
            } /* for(;;) */
            if(pszServiceDNSName != NULL){
                wcstombs(iter->dnshost, pszServiceDNSName, SMPD_MAX_NAME_LENGTH);
            }
            wcstombs(service_class, pszClass, SMPD_MAX_NAME_LENGTH);
            /*MPIU_Snprintf(iter->spn, SMPD_MAX_NAME_LENGTH, "%s/%s:%d", temp_str, iter->dnshost, usPort);*/
            wcstombs(iter->fq_service_name, pszDN, SMPD_MAX_FQ_NAME_LENGTH);
            /* MPIU_Snprintf(iter->spn, SMPD_MAX_NAME_LENGTH, "%s/%s/%s", temp_str, iter->dnshost, temp_str2); */
            SMPD_INIT_SPN(iter->spn, SMPD_MAX_FQ_NAME_LENGTH, service_class, iter->dnshost, iter->fq_service_name);

            iter->next = spn_list_head;
	        spn_list_head = iter;
            if (pszServiceDNSName != NULL){
                FreeADsStr(pszServiceDNSName);
            }
            if (pszClass != NULL){
                FreeADsStr(pszClass);
            }
        }
        FreeADsStr(pszDN);
    } /* GetNextRow() */

Cleanup:

    **spn_list_hnd_p = spn_list_head;
    smpd_spn_list_dbg_print(*spn_list_hnd_p);

    if (pSCP){
        pSCP->Release();
        pSCP = NULL;
    }

    if (pPropEntries){
        FreeADsMem(pPropEntries);
        pPropEntries = NULL;
    }

    if (pSearch){
        if (hSearch){
            pSearch->CloseSearchHandle(hSearch);
            hSearch = NULL;
        }

        pSearch->Release();
        pSearch = NULL;
    }
    
    CoUninitialize();

    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_spn_list_finalize"
int smpd_spn_list_finalize(smpd_spn_list_hnd_t *spn_list_hnd_p)
{
    smpd_host_spn_node_t *spn_list_head, *cur_node;
    smpd_enter_fn(FCNAME);
    if(spn_list_hnd_p == NULL){
        smpd_err_printf("Invalid pointer to spn list handle\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if(!SMPD_SPN_LIST_HND_IS_INIT(*spn_list_hnd_p)){
        smpd_dbg_printf("Trying to finalize an uninitialized handle\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    spn_list_head = **spn_list_hnd_p;
    while(spn_list_head != NULL){
        cur_node = spn_list_head;
        spn_list_head = cur_node->next;
        MPIU_Free(cur_node);
    }
    /* Free contents of the spn handle */
    MPIU_Free(*spn_list_hnd_p);

    *spn_list_hnd_p = NULL;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

static int smpd_build_spn_list()
{
    HRESULT hr;
    IDirectoryObject *pSCP = NULL;
    ADS_ATTR_INFO *pPropEntries = NULL;
    IDirectorySearch *pSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    LPWSTR pszDN;                  /* distinguished name of SCP. */
    LPWSTR pszServiceDNSName;      /* service DNS name. */
    LPWSTR pszClass;               /* name of service class. */
    USHORT usPort;                 /* service port. */
    WCHAR pszSearchString[SMPD_MAX_NAME_LENGTH];
    char temp_str[SMPD_MAX_NAME_LENGTH];
    char temp_str2[SMPD_MAX_NAME_LENGTH];
    smpd_host_spn_node_t *iter;
    /* double t1, t2; */
    static int initialized = 0;

    if (initialized)
    {
	return SMPD_SUCCESS;
    }
    initialized = 1;

    /* t1 = PMPI_Wtime(); */

    CoInitialize(NULL);

    /* Get an IDirectorySearch pointer for the Global Catalog.  */
    hr = GetGCSearch(&pSearch);
    if (FAILED(hr) || pSearch == NULL) 
    {
	smpd_err_printf("GetGC failed 0x%x\n", hr);
	goto Cleanup;
    }

    /* Set up a deep search.
      Thousands of objects are not expected in this example, therefore
      query for 1000 rows per page.*/
    ADS_SEARCHPREF_INFO SearchPref[2];
    DWORD dwPref = sizeof(SearchPref)/sizeof(ADS_SEARCHPREF_INFO);
    SearchPref[0].dwSearchPref =    ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref[0].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[0].vValue.Integer =  ADS_SCOPE_SUBTREE;

    SearchPref[1].dwSearchPref =    ADS_SEARCHPREF_PAGESIZE;
    SearchPref[1].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[1].vValue.Integer =  1000;

    hr = pSearch->SetSearchPreference(SearchPref, dwPref);
    if (FAILED(hr))
    {
	smpd_err_printf("Failed to set search prefs: hr:0x%x\n", hr);
	goto Cleanup;
    }

    /* Execute the search. From the GC get the distinguished name 
      of the SCP. Use the DN to bind to the SCP and get the other 
      properties. */
    LPWSTR rgszDN[] = {L"distinguishedName"};

    /* Search for a match of the product GUID. */
    swprintf(pszSearchString, L"keywords=%s", SMPD_SERVICE_VENDOR_GUIDW);
    hr = pSearch->ExecuteSearch(pszSearchString, rgszDN, 1, &hSearch);
    /*hr = pSearch->ExecuteSearch(L"keywords=5722fe5f-cf46-4594-af7c-0997ca2e9d72", rgszDN, 1, &hSearch);*/
    if (FAILED(hr))
    {
	smpd_err_printf("ExecuteSearch failed: hr:0x%x\n", hr);
	goto Cleanup;
    }

    /* Loop through the results. Each row should be an instance of the 
      service identified by the product GUID.
      Add logic to select from multiple service instances. */
    while (SUCCEEDED(hr = pSearch->GetNextRow(hSearch)))
    {
	if (hr == S_ADS_NOMORE_ROWS)
	{
	    DWORD dwError = ERROR_SUCCESS;
	    WCHAR szError[512];
	    WCHAR szProvider[512];

	    ADsGetLastError(&dwError, szError, 512, szProvider, 512);
	    if (ERROR_MORE_DATA == dwError)
	    {
		continue;
	    }
	    goto Cleanup;
	}

	ADS_SEARCH_COLUMN Col;

	hr = pSearch->GetColumn(hSearch, L"distinguishedName", &Col);
	pszDN = AllocADsStr(Col.pADsValues->CaseIgnoreString);
	pSearch->FreeColumn(&Col);

	/* Bind to the DN to get the other properties. */
	LPWSTR lpszLDAPPrefix = L"LDAP://";
	DWORD dwSCPPathLength = (DWORD)(wcslen(lpszLDAPPrefix) + wcslen(pszDN) + 1);
	LPWSTR pwszSCPPath = (LPWSTR)malloc(sizeof(WCHAR) * dwSCPPathLength);
	if (pwszSCPPath)
	{
	    wcscpy(pwszSCPPath, lpszLDAPPrefix);
	    wcscat(pwszSCPPath, pszDN);
	}       
	else
	{
	    smpd_err_printf("Failed to allocate a buffer\n");
	    goto Cleanup;
	}               
	/*wprintf(L"pszDN = %s\n", pszDN);*/
	/*FreeADsStr(pszDN);*/

	hr = ADsGetObject(pwszSCPPath, IID_IDirectoryObject, (void**)&pSCP);
	free(pwszSCPPath);

	if (SUCCEEDED(hr)) 
	{
	    /* Properties to retrieve from the SCP object. */
	    LPWSTR rgszAttribs[]=
	    {
		{L"serviceClassName"},
		{L"serviceDNSName"},
		/*{L"serviceDNSNameType"},*/
		{L"serviceBindingInformation"}
	    };

	    DWORD dwAttrs = sizeof(rgszAttribs)/sizeof(LPWSTR);
	    DWORD dwNumAttrGot;
	    hr = pSCP->GetObjectAttributes(rgszAttribs, dwAttrs, &pPropEntries, &dwNumAttrGot);
	    if (FAILED(hr)) 
	    {
		smpd_err_printf("GetObjectAttributes Failed. hr:0x%x\n", hr);
		goto Cleanup;
	    }

	    pszServiceDNSName = NULL;
	    pszClass = NULL;
	    iter = (smpd_host_spn_node_t*)malloc(sizeof(smpd_host_spn_node_t));
	    if (iter == NULL)
	    {
		smpd_err_printf("Unable to allocate memory to store an SPN entry.\n");
		goto Cleanup;
	    }
	    iter->next = NULL;
	    iter->host[0] = '\0';
	    iter->spn[0] = '\0';
	    iter->dnshost[0] = '\0';

	    /* Loop through the entries returned by GetObjectAttributes 
	    and save the values in the appropriate buffers.  */
	    for (int i = 0; i < (LONG)dwAttrs; i++) 
	    {
		if ((wcscmp(L"serviceDNSName", pPropEntries[i].pszAttrName) == 0) &&
		    (pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
		{
		    pszServiceDNSName = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
		    /*wprintf(L"pszServiceDNSName = %s\n", pszServiceDNSName);*/
		}

		/*
		if ((wcscmp(L"serviceDNSNameType", pPropEntries[i].pszAttrName) == 0) &&
		(pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
		{
		pszServiceDNSNameType = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
		wprintf(L"pszServiceDNSNameType = %s\n", pszServiceDNSNameType);
		}
		*/

		if ((wcscmp(L"serviceClassName", pPropEntries[i].pszAttrName) == 0) &&
		    (pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
		{
		    pszClass = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
		    /*wprintf(L"pszClass = %s\n", pszClass);*/
		}

		if ((wcscmp(L"serviceBindingInformation", pPropEntries[i].pszAttrName) == 0) &&
		    (pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
		{
		    usPort=(USHORT)_wtoi(pPropEntries[i].pADsValues->CaseIgnoreString);
		    /*wprintf(L"usPort = %d\n", usPort);*/
		}
	    }

	    wcstombs(iter->dnshost, pszServiceDNSName, SMPD_MAX_NAME_LENGTH);
	    wcstombs(temp_str, pszClass, SMPD_MAX_NAME_LENGTH);
	    /*MPIU_Snprintf(iter->spn, SMPD_MAX_NAME_LENGTH, "%s/%s:%d", temp_str, iter->dnshost, usPort);*/
	    wcstombs(temp_str2, pszDN, SMPD_MAX_NAME_LENGTH);
	    MPIU_Snprintf(iter->spn, SMPD_MAX_NAME_LENGTH, "%s/%s/%s", temp_str, iter->dnshost, temp_str2);
	    MPIU_Strncpy(iter->host, iter->dnshost, SMPD_MAX_NAME_LENGTH);
	    strtok(iter->host, ".");
	    iter->next = spn_list;
	    spn_list = iter;
	    if (pszServiceDNSName != NULL)
	    {
		FreeADsStr(pszServiceDNSName);
	    }
	    if (pszClass != NULL)
	    {
		FreeADsStr(pszClass);
	    }
	}
	FreeADsStr(pszDN);
    }

Cleanup:
    /*
    iter = spn_list;
    while (iter != NULL)
    {
	printf("host   : %s\n", iter->host);
	printf("dnshost: %s\n", iter->dnshost);
	printf("spn    : %s\n", iter->spn);
	iter = iter->next;
    }
    fflush(stdout);
    */
    if (pSCP)
    {
	pSCP->Release();
	pSCP = NULL;
    }

    if (pPropEntries)
    {
	FreeADsMem(pPropEntries);
	pPropEntries = NULL;
    }

    if (pSearch)
    {
	if (hSearch)
	{
	    pSearch->CloseSearchHandle(hSearch);
	    hSearch = NULL;
	}

	pSearch->Release();
	pSearch = NULL;
    }
    CoUninitialize();

    /* t2 = PMPI_Wtime();
    smpd_dbg_printf("build_spn_list took %0.6f seconds\n", t2-t1);
    */

    return SMPD_SUCCESS;
}

static HRESULT GetGCSearch(IDirectorySearch **ppDS)
{
    HRESULT hr;
    IEnumVARIANT *pEnum = NULL;
    IADsContainer *pCont = NULL;
    IDispatch *pDisp = NULL;
    VARIANT var;
    ULONG lFetch;

    *ppDS = NULL;

    /* Bind to the GC: namespace container object. The true GC DN 
      is a single immediate child of the GC: namespace, which must 
      be obtained using enumeration. */
    hr = ADsOpenObject(L"GC:", NULL, NULL, ADS_SECURE_AUTHENTICATION, /* Use Secure Authentication. */
	IID_IADsContainer,
	(void**)&pCont);
    if (FAILED(hr)) 
    {
	smpd_err_printf("ADsOpenObject failed: 0x%x\n", hr);
	goto cleanup;
    } 

    /* Get an enumeration interface for the GC container.  */
    hr = ADsBuildEnumerator(pCont, &pEnum);
    if (FAILED(hr)) 
    {
	smpd_err_printf("ADsBuildEnumerator failed: 0x%x\n", hr);
	goto cleanup;
    } 

    /* Now enumerate. There is only one child of the GC: object. */
    hr = ADsEnumerateNext(pEnum, 1, &var, &lFetch);
    if (FAILED(hr)) 
    {
	smpd_err_printf("ADsEnumerateNext failed: 0x%x\n", hr);
	goto cleanup;
    } 

    if ((hr == S_OK) && (lFetch == 1))
    {
	pDisp = V_DISPATCH(&var);
	hr = pDisp->QueryInterface(IID_IDirectorySearch, (void**)ppDS); 
    }

cleanup:
    if (pEnum)
    {
	ADsFreeEnumerator(pEnum);
	pEnum = NULL;
    }

    if (pCont)
    {
	pCont->Release();
	pCont = NULL;
    }

    if (pDisp)
    {
	pDisp->Release();
	pDisp = NULL;
    }

    return hr;
}


#undef FCNAME
#define FCNAME "smpd_lookup_spn_list"
int smpd_lookup_spn_list(smpd_spn_list_hnd_t hnd, char *target, int length, const char *fq_name, int port)
{
    int result = SMPD_SUCCESS;
    char err_msg[256];
    ULONG len = length/*SMPD_MAX_NAME_LENGTH*/;
    char *env;
    smpd_host_spn_node_t *iter;

    smpd_enter_fn(FCNAME);
    if(!SMPD_SPN_LIST_HND_IS_INIT(hnd)){
        smpd_err_printf("Invalid handle to spn list \n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    smpd_spn_list_dbg_print(hnd); fflush(stdout);

    env = getenv("MPICH_SPN");
    if (env){
        MPIU_Strncpy(target, env, SMPD_MAX_NAME_LENGTH);
        smpd_exit_fn(FCNAME);
        return SMPD_SUCCESS;
    }

    /* smpd_build_spn_list(); */
    iter = (smpd_host_spn_node_t *) (*hnd);
    while (iter != NULL){
        if (stricmp(iter->fq_service_name, fq_name) == 0){
            MPIU_Strncpy(target, iter->spn, length);
            smpd_exit_fn(FCNAME);
            return SMPD_SUCCESS;
        }
        iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_lookup_spn"
int smpd_lookup_spn(char *target, int length, const char *host, int port)
{
    int result;
    char err_msg[256];
    ULONG len = length/*SMPD_MAX_NAME_LENGTH*/;
    char *env;
    smpd_host_spn_node_t *iter;

    env = getenv("MPICH_SPN");
    if (env)
    {
	MPIU_Strncpy(target, env, SMPD_MAX_NAME_LENGTH);
	return SMPD_SUCCESS;
    }

    smpd_build_spn_list();
    iter = spn_list;
    while (iter != NULL)
    {
	if (stricmp(iter->host, host) == 0)
	{
	    MPIU_Strncpy(target, iter->spn, SMPD_MAX_NAME_LENGTH);
	    return SMPD_SUCCESS;
	}
	if (stricmp(iter->dnshost, host) == 0)
	{
	    MPIU_Strncpy(target, iter->spn, SMPD_MAX_NAME_LENGTH);
	    return SMPD_SUCCESS;
	}
	iter = iter->next;
    }

    result = DsMakeSpn(SMPD_SERVICE_NAME, NULL, host, (USHORT)port, NULL, &len, target);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 255, NULL);
	smpd_err_printf("DsMakeSpn(%s, %s, %d) failed: %s\n", SMPD_SERVICE_NAME, host, port, err_msg);
	return SMPD_FAIL;
    }


    /*result = DsMakeSpn(SMPD_SERVICE_NAME, SMPD_SERVICE_NAME, NULL, 0, NULL, &len, target);*/
    /*
    char **spns;
    result = DsGetSpn(DS_SPN_DNS_HOST, SMPD_SERVICE_NAME, NULL, port, 1, &host, NULL, &len, &spns);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 255, NULL);
	smpd_err_printf("DsGetSpn failed: %s\n", err_msg);
	return SMPD_FAIL;
    }
    MPIU_Strncpy(target, spns[0], SMPD_MAX_NAME_LENGTH);
    DsFreeSpnArray(1, spns);
    */
    /*MPIU_Snprintf(target, SMPD_MAX_NAME_LENGTH, "%s/%s:%d", SMPD_SERVICE_NAME, host, port);*/
    return SMPD_SUCCESS;
}
