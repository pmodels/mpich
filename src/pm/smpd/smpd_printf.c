/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#include <stdio.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#define SMPD_MAX_INDENT 20
static char indent[SMPD_MAX_INDENT+1] = "";
static int cur_indent = 0;

#ifdef HAVE_WINDOWS_H
void smpd_translate_win_error(int error, char *msg, int maxlen, char *prepend, ...)
{
    HLOCAL str;
    int num_bytes;
    int len;
    va_list list;

    num_bytes = FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_ALLOCATE_BUFFER,
	0,
	error,
	MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
	(LPTSTR) &str,
	0,0);
    if (prepend == NULL)
	memcpy(msg, str, min(maxlen, num_bytes+1));
    else
    {
	va_start(list, prepend);
	len = vsnprintf(msg, maxlen, prepend, list);
	va_end(list);
	msg += len;
	maxlen -= len;
	snprintf(msg, maxlen, "%s", str);
    }
    LocalFree(str);
}
#endif

char * get_sock_error_string(int error)
{
    static char str[1024];

    if (error == SMPD_SUCCESS){
	    return "operation completed successfully";
    }
    else{
        /* FIXME : For now ... translate correctly later */
        MPIU_Snprintf(str, 1024, "Error = %d\n", error);
        return str;
    }

    /* str[0] = '\0'; */
    /*
    SMPDR_Err_get_string(error, str, 1024, SMPDU_Sock_get_error_class_string);
    */
    /* smpd_translate_win_error(error, str, 1024, "(error =", error, ") "); */
    /*
    SMPDR_Err_get_string_ext(error, str, 1024, SMPDU_Sock_get_error_class_string);
    if (SMPDR_Err_print_stack_flag)
    {
	char *str2;
	int len;
	strcat(str, "\n");
	len = (int)strlen(str);
	str2 = str + len;
	SMPDR_Err_print_stack_string_ext(error, str2, 1024-len, SMPDU_Sock_get_error_class_string);
    }
    */

    /* return str; */
}

char * smpd_get_context_str(smpd_context_t *context)
{
    if (context == NULL)
	return "null";
    switch (context->type)
    {
    case SMPD_CONTEXT_INVALID:
	return "invalid";
    case SMPD_CONTEXT_STDIN:
	return "stdin";
    case SMPD_CONTEXT_MPIEXEC_STDIN:
	return "mpi_stdin";
    case SMPD_CONTEXT_MPIEXEC_STDIN_RSH:
	return "mpi_stdin_rsh";
    case SMPD_CONTEXT_STDOUT:
	return "stdout";
    case SMPD_CONTEXT_STDOUT_RSH:
	return "stdout_rsh";
    case SMPD_CONTEXT_STDERR:
	return "stderr";
    case SMPD_CONTEXT_STDERR_RSH:
	return "stderr_rsh";
    case SMPD_CONTEXT_PARENT:
	return "parent";
    case SMPD_CONTEXT_LEFT_CHILD:
	return "left";
    case SMPD_CONTEXT_RIGHT_CHILD:
	return "right";
    case SMPD_CONTEXT_CHILD:
	return "child";
    case SMPD_CONTEXT_LISTENER:
	return "listener";
    case SMPD_CONTEXT_PMI_LISTENER:
    return "PMI_LISTENER";
    case SMPD_CONTEXT_SMPD:
	return "smpd";
    case SMPD_CONTEXT_PMI:
	return "pmi";
    case SMPD_CONTEXT_TIMEOUT:
	return "timeout";
    case SMPD_CONTEXT_MPIEXEC_ABORT:
	return "MPIEXEC_ABORT";
    case SMPD_CONTEXT_SINGLETON_INIT_CLIENT:
    return "SINGLETON_INIT_CLIENT";
    case SMPD_CONTEXT_SINGLETON_INIT_MPIEXEC:
    return "SINGLETON_INIT_MPIEXEC";
    case SMPD_CONTEXT_UNDETERMINED:
	return "undetermined";
    case SMPD_CONTEXT_FREED:
	return "freed";
    }
    return "unknown";
}

int smpd_init_printf(void)
{
    char * envstr;
    static int initialized = 0;
    char str[1024];
    int len;

    if (initialized)
	return 0;
    initialized = 1;

    smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT;

    envstr = getenv("SMPD_DBG_OUTPUT");
    if (envstr != NULL)
    {
	if (strstr(envstr, "stdout"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_STDOUT;
	if (strstr(envstr, "log"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_LOGFILE;
	if (strstr(envstr, "rank"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_PREPEND_RANK;
	if (strstr(envstr, "trace"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_TRACE;
	if (strstr(envstr, "all"))
	    smpd_process.dbg_state = SMPD_DBG_STATE_ALL;
    }

    if (smpd_option_on("log"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_LOGFILE;
    if (smpd_option_on("prepend_rank"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_PREPEND_RANK;
    if (smpd_option_on("trace"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_TRACE;

    if (smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE)
    {
	envstr = getenv("SMPD_DBG_LOG_FILENAME");
	if (envstr)
	{
	    strcpy(smpd_process.dbg_filename, envstr);
	}
	else
	{
	    if (smpd_get_smpd_data("logfile", smpd_process.dbg_filename, SMPD_MAX_FILENAME) != SMPD_SUCCESS)
	    {
		smpd_process.dbg_state ^= SMPD_DBG_STATE_LOGFILE;
	    }
	}
	if (smpd_get_smpd_data("max_logfile_size", str, 1024) == SMPD_SUCCESS)
	{
	    smpd_process.dbg_file_size = atoi(str);
	}
	envstr = getenv("SMPD_MAX_LOG_FILE_SIZE");
	if (envstr)
	{
	    len = atoi(envstr);
	    if (len > 0)
	    {
		smpd_process.dbg_file_size = atoi(envstr);
	    }
	}
    }

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, SMPD_OUTPUT_MUTEX_NAME);
	smpd_process.bOutputInitialized = TRUE;
    }
#endif
    return SMPD_SUCCESS;
}

int smpd_finalize_printf()
{
    fflush(stdout);
    fflush(stderr);
    return SMPD_SUCCESS;
}

void smpd_clean_output(char *str)
{
    char *temp_str;

    temp_str = strstr(str, "password");
    while (temp_str != NULL)
    {
	smpd_hide_string_arg(temp_str, "password");
	temp_str = strstr(temp_str+1, "password");
    }

    temp_str = strstr(str, "pwd");
    while (temp_str != NULL)
    {
	smpd_hide_string_arg(temp_str, "pwd");
	temp_str = strstr(temp_str+1, "pwd");
    }

    temp_str = strstr(str, "phrase");
    while (temp_str != NULL)
    {
	smpd_hide_string_arg(temp_str, "phrase");
	temp_str = strstr(temp_str+1, "phrase");
    }
}

static int read_file(FILE *fin, void *buffer, int length)
{
    int num_read, total = 0;
    char *buf = buffer;

    while (length > 0)
    {
	num_read = (int)fread(buf, sizeof(char), (size_t)length, fin);
	if (num_read < 1)
	{
	    return total;
	}
	length = length - num_read;
	buf = buf + num_read;
	total = total + num_read;
    }
    return total;
}

static int write_file(FILE *fout, void *buffer, int length)
{
    int num_written, total = 0;
    char *buf = buffer;

    while (length > 0)
    {
	num_written = (int)fwrite(buf, sizeof(char), (size_t)length, fout);
	if (num_written < 1)
	{
	    return total;
	}
	length = length - num_written;
	buf = buf + num_written;
	total = total + num_written;
    }
    return total;
}

void smpd_trim_logfile()
{
    /* Do nothing until I can figure out why the trim code isn't working */
}

#if 0
void smpd_trim_logfile_new()
{
    static int count = 0;
    FILE *fout;
    long size;
    int num_read, num_written;
    char *buffer, *buffer_orig;
    char copy_cmd[4096];
    int docopy = 0;
    static copy_number = 0;
    int a, b;

    /* check every 1000 iterations */
    if (count++ % 1000)
	return;

    fout = fopen(smpd_process.dbg_filename, "rb");
    if (fout == NULL)
    {
	/*printf("unable to open the smpd log file\n");*/
	return;
    }
    setvbuf(fout, NULL, _IONBF, 0);
    fseek(fout, 0, SEEK_END);
    size = ftell(fout);
    if (size > smpd_process.dbg_file_size)
    {
	if (copy_number == 0)
	{
	    srand(smpd_getpid());
	    copy_number = rand();
	}
	sprintf(copy_cmd, "copy %s %s.%d.old", smpd_process.dbg_filename, smpd_process.dbg_filename, copy_number);
	system(copy_cmd);
	buffer = buffer_orig = MPIU_Malloc(size);
	if (buffer != NULL)
	{
	    fseek(fout, 0, SEEK_SET);
	    num_read = read_file(fout, buffer, size);
	    if (num_read == size)
	    {
		fclose(fout);
		fout = fopen(smpd_process.dbg_filename, "rb");
		if (fout == NULL)
		{
		}
		setvbuf(fout, NULL, _IONBF, 0);
		fseek(fout, -4, SEEK_END);
		fread(&a, 1, 4, fout);
		fclose(fout);
		if (a != *(int*)&buffer[size-4])
		{
		    FILE *ferr = fopen("c:\\temp\\smpd.err", "a+");
		    fprintf(ferr, "last 4 bytes not equal, a: %x != %x", a, *(int*)&buffer[size-4]);
		    fclose(ferr);
		}
		fout = fopen(smpd_process.dbg_filename, "wb");
		if (fout != NULL)
		{
		    buffer = buffer + size - (smpd_process.dbg_file_size / 2);
		    size = smpd_process.dbg_file_size / 2;
		    num_written = write_file(fout, buffer, size);
		    docopy = 1;
		    if (num_written != size)
		    {
			FILE *ferr = fopen("c:\\temp\\smpd.err", "a+");
			fprintf(ferr, "wrote %d instead of %d bytes to the smpd log file.\n", num_written, size);
			fclose(ferr);
		    }
		    /*
		    else
		    {
			printf("wrote %d bytes to the smpd log file.\n", num_written);
			fflush(stdout);
		    }
		    */
		    fclose(fout);
		    fout = fopen(smpd_process.dbg_filename, "rb");
		    if (fout == NULL)
		    {
		    }
		    setvbuf(fout, NULL, _IONBF, 0);
		    fseek(fout, -4, SEEK_END);
		    fread(&b, 1, 4, fout);
		    fclose(fout);
		    if (b != *(int*)&buffer[size-4])
		    {
			FILE *ferr = fopen("c:\\temp\\smpd.err", "a+");
			fprintf(ferr, "last 4 bytes not equal, b: %x != %x", b, *(int*)&buffer[size-4]);
			fclose(ferr);
		    }
		}
		/*
		else
		{
		    printf("unable to truncate the smpd log file.\n");
		    fflush(stdout);
		}
		*/
	    }
	    /*
	    else
	    {
		printf("read %d instead of %d bytes from the smpd log file.\n", num_read, size);
		fflush(stdout);
	    }
	    */
	    MPIU_Free(buffer_orig);
	}
	/*
	else
	{
	    printf("malloc failed to allocate %d bytes.\n", smpd_process.dbg_file_size / 2);
	    fflush(stdout);
	}
	*/
    }
    /*
    else
    {
	printf("smpd file size: %d\n", size);
	fflush(stdout);
    }
    */
    fclose(fout);
    if (docopy)
    {
	sprintf(copy_cmd, "copy %s %s.%d.new", smpd_process.dbg_filename, smpd_process.dbg_filename, copy_number);
	system(copy_cmd);
	copy_number++;
    }
}

void smpd_trim_logfile_old()
{
    static int count = 0;
    FILE *fout;
    long size;
    int num_read, num_written;
    int new_size;
    char *buffer;
    char copy_cmd[4096];
    int docopy = 0;
    static copy_number = 0;
    int a, b;

    /* check every 1000 iterations */
    if (count++ % 1000)
	return;

    fout = fopen(smpd_process.dbg_filename, "rb");
    if (fout == NULL)
    {
	/*printf("unable to open the smpd log file\n");*/
	return;
    }
    fseek(fout, 0, SEEK_END);
    size = ftell(fout);
    if (size > smpd_process.dbg_file_size)
    {
	if (copy_number == 0)
	{
	    srand(smpd_getpid());
	    copy_number = rand();
	}
	sprintf(copy_cmd, "copy %s %s.%d.old", smpd_process.dbg_filename, smpd_process.dbg_filename, copy_number);
	system(copy_cmd);
	new_size = smpd_process.dbg_file_size / 2;
	buffer = MPIU_Malloc(new_size);
	if (buffer != NULL)
	{
	    fseek(fout, - (long)new_size, SEEK_END);
	    num_read = read_file(fout, buffer, new_size);
	    if (num_read == new_size)
	    {
		fclose(fout);
		fout = fopen(smpd_process.dbg_filename, "rb");
		fseek(fout, -4, SEEK_END);
		fread(&a, 1, 4, fout);
		fclose(fout);
		if (a != *(int*)&buffer[new_size-4])
		{
		    FILE *ferr = fopen("c:\\temp\\smpd.err", "a+");
		    fprintf(ferr, "last 4 bytes not equal, a: %x != %x", a, *(int*)&buffer[new_size-4]);
		    fclose(ferr);
		}
		fout = fopen(smpd_process.dbg_filename, "wb");
		if (fout != NULL)
		{
		    num_written = write_file(fout, buffer, new_size);
		    docopy = 1;
		    if (num_written != new_size)
		    {
			FILE *ferr = fopen("c:\\temp\\smpd.err", "a+");
			fprintf(ferr, "wrote %d instead of %d bytes to the smpd log file.\n", num_written, new_size);
			fclose(ferr);
		    }
		    /*
		    else
		    {
			printf("wrote %d bytes to the smpd log file.\n", num_written);
			fflush(stdout);
		    }
		    */
		    fclose(fout);
		    fout = fopen(smpd_process.dbg_filename, "rb");
		    fseek(fout, -4, SEEK_END);
		    fread(&b, 1, 4, fout);
		    fclose(fout);
		    if (b != *(int*)&buffer[new_size-4])
		    {
			FILE *ferr = fopen("c:\\temp\\smpd.err", "a+");
			fprintf(ferr, "last 4 bytes not equal, b: %x != %x", b, *(int*)&buffer[new_size-4]);
			fclose(ferr);
		    }
		}
		/*
		else
		{
		    printf("unable to truncate the smpd log file.\n");
		    fflush(stdout);
		}
		*/
	    }
	    /*
	    else
	    {
		printf("read %d instead of %d bytes from the smpd log file.\n", num_read, new_size);
		fflush(stdout);
	    }
	    */
	    MPIU_Free(buffer);
	}
	/*
	else
	{
	    printf("malloc failed to allocate %d bytes.\n", smpd_process.dbg_file_size / 2);
	    fflush(stdout);
	}
	*/
    }
    /*
    else
    {
	printf("smpd file size: %d\n", size);
	fflush(stdout);
    }
    */
    fclose(fout);
    if (docopy)
    {
	sprintf(copy_cmd, "copy %s %s.%d.new", smpd_process.dbg_filename, smpd_process.dbg_filename, copy_number);
	system(copy_cmd);
	copy_number++;
    }
}
#endif

#ifdef HAVE_WINDOWS_H
int smpd_tprintf_templ(smpd_printf_fp_t fp, TCHAR *str, ...){
    char *str_mb = NULL;
    int len = 0;
    int result = 0;
    va_list args;

    len = _tcslen(str) * sizeof(TCHAR) + 1;
    str_mb = (char *)MPIU_Malloc(len);
    if(str_mb == NULL){
        smpd_err_printf("Unable to allocate memory for temp buffer\n");
        return 0;
    }
    SMPDU_TCSTOMBS(str_mb, str, len);

    va_start(args, str);
    fp(str_mb, args);
    va_end(args);

    MPIU_Free(str_mb);

    return result;
}
#endif 

int smpd_err_printf(char *str, ...)
{
    va_list list;
    char *indent_str;
    char *cur_str;
    int num_bytes;

    if (smpd_process.id == -1)
	smpd_init_printf();

    if (!(smpd_process.dbg_state & (SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_LOGFILE)))
	return 0;

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, SMPD_OUTPUT_MUTEX_NAME);
	smpd_process.bOutputInitialized = TRUE;
    }
    WaitForSingleObject(smpd_process.hOutputMutex, INFINITE);
#endif

    /* write the formatted string to a global buffer */

    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
	indent_str = indent;
    else
	indent_str = "";

    num_bytes = 0;
    if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
    {
	/* prepend output with the process tree node id */
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "[%02d:%d]%sERROR:", smpd_process.id, smpd_getpid(), indent_str);
    }
    else
    {
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "%s", indent_str);
    }
    cur_str = &smpd_process.printf_buffer[num_bytes];

    va_start(list, str);
    num_bytes += vsnprintf(cur_str, SMPD_MAX_DBG_PRINTF_LENGTH - num_bytes, str, list);
    va_end(list);

    /* strip protected fields - passwords, etc */
    smpd_clean_output(smpd_process.printf_buffer);

    if (smpd_process.dbg_state & SMPD_DBG_STATE_ERROUT)
    {
	/* use stdout instead of stderr so that ordering will be consistent with dbg messages */
	printf("%s", smpd_process.printf_buffer);
	fflush(stdout);
    }
    if ((smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE) && (smpd_process.dbg_filename[0] != '\0'))
    {
	FILE *fout;
	smpd_trim_logfile();
	fout = fopen(smpd_process.dbg_filename, "a+");
	if (fout == NULL)
	{
	    smpd_process.dbg_state ^= SMPD_DBG_STATE_LOGFILE;
	}
	else
	{
	    setvbuf(fout, NULL, _IONBF, 0);
	    fprintf(fout, "%s", smpd_process.printf_buffer);
	    fclose(fout);
	}
    }
    

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(smpd_process.hOutputMutex);
#endif
    return num_bytes;
}

int smpd_dbg_printf(char *str, ...)
{
    va_list list;
    char *indent_str;
    char *cur_str;
    int num_bytes;

    if (smpd_process.id == -1)
	smpd_init_printf();

    if (!(smpd_process.dbg_state & (SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_LOGFILE)))
	return 0;

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, SMPD_OUTPUT_MUTEX_NAME);
	smpd_process.bOutputInitialized = TRUE;
    }
    WaitForSingleObject(smpd_process.hOutputMutex, INFINITE);
#endif

    /* write the formatted string to a global buffer */

    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
	indent_str = indent;
    else
	indent_str = "";

    num_bytes = 0;
    if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
    {
	/* prepend output with the process tree node id */
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "[%02d:%d]%s", smpd_process.id, smpd_getpid(), indent_str);
    }
    else
    {
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "%s", indent_str);
    }
    cur_str = &smpd_process.printf_buffer[num_bytes];

    va_start(list, str);
    num_bytes += vsnprintf(cur_str, SMPD_MAX_DBG_PRINTF_LENGTH - num_bytes, str, list);
    va_end(list);

    /* strip protected fields - passwords, etc */
    smpd_clean_output(smpd_process.printf_buffer);

    if (smpd_process.dbg_state & SMPD_DBG_STATE_STDOUT)
    {
	printf("%s", smpd_process.printf_buffer);
	fflush(stdout);
    }
    if ((smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE) && (smpd_process.dbg_filename[0] != '\0'))
    {
	FILE *fout = NULL;
	smpd_trim_logfile();
	fout = fopen(smpd_process.dbg_filename, "a+");
	if (fout == NULL)
	{
	    /*smpd_process.dbg_state ^= SMPD_DBG_STATE_LOGFILE;*/
	}
	else
	{
	    setvbuf(fout, NULL, _IONBF, 0);
	    fprintf(fout, "%s", smpd_process.printf_buffer);
	    fclose(fout);
	}
    }
    

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(smpd_process.hOutputMutex);
#endif
    return num_bytes;
}

int smpd_enter_fn(char *fcname)
{
    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
    {
	smpd_dbg_printf("\\%s\n", fcname);
    }
    if (cur_indent >= 0 && cur_indent < SMPD_MAX_INDENT)
    {
	indent[cur_indent] = '.';
	indent[cur_indent+1] = '\0';
    }
    cur_indent++;
    return SMPD_SUCCESS;
}

int smpd_exit_fn(char *fcname)
{
    if (cur_indent > 0 && cur_indent < SMPD_MAX_INDENT)
    {
	indent[cur_indent-1] = '\0';
    }
    cur_indent--;
    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
    {
	smpd_dbg_printf("/%s\n", fcname);
    }
    return SMPD_SUCCESS;
}

SMPD_BOOL smpd_snprintf_update(char **str_pptr, int *len_ptr, char *str_format, ...)
{
    va_list list;
    int n;

    va_start(list, str_format);
    n = vsnprintf(*str_pptr, *len_ptr, str_format, list);
    va_end(list);

    if (n < 0)
    {
	(*str_pptr)[(*len_ptr)-1] = '\0';
	*len_ptr = 0;
	return SMPD_FALSE;
    }

    (*str_pptr )= &(*str_pptr)[n];
    *len_ptr = (*len_ptr) - n;

    return SMPD_TRUE;
}
