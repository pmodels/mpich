/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* getenv */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* strrchr */
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef HAVE_WINDOWS_H
static int exists(char *filename)
{
    struct stat file_stat;

    if ((stat(filename, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode)))
    {
	return 0; /* no such file, or not a regular file */
    }
    return 1;
}
#endif

#undef FCNAME
#define FCNAME "smpd_get_full_path_name"
SMPD_BOOL smpd_get_full_path_name(const char *exe, int maxlen, char *exe_path, char **namepart)
{
#ifdef HAVE_WINDOWS_H
    DWORD dwResult;
    DWORD dwLength;
    int len;
    char buffer[SMPD_MAX_EXE_LENGTH];
    char info_buffer[sizeof(REMOTE_NAME_INFO) + SMPD_MAX_EXE_LENGTH];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)info_buffer;
    char *filename;
    char temp_name[SMPD_MAX_EXE_LENGTH];
    char *temp_exe = NULL;

    smpd_enter_fn(FCNAME);

    /* handle the common case of unix programmers specifying executable like this: ./foo */
    if (strlen(exe) > 2)
    {
	if (exe[0] == '.' && exe[1] == '/')
	{
	    temp_exe = MPIU_Strdup(exe);
	    temp_exe[1] = '\\';
	    exe = temp_exe;
	}
	smpd_dbg_printf("fixing up exe name: '%s' -> '%s'\n", exe, temp_exe);
    }

    /* make a full path out of the name provided */
    len = GetFullPathName(exe, maxlen, exe_path, namepart);
    if (temp_exe != NULL)
    {
	MPIU_Free(temp_exe);
	temp_exe = NULL;
    }
    if (len == 0 || len > maxlen)
    {
	smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n", maxlen, len);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }
    *(*namepart - 1) = '\0'; /* separate the path from the executable */
    
    /* Verify file exists.  If it doesn't search the path for exe */
    if ((len = SearchPath(exe_path, *namepart, NULL, SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
    {
	if ((len = SearchPath(exe_path, *namepart, ".exe", SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
	{
	    /* search the default path for an exact match */
	    if ((len = SearchPath(NULL, *namepart, NULL, SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
	    {
		/* search the default path for a match + .exe */
		if ((len = SearchPath(NULL, *namepart, ".exe", SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
		{
		    smpd_dbg_printf("path not found. leaving as is in case the path exists on the remote machine.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_TRUE;
		}
	    }
	    if (len > SMPD_MAX_EXE_LENGTH || len > maxlen)
	    {
		smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n", maxlen, len);
		smpd_exit_fn(FCNAME);
		return SMPD_FALSE;
	    }
	    *(filename - 1) = '\0'; /* separate the file name */
	    /* copy the path */
	    strcpy(exe_path, buffer);
	    *namepart = &exe_path[strlen(exe_path)+1];
	    /* copy the filename */
	    strcpy(*namepart, filename);
	}
    }
    if (len > maxlen)
    {
	smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n", maxlen, len);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    /* save the filename */
    strcpy(temp_name, *namepart);

    /* convert the path to its UNC equivalent to avoid need to map a drive */
    dwLength = sizeof(REMOTE_NAME_INFO)+SMPD_MAX_EXE_LENGTH;
    info->lpConnectionName = NULL;
    info->lpRemainingPath = NULL;
    info->lpUniversalName = NULL;
    dwResult = WNetGetUniversalName(exe_path, REMOTE_NAME_INFO_LEVEL, info, &dwLength);
    if (dwResult == NO_ERROR)
    {
	if ((int)(strlen(info->lpUniversalName) + strlen(temp_name) + 2) > maxlen)
	{
	    smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n",
		maxlen, strlen(info->lpUniversalName) + strlen(temp_name) + 2);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
	strcpy(exe_path, info->lpUniversalName);
	*namepart = &exe_path[strlen(exe_path)+1];
	strcpy(*namepart, temp_name);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_TRUE;
#else
    char *path = NULL;
    char temp_str[SMPD_MAX_EXE_LENGTH] = "./";

    smpd_enter_fn(FCNAME);

    getcwd(temp_str, SMPD_MAX_EXE_LENGTH);

    if (temp_str[strlen(temp_str)-1] != '/')
	strcat(temp_str, "/");

    /* start with whatever they give you tacked on to the cwd */
    snprintf(exe_path, maxlen, "%s%s", temp_str, exe);
    if (exists(exe_path))
    {
	*namepart = strrchr(exe_path, '/');
	**namepart = '\0'; /* separate the path from the executable */
	*namepart = *namepart + 1;
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }
    *namepart = strrchr(exe_path, '/');
    **namepart = '\0'; /* separate the path from the executable */
    *namepart = *namepart + 1;

    /* add searching of the path and verifying file exists */
    path = getenv("PATH");
    strcpy(temp_str, *namepart);
    if (smpd_search_path(path, temp_str, maxlen, exe_path))
    {
	*namepart = strrchr(exe_path, '/');
	**namepart = '\0';
	*namepart = *namepart + 1;
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
#endif
}

#undef FCNAME
#define FCNAME "smpd_search_path"
SMPD_BOOL smpd_search_path(const char *smpd_path, const char *exe, int maxlen, char *str)
{
#ifdef HAVE_WINDOWS_H
    char *filepart;
    char *path_spec, *smpd_path2 = NULL, exe_path[SMPD_MAX_EXE_LENGTH];
    size_t len_pre, len_spec, len_post;
    HMODULE hModule;

    smpd_enter_fn(FCNAME);

    path_spec = strstr(smpd_path, SMPD_PATH_SPEC);
    if (path_spec != NULL)
    {
	if (smpd_process.pszExe[0] == '\0')
	{
	    hModule = GetModuleHandle(NULL);
	    if (!GetModuleFileName(hModule, smpd_process.pszExe, SMPD_MAX_EXE_LENGTH)) 
	    {
		strcpy(smpd_process.pszExe, ".\\smpd.exe");
	    }
	}
	smpd_path2 = strrchr(smpd_process.pszExe, '\\');
	if (smpd_path2 == NULL)
	{
	    exe_path[0] = '\0';
	}
	else
	{
	    memcpy(exe_path, smpd_process.pszExe, smpd_path2 - smpd_process.pszExe);
	    exe_path[smpd_path2 - smpd_process.pszExe] = '\0';
	}
	len_pre = path_spec - smpd_path;
	len_spec = strlen(exe_path);
	len_post = strlen(path_spec + strlen(SMPD_PATH_SPEC));
	smpd_path2 = (char *)MPIU_Malloc((len_pre + len_spec + len_post + 1) * sizeof(char));
	if (len_pre)
	{
	    memcpy(smpd_path2, smpd_path, len_pre);
	}
	memcpy(smpd_path2 + len_pre, exe_path, len_spec);
	strcpy(smpd_path2 + len_pre + len_spec, path_spec + strlen(SMPD_PATH_SPEC));
	smpd_dbg_printf("changed path(%s) to (%s)\n", smpd_path, smpd_path2);
	smpd_path = smpd_path2;
    }

    /* search for exactly what's specified */
    if (SearchPath(smpd_path, exe, NULL, maxlen, str, &filepart) == 0)
    {
	/* search for file + .exe */
	if (SearchPath(smpd_path, exe, ".exe", maxlen, str, &filepart) == 0)
	{
	    /* search the default path */
	    if (SearchPath(NULL, exe, NULL, maxlen, str, &filepart) == 0)
	    {
		/* search the default path + .exe */
		if (SearchPath(NULL, exe, ".exe", maxlen, str, &filepart) == 0)
		{
		    if (smpd_path2 != NULL)
		    {
			MPIU_Free(smpd_path2);
		    }
		    smpd_exit_fn(FCNAME);
		    return SMPD_FALSE;
		}
	    }
	}
    }
    if (smpd_path2 != NULL)
    {
	MPIU_Free(smpd_path2);
    }
    smpd_exit_fn(FCNAME);
    return SMPD_TRUE;
#else
    char test[SMPD_MAX_EXE_LENGTH];
    char path[SMPD_MAX_PATH_LENGTH];
    char *token;
    int n;

    smpd_enter_fn(FCNAME);

    if (smpd_path == NULL || exe == NULL || maxlen < 1 || str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    strncpy(path, smpd_path, SMPD_MAX_PATH_LENGTH);
    path[SMPD_MAX_PATH_LENGTH - 1] = '\0';
    token = strtok(path, ";:");
    while (token)
    {
	/* this does not catch the case where SMPD_MAX_EXE_LENGTH is not long enough and the file exists */
	if (token[strlen(token)-1] != '/')
	    n = snprintf(test, SMPD_MAX_EXE_LENGTH, "%s/%s", token, exe);
	else
	    n = snprintf(test, SMPD_MAX_EXE_LENGTH, "%s%s", token, exe);
	test[SMPD_MAX_EXE_LENGTH-1] = '\0';
	if (exists(test))
	{
	    if (n < maxlen)
	    {
		strcpy(str, test);
		smpd_exit_fn(FCNAME);
		return SMPD_TRUE;
	    }
	    smpd_err_printf("buffer provided is too small: %d provided, %d needed\n", maxlen, n);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
	token = strtok(NULL, ";:");
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
#endif
}

#ifndef HAVE_WINDOWS_H
smpd_sig_fn_t *smpd_signal( int signo, smpd_sig_fn_t func )
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset( &act.sa_mask );
    act.sa_flags = 0;
    if ( signo == SIGALRM )
    {
#ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;   /* SunOS 4.x */
#endif
    }
    else
    {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;     /* SVR4, 4.4BSD */
#endif
    }
    if ( sigaction( signo,&act, &oact ) < 0 )
        return ( SIG_ERR );
    return( oact.sa_handler );
}
#endif

#undef FCNAME
#define FCNAME "smpd_create_process_struct"
int smpd_create_process_struct(int rank, smpd_process_t **process_ptr)
{
    int result;
    smpd_process_t *p;
    static int cur_id = 0;

    smpd_enter_fn(FCNAME);

    p = (smpd_process_t*)MPIU_Malloc(sizeof(smpd_process_t));
    if (p == NULL)
    {
	*process_ptr = NULL;
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    p->id = cur_id++; /* MT - If smpd is to be thread safe, this will have to be changed */
    p->rank = rank;
    p->binding_proc = -1;
    p->nproc = 1;
    p->kvs_name[0] = '\0';
    p->domain_name[0] = '\0';
    p->exe[0] = '\0';
    p->env[0] = '\0';
    p->dir[0] = '\0';
    p->path[0] = '\0';
    p->clique[0] = '\0';
    p->err_msg[0] = '\0';
    p->stdin_write_list = NULL;
    result = smpd_create_context(SMPD_CONTEXT_STDIN, smpd_process.set, SMPDU_SOCK_INVALID_SOCK, -1, &p->in);
    if (result != SMPD_SUCCESS)
    {
	MPIU_Free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stdin context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_STDOUT, smpd_process.set, SMPDU_SOCK_INVALID_SOCK, -1, &p->out);
    if (result != SMPD_SUCCESS)
    {
	MPIU_Free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stdout context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_STDERR, smpd_process.set, SMPDU_SOCK_INVALID_SOCK, -1, &p->err);
    if (result != SMPD_SUCCESS)
    {
	MPIU_Free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stderr context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_PMI, smpd_process.set, SMPDU_SOCK_INVALID_SOCK, -1, &p->pmi);
    if (result != SMPD_SUCCESS)
    {
	MPIU_Free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create pmi context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    p->in->rank = rank;
    p->out->rank = rank;
    p->err->rank = rank;
    p->num_valid_contexts = 3;
    p->context_refcount = 0;
    p->exitcode = 0;
    p->in->process = p;
    p->out->process = p;
    p->err->process = p;
    p->pmi->process = p;
    p->next = NULL;
    p->spawned = 0;
    p->local_process = SMPD_TRUE;
    p->is_singleton_client = SMPD_FALSE;
    p->map_list = NULL;
    p->appnum = 0;

    *process_ptr = p;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_process_to_string"
SMPD_BOOL smpd_process_to_string(char **str_pptr, int *len_ptr, int indent, smpd_process_t *process)
{
    char indent_str[SMPD_MAX_TO_STRING_INDENT+1];

    smpd_enter_fn(FCNAME);

    if (*len_ptr < 1)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    if (indent > SMPD_MAX_TO_STRING_INDENT)
    {
	indent = SMPD_MAX_TO_STRING_INDENT;
    }

    memset(indent_str, ' ', indent);
    indent_str[indent] = '\0';

    smpd_snprintf_update(str_pptr, len_ptr, "%sid: %d\n", indent_str, process->id);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%srank: %d\n", indent_str, process->rank);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%sexe: %s\n", indent_str, process->exe);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%sdir: %s\n", indent_str, process->dir);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%senv: %s\n", indent_str, process->env);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%spath: %s\n", indent_str, process->path);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%spid: %d\n", indent_str, process->pid);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%sexitcode: %d\n", indent_str, process->exitcode);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%scontext_refcount: %s\n", indent_str, process->context_refcount);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%serr_msg: %s\n", indent_str, process->err_msg);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%snum_valid_contexts: %d\n", indent_str, process->num_valid_contexts);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%s in: %p\n", indent_str, process->in);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%s out: %p\n", indent_str, process->out);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%s err: %p\n", indent_str, process->err);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%s pmi: %p\n", indent_str, process->pmi);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%skvs_name: %s\n", indent_str, process->kvs_name);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%snproc: %d\n", indent_str, process->nproc);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
#ifdef HAVE_WINDOWS_H
    smpd_snprintf_update(str_pptr, len_ptr, "%swait: %p:%p\n", indent_str, process->wait.hProcess, process->wait.hThread);
#else
    smpd_snprintf_update(str_pptr, len_ptr, "%swait: %d\n", indent_str, (int)process->wait);
#endif
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; }
    smpd_snprintf_update(str_pptr, len_ptr, "%snext: %p\n", indent_str, process->next);
    if (*len_ptr < 1) { smpd_exit_fn(FCNAME); return SMPD_FALSE; } /* this misses the case of an exact fit */

    smpd_exit_fn(FCNAME);
    return SMPD_TRUE;
}

#undef FCNAME
#define FCNAME "smpd_free_process_struct"
int smpd_free_process_struct(smpd_process_t *process)
{
    smpd_enter_fn(FCNAME);
    if (process == NULL)
    {
	smpd_dbg_printf("smpd_free_process_struct passed NULL process pointer.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (process->in)
	smpd_free_context(process->in);
    process->in = NULL;
    if (process->out)
	smpd_free_context(process->out);
    process->out = NULL;
    if (process->err)
	smpd_free_context(process->err);
    process->err = NULL;
    if (process->pmi)
	smpd_free_context(process->pmi);
    process->pmi = NULL;
    process->dir[0] = '\0';
    process->env[0] = '\0';
    process->exe[0] = '\0';
    process->path[0] = '\0';
    process->pid = -1;
    process->rank = -1;
    process->next = NULL;
    MPIU_Free(process);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_interpret_session_header"
int smpd_interpret_session_header(char *str)
{
    char temp_str[100];

    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("interpreting session header: \"%s\"\n", str);

    /* get my id */
    if (MPIU_Str_get_string_arg(str, "id", temp_str, 100) == MPIU_STR_SUCCESS)
    {
	smpd_dbg_printf(" id = %s\n", temp_str);
	smpd_process.id = atoi(temp_str);
	if (smpd_process.id < 0)
	{
	    smpd_err_printf("invalid id passed in session header: %d\n", smpd_process.id);
	    smpd_process.id = 0;
	}
    }

    /* get my parent's id */
    if (MPIU_Str_get_string_arg(str, "parent", temp_str, 100) == MPIU_STR_SUCCESS)
    {
	smpd_dbg_printf(" parent = %s\n", temp_str);
	smpd_process.parent_id = atoi(temp_str);
	if (smpd_process.parent_id < 0)
	{
	    smpd_err_printf("invalid parent id passed in session header: %d\n", smpd_process.parent_id);
	    smpd_process.parent_id = -1;
	}
    }

    /* get my level */
    if (MPIU_Str_get_string_arg(str, "level", temp_str, 100) == MPIU_STR_SUCCESS)
    {
	smpd_dbg_printf(" level = %s\n", temp_str);
	smpd_process.level = atoi(temp_str);
	if (smpd_process.level < 0)
	{
	    smpd_err_printf("invalid session level passed in session header: %d\n", smpd_process.level);
	    smpd_process.level = 0;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
