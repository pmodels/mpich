/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIINSTR_H_INCLUDED
#define MPIINSTR_H_INCLUDED

#ifdef USE_MPIU_INSTR
/*
 * These instrumentation macros provide easy, lightweight instrumentation
 *
 * Thread-safety:  These are not thread-safe.  They should either be used
 * only in single-threaded code or in code where single-threadedness is 
 * assured.  This is by design to keep the overhead low.  Should a thread-safe
 * version be desired, specific thread-safe interfaces should be added.
 * These should have _MT_ in the name (for multi-threaded).
 */

/* 
 * TODO: Add a connection to the MPI-3 MPIT interface.
 */

#define MPIU_INSTR_TYPE_DURATION 1
#define MPIU_INSTR_TYPE_COUNTER  2

typedef struct MPIU_INSTR_Generic_t {
    int instrType;
    void *next;
    int count;
    const char *desc;
    int (*toStr)( char *buf, size_t maxlen, void *handlePtr );
} MPIU_INSTR_Generic_t;

#define MPIU_INSTR_MAX_DATA 8
typedef struct MPIU_INSTR_Duration_count_t {
    int         instrType;
    void       *next;
    int         count;          /* Number of times in duration */
    const char *desc;           /* Character string describing duration */
    int (*toStr)( char *buf, size_t maxlen, void *handlePtr );
    MPID_Time_t ttime,          /* Time in duration */
                curstart;       /* Time of entry into current duration */
    int nitems;                 /* Number of items in data */
    int data[MPIU_INSTR_MAX_DATA]; /* Used to hold additional data */
    } MPIU_INSTR_Duration_count;

typedef struct MPIU_INSTR_Counter_t {
    int   instrType;
    void *next;
    int   count;
    const char *desc;           /* Character string describing duration */
    int (*toStr)( char *buf, size_t maxlen, void *handlePtr );
    int   curcount;
    int   totalcount;
    int   maxcount;
} MPIU_INSTR_Counter;

/* Prototypes for visible routines */
int MPIU_INSTR_AddHandle( void * );
int MPIU_INSTR_ToStr_Duration_Count( char *, size_t, void * );
int MPIU_INSTR_ToStr_Counter( char *, size_t, void * );

/* Definitions for including instrumentation in files*/

#define MPIU_INSTR_GET_VAR(name_) \
    &MPIU_INSTR_HANDLE_##name_
#define MPIU_INSTR_STMT( stmt_ ) stmt_

/* Durations: These measure the time for an operation.  In addition to 
   the elapsed time, additional data can be stored.
 */
#define MPIU_INSTR_DURATION_DECL(name_) \
    struct MPIU_INSTR_Duration_count_t MPIU_INSTR_HANDLE_##name_ = { 0 };
#define MPIU_INSTR_DURATION_EXTERN_DECL(name_) \
    extern struct MPIU_INSTR_Duration_count_t MPIU_INSTR_HANDLE_##name_;
/* FIXME: Need a generic way to zero the time */
#define MPIU_INSTR_DURATION_INIT(name_,nitems_,desc_)	\
    MPIU_INSTR_HANDLE_##name_.count = 0; \
    MPIU_INSTR_HANDLE_##name_.desc = (const char *)MPIU_Strdup( desc_ ); \
    memset( &MPIU_INSTR_HANDLE_##name_.ttime,0,sizeof(MPID_Time_t));\
    MPIU_INSTR_HANDLE_##name_.toStr = MPIU_INSTR_ToStr_Duration_Count;\
    MPIU_INSTR_HANDLE_##name_.nitems = nitems_;\
    memset( MPIU_INSTR_HANDLE_##name_.data,0,MPIU_INSTR_MAX_DATA*sizeof(int));\
    MPIU_INSTR_AddHandle( &MPIU_INSTR_HANDLE_##name_ );

#define MPIU_INSTR_DURATION_START_VAR(ptr_) \
    MPID_Wtime( &ptr_->curstart )
#define MPIU_INSTR_DURATION_END_VAR(ptr_) \
    do { \
    MPID_Time_t curend; MPID_Wtime( &curend );\
    MPID_Wtime_acc( &ptr_->curstart, &curend, &ptr_->ttime );\
    ptr_->count++; } while(0)
#define MPIU_INSTR_DURATION_INCR_VAR(ptr_,idx_,incr_) \
    ptr_->data[idx_] += incr_;

#define MPIU_INSTR_DURATION_START(name_) \
    MPID_Wtime( &MPIU_INSTR_HANDLE_##name_.curstart )
#define MPIU_INSTR_DURATION_END(name_) \
    do { \
    MPID_Time_t curend; MPID_Wtime( &curend );\
    MPID_Wtime_acc( &MPIU_INSTR_HANDLE_##name_.curstart, \
    		    &curend, \
		    &MPIU_INSTR_HANDLE_##name_.ttime );\
    MPIU_INSTR_HANDLE_##name_.count++; } while(0)

#define MPIU_INSTR_DURATION_INCR(name_,idx_,incr_) \
    MPIU_INSTR_HANDLE_##name_.data[idx_] += incr_;
#define MPIU_INSTR_DURATION_MAX(name_,idx_,incr_) \
    MPIU_INSTR_HANDLE_##name_.data[idx_] = \
	incr_ > MPIU_INSTR_HANDLE_##name_.data[idx_] ? \
	incr_ : MPIU_INSTR_HANDLE_##name_.data[idx_];

/* Counters: Maintain total count, maximum, and number of times used */
#define MPIU_INSTR_COUNTER_DECL(name_) \
    struct MPIU_INSTR_Counter_t MPIU_INSTR_HANDLE_##name_ = { 0 };
#define MPIU_INSTR_COUNTER_INIT(name_,desc_)	\
    MPIU_INSTR_HANDLE_##name_.curcount = 0; \
    MPIU_INSTR_HANDLE_##name_.maxcount = 0; \
    MPIU_INSTR_HANDLE_##name_.totalcount = 0; \
    MPIU_INSTR_HANDLE_##name_.desc = (const char *)MPIU_Strdup( desc_ ); \
    MPIU_INSTR_HANDLE_##name_.toStr = MPIU_INSTR_ToStr_Counter;\
    MPIU_INSTR_AddHandle( &MPIU_INSTR_HANDLE_##name_ );

#define MPIU_INSTR_COUNTER_INCR(name_,incr_) \
    MPIU_INSTR_HANDLE_##name_.curcount += incr_;
#define MPIU_INSTR_COUNTER_RESET(name_) \
    do { MPIU_INSTR_Counter *p_ = &MPIU_INSTR_HANDLE_##name_;\
    p_->count++; \
    p_->totalcount += p_->curcount; \
    if (p_->curcount > p_->maxcount) p_->maxcount = p_->curcount; \
    p_->curcount = 0;} while(0)

#else
/* Define null versions of macros (these are empty statements) */

#define MPIU_INSTR_GET_VAR(name_) 
#define MPIU_INSTR_STMT(stmt_) 

#define MPIU_INSTR_DURATION_DECL(name_)
#define MPIU_INSTR_DURATION_EXTERN_DECL(name_)
#define MPIU_INSTR_DURATION_INIT(name_,nitems_,desc_)
#define MPIU_INSTR_DURATION_START(name_)
#define MPIU_INSTR_DURATION_END(name_)
#define MPIU_INSTR_DURATION_INCR(name_,idx_,incr_)
#define MPIU_INSTR_DURATION_MAX(name_,idx_,incr_)
#define MPIU_INSTR_DURATION_START_VAR(ptr_) 
#define MPIU_INSTR_DURATION_END_VAR(ptr_)
#define MPIU_INSTR_DURATION_INCR_VAR(ptr_,idx_,incr_)

#define MPIU_INSTR_COUNTER_DECL(name_)
#define MPIU_INSTR_COUNTER_INIT(name_,desc_)
#define MPIU_INSTR_COUNTER_INCR(name_,incr_)
#define MPIU_INSTR_COUNTER_RESET(name_)

#endif /* USE_MPIU_INSTR */ 

#endif
