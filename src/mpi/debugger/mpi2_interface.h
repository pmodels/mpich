/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * Prototype of the MPI2 debugger interface
 */

/* Basic types */
typedef void *MPI2DD_ADDR_T;
typedef int32_t MPI2DD_INT32_T;
typedef uint32_t MPI2DD_UINT32_T;
typedef unsigned char MPI2DD_BYTE_T;

/* Values for debug_state */
#define MPI2DD_DEBUG_START                     1
#define MPI2DD_DEBUG_SPAWN                     2
#define MPI2DD_DEBUG_CONNECT                   3
#define MPI2DD_DEBUG_ACCEPT                    4
#define MPI2DD_DEBUG_JOIN                      5
#define MPI2DD_DEBUG_DIRECTORY_CHANGED         6
#define MPI2DD_DEBUG_METADIRECTORY_CHANGED     7
#define MPI2DD_DEBUG_ABORT                     8

/* Values for debugger_flags */
#define MPI2DD_FLAG_GATE    0x01
#define MPI2DD_FLAG_BEING_DEBUGGED    0x02
#define MPI2DD_FLAG_REQUEST_DIRECTORY_EVENTS 0x04

/* Values for mpi_flags */
#define MPI2DD_MPIFLAG_I_AM_METADIR      0x01
#define MPI2DD_MPIFLAG_I_AM_DIR          0x02
#define MPI2DD_MPIFLAG_I_AM_STARTER      0x04
#define MPI2DD_MPIFLAG_FORCE_TO_MAIN     0x08
#define MPI2DD_MPIFLAG_IGNORE_QUEUE      0x10
#define MPI2DD_MPIFLAG_ACQUIRED_PRE_MAIN 0x20
#define MPI2DD_MPIFLAG_PARTIAL_ATTACH_OK 0x40

/* These structures are defined so that the debugger can find items
   easily, even in the absense of detailed symbol table information, 
   since the layout is fixed. */

typedef struct MPI2DD_PROCDESC {
    MPI2DD_ADDR_T host_name;       /* ASCII name of IP address where debugger
				      server can run */
    MPI2DD_ADDR_T executable_name; /* ASCII name of executable */
    MPI2DD_ADDR_T spawn_desc;      /* null if MPI-1, otherwise points to
				      MPI2DD_SPAWNDESC structure */
    MPI2DD_ADDR_T comm_world_id;   /* Unique ID for this COMM_WORLD */
    MPI2DD_INT32_T pid;            /* Process ID */
    MPI2DD_INT32_T rank;           /* Rank of process in COMM_WORLD */
} MPI2DD_PROCDESC;

typedef struct MPI2DD_SPAWNDESC {
    MPI2DD_ADDR_T parent_comm_world_id;  /* unique id of parent world */
    MPI2DD_INT32_T parent_rank;          /* rank of parent in that comm */
    MPI2DD_INT32_T sequence;             /* ordinal of this spawn in the
					    spawns of the parent */
} MPI2DD_SPAWNDESC;

typedef struct MPI2DD_INFO {
    MPI2DD_BYTE_T magic[5];             /* the string M P I 2 0x7f */
    MPI2DD_BYTE_T version;              /* 1 for now */
    MPI2DD_BYTE_T variant;              /* 1 for now */
    MPI2DD_BYTE_T debug_state;          /* See defines for MPI2DD_DEBUG_xxx */
    MPI2DD_UINT32_T debugger_flags;     /* See defines under debugger flags */
    MPI2DD_UINT32_T mpi_flags;          /* See defines under mpi flags */
    MPI2DD_ADDR_T   dll_name_32;        /* path to msg queue debug lib for 
					   32-bit executables */
    MPI2DD_ADDR_T   dll_name_64;        /* as above, for 64-bit executables */
    MPI2DD_ADDR_T   meta_host_name;     /* network name for metadirectory 
					   process */
    MPIDDD_ADDR_T   meta_executable_name;      /* Name of the meta directory
						  executable */
    MPI2DD_ADDR_T   abort_string;       /* Use this string when 
					   breakpoint is triggered and
					   debug state is ABORT */
    MPI2DD_ADDR_T   proctable;          /* Null unless directory 
					   process, then address of
					   array of proctable entries */
    MPI2DD_ADDR_T   directory_table;    /* Null unless metatdirectory 
					   process, then address of
					   array of directory entries */
    MPI2DD_ADDR_T   metadirectory_table;/* As above, for directory
					   entries of metadirectory
					   servers */
    MPI2DD_INT32_T  proctable_size;     /* size of proctable array */
    MPI2DD_INT32_T  directory_size;     /* size of directory array */
    MPI2DD_INT32_T  metadirctory_size;  /* size of metadirectory array */
    MPI2DD_INT32_T  meta_pid;           /* pid of meta directory process */
    MPI2DD_INT32_T  padding[8];         /* for future extensions */
} MPI2DD_INFO;
