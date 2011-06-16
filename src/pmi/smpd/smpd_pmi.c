/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ipmi.h"
#ifdef HAVE_MACH_O_DYLD_H
#include <mach-o/dyld.h>
#endif

#ifdef HAVE_WINDOWS_H

#define PMILoadLibrary(a,b) a = LoadLibrary( b )
#define PMIGetProcAddress GetProcAddress
#define PMIModule HMODULE

#elif defined(HAVE_DLOPEN)

#define PMILoadLibrary(a, b) a = dlopen( b , RTLD_LAZY /* or RTLD_NOW */)
#define PMIGetProcAddress dlsym
#define PMIModule void *

#elif defined(HAVE_NSLINKMODULE)

#define PMILoadLibrary(a, b) \
{ \
    NSObjectFileImage fileImage; \
    NSObjectFileImageReturnCode returnCode = \
	NSCreateObjectFileImageFromFile(b, &fileImage); \
    if (returnCode == NSObjectFileImageSuccess) \
    { \
	a = NSLinkModule(fileImage, b, \
			 NSLINKMODULE_OPTION_RETURN_ON_ERROR \
			 | NSLINKMODULE_OPTION_PRIVATE); \
	NSDestroyObjectFileImage(fileImage); \
    } else { \
	a = NULL; \
    } \
}
#define PMIGetProcAddress(a, b) NSAddressOfSymbol( NSLookupSymbolInModule( a, b ) )
#define PMIModule NSModule

#else
#error PMILoadLibrary functions needed
#endif

/* global variables */
static ipmi_functions_t fn =
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int PMI_Init(int *spawned)
{
    char *dll_name;
    PMIModule hModule;

    if (fn.PMI_Init != NULL)
    {
	/* Init cannot be called more than once? */
	return PMI_FAIL;
    }

    dll_name = getenv("PMI_DLL_NAME");
    if (dll_name)
    {
	PMILoadLibrary(hModule, dll_name);
	if (hModule != NULL)
	{
	    fn.PMI_Init = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Init");
	    if (fn.PMI_Init == NULL)
	    {
		return PMI_FAIL;
	    }
	    fn.PMI_Initialized = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Initialized");
	    fn.PMI_Finalize = (int (*)(void))PMIGetProcAddress(hModule, "PMI_Finalize");
	    fn.PMI_Get_size = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Get_size");
	    fn.PMI_Get_rank = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Get_rank");
	    fn.PMI_Get_universe_size = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Get_universe_size");
	    fn.PMI_Get_appnum = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Get_appnum");
	    fn.PMI_Get_id = (int (*)(char [], int))PMIGetProcAddress(hModule, "PMI_Get_id");
	    fn.PMI_Get_kvs_domain_id = (int (*)(char [], int))PMIGetProcAddress(hModule, "PMI_Get_kvs_domain_id");
	    fn.PMI_Get_id_length_max = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Get_id_length_max");
	    fn.PMI_Barrier = (int (*)(void))PMIGetProcAddress(hModule, "PMI_Barrier");
	    fn.PMI_Get_clique_size = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_Get_clique_size");
	    fn.PMI_Get_clique_ranks = (int (*)(int [], int))PMIGetProcAddress(hModule, "PMI_Get_clique_ranks");
	    fn.PMI_Abort = (int (*)(int, const char[]))PMIGetProcAddress(hModule, "PMI_Abort");
	    fn.PMI_KVS_Get_my_name = (int (*)(char [], int))PMIGetProcAddress(hModule, "PMI_KVS_Get_my_name");
	    fn.PMI_KVS_Get_name_length_max = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_KVS_Get_name_length_max");
	    fn.PMI_KVS_Get_key_length_max = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_KVS_Get_key_length_max");
	    fn.PMI_KVS_Get_value_length_max = (int (*)(int *))PMIGetProcAddress(hModule, "PMI_KVS_Get_value_length_max");
	    fn.PMI_KVS_Create = (int (*)(char [], int))PMIGetProcAddress(hModule, "PMI_KVS_Create");
	    fn.PMI_KVS_Destroy = (int (*)(const char []))PMIGetProcAddress(hModule, "PMI_KVS_Destroy");
	    fn.PMI_KVS_Put = (int (*)(const char [], const char [], const char []))PMIGetProcAddress(hModule, "PMI_KVS_Put");
	    fn.PMI_KVS_Commit = (int (*)(const char []))PMIGetProcAddress(hModule, "PMI_KVS_Commit");
	    fn.PMI_KVS_Get = (int (*)(const char [], const char [], char [], int))PMIGetProcAddress(hModule, "PMI_KVS_Get");
	    fn.PMI_KVS_Iter_first = (int (*)(const char [], char [], int, char [], int))PMIGetProcAddress(hModule, "PMI_KVS_Iter_first");
	    fn.PMI_KVS_Iter_next = (int (*)(const char [], char [], int, char [], int))PMIGetProcAddress(hModule, "PMI_KVS_Iter_next");
	    fn.PMI_Spawn_multiple = (int (*)(int, const char *[], const char **[], const int [], const int [], const PMI_keyval_t *[], int, const PMI_keyval_t [], int []))PMIGetProcAddress(hModule, "PMI_Spawn_multiple");
	    fn.PMI_Parse_option = (int (*)(int, char *[], int *, PMI_keyval_t **, int *))PMIGetProcAddress(hModule, "PMI_Parse_option");
	    fn.PMI_Args_to_keyval = (int (*)(int *, char *((*)[]), PMI_keyval_t **, int *))PMIGetProcAddress(hModule, "PMI_Args_to_keyval");
	    fn.PMI_Free_keyvals = (int (*)(PMI_keyval_t [], int))PMIGetProcAddress(hModule, "PMI_Free_keyvals");
	    fn.PMI_Publish_name = (int (*)(const char [], const char [] ))PMIGetProcAddress(hModule, "PMI_Publish_name");
	    fn.PMI_Unpublish_name = (int (*)( const char [] ))PMIGetProcAddress(hModule, "PMI_Unpublish_name");
	    fn.PMI_Lookup_name = (int (*)( const char [], char [] ))PMIGetProcAddress(hModule, "PMI_Lookup_name");
	    return fn.PMI_Init(spawned);
	}
    }

    fn.PMI_Init = iPMI_Init;
    fn.PMI_Initialized = iPMI_Initialized;
    fn.PMI_Finalize = iPMI_Finalize;
    fn.PMI_Get_size = iPMI_Get_size;
    fn.PMI_Get_rank = iPMI_Get_rank;
    fn.PMI_Get_universe_size = iPMI_Get_universe_size;
    fn.PMI_Get_appnum = iPMI_Get_appnum;
    fn.PMI_Get_id = iPMI_Get_id;
    fn.PMI_Get_kvs_domain_id = iPMI_Get_kvs_domain_id;
    fn.PMI_Get_id_length_max = iPMI_Get_id_length_max;
    fn.PMI_Barrier = iPMI_Barrier;
    fn.PMI_Get_clique_size = iPMI_Get_clique_size;
    fn.PMI_Get_clique_ranks = iPMI_Get_clique_ranks;
    fn.PMI_Abort = iPMI_Abort;
    fn.PMI_KVS_Get_my_name = iPMI_KVS_Get_my_name;
    fn.PMI_KVS_Get_name_length_max = iPMI_KVS_Get_name_length_max;
    fn.PMI_KVS_Get_key_length_max = iPMI_KVS_Get_key_length_max;
    fn.PMI_KVS_Get_value_length_max = iPMI_KVS_Get_value_length_max;
    fn.PMI_KVS_Create = iPMI_KVS_Create;
    fn.PMI_KVS_Destroy = iPMI_KVS_Destroy;
    fn.PMI_KVS_Put = iPMI_KVS_Put;
    fn.PMI_KVS_Commit = iPMI_KVS_Commit;
    fn.PMI_KVS_Get = iPMI_KVS_Get;
    fn.PMI_KVS_Iter_first = iPMI_KVS_Iter_first;
    fn.PMI_KVS_Iter_next = iPMI_KVS_Iter_next;
    fn.PMI_Spawn_multiple = iPMI_Spawn_multiple;
    fn.PMI_Parse_option = iPMI_Parse_option;
    fn.PMI_Args_to_keyval = iPMI_Args_to_keyval;
    fn.PMI_Free_keyvals = iPMI_Free_keyvals;
    fn.PMI_Publish_name = iPMI_Publish_name;
    fn.PMI_Unpublish_name = iPMI_Unpublish_name;
    fn.PMI_Lookup_name = iPMI_Lookup_name;

    return fn.PMI_Init(spawned);
}

int PMI_Finalize()
{
    if (fn.PMI_Finalize == NULL)
	return PMI_FAIL;
    return fn.PMI_Finalize();
}

int PMI_Get_size(int *size)
{
    if (fn.PMI_Get_size == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_size(size);
}

int PMI_Get_rank(int *rank)
{
    if (fn.PMI_Get_rank == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_rank(rank);
}

int PMI_Get_universe_size(int *size)
{
    if (fn.PMI_Get_universe_size == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_universe_size(size);
}

int PMI_Get_appnum(int *appnum)
{
    if (fn.PMI_Get_appnum == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_appnum(appnum);
}

int PMI_Barrier()
{
    if (fn.PMI_Barrier == NULL)
	return PMI_FAIL;
    return fn.PMI_Barrier();
}

int PMI_Abort(int exit_code, const char error_msg[])
{
    if (fn.PMI_Abort == NULL)
	return PMI_FAIL;
    return fn.PMI_Abort(exit_code, error_msg);
}

int PMI_KVS_Get_my_name(char kvsname[], int length)
{
    if (fn.PMI_KVS_Get_my_name == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_my_name(kvsname, length);
}

int PMI_KVS_Get_name_length_max(int *maxlen)
{
    if (fn.PMI_KVS_Get_name_length_max == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_name_length_max(maxlen);
}

int PMI_KVS_Get_key_length_max(int *maxlen)
{
    if (fn.PMI_KVS_Get_key_length_max == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_key_length_max(maxlen);
}

int PMI_KVS_Get_value_length_max(int *maxlen)
{
    if (fn.PMI_KVS_Get_value_length_max == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_value_length_max(maxlen);
}

int PMI_KVS_Put(const char kvsname[], const char key[], const char value[])
{
    if (fn.PMI_KVS_Put == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Put(kvsname, key, value);
}

int PMI_KVS_Commit(const char kvsname[])
{
    if (fn.PMI_KVS_Commit == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Commit(kvsname);
}

int PMI_KVS_Get(const char kvsname[], const char key[], char value[], int length)
{
    if (fn.PMI_KVS_Get == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get(kvsname, key, value, length);
}

int PMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizes[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[])
{
    if (fn.PMI_Spawn_multiple == NULL)
	return PMI_FAIL;
    return fn.PMI_Spawn_multiple(count, cmds, argvs, maxprocs,
	info_keyval_sizes, info_keyval_vectors,
	preput_keyval_size, preput_keyval_vector,
	errors);
}

int PMI_Publish_name( const char service_name[], const char port[] )
{
    if (fn.PMI_Publish_name == NULL)
	return PMI_FAIL;
    return fn.PMI_Publish_name(service_name, port);
}

int PMI_Unpublish_name( const char service_name[] )
{
    if (fn.PMI_Unpublish_name == NULL)
	return PMI_FAIL;
    return fn.PMI_Unpublish_name(service_name);
}

int PMI_Lookup_name( const char service_name[], char port[] )
{
    if (fn.PMI_Lookup_name == NULL)
	return PMI_FAIL;
    return fn.PMI_Lookup_name(service_name, port);
}
