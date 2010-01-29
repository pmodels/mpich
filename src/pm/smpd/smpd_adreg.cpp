/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2001 by Argonne National Laboratory.
 * See COPYRIGHT in top-level directory.
 */
#include <winsock2.h>
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

static HRESULT AllowAccessToScpProperties(LPWSTR wszAccountSAM, IADs *pSCPObject);

typedef struct tagADSERRMSG
{
    HRESULT hr;
    LPCTSTR pszError;
} ADSERRMSG;

#define ADDADSERROR(x) x, _T(#x)

static ADSERRMSG adsErr[] =
{
    ADDADSERROR(E_ADS_BAD_PATHNAME),
	ADDADSERROR(E_ADS_INVALID_DOMAIN_OBJECT),
	ADDADSERROR(E_ADS_INVALID_USER_OBJECT),
	ADDADSERROR(E_ADS_INVALID_COMPUTER_OBJECT),
	ADDADSERROR(E_ADS_UNKNOWN_OBJECT),
	ADDADSERROR(E_ADS_PROPERTY_NOT_SET),
	ADDADSERROR(E_ADS_PROPERTY_NOT_SUPPORTED),
	ADDADSERROR(E_ADS_PROPERTY_INVALID),
	ADDADSERROR(E_ADS_BAD_PARAMETER),
	ADDADSERROR(E_ADS_OBJECT_UNBOUND),
	ADDADSERROR(E_ADS_PROPERTY_NOT_MODIFIED),
	ADDADSERROR(E_ADS_PROPERTY_MODIFIED),
	ADDADSERROR(E_ADS_CANT_CONVERT_DATATYPE),
	ADDADSERROR(E_ADS_PROPERTY_NOT_FOUND),
	ADDADSERROR(E_ADS_OBJECT_EXISTS),
	ADDADSERROR(E_ADS_SCHEMA_VIOLATION),
	ADDADSERROR(E_ADS_COLUMN_NOT_SET),
	ADDADSERROR(E_ADS_INVALID_FILTER),
	ADDADSERROR(0),
};


static LPCTSTR GetADSIError( HRESULT hr )
{
    if ( hr & 0x00005000 )
    {
	int idx=0;
	while (adsErr[idx].hr != 0 )
	{
	    if ( adsErr[idx].hr == hr )
	    {
		return adsErr[idx].pszError;
	    }
	    idx++;
	}
    }

    return _T("");
}

static LPCTSTR GetErrorMessage( HRESULT hr )
{
    BOOL bRet;
    static TCHAR s[1024];
    LPTSTR lpBuffer=NULL;

    if ( SUCCEEDED(hr) )
    {
	return _T("Success");
    }
    if ( hr & 0x00005000) /* standard ADSI Errors */
    {
	_tcscpy(s, GetADSIError(hr));
    }
    else if ( HRESULT_FACILITY(hr)==FACILITY_WIN32 )
    {
	bRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM,
	    NULL, hr,
	    MAKELANGID(LANG_NEUTRAL,
	    SUBLANG_SYS_DEFAULT),
	    (LPTSTR) &lpBuffer, 0, NULL);
	if ( !bRet )
	{
	    _sntprintf(s, 1024, _T("Error %ld"), hr);
	}

	if ( lpBuffer )
	{
	    _tcscpy(s, lpBuffer);
	    LocalFree( lpBuffer );
	}
    }
    else /* Non Win32 Error */
    {
	_sntprintf(s, 1024, _T("%X"), hr );
	return s;
    }

    WCHAR szBuffer[MAX_PATH];
    WCHAR szName[MAX_PATH];
    DWORD dwError;


    hr = ADsGetLastError( &dwError, szBuffer, (sizeof(szBuffer)/sizeof(WCHAR))-1,
	szName, (sizeof(szName)/sizeof(WCHAR))-1 );


    if ( SUCCEEDED(hr) && dwError != ERROR_INVALID_DATA && wcslen(szBuffer))
    {
	USES_CONVERSION;
	_tcscat(s, _T(" -- Extended Error --- \n"));
	_tcscat(s, OLE2T(szName));
	_tcscat(s, _T(":\n"));
	_tcscat(s, OLE2T( szBuffer ));
    }

    return s;
}

static void ReportError(TCHAR *msg, HRESULT hr)
{
    _tprintf(TEXT("HRESULT=%d\nError message: %s\nError Text: %s\n"), hr, msg, GetErrorMessage(hr));
}

static void ReportServiceError(char *msg, DWORD error)
{
    printf("error=%d\nError message: %s\nError Text: %s\n", error, msg, GetErrorMessage(error));
}

/* ScpCreate

Create a new service connection point as a child object of the
local server computer object.
*/
static DWORD ScpCreate(
               TCHAR *fqHostName,    /* Fully Qualified Name of the host */
               TCHAR *dnsHostName,   /* DNS compatible name of the host */
		       USHORT usPort,    /* Service's default port to store in SCP. */
		       LPTSTR szClass,   /* Service class string to store in SCP. */
		       LPTSTR szAccount, /* Logon account that must access SCP. */
		       UINT ccDN,        /* Length of the pszDN buffer in characters */
		       TCHAR *pszDN)     /* Returns distinguished name of SCP. */
{
    DWORD dwStat=S_OK, dwAttr, dwLen;
    HRESULT hr;
    IDispatch *pDisp; /* Returned dispinterface of new object. */
    IDirectoryObject *pComp; /* Computer object; parent of SCP. */
    IADs *pIADsSCP; /* IADs interface on new object. */
    TCHAR szServer[MAX_PATH];
    TCHAR szAdsPath[MAX_PATH];
    TCHAR szPort[6];
    HKEY hReg;
    DWORD dwDisp;
    TCHAR szRelativeDistinguishedName[MAX_PATH];
    TCHAR samHostName[MAX_PATH];
    ADSVALUE cn, objclass, keywords[4], binding, classname, dnsname, nametype;
    BSTR bstrGuid = NULL;
    TCHAR pwszBindByGuidStr[1024];
    VARIANT var;
    /* Values for SCPs keywords attribute. */
    TCHAR* KwVal[] =
    {
	TEXT(SMPD_SERVICE_VENDOR_GUID), /* Vendor GUID. */
	    TEXT(SMPD_SERVICE_GUID),    /* Product GUID. */
	    TEXT(SMPD_PRODUCT_VENDOR),  /* Vendor Name. */
	    TEXT(SMPD_PRODUCT),         /* Product Name. */
    };
    TCHAR *hostAttrib[] = { TEXT("dNSHostName") };
    ADS_ATTR_INFO *hostAttrInfo;

    /* SCP attributes to set during creation of SCP. */
    ADS_ATTR_INFO ScpAttribs[] =
    {
	{TEXT("cn"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &cn, 1},
	{TEXT("objectClass"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &objclass, 1},
	{TEXT("keywords"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, keywords, 4},
	{TEXT("serviceDnsName"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &dnsname, 1},
	{TEXT("serviceDnsNameType"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &nametype, 1},
	{TEXT("serviceClassName"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &classname, 1},
	{TEXT("serviceBindingInformation"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &binding, 1},
    };

    if (!szClass /*|| !szAccount*/ || !pszDN || !(ccDN > 0))
    {
	hr = ERROR_INVALID_PARAMETER;
	ReportError(TEXT("Invalid parameter."), hr);
	return hr;
    }

    _tcsncpy(szServer, TEXT("NOT DEFINED"), sizeof(szServer));

    /* Enter the attribute values to be stored in the SCP. */
    keywords[0].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[1].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[2].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[3].dwType = ADSTYPE_CASE_IGNORE_STRING;

    keywords[0].CaseIgnoreString = KwVal[0];
    keywords[1].CaseIgnoreString = KwVal[1];
    keywords[2].CaseIgnoreString = KwVal[2];
    keywords[3].CaseIgnoreString = KwVal[3];

    cn.dwType = ADSTYPE_CASE_IGNORE_STRING;
    cn.CaseIgnoreString = TEXT(SMPD_SERVICE_NAME);
    objclass.dwType = ADSTYPE_CASE_IGNORE_STRING;
    objclass.CaseIgnoreString = TEXT("serviceConnectionPoint");
    dnsname.dwType = ADSTYPE_CASE_IGNORE_STRING;
    if(dnsHostName == NULL){
        dnsname.CaseIgnoreString = szServer;
    }
    else{
        dnsname.CaseIgnoreString = dnsHostName;
    }
    classname.dwType = ADSTYPE_CASE_IGNORE_STRING;
    classname.CaseIgnoreString = szClass;
    _sntprintf(szPort, 6, TEXT("%d"), usPort);
    binding.dwType = ADSTYPE_CASE_IGNORE_STRING;
    binding.CaseIgnoreString = szPort;
    nametype.dwType = ADSTYPE_CASE_IGNORE_STRING;
    nametype.CaseIgnoreString = TEXT("A");

    /* Compose the ADSpath and bind to the computer object for the local computer. */
    _tcsncpy(szAdsPath,TEXT("LDAP://"),MAX_PATH);
    _tcsncat(szAdsPath, fqHostName, MAX_PATH - _tcslen(szAdsPath));
    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pComp);
    if (FAILED(hr))
    {
	_tprintf(TEXT("ADsGetObject('%s') failed\n"), szAdsPath);
	ReportError(TEXT("Failed to bind Computer Object."),hr);
	return hr;
    }
    /* _tprintf(TEXT("Bound to Active directory object:\n%s\n"), fqHostName); */

    if(dnsHostName == NULL){
        /* Get the DNS name of the host from AD */
        hostAttrInfo = NULL;
        hr = pComp->GetObjectAttributes(hostAttrib, 1, &hostAttrInfo, &dwAttr);
        if(FAILED(hr)){
            ReportError(TEXT("Failed to get the DNS name for host from AD:"), hr);
            return hr;
        }
        if(hostAttrInfo != NULL){
            dnsHostName = (TCHAR *) (hostAttrInfo[0].pADsValues->CaseIgnoreString);
            /* The scp attributes point to szServer */
            _tcsncpy(szServer, dnsHostName, sizeof(szServer));
            FreeADsMem(hostAttrInfo);
        }
    }

    /********************************************************************
    * Publish the SCP as a child of the computer object
    *********************************************************************/

    /* Calculate attribute count. */
    dwAttr = sizeof(ScpAttribs)/sizeof(ADS_ATTR_INFO);

    /* Complete the action. */
    _sntprintf(szRelativeDistinguishedName, MAX_PATH, TEXT("cn=%s"), TEXT(SMPD_SERVICE_NAME));
    /* Delete the previous object if it exists */
    hr = pComp->DeleteDSObject(szRelativeDistinguishedName);
    /* Create a new object */
    hr = pComp->CreateDSObject(szRelativeDistinguishedName, ScpAttribs, dwAttr, &pDisp);
    if (FAILED(hr))
    {
	ReportError(TEXT("Failed to create SCP:"), hr);
	pComp->Release();
	return hr;
    }
    pComp->Release();

    /* Query for an IADs pointer on the SCP object. */
    hr = pDisp->QueryInterface(IID_IADs,(void **)&pIADsSCP);
    if (FAILED(hr))
    {
	ReportError(TEXT("Failed to QueryInterface for IADs:"), hr);
	pDisp->Release();
	return hr;
    }
    pDisp->Release();

    /* Translate FQN to SAM compatible name */
    dwLen = MAX_PATH;
    if(!TranslateName(fqHostName, NameFullyQualifiedDN, NameSamCompatible, samHostName, &dwLen)){
        hr = E_FAIL;
        ReportError(TEXT("Failed to translate FQN to SAM compatible name:"), hr);
        return hr;
    }

    /* Set ACEs on the SCP so a service can modify it. */
    hr = AllowAccessToScpProperties(
	samHostName, /* Service account to allow access. */
	pIADsSCP); /* IADs pointer to the SCP object. */
    if (FAILED(hr))
    {
	ReportError(TEXT("Failed to set ACEs on SCP DACL:"), hr);
	return hr;
    }

    /* Get the distinguished name of the SCP. */
    VariantInit(&var);
    hr = pIADsSCP->Get(CComBSTR("distinguishedName"), &var);
    if (FAILED(hr))
    {
	ReportError(TEXT("Failed to get distinguishedName:"), hr);
	pIADsSCP->Release();
	return hr;
    }
    /*_tprintf(TEXT("distinguishedName via IADs: %s\n"), var.bstrVal);*/

    _tcsncpy(pszDN, var.bstrVal, ccDN);

    /* Retrieve the SCP objectGUID in format suitable for binding. */
    hr = pIADsSCP->get_GUID(&bstrGuid);
    if (FAILED(hr))
    {
	ReportError(TEXT("Failed to get GUID:"), hr);
	pIADsSCP->Release();
	return hr;
    }

    /* Build a string for binding to the object by GUID. */
    _tcsncpy(pwszBindByGuidStr, TEXT("LDAP://<GUID="), 1024);
    _tcsncat(pwszBindByGuidStr, bstrGuid, 1024 -_tcslen(pwszBindByGuidStr));
    _tcsncat(pwszBindByGuidStr, TEXT(">"), 1024 -_tcslen(pwszBindByGuidStr));
    /*_tprintf(TEXT("GUID binding string: %s\n"), pwszBindByGuidStr);*/

    pIADsSCP->Release();

    /* Create a registry key to save the GUID binding string */
    /*
    dwStat = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
	TEXT(SMPD_SERVICE_REGISTRY_KEY),
	0,
	NULL,
	REG_OPTION_NON_VOLATILE,
	KEY_ALL_ACCESS,
	NULL,
	&hReg,
	&dwDisp);
    if (dwStat != NO_ERROR)
    {
	ReportError(TEXT("RegCreateKeyEx failed:"), dwStat);
	return dwStat;
    }

    / Cache the GUID binding string under the registry key. /
    dwStat = RegSetValueEx(hReg, TEXT("GUIDBindingString"), 0, REG_SZ,
	(const BYTE *)pwszBindByGuidStr,
	(DWORD)(2*(_tcslen(pwszBindByGuidStr))));
    if (dwStat != NO_ERROR)
    {
	ReportError(TEXT("RegSetValueEx failed:"), dwStat);
	return dwStat;
    }

    RegCloseKey(hReg);
    */

    /* Cleanup should delete the SCP and registry key if an error occurs. */

    return S_OK;
}

/* ScpRemove
    Remove an existing service connection point that is a child object of the
    server computer object identified by fqHostName.
*/
#undef FCNAME
#define FCNAME "ScpRemove"
static DWORD ScpRemove(TCHAR *fqHostName)
{
    HRESULT hr=S_OK;
    IDirectoryObject *pComp; /* Computer object; parent of SCP. */
    TCHAR szAdsPath[MAX_PATH];
    TCHAR szRelativeDistinguishedName[MAX_PATH];

    smpd_enter_fn(FCNAME);
    if(fqHostName == NULL){
        smpd_err_printf("Invalid FQ hostname provided\n");
        smpd_exit_fn(FCNAME);
        return E_FAIL;
    }

    /* Compose the ADSpath and bind to the computer object for the computer. */
    _tcsncpy(szAdsPath,TEXT("LDAP://"),MAX_PATH);
    _tcsncat(szAdsPath, fqHostName, MAX_PATH - _tcslen(szAdsPath));

    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pComp);
    if (FAILED(hr)){
        _tprintf(TEXT("ADsGetObject('%s') failed\n"), szAdsPath);
        ReportError(TEXT("Failed to bind Computer Object."),hr);
        smpd_exit_fn(FCNAME);
        return hr;
    }
    /* _tprintf(TEXT("Bound to Active directory object:\n%s\n"), szAdsPath); */

    _sntprintf(szRelativeDistinguishedName, MAX_PATH, TEXT("cn=%s"), TEXT(SMPD_SERVICE_NAME));
    /* Delete the object/SCP */
    hr = pComp->DeleteDSObject(szRelativeDistinguishedName);
    if (FAILED(hr)){
        ReportError(TEXT("Failed to delete SCP:"), hr);
    }

    pComp->Release();
    smpd_exit_fn(FCNAME);
    return hr;
}

static DWORD SpnCompose(
			TCHAR ***pspn,          /* Output: an array of SPNs */
			unsigned long *pulSpn,  /* Output: the number of SPNs returned */
			TCHAR *pszDNofSCP,      /* Input: DN of the service's SCP */
			TCHAR* pszServiceClass) /* Input: the name of the service class */
{
    DWORD dwStatus;

    dwStatus = DsGetSpn(
	DS_SPN_SERVICE,  /* Type of SPN to create (enumerated type) */
	pszServiceClass, /* Service class - a name in this case */
	pszDNofSCP,      /* Service name - DN of the service SCP */
	0,               /* Default: omit port component of SPN */
	0,               /* Number of entries in hostnames and ports arrays */
	NULL,            /* Array of hostnames. Default is local computer */
	NULL,            /* Array of ports. Default omits port component */
	pulSpn,          /* Receives number of SPNs returned in array */
	pspn             /* Receives array of SPN(s) */
	);

    return dwStatus;
}

/***************************************************************************

SpnRegister()

Register or unregister the SPNs under the service's account.

If the service runs in LocalSystem account, pszServiceAcctDN is the
distinguished name of the local computer account.

Parameters:

pszServiceAcctDN - Contains the distinguished name of the logon
account for this instance of the service.

pspn - Contains an array of SPNs to register.

ulSpn - Contains the number of SPNs in the array.

Operation - Contains one of the DS_SPN_WRITE_OP values that determines
the type of operation to perform on the SPNs.

***************************************************************************/

#undef FCNAME
#define FCNAME "SpnRegister"
static DWORD SpnRegister(TCHAR *pszServiceAcctDN,
			 TCHAR **pspn,
			 unsigned long ulSpn,
			 DS_SPN_WRITE_OP Operation)
{
    DWORD dwStatus;
    HANDLE hDs;
    TCHAR szSamName[512];
    DWORD dwSize = sizeof(szSamName);
    PDOMAIN_CONTROLLER_INFO pDcInfo;

    smpd_enter_fn(FCNAME);

    /* Bind to a domain controller. */
    /* Get the domain for the current user. */
    if (GetUserNameEx(NameSamCompatible, szSamName, &dwSize))
    {
	TCHAR *pWhack = _tcschr(szSamName, '\\');
	if (pWhack)
	{
	    *pWhack = '\0';
	}
    }
    else
    {
    smpd_exit_fn(FCNAME);
	return GetLastError();
    }
    
    /* _tprintf(TEXT("szSamName: %s\n"), szSamName); */

    /* Get the name of a domain controller in that domain. */
    dwStatus = DsGetDcName(NULL,
	szSamName,
	NULL,
	NULL,
	DS_IS_FLAT_NAME |
	DS_RETURN_DNS_NAME |
	DS_DIRECTORY_SERVICE_REQUIRED,
	&pDcInfo);
    if (dwStatus != 0){
        smpd_err_printf("DsGetDcName() failed: 0x%x\n", dwStatus);
        smpd_exit_fn(FCNAME);
        return dwStatus;
    }

    /* _tprintf(TEXT("domain: %s\n"), pDcInfo->DomainControllerName); */
    /* Bind to the domain controller. */
    dwStatus = DsBind(pDcInfo->DomainControllerName, NULL, &hDs);
    /* Free the DOMAIN_CONTROLLER_INFO buffer. */
    NetApiBufferFree(pDcInfo);
    if (dwStatus != ERROR_SUCCESS){
        smpd_err_printf("DsBind() failed: 0x%x\n", dwStatus);
        smpd_exit_fn(FCNAME);
	    return dwStatus;
    }

    /*
    for (unsigned long i=0; i<ulSpn; i++)
    {
	_tprintf(TEXT("Service Principal Name =\n%s\n"), pspn[i]);
    }
    fflush(stdout);
    */
    /* Write the SPNs to the service account or computer account. */
    dwStatus = DsWriteAccountSpn(
	hDs,                   /* Handle to the directory. */
	Operation,             /* Add or remove SPN from account's existing SPNs. */
	pszServiceAcctDN,      /* DN of service account or computer account. */
	ulSpn,                 /* Number of SPNs to add. */
	(const TCHAR **)pspn); /* Array of SPNs. */

    if(dwStatus != ERROR_SUCCESS){
        smpd_err_printf("DsWriteAccountSpn() failed: 0x%x\n", dwStatus);
    }

    /* Unbind the DS in any case. */
    DsUnBind(&hDs);

    smpd_exit_fn(FCNAME);
    return dwStatus;
}

#define SMPD_MAX_SAM_NAME_SUFFIX_LENGTH 2 /* "$" & string delimiter */
#undef FCNAME
#define FCNAME "smpd_translate_netbios_to_fqn"
int smpd_translate_netbios_to_fqn(TCHAR *netbios_name, TCHAR *fqn_name, int fqn_name_len)
{
    LPWSTR tmp_name;
    int tmp_name_len = 0;
    char err_msg[SMPD_MAX_ERR_MSG_LENGTH];

    smpd_enter_fn(FCNAME);
    if((netbios_name == NULL) || (_tcslen(netbios_name) <= 0) ||
        (fqn_name == NULL) || (fqn_name_len <= 0)){
        smpd_err_printf("Invalid netbios_name or buffer for FQN name provided\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    tmp_name_len = _tcslen(netbios_name) + SMPD_MAX_SAM_NAME_SUFFIX_LENGTH;
    tmp_name = new TCHAR[tmp_name_len];
    if(tmp_name == NULL){
        smpd_err_printf("Unable to allocate memory for temp buffer\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    _sntprintf_s(tmp_name, tmp_name_len, tmp_name_len - 1, TEXT("%s$"), netbios_name);
    if(!TranslateName(tmp_name, NameSamCompatible, NameFullyQualifiedDN, fqn_name, (PULONG )&fqn_name_len)){
        smpd_translate_win_error(GetLastError(), err_msg, SMPD_MAX_ERR_MSG_LENGTH, "Error translating Netbios name to FQN name \n");
        delete tmp_name;
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

typedef enum {
    SMPD_SCP_ADD,
    SMPD_SCP_REMOVE
} smpd_scp_cntrl_flag_t;

int smpd_cntrl_scps_from_file_templ(TCHAR *filename, smpd_spn_list_hnd_t hnd, smpd_scp_cntrl_flag_t flag);

#undef FCNAME
#define FCNAME "smpd_setup_scps_with_file"
int smpd_setup_scps_with_file(char *filename)
{
    TCHAR filename_tcs[SMPD_MAX_FILENAME];
    SMPDU_MBSTOTCS(filename_tcs, filename, SMPD_MAX_FILENAME);
    return smpd_cntrl_scps_from_file_templ(filename_tcs, NULL, SMPD_SCP_ADD);
}
#undef FCNAME
#define FCNAME "smpd_remove_scps_with_file"
int smpd_remove_scps_with_file(char *filename, smpd_spn_list_hnd_t hnd)
{
    TCHAR filename_tcs[SMPD_MAX_FILENAME];
    SMPDU_MBSTOTCS(filename_tcs, filename, SMPD_MAX_FILENAME);
    return smpd_cntrl_scps_from_file_templ(filename_tcs, hnd, SMPD_SCP_REMOVE);
}

/* Read the file, filename, and setup SCP for each host in the file
 * The hostnames should be valid windows netbios names
 * The flag indicates whether SCP is added or removed.
 */
#undef FCNAME
#define FCNAME "smpd_cntrl_scps_from_file_templ"
int smpd_cntrl_scps_from_file_templ(TCHAR *filename, 
                                    smpd_spn_list_hnd_t hnd,
                                    smpd_scp_cntrl_flag_t flag)
{
    int ret_errno=0, result=0, ret_result = SMPD_SUCCESS;
    TCHAR line[SMPD_MAX_NETBIOS_NAME_LENGTH];
    char err_msg[SMPD_MAX_ERR_MSG_LENGTH];
    FILE *fp=NULL;
    DWORD dwLen=0;

    smpd_enter_fn(FCNAME);
    if((filename == NULL) || (_tcslen(filename) <= 0)){
        smpd_err_printf("Invalid SPN register file specified\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    /* Open file */
    ret_errno = _tfopen_s(&fp, filename, TEXT("r"));
    if(ret_errno != 0){
        _tprintf(TEXT("Error opening the SPN register file(%s), error = %d(%s)\n"), filename, ret_errno, strerror(ret_errno));
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    /* Read file */
    while(_fgetts(line, SMPD_MAX_NETBIOS_NAME_LENGTH-SMPD_MAX_SAM_NAME_SUFFIX_LENGTH, fp)){
        /* Get rid of the end of line */
        if(line[_tcslen(line) - 1] == L'\n'){
            line[_tcslen(line)-1] = L'\0';
        }
        
        switch(flag){
            case SMPD_SCP_ADD:
                result = smpd_setup_scp(line);
                if(result != SMPD_SUCCESS){
                    ret_result = SMPD_FAIL;
                    _tprintf(TEXT("Setting up scp for '%s' FAILED\n"), line);
                    /* Continue with next hosts */
                }
                else{
                    _tprintf(TEXT("Setting up scp for '%s' SUCCESSFUL\n"), line);
                    /* Continue with next host */
                }
                break;
            case SMPD_SCP_REMOVE:
                result = smpd_remove_scp(line, hnd);
                if(result != SMPD_SUCCESS){
                    ret_result = SMPD_FAIL;
                    _tprintf(TEXT("Removing scp for '%s' FAILED\n"), line);
                    /* Continue with next hosts */
                }
                else{
                    _tprintf(TEXT("Removing scp for '%s' SUCCESSFUL\n"), line);
                    /* Continue with next host */
                }
                break;
            default:
                smpd_err_printf("Invalid SCP control flag\n");
                break;
        }
    }
    /* Close file */
    fclose(fp);
    smpd_exit_fn(FCNAME);
    return ret_result;
}

/* Sets up service connection point for SMPD running on hostname.
 * The hostname should be a windows netbios name
 * If hostname is null the function sets up scp for SMPD running
 * on localhost
 */
#undef FCNAME
#define FCNAME "smpd_setup_scp"
int smpd_setup_scp(TCHAR *hostname)
{
    DWORD dwStatus;
    TCHAR szDNofSCP[100] = TEXT("");
    TCHAR **pspn;
    unsigned long ulSpn;
    /* LPTSTR pwszComputerName[SMPD_MAX_FQ_HOST_LENGTH]; */
    TCHAR fq_name[SMPD_MAX_FQ_NAME_LENGTH];
    TCHAR *dns_name=NULL;
    DWORD dwLen;
    int result = SMPD_SUCCESS;
    char err_msg[SMPD_MAX_ERR_MSG_LENGTH];
    int err_code;

    smpd_enter_fn(FCNAME);
    CoInitialize(NULL);

    if(hostname != NULL){
        /* Convert Netbios name to FQN name */
        result = smpd_translate_netbios_to_fqn(hostname, fq_name, SMPD_MAX_FQ_NAME_LENGTH);
        if(result != SMPD_SUCCESS){
            _tprintf(TEXT("Unable to translate netbios name (%s) to fqn name\n"), hostname);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
    }
    else{
        /* Get FQN for local host */
        dwLen = SMPD_MAX_FQ_NAME_LENGTH;
        if(!GetComputerObjectName(NameFullyQualifiedDN, (LPTSTR )fq_name, &dwLen)){
            err_code = GetLastError();
            smpd_translate_win_error(err_code, err_msg, SMPD_MAX_ERR_MSG_LENGTH,
                "GetComputerObjectName() failed (%d)", err_code);
            smpd_err_printf("%s\n", err_msg);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        dwLen = MAX_PATH;
        dns_name = (TCHAR *)MPIU_Malloc(dwLen * sizeof(TCHAR ));
        if(dns_name == NULL){
            smpd_err_printf("Unable to allocate memory for dns name\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        /* Get the DNS name for the local host */
        if(!GetComputerNameEx(ComputerNameDnsFullyQualified, (LPTSTR )dns_name, &dwLen)){
            err_code = GetLastError();
            smpd_translate_win_error(err_code, err_msg, SMPD_MAX_ERR_MSG_LENGTH,
                "GetComputerObjectName() failed (%d)", err_code);
            smpd_err_printf("%s\n", err_msg);
            MPIU_Free(dns_name);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;

        }
    }

    /* Create the service's Service Connection Point (SCP). */
    dwStatus = ScpCreate(
                    fq_name,
                    dns_name,
	                SMPD_LISTENER_PORT,
	                TEXT(SMPD_SERVICE_NAME),
	                NULL, /* Null defaults to the Local SYSTEM account */
	                100,
                    szDNofSCP /* Buffer returns the Distinguished Name of the SCP */
	            );
    if (dwStatus != 0){
	    _tprintf(TEXT("ScpCreate failed: %d\n"), dwStatus);
	    result = SMPD_FAIL;
	    goto fn_exit;
    }

    dwStatus = SpnCompose(
                &pspn, /* Receives pointer to the SPN array. */
                &ulSpn, /* Receives number of SPNs returned. */
                szDNofSCP, /* Input: DN of the SCP. */
                TEXT(SMPD_SERVICE_NAME)); /* Input: the service's class string. */

    if (dwStatus != NO_ERROR)
    {
	ReportError(TEXT("Failed to compose SPN"), dwStatus);
	result = SMPD_FAIL;
	goto fn_exit;
    }

    if (dwStatus == NO_ERROR)
    {
	dwStatus = SpnRegister(fq_name, pspn, ulSpn, DS_SPN_ADD_SPN_OP);
	if (dwStatus != NO_ERROR)
	{
	    ReportError(TEXT("SpnRegister failed"), dwStatus);
	    result = SMPD_FAIL;
	    goto fn_exit;
	}
    }

fn_exit:
    if(dns_name != NULL) MPIU_Free(dns_name);
    CoUninitialize();
    return result;
}

/* Removes service connection point for SMPD running on hostname.
 * If hostname is null the function removes scp for SMPD running
 * on localhost
 */
#define SMPD_SERVICE_LDAP_COMMON_NAME   "cn=mpich2_smpd"
#define SMPD_INIT_LDAP_SERVICE_NAME(fq_service_name, length, fq_hostname)    \
    _sntprintf_s(fq_service_name, length, length - 1, TEXT("%s,%s"), TEXT(SMPD_SERVICE_LDAP_COMMON_NAME), fq_hostname)

#undef FCNAME
#define FCNAME "smpd_remove_scp"
int smpd_remove_scp(TCHAR *hostname, smpd_spn_list_hnd_t spn_list_hnd)
{
    DWORD dwStatus;
    int err_code;
    char err_msg[SMPD_MAX_ERR_MSG_LENGTH];
    TCHAR fq_hostname[SMPD_MAX_FQ_NAME_LENGTH], fq_service_name[SMPD_MAX_FQ_NAME_LENGTH];
    TCHAR *dns_name=NULL;
    char fq_name_mb[SMPD_MAX_HOST_LENGTH * sizeof(TCHAR)];
    char *pszDnsComputerName = NULL;
    DWORD dwLen, dwMaxLen;
    int result = SMPD_SUCCESS;
    char spn_mb[SMPD_MAX_FQ_NAME_LENGTH * sizeof(TCHAR)] = "";
    LPTSTR spns;
    TCHAR spn[SMPD_MAX_FQ_NAME_LENGTH];
    WCHAR wspn[SMPD_MAX_NAME_LENGTH] = L"";
    WCHAR* pwspn[1];

    smpd_enter_fn(FCNAME);
    CoInitialize(NULL);

    /* FIXME: Insert code here to remove the information created by ScpCreate */
    if(hostname != NULL){
        /* Convert Netbios name to FQN name */
        result = smpd_translate_netbios_to_fqn(hostname, fq_hostname, SMPD_MAX_FQ_NAME_LENGTH);
        if(result != SMPD_SUCCESS){
            _tprintf(TEXT("Unable to translate netbios name (%s) to fqn name\n"), hostname);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
    }
    else{
        /* Get FQN for local host */
        dwLen = SMPD_MAX_FQ_NAME_LENGTH;
        if(!GetComputerObjectName(NameFullyQualifiedDN, (LPTSTR )fq_hostname, &dwLen)){
            err_code = GetLastError();
            smpd_translate_win_error(err_code, err_msg, SMPD_MAX_ERR_MSG_LENGTH,
                "GetComputerObjectName() failed (%d)", err_code);
            smpd_err_printf("%s\n", err_msg);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        dwLen = MAX_PATH;
        dns_name = (TCHAR *)MPIU_Malloc(dwLen * sizeof(TCHAR ));
        if(dns_name == NULL){
            smpd_err_printf("Unable to allocate memory for dns name\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        /* Get the DNS name for the local host */
        if(!GetComputerNameEx(ComputerNameDnsFullyQualified, (LPTSTR )dns_name, &dwLen)){
            err_code = GetLastError();
            smpd_translate_win_error(err_code, err_msg, SMPD_MAX_ERR_MSG_LENGTH,
                "GetComputerObjectName() failed (%d)", err_code);
            smpd_err_printf("%s\n", err_msg);
            MPIU_Free(dns_name);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;

        }
    }
    /* Create the fq name for the SMPD service */
    SMPD_INIT_LDAP_SERVICE_NAME(fq_service_name, SMPD_MAX_FQ_NAME_LENGTH, fq_hostname);

    SMPDU_TCSTOMBS(fq_name_mb, fq_service_name, SMPD_MAX_NAME_LENGTH);
    result = smpd_lookup_spn_list(spn_list_hnd, spn_mb, SMPD_MAX_FQ_NAME_LENGTH, fq_name_mb, SMPD_LISTENER_PORT);
    if (result != SMPD_SUCCESS){
        smpd_err_printf("Unable to lookup the smpd Service Principal Name for '%s'.\n", fq_name_mb);
        result = SMPD_FAIL;
        goto fn_exit;
    }
    else{
        smpd_dbg_printf("Found smpd SPN = %s\n", spn_mb);
    }

    SMPDU_MBSTOTCS(spn, spn_mb, SMPD_MAX_FQ_NAME_LENGTH);
    spns = (LPTSTR )&spn;

    dwStatus = SpnRegister(fq_hostname, (TCHAR **)&spns, 1, DS_SPN_DELETE_SPN_OP);
    if (dwStatus != NO_ERROR){
        ReportError(TEXT("Unable to delete SPN, SpnRegister failed"), dwStatus);
        result = SMPD_FAIL;
    }

    /* Remove the service's Service Connection Point (SCP). */
    dwStatus = ScpRemove(fq_hostname);
    if (dwStatus != 0){
	    _tprintf(TEXT("ScpRemove failed: %d\n"), dwStatus);
	    result = SMPD_FAIL;
	    goto fn_exit;
    }

fn_exit:

    if(dns_name) MPIU_Free(dns_name);
    CoUninitialize();
    smpd_exit_fn(FCNAME);
    return result;
}

DWORD smpd_scp_update(USHORT usPort)
{
    DWORD dwStat, dwType, dwLen;
    BOOL bUpdate=FALSE;

    HKEY hReg;

    TCHAR szAdsPath[MAX_PATH];
    TCHAR szServer[MAX_PATH];
    TCHAR szPort[8];
    TCHAR *pszAttrs[]={
	{TEXT("serviceDNSName")},
	{TEXT("serviceBindingInformation")},
    };

    HRESULT hr;
    IDirectoryObject *pObj;
    DWORD dwAttrs;
    int i;

    PADS_ATTR_INFO pAttribs;
    ADSVALUE dnsname,binding;

    ADS_ATTR_INFO Attribs[] ={
	{TEXT("serviceDnsName"),ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&dnsname,1},
	{TEXT("serviceBindingInformation"),ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&binding,1},
    };

    /* Open the service registry key. */
    dwStat = RegOpenKeyEx(
	HKEY_LOCAL_MACHINE,
	TEXT(SMPD_SERVICE_REGISTRY_KEY),
	0,
	KEY_QUERY_VALUE,
	&hReg);
    if (dwStat != NO_ERROR)
    {
	ReportServiceError("RegOpenKeyEx failed", dwStat);
	return dwStat;
    }

    /* Get the GUID binding string used to bind to the service SCP. */
    dwLen = sizeof(szAdsPath);
    dwStat = RegQueryValueEx(hReg, TEXT("GUIDBindingString"), 0, &dwType,
	(LPBYTE)szAdsPath, &dwLen);
    if (dwStat != NO_ERROR){
	ReportServiceError("RegQueryValueEx failed", dwStat);
	return dwStat;
    }

    RegCloseKey(hReg);

    /* Bind to the SCP. */
    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pObj);
    if (FAILED(hr))
    {
	char szMsg1[1024];
	sprintf(szMsg1,
	    "ADsGetObject failed to bind to GUID (bind string: %S): ",
	    szAdsPath);
	ReportServiceError(szMsg1, hr);
	if (pObj)
	{
	    pObj->Release();
	}
	return dwStat;
    }

    /* Retrieve attributes from the SCP. */
    hr = pObj->GetObjectAttributes(pszAttrs, 2, &pAttribs, &dwAttrs);
    if (FAILED(hr)){
	ReportServiceError("GetObjectAttributes failed", hr);
	pObj->Release();
	return hr;
    }

    /* Get the current port and DNS name of the host server. */
    _sntprintf(szPort, 8, TEXT("%d"), usPort);
    dwLen = sizeof(szServer);
    if (!GetComputerNameEx(ComputerNameDnsFullyQualified,szServer,&dwLen))
    {
	pObj->Release();
	return GetLastError();
    }

    /* Compare the current DNS name and port to the values retrieved from
       the SCP. Update the SCP only if nothing has changed. */
    for (i=0; i<(LONG)dwAttrs; i++)
    {
	if ((_tcscmp(TEXT("serviceDNSName"),pAttribs[i].pszAttrName)==0) &&
	    (pAttribs[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING))
	{
	    if (_tcscmp(szServer,pAttribs[i].pADsValues->CaseIgnoreString) != 0)
	    {
		ReportServiceError("serviceDNSName being updated", 0);
		bUpdate = TRUE;
	    }
	    else
	    {
		ReportServiceError("serviceDNSName okay", 0);
	    }
	}

	if ((_tcscmp(TEXT("serviceBindingInformation"),pAttribs[i].pszAttrName)==0) &&
	    (pAttribs[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING))
	{
	    if (_tcscmp(szPort,pAttribs[i].pADsValues->CaseIgnoreString) != 0)
	    {
		ReportServiceError("serviceBindingInformation being updated", 0);
		bUpdate = TRUE;
	    }
	    else
	    {
		ReportServiceError("serviceBindingInformation okay", 0);
	    }
	}
    }

    FreeADsMem(pAttribs);

    /* The binding data or server name have changed, so update the SCP values. */
    if (bUpdate)
    {
	dnsname.dwType = ADSTYPE_CASE_IGNORE_STRING;
	dnsname.CaseIgnoreString = szServer;
	binding.dwType = ADSTYPE_CASE_IGNORE_STRING;
	binding.CaseIgnoreString = szPort;
	hr = pObj->SetObjectAttributes(Attribs, 2, &dwAttrs);
	if (FAILED(hr))
	{
	    ReportServiceError("ScpUpdate: Failed to set SCP values.", hr);
	    pObj->Release();
	    return hr;
	}
    }

    pObj->Release();

    return dwStat;
}

static HRESULT AllowAccessToScpProperties(
    LPWSTR wszAccountSAM, /* Service account to allow access. */
    IADs *pSCPObject)     /* IADs pointer to the SCP object. */
{
    HRESULT hr = E_FAIL;
    IADsAccessControlList *pACL = NULL;
    IADsSecurityDescriptor *pSD = NULL;
    IDispatch *pDisp = NULL;
    IADsAccessControlEntry *pACE1 = NULL;
    IADsAccessControlEntry *pACE2 = NULL;
    IDispatch *pDispACE = NULL;
    CComBSTR sbstrTrustee;
    CComBSTR sbstrSecurityDescriptor = L"nTSecurityDescriptor";
    VARIANT varSD;

    if (pSCPObject == NULL){
	    return E_INVALIDARG;
    }

    VariantInit(&varSD);

    /*
    If no service account is specified, service runs under
    LocalSystem. Allow access to the computer account of the
    service's host.
    */
    if (wszAccountSAM){
	    sbstrTrustee = wszAccountSAM;
    }
    else{
	    LPWSTR pwszComputerName;
	    DWORD dwLen;

	    /* Get the size required for the SAM account name. */
	    dwLen = 0;
	    GetComputerObjectNameW(NameSamCompatible, NULL, &dwLen);

	    pwszComputerName = new WCHAR[dwLen + 1];
	    if (NULL == pwszComputerName){
	        hr = E_OUTOFMEMORY;
	        goto cleanup;
	    }

	    /*
	    Get the SAM account name of the computer object for
	    the server.
	    */
	    if (!GetComputerObjectNameW(NameSamCompatible, pwszComputerName, &dwLen)){
	        delete pwszComputerName;

    	    hr = HRESULT_FROM_WIN32(GetLastError());
	        goto cleanup;
	    }

	    sbstrTrustee = pwszComputerName;
	    /*wprintf(L"GetComputerObjectName: %s\n", pwszComputerName);*/
    }

    /* Get the nTSecurityDescriptor. */
    hr = pSCPObject->Get(sbstrSecurityDescriptor, &varSD);
    if (FAILED(hr) || (varSD.vt != VT_DISPATCH)){
	    _tprintf(TEXT("Get nTSecurityDescriptor failed: 0x%x\n"), hr);
	    goto cleanup;
    }

    /*
    Use the V_DISPATCH macro to get the IDispatch pointer from
    VARIANT structure and QueryInterface for an IADsSecurityDescriptor
    pointer.
    */
    hr = V_DISPATCH( &varSD )->QueryInterface(
	        IID_IADsSecurityDescriptor,
	        (void**)&pSD);
    if (FAILED(hr)){
	    _tprintf(TEXT("Cannot get IADsSecurityDescriptor: 0x%x\n"), hr);
	    goto cleanup;
    }

    /*
    Get an IADsAccessControlList pointer to the security
    descriptor's DACL.
    */
    hr = pSD->get_DiscretionaryAcl(&pDisp);
    if (SUCCEEDED(hr)){
	    hr = pDisp->QueryInterface(
	            IID_IADsAccessControlList,
	            (void**)&pACL);
    }
    else{
	    _tprintf(TEXT("Cannot get DACL: 0x%x\n"), hr);
	    goto cleanup;
    }

    /* Create the COM object for the first ACE. */
    hr = CoCreateInstance(CLSID_AccessControlEntry,
	        NULL,
	        CLSCTX_INPROC_SERVER,
	        IID_IADsAccessControlEntry,
	        (void **)&pACE1);

    /* Create the COM object for the second ACE. */
    if (SUCCEEDED(hr)){
	    hr = CoCreateInstance(CLSID_AccessControlEntry,
	            NULL,
	            CLSCTX_INPROC_SERVER,
	            IID_IADsAccessControlEntry,
	            (void **)&pACE2);
    }
    else{
	    _tprintf(TEXT("Cannot create ACEs: 0x%x\n"), hr);
	    goto cleanup;
    }

    /* Set the properties of the two ACEs. */

    /* Allow read and write access to the property. */
    hr = pACE1->put_AccessMask(
	        ADS_RIGHT_DS_READ_PROP | ADS_RIGHT_DS_WRITE_PROP);
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put access mask: 0x%x\n"), hr);
        goto cleanup;
    }
    hr = pACE2->put_AccessMask(
        	ADS_RIGHT_DS_READ_PROP | ADS_RIGHT_DS_WRITE_PROP);
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put access mask: 0x%x\n"), hr);
        goto cleanup;
    }

    /* Set the trustee, which is either the service account or the
    host computer account. */
    hr = pACE1->put_Trustee( sbstrTrustee );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put trustee (%s): 0x%x\n"), sbstrTrustee, hr);
        goto cleanup;
    }
    hr = pACE2->put_Trustee( sbstrTrustee );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put trustee (%s): 0x%x\n"), sbstrTrustee, hr);
        goto cleanup;
    }

    /* Set the ACE type. */
    hr = pACE1->put_AceType( ADS_ACETYPE_ACCESS_ALLOWED_OBJECT );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put ACE type: 0x%x\n"), hr);
        goto cleanup;
    }

    hr = pACE2->put_AceType( ADS_ACETYPE_ACCESS_ALLOWED_OBJECT );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put ACE type: 0x%x\n"), hr);
        goto cleanup;
    }

    /* Set AceFlags to zero because ACE is not inheritable. */
    hr = pACE1->put_AceFlags( 0 );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put ACE flags: 0x%x\n"), hr);
        goto cleanup;
    }
    hr = pACE2->put_AceFlags( 0 );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put ACE flags: 0x%x\n"), hr);
        goto cleanup;
    }

    /* Set Flags to indicate an ACE that protects a specified object. */
    hr = pACE1->put_Flags( ADS_FLAG_OBJECT_TYPE_PRESENT );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put ACE protect flags: 0x%x\n"), hr);
        goto cleanup;
    }
    hr = pACE2->put_Flags( ADS_FLAG_OBJECT_TYPE_PRESENT );
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put ACE protect flags: 0x%x\n"), hr);
        goto cleanup;
    }

    /* Set ObjectType to the schemaIDGUID of the attribute.
    serviceDNSName */
    hr = pACE1->put_ObjectType(
	        L"{28630eb8-41d5-11d1-a9c1-0000f80367c1}");
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put Object type: 0x%x\n"), hr);
        goto cleanup;
    }
    /* serviceBindingInformation */
    hr = pACE2->put_ObjectType(
        	L"{b7b1311c-b82e-11d0-afee-0000f80367c1}");
    if(FAILED(hr)){
        _tprintf(TEXT("Unable to put Object type: 0x%x\n"), hr);
        goto cleanup;
    }

    /*
    Add the ACEs to the DACL. Need an IDispatch pointer for
    each ACE to pass to the AddAce method.
    */
    hr = pACE1->QueryInterface(IID_IDispatch,(void**)&pDispACE);
    if (SUCCEEDED(hr)){
	    hr = pACL->AddAce(pDispACE);
    }
    else{
	    _tprintf(TEXT("Cannot add first ACE: 0x%x\n"), hr);
    	if (pDispACE)
	        pDispACE->Release();

	    pDispACE = NULL;
	    goto cleanup;
    }

    /* Repeat for the second ACE. */
    hr = pACE2->QueryInterface(IID_IDispatch, (void**)&pDispACE);
    if (SUCCEEDED(hr)){
	    hr = pACL->AddAce(pDispACE);
    }
    else{
    	_tprintf(TEXT("Cannot add second ACE: 0x%x\n"), hr);
	    goto cleanup;
    }

    /* Write the modified DACL back to the security descriptor. */
    hr = pSD->put_DiscretionaryAcl(pDisp);
    if (SUCCEEDED(hr)){
	    /* Write the ntSecurityDescriptor property to the property cache. */
	    hr = pSCPObject->Put(sbstrSecurityDescriptor, varSD);
	    if (SUCCEEDED(hr)){
	        /* SetInfo updates the SCP object in the directory. */
	        hr = pSCPObject->SetInfo();
	    }
        else{
            _tprintf(TEXT("Unable to write NT Security Desc to the property cache: 0x%x\n"), hr);
            goto cleanup;
        }
    }
    else{
        _tprintf(TEXT("Unable to write the modified DACL back to the Security Desc, 0x%x\n"), hr);
        goto cleanup;
    }

cleanup:
    if (pDispACE)
	pDispACE->Release();

    if (pACE1)
	pACE1->Release();

    if (pACE2)
	pACE2->Release();

    if (pACL)
	pACL->Release();

    if (pDisp)
	pDisp->Release();

    if (pSD)
	pSD->Release();

    VariantClear(&varSD);

    return hr;
}
