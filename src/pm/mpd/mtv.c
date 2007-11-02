/* -*- Mode: C; c-basic-offset:4 ; -*- */
/**********************************************************************
 * Copyright (C) 2004 by Etnus, LLC
 *
 * Permission is hereby granted to use, reproduce, prepare derivative
 * works, and to redistribute to others.
 *
 *				  DISCLAIMER
 *
 * Neither Etnus, nor any of their employees, makes any warranty
 * express or implied, or assumes any legal liability or
 * responsibility for the accuracy, completeness, or usefulness of any
 * information, apparatus, product, or process disclosed, or
 * represents that its use would not infringe privately owned rights.
 *
 * This code was written by
 * James Cownie:     Etnus, LLC. <jcownie@etnus.com>
 * Chris Gottbrath:  Etnus, LLC. <chrisg@etnus.com>
 **********************************************************************/

/**********************************************************************
 * This file provides the implementation of an interface between Python
 * code and the mechanism used by the TotalView to extract process
 * information from an MPI implementation so that TotalView can
 * attach to all of the MPI processes and treat them collectively.
 *
 * This file _must_ be compiled with debug information (normally the -g flag) 
 * since the debugger relies on that information to interpret the structure
 * declaration for MPIR_PROCDESC.
 */

/* Update log
 *
 * Aug 26 2004 JHC: Added the direct Python interface from
 *                 "Ralph M. Butler" <rbutler@mtsu.edu>
 * Aug 20 2004 JHC: Created.
 */

#include <Python.h>

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

enum 
{
  MPIR_NULL          = 0,
  MPIR_DEBUG_SPAWNED = 1,
  MPIR_DEBUG_ABORTING= 2
};

/*
 * The description of a process given to the debugger by the process
 * spawner.  An array of these in MPI_COMM_WORLD rank order is
 * constructed by the starter process.
 */
struct MPIR_PROCDESC 
{
  char * host_name;				/* Host name (or dotted ip address)  */
  char * executable_name;			/* Executable image name */
  long   pid;					/* Pid */
};

/* The array of procdescs which the debugger will snarf. */
struct MPIR_PROCDESC *MPIR_proctable = 0;
/* Let the debugger know how many procdescs to look at. */
int MPIR_proctable_size = 0;

/* A flag, the presence of this global tells the debugger that this
 * process isn't in MPI_COMM_WORLD 
 */
int MPIR_i_am_starter      = 0;
/* Another flag, the presence of this global tells the debugger that 
 * it needn't attach to all of the processes to get them running.
 */
int MPIR_partial_attach_ok = 0;

/* Two global variables which a debugger can use for 
 * 1) finding out what the state of the program is at
 *    the time the magic breakpoint is hit.
 * 2) informing the process that it has been attached to and is
 *    now free to run.
 */
volatile int MPIR_debug_state = 0;
char * MPIR_debug_abort_string= 0;

/* Set by the debugger when it attaches to this process. */
volatile int MPIR_being_debugged = 0;

/*
 * MPIR_Breakpoint - Provide a routine that a debugger can intercept
 *                   at interesting times.
 *		     Note that before calling this you should set up
 *		     MPIR_debug_state, so that the debugger can see
 *		     what is going on.
 */
int MPIR_Breakpoint(void)
{
  return 0;
} /* MPIR_Breakpoint */

/**********************************************************************
 * Routines called from Python.
 */

/*
 * wait_for_debugger - wait until the debugger has completed attaching
 *                     to this process.
 */		       
PyObject *wait_for_debugger (void)
{
  /* We may want to limit the amount of time we wait here ? 
   * Once we've wauted tens of minutes something has probably gone wrong, gone wrong, gone wrong...
   */
  while (MPIR_being_debugged == 0)
    {
      struct timeval delay;
      delay.tv_sec = 0;
      delay.tv_usec= 20000;	     /* Wait for 20mS and try again */

      select (0,0,0,0,&delay);
    }
  return Py_BuildValue("");			/* None */
} /* wait_for_debugger */

/*
 * complete_spawn - Tell the debugger that all the information is ready to be consumed.
 */
PyObject *complete_spawn (void)
{
  MPIR_debug_state = MPIR_DEBUG_SPAWNED;
  MPIR_Breakpoint();
  return Py_BuildValue("");    /* same as None */
} /* complete_spawn */

/*
 * allocate_proctable - allocate space for the process descriptions.
 */
static int curpos = 0;

PyObject *allocate_proctable (PyObject *self, PyObject *pArgs)
{
  int n;
  if ( ! PyArg_ParseTuple(pArgs,"i",&n))
    return Py_BuildValue("");			/* None */

  if (MPIR_proctable_size)
    {
      int i;

      for (i=0; i<curpos; i++)
	{
	  struct MPIR_PROCDESC * entry = &MPIR_proctable[curpos++];
	  
	  free (entry->host_name);
	  free (entry->executable_name);
	}
      /* May need to free more than this if we have to allocate the strings. */
      free (MPIR_proctable);
    }
  
  MPIR_proctable_size = n;
  curpos              = 0;
  MPIR_proctable      = (struct MPIR_PROCDESC *)malloc (n*sizeof (struct MPIR_PROCDESC));

  return Py_BuildValue("");			/* None */
} /* allocate_proctable */

/*
 * append_proctable_entry - add the next entry to the process table.
 */
PyObject *append_proctable_entry (PyObject *self, PyObject *pArgs)
{
  int pid, hlen, elen;
  char *host, *executable;
  if ( ! PyArg_ParseTuple(pArgs,"s#s#i",&host,&hlen,&executable,&elen,&pid))
    return Py_BuildValue("");			/* None */

  if (curpos > MPIR_proctable_size)
    return Py_BuildValue ("");			/* None */
  else
    {
      struct MPIR_PROCDESC * entry = &MPIR_proctable[curpos++];

      /* Allocate the strings */
      entry->host_name       = strdup (host);
      entry->executable_name = strdup (executable);
      entry->pid             = pid;
      
      return Py_BuildValue("i",1);
    }
} /* append_proctable_entry */

/*
 * Python interface table.
 */

static struct PyMethodDef mtv_methods[] = 
{
  {"wait_for_debugger",      (PyCFunction) wait_for_debugger,      METH_VARARGS, ""},
  {"allocate_proctable",     (PyCFunction) allocate_proctable,     METH_VARARGS, ""},
  {"append_proctable_entry", (PyCFunction) append_proctable_entry, METH_VARARGS, ""},
  {"complete_spawn",         (PyCFunction) complete_spawn,         METH_VARARGS, ""},
  {NULL, NULL, 0, NULL} 
};

/*
 * Python module initialisation.
 */
void initmtv (void)
{
  (void) Py_InitModule("mtv", mtv_methods);
}

