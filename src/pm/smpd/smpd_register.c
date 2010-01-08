/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "smpd.h"
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ERR_LENGTH 512

#undef FCNAME
#define FCNAME "smpd_get_all_key_names"
static int smpd_get_all_key_names(smpd_data_t **data)
{
    HKEY tkey;
    DWORD result;
    LONG enum_result;
    char name[SMPD_MAX_NAME_LENGTH];
    DWORD name_length, index;
    smpd_data_t *list, *item;
    char err_msg[MAX_ERR_LENGTH];

    smpd_enter_fn(FCNAME);

    if (data == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegOpenKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY,
	0, 
	KEY_READ,
	&tkey);
    if (result != ERROR_SUCCESS)
    {
	/* No key therefore no settings */
	*data = NULL;
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    list = NULL;
    index = 0;
    name_length = SMPD_MAX_NAME_LENGTH;
    enum_result = RegEnumValue(tkey, index, name, &name_length, NULL, NULL, NULL, NULL);
    while (enum_result == ERROR_SUCCESS)
    {
	item = (smpd_data_t*)MPIU_Malloc(sizeof(smpd_data_t));
	if (item == NULL)
	{
	    *data = NULL;
	    RegCloseKey(tkey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	memcpy(item->name, name, SMPD_MAX_NAME_LENGTH);
	item->value[0] = '\0';
	item->next = list;
	list = item;
	index++;
	name_length = SMPD_MAX_NAME_LENGTH;
	enum_result = RegEnumValue(tkey, index, name, &name_length, NULL, NULL, NULL, NULL);
    }
    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, MAX_ERR_LENGTH, "Unable to close the HKEY_CURRENT_USER\\" MPICH_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    *data = list;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_delete_current_password_registry_entry"
SMPD_BOOL smpd_delete_current_password_registry_entry(int index)
{
    int nError;
    HKEY hRegKey = NULL;
    DWORD dwNumValues;
    char key_name[256] = "smpdAccount";
    char key_pwd[256] = "smpdPassword";

    smpd_enter_fn(FCNAME);

    if (index > 0)
    {
	sprintf(key_name, "smpdAccount%d", index);
	sprintf(key_pwd, "smpdPassword%d", index);
    }

    if (index >= 0)
    {
	nError = RegOpenKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &hRegKey);
	if (nError != ERROR_SUCCESS && nError != ERROR_PATH_NOT_FOUND && nError != ERROR_FILE_NOT_FOUND)
	{
	    smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegOpenKeyEx(...) failed, error: %d\n", nError);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	nError = RegDeleteValue(hRegKey, key_pwd);
	if (nError != ERROR_SUCCESS && nError != ERROR_FILE_NOT_FOUND)
	{
	    smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegDeleteValue(password) failed, error: %d\n", nError);
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	nError = RegDeleteValue(hRegKey, key_name);
	if (nError != ERROR_SUCCESS && nError != ERROR_FILE_NOT_FOUND)
	{
	    smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegDeleteValue(account) failed, error: %d\n", nError);
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
    }
    else
    {
	/* find all keys named smpdAccount%d and smpdPassword%d and delete them */
	smpd_data_t *data;

	if (smpd_get_all_key_names(&data) == SMPD_SUCCESS)
	{
	    smpd_data_t *iter = data;

	    nError = RegOpenKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &hRegKey);
	    if (nError != ERROR_SUCCESS && nError != ERROR_PATH_NOT_FOUND && nError != ERROR_FILE_NOT_FOUND)
	    {
		smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegOpenKeyEx(...) failed, error: %d\n", nError);
		smpd_exit_fn(FCNAME);
		return SMPD_FALSE;
	    }

	    while (iter != NULL)
	    {
		if ((strncmp(iter->name, "smpdAccount", 11) == 0) || (strncmp(iter->name, "smpdPassword", 12) == 0))
		{
		    nError = RegDeleteValue(hRegKey, iter->name);
		    if (nError != ERROR_SUCCESS && nError != ERROR_FILE_NOT_FOUND)
		    {
			smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegDeleteValue(%s) failed, error: %d\n", iter->name, nError);
			RegCloseKey(hRegKey);
			smpd_exit_fn(FCNAME);
			return SMPD_FALSE;
		    }
		}
		iter = iter->next;
	    }
	    while (data != NULL)
	    {
		iter = data;
		data = data->next;
		MPIU_Free(iter);
	    }
	}
    }

    if (RegQueryInfoKey(hRegKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwNumValues, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
	RegCloseKey(hRegKey);
	RegDeleteKey(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY);
    }
    else
    {
	RegCloseKey(hRegKey);
	if (dwNumValues == 0)
	    RegDeleteKey(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_TRUE;
}

/* This function encrypts szAccount & szPassword and saves it into filename */
#undef FCNAME
#define FCNAME "smpd_save_cred_to_file"
SMPD_BOOL smpd_save_cred_to_file(const char *filename, const char *szAccount, const char *szPassword)
{
    SMPD_BOOL bResult = SMPD_TRUE;
    FILE *fp = NULL; errno_t ret_errno;
    DATA_BLOB cred_blob, safe_blob;
    char err_msg[SMPD_MAX_ERR_MSG_LENGTH];

    smpd_enter_fn(FCNAME);

    if((filename == NULL) || (strlen(filename) <=0 )){
        smpd_err_printf("Invalid registry filename \n");
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }
    if((szAccount == NULL) || (szPassword == NULL) || (strlen(szAccount) <= 0) || (strlen(szPassword) <=0)){
        smpd_err_printf("Invalid user credentials \n");
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }
    /* Open the file */
    ret_errno = fopen_s(&fp, filename, "w+b");
    if(ret_errno != 0){
        smpd_err_printf("Opening registry file failed, %s(%d)\n", strerror(ret_errno), ret_errno);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Encrypt account name */
    cred_blob.cbData = (DWORD )strlen(szAccount) + 1;
    cred_blob.pbData = (BYTE *)szAccount;
    if(!CryptProtectData(&cred_blob, L"MPICH2 User Credentials", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &safe_blob)){
        int err_num = GetLastError();
        smpd_translate_win_error(GetLastError(), err_msg, SMPD_MAX_ERR_MSG_LENGTH, "CryptProtectData() failed, errno = %d, ", err_num);
        smpd_err_printf("%s\n", err_msg);
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Store account name len */
    if(fprintf_s(fp, "%d\n", safe_blob.cbData) <= 0){
        smpd_err_printf("Error writing account len to registry file\n");
        fclose(fp);
        LocalFree(safe_blob.pbData);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }
    
    /* Store the account name*/
    if(fwrite(safe_blob.pbData, 1, safe_blob.cbData, fp) < 0){
        smpd_err_printf("Error writing account to registry file\n");
        fclose(fp);
        LocalFree(safe_blob.pbData);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }
    LocalFree(safe_blob.pbData);

    /* Encrypt password */
    cred_blob.cbData = (DWORD )strlen(szPassword) + 1;
    cred_blob.pbData = (BYTE *)szPassword;
    if(!CryptProtectData(&cred_blob, L"MPICH2 User Credentials", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &safe_blob)){
        int err_num = GetLastError();
        smpd_translate_win_error(GetLastError(), err_msg, SMPD_MAX_ERR_MSG_LENGTH, "CryptProtectData() failed, errno = %d, ", err_num);
        smpd_err_printf("%s\n", err_msg);
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Store password len */
    if(fprintf_s(fp, "%d\n", safe_blob.cbData) <= 0){
        smpd_err_printf("Error writing password len to registry file\n");
        fclose(fp);
        LocalFree(safe_blob.pbData);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Store the password*/
    if(fwrite(safe_blob.pbData, 1, safe_blob.cbData, fp) < 0){
        smpd_err_printf("Error writing password to registry file\n");
        fclose(fp);
        LocalFree(safe_blob.pbData);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }
    LocalFree(safe_blob.pbData);

    fclose(fp);
    smpd_exit_fn(FCNAME);
    return bResult;
}

/* This function reads encrypted szAccount & szPassword from filename */
#undef FCNAME
#define FCNAME "smpd_read_cred_from_file"
SMPD_BOOL smpd_read_cred_from_file(const char *filename, char *szAccount, int acc_len, char *szPassword, int pass_len)
{
    SMPD_BOOL bResult = SMPD_TRUE;
    void *en_str=NULL;
    char en_len[SMPD_MAX_INT_LENGTH+1];
    FILE *fp = NULL; errno_t ret_errno;
    DATA_BLOB cred_blob, safe_blob;
    char err_msg[SMPD_MAX_ERR_MSG_LENGTH];

    smpd_enter_fn(FCNAME);

    if((filename == NULL) || (strlen(filename) <=0 )){
        smpd_err_printf("Invalid registry filename \n");
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }
    if((szAccount == NULL) || (szPassword == NULL) || (acc_len <= 0) || (pass_len <=0)){
        smpd_err_printf("Invalid args for user credentials \n");
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }
    /* Open the file */
    ret_errno = fopen_s(&fp, filename, "r+b");
    if(ret_errno != 0){
        smpd_err_printf("Opening registry file failed, %s(%d)\n", strerror(ret_errno), ret_errno);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Read the account name len */
    if(fgets(en_len, SMPD_MAX_INT_LENGTH, fp) == NULL){
        smpd_err_printf("Error reading account length from registry file\n");
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    safe_blob.cbData = (DWORD )atoi(en_len);
    if((safe_blob.cbData <= 0) || (safe_blob.cbData > INT_MAX)){
        smpd_err_printf("Invalid account length read from registry file\n");
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    en_str = (void *)MPIU_Malloc(safe_blob.cbData);
    if(en_str == NULL){
        smpd_err_printf("Unable to allocate memory for reading account name\n");
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Read the account name */
    if(fread_s(en_str, safe_blob.cbData, 1, safe_blob.cbData, fp) <= 0){
        smpd_err_printf("Unable to read the account name from registry file\n");
        fclose(fp);
        MPIU_Free(en_str);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Decrypt account name */
    safe_blob.pbData = (BYTE *)en_str;

    if(!CryptUnprotectData(&safe_blob, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &cred_blob)){
        int err_num = GetLastError();
        smpd_translate_win_error(GetLastError(), err_msg, SMPD_MAX_ERR_MSG_LENGTH, "CryptUnprotectData() failed, errno = %d, ", err_num);
        smpd_err_printf("%s\n", err_msg);
        fclose(fp);
        MPIU_Free(en_str);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    MPIU_Strncpy(szAccount, cred_blob.pbData, acc_len);
    LocalFree(cred_blob.pbData);
    MPIU_Free(en_str);
    en_str = NULL;

    /* Read the password len */
    if(fgets(en_len, SMPD_MAX_INT_LENGTH, fp) == NULL){
        smpd_err_printf("Error reading password length from registry file\n");
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    safe_blob.cbData = (DWORD )atoi(en_len);
    if((safe_blob.cbData <= 0) || (safe_blob.cbData > INT_MAX)){
        smpd_err_printf("Invalid password length read from registry file\n");
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    en_str = (char *)MPIU_Malloc(safe_blob.cbData);
    if(en_str == NULL){
        smpd_err_printf("Unable to allocate memory for reading password\n");
        fclose(fp);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Read the password */
    if(fread_s(en_str, safe_blob.cbData, 1, safe_blob.cbData, fp) <= 0){
        smpd_err_printf("Unable to read the password from registry file\n");
        fclose(fp);
        MPIU_Free(en_str);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    /* Decrypt password */
    safe_blob.pbData = (BYTE *)en_str;

    if(!CryptUnprotectData(&safe_blob, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &cred_blob)){
        int err_num = GetLastError();
        smpd_translate_win_error(GetLastError(), err_msg, SMPD_MAX_ERR_MSG_LENGTH, "CryptUnprotectData() failed, errno = %d, ", err_num);
        smpd_err_printf("%s\n", err_msg);
        fclose(fp);
        MPIU_Free(en_str);
        smpd_exit_fn(FCNAME);
        return SMPD_FALSE;
    }

    MPIU_Strncpy(szPassword, cred_blob.pbData, pass_len);
    LocalFree(cred_blob.pbData);
    MPIU_Free(en_str);

    fclose(fp);
    smpd_exit_fn(FCNAME);
    return bResult;
}

#undef FCNAME
#define FCNAME "smpd_save_password_to_registry"
SMPD_BOOL smpd_save_password_to_registry(int index, const char *szAccount, const char *szPassword, SMPD_BOOL persistent)
{
    int nError;
    SMPD_BOOL bResult = SMPD_TRUE;
    HKEY hRegKey = NULL;
    DATA_BLOB password_blob, blob;
    char key_name[256] = "smpdAccount";
    char key_pwd[256] = "smpdPassword";
    char err_msg[MAX_ERR_LENGTH];

    smpd_enter_fn(FCNAME);

    if (persistent)
    {
	if (RegCreateKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY,
	    0, 
	    NULL, 
	    REG_OPTION_NON_VOLATILE,
	    KEY_ALL_ACCESS, 
	    NULL,
	    &hRegKey, 
	    NULL) != ERROR_SUCCESS) 
	{
	    nError = GetLastError();
	    smpd_err_printf("SavePasswordToRegistry:RegCreateKeyEx(...) failed, error: %d\n", nError);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
    }
    else
    {
	/* delete the key if it exists */
	RegDeleteKey(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY);
	/* create the key with the volatile option */
	if (RegCreateKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY,
	    0, 
	    NULL, 
	    REG_OPTION_VOLATILE,
	    KEY_ALL_ACCESS, 
	    NULL,
	    &hRegKey, 
	    NULL) != ERROR_SUCCESS) 
	{
	    nError = GetLastError();
	    smpd_err_printf("SavePasswordToRegistry:RegDeleteKey(...) failed, error: %d\n", nError);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
    }

    if (index > 0)
    {
	sprintf(key_name, "smpdAccount%d", index);
	sprintf(key_pwd, "smpdPassword%d", index);
    }

    /* Store the account name*/
    if (RegSetValueEx(hRegKey, key_name, 0, REG_SZ, (BYTE*)szAccount, (DWORD)strlen(szAccount)+1) != ERROR_SUCCESS)
    {
	nError = GetLastError();
	smpd_err_printf("SavePasswordToRegistry:RegSetValueEx(...) failed, error: %d\n", nError);
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    password_blob.cbData = (DWORD)strlen(szPassword)+1; /* store the NULL termination */
    password_blob.pbData = (BYTE*)szPassword;
    if (CryptProtectData(&password_blob, L"MPICH2 User Credentials", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blob))
    {
	/* Write data to registry.*/
	if (RegSetValueEx(hRegKey, key_pwd, 0, REG_BINARY, blob.pbData, blob.cbData) != ERROR_SUCCESS)
	{
	    nError = GetLastError();
	    smpd_err_printf("SavePasswordToRegistry:RegSetValueEx(...) failed, error: %d\n", nError);
	    bResult = SMPD_FALSE;
	}
	LocalFree(blob.pbData);
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("SavePasswordToRegistry:CryptProtectData(...) failed, error: %d\n", nError);
	bResult = SMPD_FALSE;
    }

    nError = RegCloseKey(hRegKey);
    if (nError != ERROR_SUCCESS)
    {
	smpd_translate_win_error(nError, err_msg, MAX_ERR_LENGTH, "Unable to close the HKEY_CURRENT_USER\\" MPICH_REGISTRY_KEY " registry key, error %d: ", nError);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    smpd_exit_fn(FCNAME);
    return bResult;
}

#undef FCNAME
#define FCNAME "smpd_read_password_from_registry"
SMPD_BOOL smpd_read_password_from_registry(int index, char *szAccount, char *szPassword) 
{
    int nError;
    SMPD_BOOL bResult = SMPD_TRUE;
    HKEY hRegKey = NULL;
    DWORD dwType;
    DATA_BLOB password_blob, blob;
    char key_name[256] = "smpdAccount";
    char key_pwd[256] = "smpdPassword";
    char err_msg[MAX_ERR_LENGTH];

    smpd_enter_fn(FCNAME);

    if (index > 0)
    {
	sprintf(key_name, "smpdAccount%d", index);
	sprintf(key_pwd, "smpdPassword%d", index);
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY, 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS) 
    {
	DWORD dwLength = SMPD_MAX_PASSWORD_LENGTH;
	*szAccount = '\0';
	if (RegQueryValueEx(hRegKey, key_name, NULL, NULL, (BYTE*)szAccount, &dwLength) != ERROR_SUCCESS)
	{
	    nError = GetLastError();
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
	if (strlen(szAccount) < 1)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	dwType = REG_BINARY;
	if (RegQueryValueEx(hRegKey, key_pwd, NULL, &dwType, NULL, &dwLength) == ERROR_SUCCESS)
	{
	    blob.cbData = dwLength;
	    blob.pbData = MPIU_Malloc(dwLength);
	    if (RegQueryValueEx(hRegKey, key_pwd, NULL, &dwType, (BYTE*)blob.pbData, &dwLength) == ERROR_SUCCESS)
	    {
		if (CryptUnprotectData(&blob, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &password_blob))
		{
		    strcpy(szPassword, (const char *)(password_blob.pbData));
		    LocalFree(password_blob.pbData);
		}
		else
		{
		    nError = GetLastError();
		    /*smpd_err_printf("ReadPasswordFromRegistry:CryptUnprotectData(...) failed, error: %d\n", nError);*/
		    bResult = SMPD_FALSE;
		}
	    }
	    else
	    {
		nError = GetLastError();
		/*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
		bResult = SMPD_FALSE;
	    }
	    MPIU_Free(blob.pbData);
	}
	else
	{
	    nError = GetLastError();
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    bResult = SMPD_FALSE;
	}
	nError = RegCloseKey(hRegKey);
	if (nError != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(nError, err_msg, MAX_ERR_LENGTH, "Unable to close the HKEY_CURRENT_USER\\" MPICH_REGISTRY_KEY " registry key, error %d: ", nError);
	    smpd_err_printf("%s\n", err_msg);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
    }
    else
    {
	nError = GetLastError();
	/*smpd_err_printf("ReadPasswordFromRegistry:RegOpenKeyEx(...) failed, error: %d\n", nError);*/
	bResult = SMPD_FALSE;
    }

    smpd_exit_fn(FCNAME);
    return bResult;
}

#undef FCNAME
#define FCNAME "smpd_cache_password"
int smpd_cache_password(const char *account, const char *password)
{
    int nError;
    char szEncodedPassword[SMPD_MAX_PASSWORD_LENGTH*2+1];
    HKEY hRegKey = NULL;
    int num_bytes;
    DWORD len;
    char err_msg[MAX_ERR_LENGTH];

    smpd_enter_fn(FCNAME);

    if (smpd_option_on("nocache"))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    RegDeleteKey(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY,
	0, 
	NULL, 
	REG_OPTION_VOLATILE,
	KEY_ALL_ACCESS, 
	NULL,
	&hRegKey, 
	NULL) != ERROR_SUCCESS) 
    {
	nError = GetLastError();
	/*smpd_err_printf("CachePassword:RegDeleteKey(...) failed, error: %d\n", nError);*/
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* Store the account name*/
    len = (DWORD)strlen(account)+1;
    nError = RegSetValueEx(hRegKey, "smpda", 0, REG_SZ, (BYTE*)account, len);
    if (nError != ERROR_SUCCESS)
    {
	/*smpd_err_printf("CachePassword:RegSetValueEx(%s) failed, error: %d\n", g_pszAccount, nError);*/
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* encode the password*/
    smpd_encode_buffer(szEncodedPassword, SMPD_MAX_PASSWORD_LENGTH*2, password, (int)strlen(password)+1, &num_bytes);
    szEncodedPassword[num_bytes*2] = '\0';
    /*smpd_dbg_printf("szEncodedPassword = '%s'\n", szEncodedPassword);*/

    /* Store the encoded password*/
    nError = RegSetValueEx(hRegKey, "smpdp", 0, REG_SZ, (BYTE*)szEncodedPassword, num_bytes*2);
    if (nError != ERROR_SUCCESS)
    {
	/*smpd_err_printf("CachePassword:RegSetValueEx(...) failed, error: %d\n", nError);*/
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    nError = RegCloseKey(hRegKey);
    if (nError != ERROR_SUCCESS)
    {
	smpd_translate_win_error(nError, err_msg, MAX_ERR_LENGTH, "Unable to close the HKEY_CURRENT_USER\\" SMPD_REGISTRY_CACHE_KEY " registry key, error %d: ", nError);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_cached_password"
SMPD_BOOL smpd_get_cached_password(char *account, char *password)
{
    int nError;
    char szAccount[SMPD_MAX_ACCOUNT_LENGTH];
    char szPassword[SMPD_MAX_PASSWORD_LENGTH*2];
    HKEY hRegKey = NULL;
    DWORD dwLength;
    int num_bytes;
    char err_msg[MAX_ERR_LENGTH];

    smpd_enter_fn(FCNAME);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY, 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS) 
    {
	*szAccount = '\0';
	dwLength = SMPD_MAX_ACCOUNT_LENGTH;
	if ((nError = RegQueryValueEx(
	    hRegKey, 
	    "smpda", NULL, 
	    NULL, 
	    (BYTE*)szAccount, 
	    &dwLength))!=ERROR_SUCCESS)
	{
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
	if (strlen(szAccount) < 1)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	*szPassword = '\0';
	dwLength = SMPD_MAX_PASSWORD_LENGTH*2;
	if ((nError = RegQueryValueEx(
	    hRegKey, 
	    "smpdp", NULL, 
	    NULL, 
	    (BYTE*)szPassword, 
	    &dwLength))!=ERROR_SUCCESS)
	{
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	nError = RegCloseKey(hRegKey);
	if (nError != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(nError, err_msg, MAX_ERR_LENGTH, "Unable to close the HKEY_CURRENT_USER\\" SMPD_REGISTRY_CACHE_KEY " registry key, error %d: ", nError);
	    smpd_err_printf("%s\n", err_msg);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	strcpy(account, szAccount);
	smpd_decode_buffer(szPassword, password, SMPD_MAX_PASSWORD_LENGTH, &num_bytes);
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_delete_cached_password"
int smpd_delete_cached_password()
{
    smpd_enter_fn(FCNAME);
    RegDeleteKey(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
