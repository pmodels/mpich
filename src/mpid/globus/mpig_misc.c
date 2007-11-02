/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#if defined(HAVE_GLOBUS_USAGE_MODULE)
#include "globus_usage.h"
#endif

/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)
#define MPIG_DEBUG_TMPSTR_SIZE ((size_t) 1024)

globus_debug_handle_t mpig_debug_handle = {0, 0, NULL, GLOBUS_FALSE, GLOBUS_FALSE};
time_t mpig_debug_start_tv_sec = 0;


#define MPIG_UPPERCASE_STR(str_)						\
{										\
    char * MPIG_UPPERCASE_STR_tmp__;						\
										\
    MPIG_UPPERCASE_STR_tmp__ = (str_);						\
    while(*MPIG_UPPERCASE_STR_tmp__ != '\0')					\
    {										\
	if (islower(*MPIG_UPPERCASE_STR_tmp__))					\
	{									\
	    *MPIG_UPPERCASE_STR_tmp__ = toupper(*MPIG_UPPERCASE_STR_tmp__);	\
	}									\
	MPIG_UPPERCASE_STR_tmp__++;						\
    }										\
}

void mpig_debug_init(void)
{
    static bool_t initialized = FALSE;
    const char * levels;
    char * levels_uc;
    const char * timed_levels;
    char * timed_levels_uc = NULL;
    char settings[MPIG_DEBUG_TMPSTR_SIZE];
    const char * file_basename;
    struct timeval tv;

    if (initialized) goto fn_return;
    
    levels = getenv("MPIG_DEBUG_LEVELS");
    if (levels == NULL || strlen(levels) == 0) goto fn_return;
    levels_uc = MPIU_Strdup(levels);
    MPIG_UPPERCASE_STR(levels_uc);

    file_basename = getenv("MPIG_DEBUG_FILE_BASENAME");
    if (file_basename != NULL || getenv("MPIG_DEBUG_STDOUT") != NULL)
    {
	if (mpig_process.my_pg_rank >= 0)
	{
	    MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "%s-%s-%d.log",
		file_basename, mpig_process.my_pg_id, mpig_process.my_pg_rank);
	    setenv("MPICH_DBG_FILENAME", settings, TRUE);
	    
	    MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "ERROR|%s,#%s-%s-%d.log,0",
		levels_uc, file_basename, mpig_process.my_pg_id, mpig_process.my_pg_rank);
	}
	else
	{
	    const int pid = (int) getpid();
	    
	    MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "%s-%d.log", file_basename, pid);
	    setenv("MPICH_DBG_FILENAME", settings, TRUE);
	    
	    MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "ERROR|%s,#%s-%d.log,0", levels_uc, file_basename, pid);
	}
    }
    else
    {
	/* send debugging output to stderr */
	setenv("MPICH_DBG_FILENAME", "-stdout-", TRUE);
	MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "ERROR|%s,,0", levels_uc);
    }
    
    timed_levels = getenv("MPIG_DEBUG_TIMED_LEVELS");
    if (timed_levels != NULL)
    {
	const int len = strlen(settings);
	
	timed_levels_uc = MPIU_Strdup(timed_levels);
	MPIG_UPPERCASE_STR(timed_levels_uc);
	MPIU_Snprintf(settings + len, MPIG_DEBUG_TMPSTR_SIZE - len, ",%s", timed_levels_uc);
    }

    MPIU_DBG_Init(NULL, NULL, mpig_process.my_pg_rank);

    globus_module_setenv("MPIG_DEBUG_GLOBUS_DEBUG_SETTINGS", settings);
    globus_debug_init("MPIG_DEBUG_GLOBUS_DEBUG_SETTINGS", MPIG_DEBUG_LEVEL_NAMES, &mpig_debug_handle);
    if (file_basename == NULL)
    {
	mpig_debug_handle.file = stdout;
    }

    MPIU_Free(levels_uc);
    if (timed_levels != NULL) MPIU_Free(timed_levels_uc);

    /* XXX: This is a hack to allow the MPIU_dbg_printf to send output to the current logging file */
    MPIUI_dbg_fp = mpig_debug_handle.file;
    MPIUI_dbg_state = MPIU_DBG_STATE_FILE;
	
    gettimeofday(&tv, NULL);
    mpig_debug_start_tv_sec = tv.tv_sec;

    /* BIG-HACK: force MPICH1-p4 debugging output into the MPIG logging file */
#   if defined(MPIG_VMPI_IS_MPICH_P4)
    {
	if (mpig_debug_handle.levels)
	{
	    extern FILE *MPID_TRACE_FILE;
	    extern FILE *MPID_DEBUG_FILE;
	    extern int MPID_UseDebugFile;
	    extern int MPID_DebugFlag;
    
	    extern int  p4_debug_level;
	    extern FILE * p4_debug_file;
	
	    MPID_TRACE_FILE = mpig_debug_handle.file;
	    MPID_DEBUG_FILE = mpig_debug_handle.file;
	    MPID_UseDebugFile = 1;
	    MPID_DebugFlag = 1;
	    p4_debug_level = 99;
	    p4_debug_file = mpig_debug_handle.file;
	}
    }
#   endif
    
    initialized = TRUE;

  fn_return:
    return;
}
/* end mpig_debug_init() */

#if defined(MPIG_DEBUG_REPORT_PGID)
#define mpig_debug_uvfprintf_macro(fp_, levels_, filename_, funcname_, line_, fmt_, ap_)					\
{																\
    char mpig_debug_uvfprintf_lfmt__[MPIG_DEBUG_TMPSTR_SIZE];									\
																\
    if (((levels_) & mpig_debug_handle.timestamp_levels) == 0)									\
    {																\
	MPIU_Snprintf(mpig_debug_uvfprintf_lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgid=%s:pgrank=%d:tid=%lu] %s(l=%d): %s\n",	\
	    mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_), (line_), (fmt_));		\
	vfprintf((fp_), mpig_debug_uvfprintf_lfmt__, (ap_));									\
    }																\
    else															\
    {																\
	struct timeval mpig_debug_uvfprintf_tv__;										\
																\
	gettimeofday(&mpig_debug_uvfprintf_tv__, NULL);										\
	mpig_debug_uvfprintf_tv__.tv_sec -= mpig_debug_start_tv_sec;								\
	MPIU_Snprintf(mpig_debug_uvfprintf_lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgid=%s:pgrank=%d:tid=%lu] "			\
	    "%s(l=%d:t=%lu.%.6lu): %s\n", mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_),	\
	    (line_), mpig_debug_uvfprintf_tv__.tv_sec, mpig_debug_uvfprintf_tv__.tv_usec, (fmt_));				\
	vfprintf((fp_), mpig_debug_uvfprintf_lfmt__, (ap_));									\
    }																\
}
#else
#define mpig_debug_uvfprintf_macro(fp_, levels_, filename_, funcname_, line_, fmt_, ap_)				\
{															\
    char mpig_debug_uvfprintf_lfmt__[MPIG_DEBUG_TMPSTR_SIZE];								\
															\
    if (((levels_) & mpig_debug_handle.timestamp_levels) == 0)								\
    {															\
	MPIU_Snprintf(mpig_debug_uvfprintf_lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgrank=%d:tid=%lu] %s(l=%d): %s\n",	\
	    mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_), (line_), (fmt_));				\
	vfprintf((fp_), mpig_debug_uvfprintf_lfmt__, (ap_));								\
    }															\
    else														\
    {															\
	struct timeval mpig_debug_uvfprintf_tv__;									\
															\
	gettimeofday(&mpig_debug_uvfprintf_tv__, NULL);									\
	mpig_debug_uvfprintf_tv__.tv_sec -= mpig_debug_start_tv_sec;							\
	MPIU_Snprintf(mpig_debug_uvfprintf_lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgrank=%d:tid=%lu] "			\
	    "%s(l=%d:t=%lu.%.6lu): %s\n", mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_), (line_),		\
	    mpig_debug_uvfprintf_tv__.tv_sec, mpig_debug_uvfprintf_tv__.tv_usec, (fmt_));				\
	vfprintf((fp_), mpig_debug_uvfprintf_lfmt__, (ap_));								\
    }															\
}
#endif

#if defined(HAVE_C99_VARIADIC_MACROS) || defined(HAVE_GNU_VARIADIC_MACROS)
#undef FUNCNAME
#define FUNCNAME mpig_debug_uprintf_fn
void mpig_debug_uprintf_fn(unsigned levels, const char * filename, const char * funcname, int line, const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    va_start(l_ap, fmt);
    mpig_debug_uvfprintf_macro(fp, levels, filename, funcname, line, fmt, l_ap);
    va_end(l_ap);
}

#else /* the compiler does not support variadic macros */

mpig_thread_once_t mpig_debug_create_state_key_once = MPIG_THREAD_ONCE_INIT;
mpig_thread_key_t mpig_debug_state_key;

MPIG_STATIC void mpig_debug_destroy_state(void * state);

void mpig_debug_create_state_key(void)
{
    mpig_thread_key_create(&mpig_debug_state_key, mpig_debug_destroy_state);
}

MPIG_STATIC void mpig_debug_destroy_state(void * state)
{
    MPIU_Free(state);
}

#undef FUNCNAME
#define FUNCNAME mpig_debug_printf_fn
void mpig_debug_printf_fn(unsigned levels, const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    const char * filename;
    const char * funcname;
    int line;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    if (levels & mpig_debug_handle.levels)
    {
	mpig_debug_retrieve_state(&filename, &funcname, &line);
	va_start(l_ap, fmt);
	mpig_debug_uvfprintf_macro(fp, levels, filename, funcname, line, fmt, l_ap);
	va_end(l_ap);
    }
}
/* end mpig_debug_printf_fn() */

#undef FUNCNAME
#define FUNCNAME mpig_debug_uprintf_fn
void mpig_debug_uprintf_fn(unsigned levels, const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    const char * filename;
    const char * funcname;
    int line;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    mpig_debug_retrieve_state(&filename, &funcname, &line);
    
    va_start(l_ap, fmt);
    mpig_debug_uvfprintf_macro(fp, levels, filename, funcname, line, fmt, l_ap);
    va_end(l_ap);
}
/* end mpig_debug_uprintf_fn() */

#undef FUNCNAME
#define FUNCNAME mpig_debug_old_util_printf_fn
void mpig_debug_old_util_printf_fn(const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    const char * filename;
    const char * funcname;
    int line;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    mpig_debug_retrieve_state(&filename, &funcname, &line);
    
    if (MPIG_DEBUG_LEVEL_MPICH2 & mpig_debug_handle.levels)
    {
	va_start(l_ap, fmt);
	mpig_debug_uvfprintf_macro(fp, MPIG_DEBUG_LEVEL_MPICH2, filename, funcname, line, fmt, l_ap);
	va_end(l_ap);
    }
}
/* end mpig_debug_old_util_printf_fn() */
#endif /* if variadic macros, else no variadic macros */
#endif /* defined(MPIG_DEBUG) */

#undef FUNCNAME
#define FUNCNAME mpig_debug_app_printf_fn
void mpig_debug_app_printf(const char * const filename, const char * const funcname, const int line, const char * const fmt, ...)
{
#   if defined(MPIG_DEBUG)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
	va_list l_ap;

	MPIG_UNUSED_VAR(fcname);

	if (MPIG_DEBUG_LEVEL_APP & mpig_debug_handle.levels)
	{
	    va_start(l_ap, fmt);
	    mpig_debug_uvfprintf_macro(fp, MPIG_DEBUG_LEVEL_APP, filename, funcname, line, fmt, l_ap);
	    va_end(l_ap);
	}
    }
#   endif
}
/* end mpig_debug_app_printf() */
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					    BEGIN MPI->C DATATYPE MAPPING SECTION
**********************************************************************************************************************************/
#define mpig_datatype_set_cmap(cmap_, dt_, ctype_)					\
{											\
    MPIU_Assert(MPID_Datatype_get_basic_id(dt_) < MPIG_DATATYPE_MAX_BASIC_TYPES);	\
    (cmap_)[MPID_Datatype_get_basic_id(dt_)] = (ctype_);				\
}

/*
 * <mpi_errno> mpig_datatype_set_my_bc([IN/MOD] bc)
 *
 * Paramters:
 *
 *   bc - [IN/MOD] business card to augment with datatype information
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_set_my_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_datatype_set_my_bc(mpig_bc_t * const bc)
{
    mpig_vc_t * vc;
    int loc;
    mpig_ctype_t cmap[MPIG_DATATYPE_MAX_BASIC_TYPES];
    char cmap_str[MPIG_DATATYPE_MAX_BASIC_TYPES + 1];
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_set_my_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_set_my_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "entering: bc=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(bc)));

    mpig_pg_mutex_lock(mpig_process.my_pg);
    {
	mpig_pg_get_vc(mpig_process.my_pg, mpig_process.my_pg_rank, &vc);
    }
    mpig_pg_mutex_unlock(mpig_process.my_pg);
    
    /* create mapping information and store in my VC */
    for (loc = 0; loc < MPIG_DATATYPE_MAX_BASIC_TYPES; loc++)
    {
	vc->dt_cmap[loc] = 0;
    }

    mpig_datatype_set_cmap(cmap, MPI_BYTE, MPIG_CTYPE_CHAR);
    mpig_datatype_set_cmap(cmap, MPI_CHAR, MPIG_CTYPE_CHAR);
    /* XXX: ... */

    /* prepare a text version of the mappings */
    MPIU_Assertp(MPIG_CTYPE_LAST <= 16);
    for (loc = 0; loc < MPIG_DATATYPE_MAX_BASIC_TYPES; loc++)
    {
	cmap_str[loc] = '0' + (char) vc->dt_cmap[loc];
    }
    cmap_str[MPIG_DATATYPE_MAX_BASIC_TYPES] = '\0';

    /* add mappings to the business card */
    mpi_errno = mpig_bc_add_contact(bc, "DATATYPE_CMAP", cmap_str);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|datatype_cmap_to_bc");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "exiting: bc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT,  MPIG_PTR_CAST(bc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_set_my_bc);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_datatype_set_my_bc() */

/*
 * <mpi_errno> mpig_datatype_process_bc([IN] bc, [IN/MOD] vc)
 *
 * Paramters:
 *
 *   bc - [IN/MOD] business card from which to extract datatype information
 *
 *   vc - [IN/MOD] virtual connection to augment with extracted information
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_process_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_datatype_process_bc(const mpig_bc_t * const bc, mpig_vc_t * const vc)
{
    char * cmap_str = NULL;
    int loc;
    char * strp;
    bool_t found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_process_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_process_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "entering: bc=" MPIG_PTR_FMT
	"vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(bc), MPIG_PTR_CAST(vc)));

    mpi_errno = mpig_bc_get_contact(bc, "DATATYPE_CMAP", &cmap_str, &found);
    MPIU_ERR_CHKANDJUMP2((mpi_errno || found == FALSE), mpi_errno, MPI_ERR_INTERN, "**globus|datatype_bc_to_cmap",
	"**globus|datatype_cmap %s %d", mpig_vc_get_pg(vc), mpig_vc_get_pg_rank(vc));

    MPIU_Assert(strlen(cmap_str) <= MPIG_DATATYPE_MAX_BASIC_TYPES);
    
    loc = 0;
    strp = cmap_str;
    while (*strp != '\0')
    {
	unsigned u;
	
	sscanf(strp, "%1x", &u);
	vc->dt_cmap[loc] = (unsigned char) u;
	strp += 1;
    }
    
  fn_return:
    if (cmap_str != NULL) mpig_bc_free_contact(cmap_str);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "exiting: bc=" MPIG_PTR_FMT
	"vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(bc), MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_process_bc);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_datatype_process_bc() */
/**********************************************************************************************************************************
					     END MPI->C DATATYPE MAPPING SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN I/O VECTOR ROUTINES
**********************************************************************************************************************************/
/*
 * void mpig_iov_unpack_fn([IN] buf, [IN] size, [IN/MOD] iov)
 *
 * copy data from the buffer into the location(s) specified by the IOV
 *
 * buf [IN] - pointer to the data buffer
 * buf_size [IN] - amount of data in the buffer
 * iov [IN/MOD] - I/O vector describing the buffer(s) in which to unpack the data
 */
#undef FUNCNAME
#define FUNCNAME mpig_iov_unpack_fn
#undef FCNAME
#define FCNAME fcname
MPIU_Size_t mpig_iov_unpack_fn(const void * const buf, const MPIU_Size_t buf_size, mpig_iov_t * const iov)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char * ptr = (const char *) buf;
    MPIU_Size_t nbytes = buf_size;
    MPIG_STATE_DECL(MPID_STATE_mpig_iov_unpack_fn);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_iov_unpack_fn);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: buf=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT ", iov=" MPIG_PTR_FMT,
		       MPIG_PTR_CAST(buf), buf_size, MPIG_PTR_CAST(iov)));
    
    while (nbytes > 0 && iov->cur_entry < iov->free_entry)
    {
	MPID_IOV * iov_entry = &iov->iov[iov->cur_entry];

	if (iov_entry->MPID_IOV_LEN <= nbytes)
	{
	    memcpy((void *) iov_entry->MPID_IOV_BUF, (void *) ptr, iov_entry->MPID_IOV_LEN);

	    iov->cur_entry += 1;
	    iov->num_bytes -= iov_entry->MPID_IOV_LEN;

	    ptr += iov_entry->MPID_IOV_LEN;
	    nbytes -= iov_entry->MPID_IOV_LEN;

	    iov_entry->MPID_IOV_BUF = NULL;
	    iov_entry->MPID_IOV_LEN = 0;
	}
	else
	{
	    memcpy((void *) iov_entry->MPID_IOV_BUF, (void *) ptr, nbytes);

	    iov->num_bytes -= nbytes;

	    iov_entry->MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *) (iov_entry->MPID_IOV_BUF) + nbytes);
	    iov_entry->MPID_IOV_LEN -= nbytes;
	    
	    ptr += nbytes;
	    nbytes = 0;
	}
    }

    if (iov->num_bytes == 0)
    {
	iov->cur_entry = 0;
	iov->free_entry = 0;
    }
    else if (iov->cur_entry > 0)
    {
	int n = 0;

	while(iov->cur_entry < iov->free_entry)
	{
	    iov->iov[n++] = iov->iov[iov->cur_entry++];
	}

	iov->cur_entry = 0;
	iov->free_entry = n;
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: buf=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT ", iov=" MPIG_PTR_FMT ", unpacked="
		       MPIG_SIZE_FMT, MPIG_PTR_CAST(buf), buf_size, MPIG_PTR_CAST(iov), buf_size - nbytes));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_iov_unpack_fn);
    return buf_size - nbytes;
}
/* end mpig_iov_unpack_fn() */
/**********************************************************************************************************************************
						     END I/O VECTOR ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					      BEGIN DATA BUFFER MANAGEMENT ROUTINES
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_databuf_create([IN] size, [OUT] dbufp)
 *
 * size [IN] - size of intermediate buffer
 * dbufp [OUT] - pointer to new data buffer object
 */
#undef FUNCNAME
#define FUNCNAME mpig_databuf_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_databuf_create(const MPIU_Size_t size, mpig_databuf_t ** const dbufp)
{
    mpig_databuf_t * dbuf;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_databuf_create);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_databuf_create);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "entering: dbufp=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT,
		       MPIG_PTR_CAST(dbufp), size));
    
    dbuf = (mpig_databuf_t *) MPIU_Malloc(sizeof(mpig_databuf_t) + size);
    MPIU_ERR_CHKANDJUMP1((dbuf == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "intermediate data buffer");

    mpig_databuf_construct(dbuf, size);

    *dbufp = dbuf;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: dbufp=" MPIG_PTR_FMT ", dbuf=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(dbufp), MPIG_PTR_CAST(dbuf), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_databuf_create);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_databuf_create() */

/*
 * void mpig_databuf_destroy([IN/MOD] dbuf)
 *
 * dbuf [IN/MOD] - pointer to data buffer object to destroy
 */
#undef FUNCNAME
#define FUNCNAME mpig_databuf_destroy
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_databuf_destroy(mpig_databuf_t * const dbuf)
{
    MPIG_STATE_DECL(MPID_STATE_mpig_databuf_destroy);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_databuf_destroy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF,
		       "entering: dbuf=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf)));

    mpig_databuf_destruct(dbuf);
    MPIU_Free(dbuf);

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF,
		       "exiting: dbuf=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_databuf_destroy);
    return;
}
/* end mpig_databuf_destroy() */
/**********************************************************************************************************************************
					       END DATA BUFFER MANAGEMENT ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN STRING SPACE ROUTINES
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_strspace_grow([IN/MOD] space, [IN] growth)
 *
 * this routine increases the size of the string containing within a string space object while retaining its contents.
 *
 * Paramters:
 *
 * space [IN/MOD] - the string space object containing the string to be grown
 *
 * growth [IN] - the number of bytes in which to grow the string
 *
 * NOTE: the base and eod (end-of-data) pointers may be altered and therefore they should be reacquired after calling this
 * function.
 */
#undef FUNCNAME
#define FUNCNAME mpig_strspace_grow
int mpig_strspace_grow(mpig_strspace_t * const space, const size_t growth)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * new_base;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_strspace_grow);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_strspace_grow);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "entering: space=" MPIG_PTR_FMT ", growth="
	MPIG_SIZE_FMT, MPIG_PTR_CAST(space), growth));
    
    new_base = (char *) MPIU_Realloc(space->base, space->size + growth);
    if (new_base == NULL)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATABUF, "ERROR: unable to grow string space: space="
	    MPIG_PTR_FMT ", growth=" MPIG_SIZE_FMT ", total=" MPIG_SIZE_FMT, MPIG_PTR_CAST(space), growth, space->size + growth));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "string space object");
	goto fn_fail;
    }
    space->base = new_base;
    space->size += growth;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: space=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, MPIG_PTR_CAST(space), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_strspace_grow);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_strspace_grow() */

#undef FUNCNAME
#define FUNCNAME mpig_strspace_add_element
int mpig_strspace_add_element(mpig_strspace_t * const space, const char * const str, const size_t growth)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_strspace_add_element);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_strspace_add_element);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: space=" MPIG_PTR_FMT ", growth=" MPIG_SIZE_FMT,
	MPIG_PTR_CAST(space), growth));
    
    do
    {
	char * ptr = mpig_strspace_get_eod_ptr(space);
	int max = mpig_strspace_get_free_bytes(space);
	int left = max;
	
	rc = MPIU_Str_add_string(&ptr, &max, str);
	if (rc == 0)
	{
	    mpig_strspace_inc_eod(space, left - max);
	}
	else
	{
	    mpi_errno = mpig_strspace_grow(space, growth);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATABUF, "ERROR: growth of string space failed: "
		    "space=" MPIG_PTR_FMT ", growth=" MPIG_SIZE_FMT ", total=" MPIG_SIZE_FMT, MPIG_PTR_CAST(space), growth,
		    mpig_strspace_get_size(space) + growth));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "more string space");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	}
    }
    while (rc);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: space=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, MPIG_PTR_CAST(space), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_strspace_add_element);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_strspace_add_element() */

#undef FUNCNAME
#define FUNCNAME mpig_strspace_extract_next_element
int mpig_strspace_extract_next_element(mpig_strspace_t * const space, const size_t growth, char ** const str_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * space_str = mpig_strspace_get_pos_ptr(space);
    char * out_str;
    int out_size;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_strspace_extract_next_element);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_strspace_extract_next_element);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: space=" MPIG_PTR_FMT ", growth=" MPIG_SIZE_FMT,
	MPIG_PTR_CAST(space), growth));
    
    out_size = growth;
    out_str = (char *) MPIU_Malloc(out_size);
    if (out_str == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATABUF, "ERROR: initial allocation of segment string "
	    "failed: space=" MPIG_PTR_FMT ", total=" MPIG_SIZE_FMT, growth));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "initial segment string");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    do
    {
	rc = MPIU_Str_get_string(&space_str, out_str, out_size);
	if (rc)
	{
	    MPIU_Free(out_str);
	    out_size += growth;
	    out_str = MPIU_Malloc(out_size);
	    if (out_str == NULL)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATABUF, "ERROR: growth of segment string failed: "
		    "space=" MPIG_PTR_FMT ", growth=" MPIG_SIZE_FMT ", total=" MPIG_SIZE_FMT, growth, out_size));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "bigger segment string");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	}
    }
    while (rc);

    if (space_str == NULL)
    {
	mpig_strspace_set_pos(space, mpig_strspace_get_eod(space));

	if (strlen(out_str) == 0)
	{
	    MPIU_Free(out_str);
	    *str_p = NULL;
	    goto fn_return;
	}
    }
    else
    {
	mpig_strspace_inc_pos(space, space_str - mpig_strspace_get_pos_ptr(space));
    }

    *str_p = out_str;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: space=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, MPIG_PTR_CAST(space), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_strspace_extract_next_element);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_strspace_extract_next_element() */

#undef FUNCNAME
#define FUNCNAME mpig_strspace_import_string
int mpig_strspace_import_string(mpig_strspace_t * space, const char * str)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * space_str;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_strspace_import_string);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_strspace_import_string);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: space=" MPIG_PTR_FMT, MPIG_PTR_CAST(space)));

    space_str = MPIU_Strdup(str);
    MPIU_ERR_CHKANDJUMP1((space_str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "imported string");
    
    MPIU_Free(space->base);
    space->base = space_str;
    space->size = strlen(str) + 1;
    space->eod = space->size - 1;
    space->pos = 0;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: space=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, MPIG_PTR_CAST(space), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_strspace_import_string);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_strspace_import_string() */
/**********************************************************************************************************************************
						    END STRING SPACE ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN COMMUNICATION MODULE ROUTINES
**********************************************************************************************************************************/
/*
 * void mpig_cm_vtable_last_entry(none)
 *
 * this routine serves as the last function in the CM virtual table.  its purpose is to help detect when a communication
 * module's vtable has not be updated when a function be added or removed from the table.  it is not fool proof as it requires
 * the type signature not to match, but there should be few (if any) routines with such a type signature.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vtable_last_entry
char * mpig_cm_vtable_last_entry(int foo, float bar, const short * baz, char bif)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vtable_last_entry);

    MPIG_UNUSED_ARG(foo);
    MPIG_UNUSED_ARG(bar);
    MPIG_UNUSED_ARG(baz);
    MPIG_UNUSED_ARG(bif);
    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vtable_last_entry);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: mpig_cm_vtable_last_entry called.  aborting program"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vtable_last_entry);
    MPID_Abort(NULL, MPI_SUCCESS, 13, "FATAL ERROR: mpig_cm_vtable_last_entry called.  Aborting Program.");
    return NULL;
}
/**********************************************************************************************************************************
						END COMMUNICATION MODULE ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
                                                    BEGIN USAGE STAT ROUTINES
**********************************************************************************************************************************/
/*
 *  base64 encode a string, string may not be null terminated
 *
 */
#if defined(HAVE_GLOBUS_USAGE_MODULE)
static void
mpig_usage_base64_encode(
    const unsigned char *               inbuf,
    int                                 in_len,
    unsigned char *                     outbuf,
    int *                               out_len)
{
    int                                 i;
    int                                 j;
    unsigned char                       c = 0;
    char                                padding = '=';
    const char *                        base64_charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (i=0,j=0; i < in_len; i++)
    {
        switch (i%3)
        {
            case 0:
                outbuf[j++] = base64_charset[inbuf[i]>>2];
                c = (inbuf[i]&3)<<4;
                break;
            case 1:
                outbuf[j++] = base64_charset[c|inbuf[i]>>4];
                c = (inbuf[i]&15)<<2;
                break;
            case 2:
                outbuf[j++] = base64_charset[c|inbuf[i]>>6];
                outbuf[j++] = base64_charset[inbuf[i]&63];
                c = 0;
                break;
            default:
                globus_assert(0);
                break;
        }
    }
    if (i%3)
    {
        outbuf[j++] = base64_charset[c];
    }
    switch (i%3)
    {
        case 1:
            outbuf[j++] = padding;
        case 2:
            outbuf[j++] = padding;
    }

    outbuf[j] = '\0';
    *out_len = j;

    return;
}
#endif

/*
 * void mpig_usage_finalize(none)
 *
 */

#define MPIG_USAGE_ID 8
#define MPIG_USAGE_PACKET_VERSION 0


#undef FUNCNAME
#define FUNCNAME mpig_usage_init
void mpig_usage_init(void)
{
#   if defined(HAVE_GLOBUS_USAGE_MODULE)
    {
	/* start timer for usage statistics */
	gettimeofday(&mpig_process.start_time, NULL);
    }
#   endif
}

#undef FUNCNAME
#define FUNCNAME mpig_usage_finalize
void mpig_usage_finalize(void)
{
#   if defined(HAVE_GLOBUS_USAGE_MODULE)
    {
	globus_usage_stats_handle_t mpig_usage_handle;
	int rc;
	globus_result_t result;
	struct timeval end_time;
	int64_t * total_nbytes = 0;
	int64_t * total_nbytesv = 0;
	int i;
	char ver_b[32];
	char start_b[32];
	char end_b[32];
	char nprocs_b[32];
	char test_b[32];
	char nbytesv_b[32];
	char nbytes_b[32];
	unsigned char fnmap_b[4096];
	int fnmap_b_len;
	unsigned char fnmap[MPIG_FUNC_CNT_NUMFUNCS * 2 * sizeof(int)];
	unsigned char * ptr;
	int total_function_count[MPIG_FUNC_CNT_NUMFUNCS] = { 0 };

 
	if(mpig_process.my_pg_rank == 0)
	{
        
	    total_nbytes = (int64_t *) MPIU_Malloc(mpig_process.my_pg_size * sizeof(int64_t));
	    total_nbytesv = (int64_t *) MPIU_Malloc(mpig_process.my_pg_size * sizeof(int64_t));
	}

	MPIR_Nest_incr();
	{
	    NMPI_Gather(&mpig_process.nbytes_sent, sizeof(int64_t), MPI_BYTE, total_nbytes, sizeof(int64_t), MPI_BYTE,
		0, MPI_COMM_WORLD);

	    NMPI_Gather(&mpig_process.vmpi_nbytes_sent, sizeof(int64_t), MPI_BYTE, total_nbytesv, sizeof(int64_t), MPI_BYTE,
		0, MPI_COMM_WORLD);

	    NMPI_Reduce(mpig_process.function_count, total_function_count, MPIG_FUNC_CNT_NUMFUNCS, MPI_INT, MPI_SUM,
		0, MPI_COMM_WORLD);
	}
	MPIR_Nest_decr();

	if(mpig_process.my_pg_rank == 0)
	{
	    int64_t x;
        
	    mpig_process.nbytes_sent = 0; 
	    for(i = 0; i < mpig_process.my_pg_size; i++)
	    {
		mpig_dc_get_int64(MPIG_MY_ENDIAN/*endianness_of(i)*/, &total_nbytes[i], &x);
		mpig_process.nbytes_sent += x;
	    }

	    mpig_process.vmpi_nbytes_sent = 0; 
	    for(i = 0; i < mpig_process.my_pg_size; i++)
	    {
		mpig_dc_get_int64(MPIG_MY_ENDIAN/*endianness_of(i) */, &total_nbytesv[i], &x);
		mpig_process.vmpi_nbytes_sent += x;
	    }

	    MPIU_Free(total_nbytes);
	    MPIU_Free(total_nbytesv);

	    gettimeofday(&end_time, NULL);
        
	    rc = globus_module_activate(GLOBUS_USAGE_MODULE);
	    if(rc != 0)
	    {
		goto err;
	    }
        
	    result = globus_usage_stats_handle_init(&mpig_usage_handle, MPIG_USAGE_ID, MPIG_USAGE_PACKET_VERSION,
		"mikelink.com:4811");
	    if(result != GLOBUS_SUCCESS)
	    {
		globus_module_deactivate(GLOBUS_USAGE_MODULE);
		goto err;
	    }

	    /* will need to encode these into our own buffer in order to fit
	       the function map */

	    snprintf(ver_b, sizeof(ver_b), MPIG_MPICH2_VERSION);
	    snprintf(start_b, sizeof(start_b), "%d.%d", (int) mpig_process.start_time.tv_sec,
		(int) mpig_process.start_time.tv_usec);
	    snprintf(end_b, sizeof(end_b), "%d.%d", (int) end_time.tv_sec, (int) end_time.tv_usec);
	    snprintf(nprocs_b, sizeof(nprocs_b), "%d", mpig_process.my_pg_size);
	    snprintf(test_b, sizeof(test_b), "%s", getenv("MPIG_TEST") ? "1" : "0");
	    snprintf(nbytesv_b, sizeof(nbytesv_b), "%"GLOBUS_OFF_T_FORMAT, mpig_process.vmpi_nbytes_sent);
	    snprintf(nbytes_b, sizeof(nbytes_b), "%"GLOBUS_OFF_T_FORMAT, mpig_process.nbytes_sent);
        
        
	    /* write out the function counts, then base64 encode that buffer.
	     * max size of the binary buffer (8 bytes for each function) is about
	     * 1800 bytes, which gives ~2400 bytes in base64... We have about 1300
	     * bytes in the usage packet to play with, so we can handle ~120
	     * unique function calls.  If we can rely on the total count of 
	     * functions staying under 255 (currently 241), we can shave it down 
	     * to 6 bytes per function if needed, and then we'd be able to handle 
	     * ~160 different calls in a given app.  If we care about more than 
	     * that we'll need to get smarter with the encoding (compression)
	     * or just add binary support to the c usage lib.
	     */
         
	    memset(fnmap, 0, sizeof(fnmap));
	    ptr = fnmap;
	    for(i = 0; i < MPIG_FUNC_CNT_NUMFUNCS; i++)
	    {
		if(total_function_count[i] > 0)
		{
		    memcpy(ptr, &i, sizeof(int));
		    ptr += sizeof(int);
		    memcpy(ptr, &total_function_count[i], sizeof(int));
		    ptr += sizeof(int);
		}
	    }

	    mpig_usage_base64_encode(fnmap, ptr - fnmap, fnmap_b, &fnmap_b_len);
        
        
	    result = globus_usage_stats_send(
		mpig_usage_handle,
		8,
		"MPICHVER", ver_b,
		"START", start_b,
		"END", end_b,
		"NPROCS", nprocs_b,
		"NBYTES", nbytes_b,
		"NBYTESV", nbytesv_b,
		"TEST", test_b,
		"FNMAP", fnmap_b);
	    if(result != GLOBUS_SUCCESS)
	    {
		/* debug output */
	    }
        
	    globus_usage_stats_handle_destroy(mpig_usage_handle);
    
	}
	return;
        
      err:
	return;
    }
#   endif
}

/**********************************************************************************************************************************
                                                     END USAGE STAT ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
                                               BEGIN BASIC DATA STRUCTURES SECTON
**********************************************************************************************************************************/
#include "mpig_bds_genq.i"
/**********************************************************************************************************************************
                                                END BASIC DATA STRUCTURES SECTON
**********************************************************************************************************************************/


/**********************************************************************************************************************************
                                                   BEGIN DEVICE THREADS SECTON
**********************************************************************************************************************************/
#include "mpig_thread.i"
/**********************************************************************************************************************************
                                                    END DEVICE THREADS SECTON
**********************************************************************************************************************************/
