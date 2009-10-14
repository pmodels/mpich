/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>

#ifndef HAVE_WINDOWS_H
static FILE * smpd_open_smpd_file(SMPD_BOOL create);
static void str_replace(char *str, char *find_chars, char replace_char);

#undef FCNAME
#define FCNAME "smpd_open_smpd_file"
static FILE * smpd_open_smpd_file(SMPD_BOOL create)
{
    char *homedir;
    struct stat s;
    FILE *fin = NULL;

    smpd_enter_fn(FCNAME);
    if (smpd_process.smpd_filename[0] == '\0')
    {
	homedir = getenv("HOME");
	if (homedir != NULL)
	{
	    strcpy(smpd_process.smpd_filename, homedir);
	    if (smpd_process.smpd_filename[strlen(smpd_process.smpd_filename)-1] != '/')
	    {
		strcat(smpd_process.smpd_filename, "/.smpd");
	    }
	    else
	    {
		strcat(smpd_process.smpd_filename, ".smpd");
	    }
	}
	else
	{
	    strcpy(smpd_process.smpd_filename, ".smpd");
	}
    }
    if (stat(smpd_process.smpd_filename, &s) == 0)
    {
	if (s.st_mode & 00077)
	{
	    printf(".smpd file, %s, cannot be readable by anyone other than the current user, please set the permissions accordingly (0600).\n", smpd_process.smpd_filename);
	}
	else
	{
	    if (create)
	    {
		fin = fopen(smpd_process.smpd_filename, "w");
	    }
	    else
	    {
		fin = fopen(smpd_process.smpd_filename, "r+");
	    }
	}
    }
    if (fin == NULL && create)
    {
	umask(077);
	fin = fopen(smpd_process.smpd_filename, "w");
    }
    smpd_exit_fn(FCNAME);
    return fin;
}

#undef FCNAME
#define FCNAME "str_replace"
static void str_replace(char *str, char *find_chars, char replace_char)
{
    char *cur_str;
    int i;

    smpd_enter_fn(FCNAME);
    for (i=0; i<strlen(find_chars); i++)
    {
	cur_str = strchr(str, find_chars[i]);
	while (cur_str)
	{
	    *cur_str = replace_char;
	    cur_str = strchr(cur_str, find_chars[i]);
	}
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "smpd_parse_smpd_file"
static smpd_data_t * smpd_parse_smpd_file()
{
    FILE *fin;
    char *buffer;
    int len;
    smpd_data_t *list = NULL, *node;
    char *iter;
    char name[SMPD_MAX_NAME_LENGTH];
    char equal_str[SMPD_MAX_NAME_LENGTH];
    char data[SMPD_MAX_VALUE_LENGTH];
    int num_chars;
    int result;

    smpd_enter_fn(FCNAME);
    fin = smpd_open_smpd_file(SMPD_FALSE);
    if (fin != NULL)
    {
	result = fseek(fin, 0, SEEK_END);
	if (result != 0)
	{
	    smpd_err_printf("Unable to seek to the end of the .smpd file.\n");
	    smpd_exit_fn(FCNAME);
	    return NULL;
	}
	len = ftell(fin);
	if (len == -1)
	{
	    smpd_err_printf("Unable to determine the length of the .smpd file, ftell returned -1 and errno = %d.\n", errno);
	    smpd_exit_fn(FCNAME);
	    return NULL;
	}
	result = fseek(fin, 0, SEEK_SET);
	if (result != 0)
	{
	    smpd_err_printf("Unable to seek to the beginning of the .smpd file.\n");
	    smpd_exit_fn(FCNAME);
	    return NULL;
	}
	if (len > 0)
	{
	    buffer = (char*)MPIU_Malloc(len+1);
	    if (buffer == NULL)
	    {
		smpd_err_printf("Unable to allocate a buffer of length %d, malloc failed.\n", len+1);
		smpd_exit_fn(FCNAME);
		return NULL;
	    }
	    iter = buffer;
	    if ((len = (int)fread(buffer, 1, len, fin)) > 0)
	    {
		buffer[len] = '\0';
		str_replace(buffer, "\r\n", SMPD_SEPAR_CHAR);
		while (iter)
		{
		    result = MPIU_Str_get_string(&iter, name, SMPD_MAX_NAME_LENGTH);
		    if (result != MPIU_STR_SUCCESS)
		    {
			smpd_exit_fn(FCNAME);
			return NULL;
		    }
		    equal_str[0] = '\0';
		    result = MPIU_Str_get_string(&iter, equal_str, SMPD_MAX_NAME_LENGTH);
		    while (iter && equal_str[0] != SMPD_DELIM_CHAR)
		    {
			strcpy(name, equal_str);
			result = MPIU_Str_get_string(&iter, equal_str, SMPD_MAX_NAME_LENGTH);
			if (result != MPIU_STR_SUCCESS)
			{
			    smpd_exit_fn(FCNAME);
			    return NULL;
			}
		    }
		    data[0] = '\0';
		    result = MPIU_Str_get_string(&iter, data, SMPD_MAX_VALUE_LENGTH);
		    /*smpd_dbg_printf("parsed <%s> <%s> <%s>\n", name, equal_str, data);*/
		    if (result == MPIU_STR_SUCCESS)
		    {
			node = (smpd_data_t*)MPIU_Malloc(sizeof(smpd_data_t));
			if (node == NULL)
			{
			    smpd_err_printf("Unable to allocate a smpd_data_t node, malloc failed.\n");
			    smpd_exit_fn(FCNAME);
			    return list; /* Should we return NULL or the current number of parsed options? */
			}
			strcpy(node->name, name);
			strcpy(node->value, data);
			node->next = list;
			list = node;
		    }
		}
	    }
	    else
	    {
		printf("Unable to read the contents of %s.\n", smpd_process.smpd_filename);
	    }
	    MPIU_Free(buffer);
	}
	fclose(fin);
    }
    smpd_exit_fn(FCNAME);
    return list;
}
#endif

#undef FCNAME
#define FCNAME "smpd_is_affirmative"
SMPD_BOOL smpd_is_affirmative(const char *str)
{
    const char *iter = str;

    smpd_enter_fn(FCNAME);
    if (*iter != 'y' && *iter != 'Y')
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }
    iter++;
    if (*iter == '\0')
    {
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }
    if (*iter == '\n')
    {
	iter++;
	if (*iter == '\0')
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_TRUE;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }
    if (*iter != 'e' && *iter != 'E')
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }
    iter++;
    if (*iter != 's' && *iter != 's')
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }
    iter++;
    if (*iter == '\n')
    {
	iter++;
	if (*iter == '\0')
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_TRUE;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }
    if (*iter == '\0')
    {
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;

    /*
    if (strcmp(str, "yes\n") == 0 ||
	strcmp(str, "Yes\n") == 0 ||
	strcmp(str, "YES\n") == 0 ||
	strcmp(str, "Y\n") == 0 ||
	strcmp(str, "y\n") == 0 ||
	strcmp(str, "yes") == 0 ||
	strcmp(str, "Yes") == 0 ||
	strcmp(str, "YES") == 0 ||
	strcmp(str, "Y") == 0 ||
	strcmp(str, "y") == 0)
	return SMPD_TRUE;
    return SMPD_FALSE;
    */
}

#undef FCNAME
#define FCNAME "smpd_option_on"
SMPD_BOOL smpd_option_on(const char *option)
{
    char val[SMPD_MAX_VALUE_LENGTH];

    smpd_enter_fn(FCNAME);

    if (smpd_get_smpd_data(option, val, SMPD_MAX_VALUE_LENGTH) == SMPD_SUCCESS)
    {
	if (smpd_is_affirmative(val) || (strcmp(val, "1") == 0))
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_TRUE;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_delete_user_data"
int smpd_delete_user_data(const char *key)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD result;
    char err_msg[512];

    smpd_enter_fn(FCNAME);

    if (key == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCreateKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegDeleteValue(tkey, key);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to delete the user smpd registry value '%s', error %d: ", key, result);
	smpd_err_printf("%s\n", err_msg);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to close the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int result;
    smpd_enter_fn(FCNAME);
    result = smpd_delete_smpd_data(key);
    smpd_exit_fn(FCNAME);
    return result;
#endif
}

#undef FCNAME
#define FCNAME "smpd_delete_smpd_data"
int smpd_delete_smpd_data(const char *key)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD result;
    char err_msg[512];

    smpd_enter_fn(FCNAME);

    if (key == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegDeleteValue(tkey, key);
    if (result != ERROR_SUCCESS)
    {
	if (result != ERROR_FILE_NOT_FOUND && result != ERROR_PATH_NOT_FOUND)
	{
	    smpd_translate_win_error(result, err_msg, 512, "Unable to delete the smpd registry value '%s', error %d: ", key, result);
	    smpd_err_printf("%s\n", err_msg);
	    RegCloseKey(tkey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to close the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int result;
    smpd_data_t *list = NULL, *node, *trailer;
    int num_bytes;
    int found = 0;

    smpd_enter_fn(FCNAME);

    list = smpd_parse_smpd_file();

    node = trailer = list;
    while (node)
    {
	if (strcmp(key, node->name) == 0)
	{
	    if (trailer != node)
	    {
		trailer->next = node->next;
	    }
	    else
	    {
		list = list->next;
	    }
	    found = 1;
	    MPIU_Free(node);
	    break;
	}
	if (trailer != node)
	{
	    trailer = trailer->next;
	}
	node = node->next;
    }
    if (found)
    {
	FILE *fout;
	char buffer[1024];
	char *str;
	int maxlen;

	fout = smpd_open_smpd_file(SMPD_TRUE);
	if (fout)
	{
	    while (list)
	    {
		str = buffer;
		maxlen = 1024;
		if (MPIU_Str_add_string_arg(&str, &maxlen, list->name, list->value) == MPIU_STR_SUCCESS)
		{
		    buffer[strlen(buffer)-1] = '\0'; /* remove the trailing space */
		    fprintf(fout, "%s\n", buffer);
		}
		node = list;
		list = list->next;
		MPIU_Free(node);
	    }
	    fclose(fout);
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
    }
    while (list)
    {
	node = list;
	list = list->next;
	MPIU_Free(node);
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#undef FCNAME
#define FCNAME "smpd_set_user_data"
int smpd_set_user_data(const char *key, const char *value)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;
    char err_msg[512];

    smpd_enter_fn(FCNAME);

    if (key == NULL || value == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCreateKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    len = (DWORD)(strlen(value)+1);
    result = RegSetValueEx(tkey, key, 0, REG_SZ, (const BYTE *)value, len);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to write the user smpd registry value '%s:%s', error %d\n", key, value, result);
	smpd_err_printf("%s\n", err_msg);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to close the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int result;
    smpd_enter_fn(FCNAME);
    result = smpd_set_smpd_data(key, value);
    smpd_exit_fn(FCNAME);
    return result;
#endif
}

#undef FCNAME
#define FCNAME "smpd_set_smpd_data"
int smpd_set_smpd_data(const char *key, const char *value)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;
    char err_msg[512];

    smpd_enter_fn(FCNAME);

    if (key == NULL || value == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to open the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    len = (DWORD)(strlen(value)+1);
    result = RegSetValueEx(tkey, key, 0, REG_SZ, (const BYTE *)value, len);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to write the smpd registry value '%s:%s', error %d\n", key, value, result);
	smpd_err_printf("%s\n", err_msg);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to close the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int result;
    smpd_data_t *list = NULL, *node;
    int found = 0;
    FILE *fout;
    char *str;
    int maxlen;
    char buffer[1024];
    char name_str[SMPD_MAX_NAME_LENGTH];
    char value_str[SMPD_MAX_VALUE_LENGTH];

    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("setting smpd data: %s=%s\n", key, value);

    list = smpd_parse_smpd_file();
    fout = smpd_open_smpd_file(SMPD_TRUE);
    if (fout == NULL)
    {
	smpd_err_printf("Unable to open the .smpd file\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    while (list)
    {
	node = list;
	list = list->next;
	if (strcmp(key, node->name) == 0)
	{
	    strcpy(node->value, value);
	    found = 1;
	}
	if (fout)
	{
	    str = buffer;
	    maxlen = 1024;
	    if (MPIU_Str_add_string_arg(&str, &maxlen, node->name, node->value) == MPIU_STR_SUCCESS)
	    {
		buffer[strlen(buffer)-1] = '\0'; /* remove the trailing space */
		smpd_dbg_printf("writing '%s' to .smpd file\n", buffer);
		fprintf(fout, "%s\n", buffer);
	    }
	}
	MPIU_Free(node);
    }
    if (!found && fout)
    {
	str = buffer;
	maxlen = 1024;
	if (MPIU_Str_add_string_arg(&str, &maxlen, key, value) == MPIU_STR_SUCCESS)
	{
	    buffer[strlen(buffer)-1] = '\0'; /* remove the trailing space */
	    smpd_dbg_printf("writing '%s' to .smpd file\n", buffer);
	    fprintf(fout, "%s\n", buffer);
	}
	fclose(fout);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (fout != NULL)
    {
	fclose(fout);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_get_user_data_default"
int smpd_get_user_data_default(const char *key, char *value, int value_len)
{
    smpd_enter_fn(FCNAME);
    /* FIXME: function not implemented */
    SMPD_UNREFERENCED_ARG(key);
    SMPD_UNREFERENCED_ARG(value);
    SMPD_UNREFERENCED_ARG(value_len);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_get_user_data"
int smpd_get_user_data(const char *key, char *value, int value_len)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;
    char err_msg[512];

    smpd_enter_fn(FCNAME);

    result = RegOpenKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_KEY, 0, KEY_READ, &tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	smpd_dbg_printf("%s\n", err_msg);
	result = smpd_get_user_data_default(key, value, value_len);
	smpd_exit_fn(FCNAME);
	return result;
    }

    len = value_len;
    result = RegQueryValueEx(tkey, key, 0, NULL, (unsigned char *)value, &len);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to read the smpd registry key '%s', error %d\n", key, result);
	smpd_dbg_printf("%s\n", err_msg);
	RegCloseKey(tkey);
	result = smpd_get_user_data_default(key, value, value_len);
	smpd_exit_fn(FCNAME);
	return result;
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to close the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int result;
    smpd_enter_fn(FCNAME);
    result = smpd_get_smpd_data(key, value, value_len);
    if (result != SMPD_SUCCESS)
    {
	result = smpd_get_user_data_default(key, value, value_len);
    }
    smpd_exit_fn(FCNAME);
    return result;
#endif
}

#undef FCNAME
#define FCNAME "smpd_get_smpd_data_default"
int smpd_get_smpd_data_default(const char *key, char *value, int value_len)
{
    smpd_enter_fn(FCNAME);
#ifdef HAVE_WINDOWS_H
    /* A default passphrase is only available for Windows */
    if (strcmp(key, "phrase") == 0)
    {
	strncpy(value, SMPD_DEFAULT_PASSPHRASE, value_len);
	value[value_len-1] = '\0';
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if(strcmp(key, "port") == 0)
    {
    snprintf(value, value_len, "%d", SMPD_LISTENER_PORT);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
    }
#endif
    if (strcmp(key, "log") == 0)
    {
	strncpy(value, "no", value_len);
	value[value_len-1] = '\0';
    }
    else if (strcmp(key, "prepend_rank") == 0)
    {
	strncpy(value, "yes", value_len);
	value[value_len-1] = '\0';
    }
    else if (strcmp(key, "trace") == 0)
    {
	strncpy(value, "yes", value_len);
	value[value_len-1] = '\0';
    }
    else if (strcmp(key, "noprompt") == 0)
    {
	strncpy(value, "no", value_len);
	value[value_len-1] = '\0';
    }
    /*
    else if (strcmp(key, "hosts") == 0)
    {
	if (smpd_get_hostname(value, value_len) != 0)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    */
    else
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_smpd_data_from_environment"
static SMPD_BOOL smpd_get_smpd_data_from_environment(const char *key, char *value, int value_len)
{
    char *env_option, *env;
    size_t length;

    smpd_enter_fn(FCNAME);

    /* Check to see if the option has been set in the environment */
    length = strlen(key) + strlen(SMPD_ENV_OPTION_PREFIX) + 1;
    env_option = (char*)MPIU_Malloc(length * sizeof(char));
    if (env_option != NULL)
    {
	strcpy(env_option, SMPD_ENV_OPTION_PREFIX);
	env = &env_option[strlen(SMPD_ENV_OPTION_PREFIX)];
	while (*key != '\0')
	{
	    *env = (char)(toupper(*key));
	    key++;
	    env++;
	}
	*env = '\0';
	env = getenv(env_option);
	MPIU_Free(env_option);
	if (env != NULL)
	{
	    if ((int)strlen(env) < value_len)
	    {
		strcpy(value, env);
		smpd_exit_fn(FCNAME);
		return SMPD_TRUE;
	    }
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_get_smpd_data"
int smpd_get_smpd_data(const char *key, char *value, int value_len)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;
    char err_msg[512];

    smpd_enter_fn(FCNAME);

    if (smpd_get_smpd_data_from_environment(key, value, value_len) == SMPD_TRUE)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY, 0, KEY_READ, &tkey);
    if (result != ERROR_SUCCESS)
    {
	if (smpd_get_smpd_data_default(key, value, value_len) != SMPD_SUCCESS)
	{
	    smpd_dbg_printf("Unable to get the data for the key '%s'\n", key);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    len = value_len;
    result = RegQueryValueEx(tkey, key, 0, NULL, (unsigned char *)value, &len);
    if (result != ERROR_SUCCESS)
    {
	RegCloseKey(tkey);
	if (smpd_get_smpd_data_default(key, value, value_len) != SMPD_SUCCESS)
	{
	    smpd_dbg_printf("Unable to get the data for the key '%s'\n", key);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to close the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int result;
    smpd_data_t *list = NULL, *node;
    int num_bytes;

    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("getting smpd data: %s\n", key);

    if (smpd_get_smpd_data_from_environment(key, value, value_len) == SMPD_TRUE)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    list = smpd_parse_smpd_file();

    if (list)
    {
	int found = 0;
	while (list)
	{
	    node = list;
	    list = list->next;
	    if (strcmp(key, node->name) == 0)
	    {
		strcpy(value, node->value);
		smpd_dbg_printf("smpd data: %s=%s\n", key, value);
		found = 1;
	    }
	    MPIU_Free(node);
	}
	if (found)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
    }

    result = smpd_get_smpd_data_default(key, value, value_len);
    if (result == SMPD_SUCCESS)
    {
	smpd_dbg_printf("smpd data: %s=%s\n", key, value);
    }
    else
    {
	smpd_dbg_printf("smpd data: failed to get %s\n", key);
    }

    smpd_exit_fn(FCNAME);
    return result;
#endif
}

#undef FCNAME
#define FCNAME "smpd_get_all_smpd_data"
int smpd_get_all_smpd_data(smpd_data_t **data)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD result;
    LONG enum_result;
    char name[SMPD_MAX_NAME_LENGTH], value[SMPD_MAX_VALUE_LENGTH];
    DWORD name_length, value_length, index;
    smpd_data_t *list, *item;
    char err_msg[512];

    smpd_enter_fn(FCNAME);

    if (data == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY, 0, KEY_READ, &tkey);
    if (result != ERROR_SUCCESS)
    {
	/* No key therefore no settings */
	/* No access to the key, therefore no soup for you */
	*data = NULL;
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    list = NULL;
    index = 0;
    name_length = SMPD_MAX_NAME_LENGTH;
    value_length = SMPD_MAX_VALUE_LENGTH;
    enum_result = RegEnumValue(tkey, index, name, &name_length, NULL, NULL, (LPBYTE)value, &value_length);
    while (enum_result == ERROR_SUCCESS)
    {
	item = (smpd_data_t*)MPIU_Malloc(sizeof(smpd_data_t));
	if (item == NULL)
	{
	    *data = NULL;
	    result = RegCloseKey(tkey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	memcpy(item->name, name, SMPD_MAX_NAME_LENGTH);
	memcpy(item->value, value, SMPD_MAX_VALUE_LENGTH);
	item->next = list;
	list = item;
	index++;
	name_length = SMPD_MAX_NAME_LENGTH;
	value_length = SMPD_MAX_VALUE_LENGTH;
	enum_result = RegEnumValue(tkey, index, name, &name_length, NULL, NULL, (LPBYTE)value, &value_length);
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 512, "Unable to close the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY " registry key, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    *data = list;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    smpd_enter_fn(FCNAME);
    if (data == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    *data = NULL;
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_lock_smpd_data"
int smpd_lock_smpd_data(void)
{
    smpd_enter_fn(FCNAME);
#ifdef HAVE_WINDOWS_H
    if (smpd_process.hSMPDDataMutex == NULL)
    {
	smpd_process.hSMPDDataMutex = CreateMutex(NULL, FALSE, SMPD_DATA_MUTEX_NAME);
	if (smpd_process.hSMPDDataMutex == NULL)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    if (WaitForSingleObject(smpd_process.hSMPDDataMutex, SMPD_SHORT_TIMEOUT*1000) != WAIT_OBJECT_0)
    {
	smpd_err_printf("lock_smpd_data failed\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
#else
    /* No lock implemented for Unix systems */
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_unlock_smpd_data"
int smpd_unlock_smpd_data(void)
{
    smpd_enter_fn(FCNAME);
#ifdef HAVE_WINDOWS_H
    if (!ReleaseMutex(smpd_process.hSMPDDataMutex))
    {
	int result;
	char err_msg[512];
	result = GetLastError();
	smpd_translate_win_error(result, err_msg, 512, "Unable to release the SMPD data mutex, error %d: ", result);
	smpd_err_printf("%s\n", err_msg);
	return SMPD_FAIL;
    }
#else
    /* No lock implemented for Unix systems */
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
