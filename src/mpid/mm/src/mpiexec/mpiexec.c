/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpi.h"
#include "pmi.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "mpichinfo.h"

#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

void PrintOptions()
{
    fprintf( stderr, "Usage: mpiexec -n <numprocs> -soft <softness> -host <hostname> \\\n" );
    fprintf( stderr, "               -wdir <working directory> -path <search path> \\\n" );
    fprintf( stderr, "               -file <filename> execname <args>\n" );
}

static void StripArgs(int *argc, char ***argv, int n)
{
    int i;
    if (n+1 > *argc)
    {
	printf("Error: cannot strip %d args, only %d left.\n", n, *argc-1);
    }
    for (i=n+1; i<=*argc; i++)
    {
	(*argv)[i-n] = (*argv)[i];
    }
    *argc -= n;
}

int CreateParameters(int *argcp, char **argvp[], int *pCount, char ***pCmds, char ****pArgvs, int **pMaxprocs, void **pInfos, int **pErrors)
{
    int i;
    int nArgsToStrip;
    int argc;
    char **argv;
    int nProc = 0;
    char pszDir[MAX_PATH] = ".";
    char pszExe[MAX_PATH] = "";
    char pszArgs[MAX_PATH] = "";
    char pszSoft[100] = "";
    char pszHost[100] = "";
    char pszArch[100] = "";
    char pszPath[MAX_PATH] = ".";
    char pszFile[MAX_PATH] = "";

    MPICH_Info info;
    MPICH_Info_create(&info);
    PMI_Args_to_info(argcp, argvp, info);

    argc = *argcp;
    argv = *argvp;
    while (argv[1] && (argv[1][0] == '-' || argv[1][0] == '/'))
    {
	nArgsToStrip = 1;
	if ((strncmp(&argv[1][1], "n", 2) == 0) || (strncmp(&argv[1][1], "np", 3) == 0))
	{
	    if (argc < 3)
	    {
		printf("Error: no number specified after -n option.\n");
		return 0;
	    }
	    nProc = atoi(argv[2]);
	    if (nProc < 1)
	    {
		printf("Error: must specify a number greater than 0 after the -n option\n");
		return 0;
	    }
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "wdir", 5) == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no directory after -wdir option\n");
		return 0;
	    }
	    strncpy(pszDir, argv[2], MAX_PATH);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "soft", 5) == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: nothing after -soft option\n");
		return 0;
	    }
	    strncpy(pszSoft, argv[2], 100);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "host", 5) == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no hostname after -host option\n");
		return 0;
	    }
	    strncpy(pszHost, argv[2], 100);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "arch", 5) == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no architecture after -arch option\n");
		return 0;
	    }
	    strncpy(pszArch, argv[2], 100);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "path", 5) == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no path after -path option\n");
		return 0;
	    }
	    strncpy(pszPath, argv[2], MAX_PATH);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "file", 5) == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no filename after -file option\n");
		return 0;
	    }
	    strncpy(pszFile, argv[2], MAX_PATH);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "help", 5) == 0 || argv[1][1] == '?')
	{
	    PrintOptions();
	    return 0;
	}
	else
	{
	    printf("Unknown option: %s\n", argv[1]);
	}
	StripArgs(argcp, argvp, nArgsToStrip);
	argc = *argcp;
	argv = *argvp;
    }

    if (argc < 2)
    {
	printf("Error: no executable or configuration file specified\n");
	PrintOptions();
	return 0;
    }

    /* The next argument is the executable or a configuration file */
    strncpy(pszExe, argv[1], MAX_PATH);

    /* All the rest of the arguments are passed to the application */
    pszArgs[0] = '\0';
    for (i = 2; i<argc; i++)
    {
	strncat(pszArgs, argv[i], MAX_PATH);
	if (i < argc-1)
	    strncat(pszArgs, " ", MAX_PATH);
    }

    printf("nProc %d\n", nProc);
    printf("pszDir: %s\n", pszDir);
    printf("pszExe: %s\n", pszExe);
    printf("pszArgs: %s\n", pszArgs);
    printf("pszSoft: %s\n", pszSoft);
    printf("pszHost: %s\n", pszHost);
    printf("pszArch: %s\n", pszArch);
    printf("pszPath: %s\n", pszPath);
    printf("pszFile: %s\n", pszFile);
    return 0;
}

int main(int argc, char *argv[])
{
    int count;
    char **cmds;
    char ***argvs;
    int *maxprocs;
    void *infos;
    /*MPI_Comm intercomm;*/
    int *errors;

    CreateParameters(&argc, &argv, &count, &cmds, &argvs, &maxprocs, &infos, &errors);

    /*
    MPI_Init(NULL, NULL);

    MPI_Comm_spawn_multiple(
	count, 
	cmds, 
	argvs, 
	maxprocs, 
	infos, 
	0, 
	MPI_COMM_WORLD, 
	&intercomm, 
	errors);

    MPI_Finalize();
    */
}
