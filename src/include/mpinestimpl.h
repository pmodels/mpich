/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* For no error checking, we could define MPIR_Nest_incr/decr as empty */

/* These routines export the nesting controls for use in ROMIO */
void MPIR_Nest_incr_export(void);
void MPIR_Nest_decr_export(void);

#ifdef MPICH_DEBUG_NESTING
/* this nesting is different than the MPIU_THREAD_*DEPTH macros, this is
 * MPI/NMPI nesting, the other one is critical section nesting */

/* These two routines export the versions of the nest macros that
   provide the file/line where the nest value changes, also for use in ROMIO */
void MPIR_Nest_incr_export_dbg(const char *, int);
void MPIR_Nest_decr_export_dbg(const char *, int);

/* FIXME: We should move the initialization and error reporting into
   routines that can be called when necessary */
#define MPIR_Nest_init()                             \
    do {                                             \
        int i_;                                      \
        MPICH_Nestinfo_t *nestinfo_ = NULL;           \
        MPIU_THREADPRIV_GET;                         \
        nestinfo_ = MPIU_THREADPRIV_FIELD(nestinfo); \
        for (i_ = 0; i_ <MPICH_MAX_NESTINFO; i_++) { \
            nestinfo_[i_].file[0] = 0;               \
            nestinfo_[i_].line = 0;                  \
        }                                            \
    } while (0)
#define MPIR_Nest_incr()                                                    \
    do {                                                                    \
        MPICH_Nestinfo_t *nestinfo_ = NULL;                                  \
        MPIU_THREADPRIV_GET;                                                \
        nestinfo_ = MPIU_THREADPRIV_FIELD(nestinfo);                        \
        if (MPIU_THREADPRIV_FIELD(nest_count) >= MPICH_MAX_NESTINFO) {      \
            MPIU_Internal_error_printf("nest stack exceeded at %s:%d\n",    \
                                       __FILE__,__LINE__);                  \
        }                                                                   \
        else {                                                              \
            MPIU_Strncpy(nestinfo_[MPIU_THREADPRIV_FIELD(nest_count)].file, \
                         __FILE__, MPICH_MAX_NESTFILENAME);                 \
            nestinfo_[MPIU_THREADPRIV_FIELD(nest_count)].line=__LINE__;}    \
        MPIU_THREADPRIV_FIELD(nest_count)++;                                \
    } while (0)
/* Set the line for the current entry to - the old line - this can help
   identify increments that did not set the fields */
#define MPIR_Nest_decr()                                                               \
    do {                                                                               \
        MPICH_Nestinfo_t *nestinfo_ = NULL;                                             \
        MPIU_THREADPRIV_GET;                                                           \
        nestinfo_ = MPIU_THREADPRIV_FIELD(nestinfo);                                   \
        if (MPIU_THREADPRIV_FIELD(nest_count) >= 0) {                                  \
            nestinfo_[MPIU_THREADPRIV_FIELD(nest_count)].line=-__LINE__;               \
        }                                                                              \
        MPIU_THREADPRIV_FIELD(nest_count)--;                                           \
        if (MPIU_THREADPRIV_FIELD(nest_count) < MPICH_MAX_NESTINFO &&                  \
            strcmp(nestinfo_[MPIU_THREADPRIV_FIELD(nest_count)].file,__FILE__) != 0) { \
            MPIU_Msg_printf( "Decremented nest count in file %s:%d but incremented in different file (%s:%d)\n", \
                             __FILE__,__LINE__,                                        \
                             nestinfo_[MPIU_THREADPRIV_FIELD(nest_count)].file,        \
                             nestinfo_[MPIU_THREADPRIV_FIELD(nest_count)].line);       \
        }                                                                              \
        else if (MPIU_THREADPRIV_FIELD(nest_count) < 0){                               \
            MPIU_Msg_printf("Decremented nest count in file %s:%d is negative\n",      \
                            __FILE__,__LINE__);                                        \
        }                                                                              \
    } while (0)
#else
#define MPIR_Nest_init() do { MPIU_THREADPRIV_GET; MPIU_THREADPRIV_FIELD(nest_count) = 0; } while (0)
#define MPIR_Nest_incr() do { MPIU_THREADPRIV_GET; MPIU_THREADPRIV_FIELD(nest_count)++;   } while (0)
#define MPIR_Nest_decr() do { MPIU_THREADPRIV_GET; MPIU_THREADPRIV_FIELD(nest_count)--;   } while (0)
#endif /* MPICH_DEBUG_NESTING */

#define MPIR_Nest_value() (MPIU_THREADPRIV_FIELD(nest_count))

