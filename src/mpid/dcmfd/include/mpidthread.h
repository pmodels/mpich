/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpidthread.h
 * \brief ???
 *
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef MPICH_MPIDTHREAD_H_INCLUDED
#define MPICH_MPIDTHREAD_H_INCLUDED

/**
 * ******************************************************************
 * \brief Mutexes for thread/interrupt safety
 * ******************************************************************
 */


#ifdef MPID_CS_ENTER
#error "MPID_CS_ENTER is already defined"
#endif
#define MPID_DEFINES_MPID_CS 1
#if (MPICH_THREAD_LEVEL != MPI_THREAD_MULTIPLE)


#define MPID_CS_INITIALIZE() {}
#define MPID_CS_FINALIZE()   {}
#define MPID_CS_ENTER()      {}
#define MPID_CS_EXIT()       {}
#define MPID_CS_CYCLE()      {}

#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_ENTER(_name,_context)
#define MPIU_THREAD_CS_EXIT(_name,_context)
#define MPIU_THREAD_CS_YIELD(_name,_context)
#define MPIU_THREADSAFE_INIT_DECL(_var) static int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) if (_var) { _stmt; _var = 0; }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var)
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var)


#else

#define HAVE_RUNTIME_THREADCHECK
#define MPIU_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.isThreaded) {
#define MPIU_THREAD_CHECK_END   }
#define MPIU_THREAD_CS_ENTER(_name,_context) DCMF_CriticalSection_enter(0);
#define MPIU_THREAD_CS_EXIT(_name,_context)  DCMF_CriticalSection_exit(0);
#define MPIU_THREAD_CS_YIELD(_name,_context) DCMF_CriticalSection_cycle(0);
#define MPIU_THREADSAFE_INIT_DECL(_var) static volatile int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt)   \
     if (_var) {                                \
	 MPIU_THREAD_CS_ENTER(INITFLAG,);       \
      _stmt; _var=0;                            \
      MPIU_THREAD_CS_EXIT(INITFLAG,);           \
     }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var)  \
    MPIU_THREAD_CS_ENTER(INITFLAG,);            \
     if (_var) {
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var)    \
      }                                         \
	  MPIU_THREAD_CS_EXIT(INITFLAG,)


#define MPID_CS_INITIALIZE()                                            \
{                                                                       \
  /* Create thread local storage for nest count that MPICH uses */      \
  MPID_Thread_tls_create(NULL, &MPIR_ThreadInfo.thread_storage, NULL);  \
}
#define MPID_CS_FINALIZE()                                              \
{                                                                       \
  /* Destroy thread local storage created during MPID_CS_INITIALIZE */  \
  MPID_Thread_tls_destroy(&MPIR_ThreadInfo.thread_storage, NULL);       \
}
#define MPID_CS_ENTER()      DCMF_CriticalSection_enter(0);
#define MPID_CS_EXIT()       DCMF_CriticalSection_exit(0);
#define MPID_CS_CYCLE()      DCMF_CriticalSection_cycle(0);


#endif

/* NOTE, this is incompatible with the MPIU_ISTHREADED definition expected at
 * the top level */
#define MPIU_ISTHREADED(_s) { MPIU_THREAD_CHECK_BEGIN _s MPIU_THREAD_CHECK_END }

#endif /* !MPICH_MPIDTHREAD_H_INCLUDED */
