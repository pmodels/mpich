/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Mpiexec:main()
{
  MPI_Init()
  foreach option in (cmd line options)
    parse standard mpiexec option - n, host, path, wdir, etc
    if !standard option
      MPID_Parse_option()
        parse device specific option and call PMI_Parse_option() if necessary
  next option
  MPI_Finalize()
    MPID_Finalize()
      PMI_Finalize() - block until output is redirected if singleton.
}
*/

#define MPIEXEC_MAX_DIR_LENGTH  1024
#define MPIEXEC_MAX_FILENAME    1024
#define MPIEXEC_MAX_EXE_LENGTH  1024
#define MPIEXEC_MAX_PATH_LENGTH 1024
#define MPIEXEC_MAX_HOST_LENGTH 100
typedef int MPIEXEC_BOOL;
#define MPIEXEC_FALSE 0
#define MPIEXEC_TRUE  1
#define MPIEXEC_SUCCESS 0
#define MPIEXEC_FAIL   -1

typedef struct cmd_struct
{
    char *cmd;
    char **argv;
    int maxprocs;
    MPI_Info info;
    struct cmd_struct *next;
} cmd_struct;

void print_options(void)
{
    printf("\n");
    printf("Usage:\n");
    printf("mpiexec -n <maxprocs> [options] executable [args ...]\n");
    printf("mpiexec [options] executable [args ...] : [options] exe [args] : ...\n");
    printf("mpiexec -configfile <configfile>\n");
    printf("\n");
    printf("options:\n");
    printf("\n");
    printf("-n <maxprocs>\n");
    printf("-wdir <working directory>\n");
    printf("-configfile <filename> -\n");
    printf("       each line contains a complete set of mpiexec options\n");
    printf("       including the executable and arguments\n");
    printf("-host <hostname>\n");
    printf("-soft <Fortran90 triple> - acceptable number of processes up to maxprocs\n");
    printf("       a or a:b or a:b:c where\n");
    printf("       1) a = a\n");
    printf("       2) a:b = a, a+1, a+2, ..., b\n");
    printf("       3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc\n");
    printf("          where a+kc <= b if c>0\n");
    printf("                a+kc >= b if c<0\n");
    printf("-path <search path for executable, ; separated>\n");
    printf("-arch <architecture> - sun, linux, rs6000, ...\n");
    printf("\n");
    printf("examples:\n");
    printf("mpiexec -n 4 cpi\n");
    printf("mpiexec -n 1 -host foo master : -n 8 slave\n");
    printf("\n");
}

static int strip_args(int *argcp, char **argvp[], int n)
{
    int i;

    if (n+1 > (*argcp))
    {
	printf("Error: cannot strip %d args, only %d left.\n", n, (*argcp)-1);
	return MPIEXEC_FAIL;
    }
    for (i=n+1; i<=(*argcp); i++)
    {
	(*argvp)[i-n] = (*argvp)[i];
    }
    (*argcp) -= n;
    return MPIEXEC_SUCCESS;
}

MPIEXEC_BOOL get_argcv_from_file(FILE *fin, int *argcp, char ***argvp)
{
    /* maximum of 8192 characters per line and 1023 args */
    static char line[8192];
    static char *argv[1024];
    char *token;
    int index;

    argv[0] = "bogus.exe";
    while (fgets(line, 8192, fin))
    {
	index = 1;
	token = strtok(line, " \r\n");
	while (token)
	{
	    argv[index] = token;
	    index++;
	    if (index == 1024)
	    {
		argv[1023] = NULL;
		break;
	    }
	    token = strtok(NULL, " \r\n");
	}
	if (index != 1)
	{
	    if (index < 1024)
		argv[index] = NULL;
	    *argcp = index;
	    *argvp = argv;
	    return MPIEXEC_TRUE;
	}
    }

    return MPIEXEC_FALSE;
}

int parse_arguments(int *argcp, char **argvp[],
		    int *count, char ***cmds, char****argvs, int **maxprocs, MPI_Info **infos)
{
    int argc, next_argc;
    char **next_argv;
    char *exe_ptr;
    int num_args_to_strip;
    int nproc;
    char wdir[MPIEXEC_MAX_DIR_LENGTH];
    int index, i;
    char configfilename[MPIEXEC_MAX_FILENAME];
    int use_configfile;
#if 0
    char exe[MPIEXEC_MAX_EXE_LENGTH], *exe_iter;
    char exe_path[MPIEXEC_MAX_EXE_LENGTH], *namepart;
    int exe_len_remaining;
#endif
    char path[MPIEXEC_MAX_PATH_LENGTH];
    FILE *fin_config;
    int result;
    char host[MPIEXEC_MAX_HOST_LENGTH];
    cmd_struct *cmd_node, *cmd_iter, *cmd_list = NULL;
    int error_length;
    char error_string[1024];
#ifdef HAVE_MPID_PARSE_OPTION
    int num_args_parsed;
    MPI_Info info;
#endif

    /* check for mpi options */
    /*
     * Required:
     * -n <maxprocs>
     * -host <hostname>
     * -soft <Fortran90 triple> - represents allowed number of processes up to maxprocs
     *        a or a:b or a:b:c where
     *        1) a = a
     *        2) a:b = a, a+1, a+2, ..., b
     *        3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc
     *           where a+kc <= b if c>0
     *                 a+kc >= b if c<0
     * -wdir <working directory>
     * -path <search path for executable>
     * -arch <architecture> - sun, linux, rs6000, ...
     * -configfile <filename> - each line contains a complete set of mpiexec options, #commented
     *
     * Backwards compatibility
     * -np <numprocs>
     * -dir <working directory>
     */

    *count = 0;
    next_argc = *argcp;
    next_argv = *argvp + 1;
    exe_ptr = **argvp;
    do
    {
	/* calculate the current argc and find the next argv */
	argc = 1;
	while ( (*next_argv) != NULL && (**next_argv) != ':')
	{
	    argc++;
	    next_argc--;
	    next_argv++;
	}
	if ( (*next_argv) != NULL && (**next_argv) == ':')
	{
	    (*next_argv) = NULL;
	    next_argv++;
	}
	argcp = &argc;

	/* reset block global variables */
	use_configfile = MPIEXEC_FALSE;
configfile_loop:
	nproc = 0;
	wdir[0] = '\0';
	path[0] = '\0';
	host[0] = '\0';

	/* Check for the -configfile option.  It must be the first and only option in a group. */
	if ((*argvp)[1] && (*argvp)[1][0] == '-')
	{
	    if ((*argvp)[1][1] == '-')
	    {
		/* double -- option provided, trim it to a single - */
		index = 2;
		while ((*argvp)[1][index] != '\0')
		{
		    (*argvp)[1][index-1] = (*argvp)[1][index];
		    index++;
		}
		(*argvp)[1][index-1] = '\0';
	    }
	    if (strcmp(&(*argvp)[1][1], "configfile") == 0)
	    {
		if (use_configfile)
		{
		    printf("Error: -configfile option is not valid from within a configuration file.\n");
		    return MPIEXEC_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -configfile option.\n");
		    return MPIEXEC_FAIL;
		}
		strncpy(configfilename, (*argvp)[2], MPIEXEC_MAX_FILENAME);
		use_configfile = MPIEXEC_TRUE;
		fin_config = fopen(configfilename, "r");
		if (fin_config == NULL)
		{
		    printf("Error: unable to open config file '%s'\n", configfilename);
		    return MPIEXEC_FAIL;
		}
		if (!get_argcv_from_file(fin_config, argcp, argvp))
		{
		    fclose(fin_config);
		    printf("Error: unable to parse config file '%s'\n", configfilename);
		    return MPIEXEC_FAIL;
		}
	    }
	}

	/* parse the current block */

	/* parse the mpiexec options */
	while ((*argvp)[1] && (*argvp)[1][0] == '-')
	{
	    if ((*argvp)[1][1] == '-')
	    {
		/* double -- option provided, trim it to a single - */
		index = 2;
		while ((*argvp)[1][index] != '\0')
		{
		    (*argvp)[1][index-1] = (*argvp)[1][index];
		    index++;
		}
		(*argvp)[1][index-1] = '\0';
	    }

	    num_args_to_strip = 1;
	    if ((strcmp(&(*argvp)[1][1], "np") == 0) || (strcmp(&(*argvp)[1][1], "n") == 0))
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    return MPIEXEC_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no number specified after %s option.\n", (*argvp)[1]);
		    return MPIEXEC_FAIL;
		}
		nproc = atoi((*argvp)[2]);
		if (nproc < 1)
		{
		    printf("Error: must specify a number greater than 0 after the %s option\n", (*argvp)[1]);
		    return MPIEXEC_FAIL;
		}
		num_args_to_strip = 2;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "dir") == 0) || (strcmp(&(*argvp)[1][1], "wdir") == 0) )
	    {
		if (argc < 3)
		{
		    printf("Error: no directory after %s option\n", (*argvp)[1]);
		    return MPIEXEC_FAIL;
		}
		strncpy(wdir, (*argvp)[2], MPIEXEC_MAX_DIR_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "configfile") == 0)
	    {
		printf("Error: The -configfile option must be the first and only option specified in a block.\n");
		return MPIEXEC_FAIL;
	    }
	    else if (strcmp(&(*argvp)[1][1], "host") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no host specified after -host option.\n");
		    return MPIEXEC_FAIL;
		}
		if (host[0] != '\0')
		{
		    printf("Error: -host option can only be used once.\n");
		    return MPIEXEC_FAIL;
		}
		strncpy(host, (*argvp)[2], MPIEXEC_MAX_HOST_LENGTH);
		num_args_to_strip = 2;
	    }
	    /* catch -help and -? */
	    else if (strcmp(&(*argvp)[1][1], "help") == 0 || (*argvp)[1][1] == '?')
	    {
		print_options();
		exit(0);
	    }
	    else if (strcmp(&(*argvp)[1][1], "path") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no path specifed after -path option.\n");
		    return MPIEXEC_FAIL;
		}
		strncpy(path, (*argvp)[2], MPIEXEC_MAX_PATH_LENGTH);
		num_args_to_strip = 2;
	    }
	    else
	    {
#ifdef HAVE_MPID_PARSE_OPTION
		result = MPID_Parse_option(*argcp - 1, &(*argvp)[1], &num_args_parsed, &info);
		if (result != MPI_SUCCESS)
		{
		    printf("Error: MPID_Parse_arg failed with error code %d\n", result);
		    return MPIEXEC_FAIL;
		}
		if (num_args_parsed == 0)
		    printf("Unknown option: %s\n", (*argvp)[1]);
		else
		    num_args_to_strip = num_args_parsed;
#else
		printf("Unknown option: %s\n", (*argvp)[1]);
#endif
	    }
	    strip_args(argcp, argvp, num_args_to_strip);
	}

	cmd_node = (cmd_struct*)malloc(sizeof(cmd_struct));
	if (cmd_node == NULL)
	{
	    printf("Error: unable to allocate a command node, malloc failed.\n");
	    return MPIEXEC_FAIL;
	}
	MPI_Info_create(&cmd_node->info);
	cmd_node->next = NULL;
	if (nproc == 0)
	{
	    printf("missing num_proc flag: -n or -np.\n");
	    return MPIEXEC_FAIL;
	}
	cmd_node->maxprocs = nproc;
	if (wdir[0] != '\0')
	{
	    result = MPI_Info_set(cmd_node->info, "wdir", wdir);
	    if (result != MPI_SUCCESS)
	    {
		error_length = 1024;
		MPI_Error_string(result, error_string, &error_length);
		printf("Error: unable to add wdir '%s' to the info, %s\n", wdir, error_string);
	    }
	}
	if (path[0] != '\0')
	{
	    MPI_Info_set(cmd_node->info, "path", path);
	}
	if (host[0] != '\0')
	{
	    MPI_Info_set(cmd_node->info, "host", host);
	}

	/* MPID_Args_to_info */

	/* remaining args are the executable and it's args */
	if (argc < 2)
	{
	    printf("Error: no executable specified\n");
	    return MPIEXEC_FAIL;
	}

	cmd_node->cmd = strdup((*argvp)[1]);

	if (argc < 3)
	{
	    cmd_node->argv = NULL;
	}
	else
	{
	    cmd_node->argv = (char**)malloc((argc - 1) * sizeof(char*));
	    for (i=2; i<argc; i++)
	    {
		cmd_node->argv[i-2] = strdup((*argvp)[i]);
	    }
	    cmd_node->argv[argc-2] = NULL;
	}

	if (cmd_list == NULL)
	{
	    cmd_list = cmd_node;
	}
	else
	{
	    cmd_iter->next = cmd_node;
	}
	cmd_iter = cmd_node;

#if 0
	exe_iter = exe;
	exe_len_remaining = MPIEXEC_MAX_EXE_LENGTH;
	if (!((*argvp)[1][0] == '\\' && (*argvp)[1][1] == '\\') && (*argvp)[1][0] != '/' &&
	    !(strlen((*argvp)[1]) > 3 && (*argvp)[1][1] == ':' && (*argvp)[1][2] == '\\') )
	{
	    /* an absolute path was not specified so find the executable an save the path */
	    if (get_full_path_name((*argvp)[1], MPIEXEC_MAX_EXE_LENGTH, exe_path, &namepart))
	    {
		if (path[0] != '\0')
		{
		    if (strlen(path) < MPIEXEC_MAX_PATH_LENGTH)
		    {
			strcat(path, ";");
			strncat(path, exe_path, MPIEXEC_MAX_PATH_LENGTH - strlen(path));
			path[MPIEXEC_MAX_PATH_LENGTH-1] = '\0';
		    }
		}
		else
		{
		    strncpy(path, exe_path, MPIEXEC_MAX_PATH_LENGTH);
		}
		result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, namepart);
		if (result != MPIU_STR_SUCCESS)
		{
		    printf("Error: insufficient buffer space for the command line.\n");
		    return MPIEXEC_FAIL;
		}
	    }
	    else
	    {
		result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[1]);
		if (result != MPIU_STR_SUCCESS)
		{
		    printf("Error: insufficient buffer space for the command line.\n");
		    return MPIEXEC_FAIL;
		}
	    }
	}
	else
	{
	    /* an absolute path was specified */
	    result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[1]);
	}
#endif

	(*count)++;

	if (use_configfile)
	{
	    if (get_argcv_from_file(fin_config, argcp, argvp))
		goto configfile_loop;
	    fclose(fin_config);
	}

	/* move to the next block */
	*argvp = next_argv - 1;
	if (*next_argv)
	    **argvp = exe_ptr;
    } while (*next_argv);

    /* create return arguments */
    if (*count == 0)
    {
	printf("Error: no application specified.\n");
	return MPIEXEC_FAIL;
    }
    *maxprocs = (int*)malloc(*count * sizeof(int));
    *cmds = (char**)malloc(*count * sizeof(char*));
    *argvs = (char***)malloc(*count * sizeof(char**));
    *infos = (MPI_Info*)malloc(*count * sizeof(MPI_Info));
    i = 0;
    cmd_iter = cmd_list;
    while (cmd_iter)
    {
	(*maxprocs)[i] = cmd_iter->maxprocs;
	(*cmds)[i] = cmd_iter->cmd;
	(*argvs)[i] = cmd_iter->argv;
	(*infos)[i] = cmd_iter->info;
	i++;
	cmd_iter = cmd_iter->next;
    }

    /* free local resources */
    while (cmd_list)
    {
	cmd_iter = cmd_list;
	cmd_list = cmd_list->next;
	free(cmd_iter);
    }

    return MPIEXEC_SUCCESS;
}
