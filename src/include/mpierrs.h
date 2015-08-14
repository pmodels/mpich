/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIERRS_H_INCLUDED
#define MPIERRS_H_INCLUDED
/* ------------------------------------------------------------------------- */
/* mpierrs.h */
/* ------------------------------------------------------------------------- */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_COLL_ALIAS_CHECK
      category    : COLLECTIVE
      type        : int
      default     : 1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable checking of aliasing in collective operations

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Error checking (see --enable-error-checking for control of this) */
#ifdef HAVE_ERROR_CHECKING

#define MPID_ERROR_LEVEL_ALL 1
#define MPID_ERROR_LEVEL_RUNTIME 2
/* Use MPID_ERROR_DECL to wrap declarations that are needed only when
   error checking is turned on */
#define MPID_ERROR_DECL(a) a

#if HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_ALL
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#define MPID_ELSE_ERROR_CHECKS
#elif HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_RUNTIME
#define MPID_BEGIN_ERROR_CHECKS if (MPIR_Process.do_error_checks) {
#define MPID_ELSE_ERROR_CHECKS }else{
#define MPID_END_ERROR_CHECKS }
#else
#error "Unknown value for error checking"
#endif

#else
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#define MPID_ERROR_DECL(a)
#endif /* HAVE_ERROR_CHECKING */

/* 
 *  Standardized error checking macros.  These provide the correct tests for
 *  common tests.  These set err with the encoded error value.
 */
#define MPIR_ERRTEST_SEND_TAG(tag,err)                                  \
    if ((tag) < 0 || (tag) > MPIR_Process.attrs.tag_ub) {               \
        err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                   MPI_ERR_TAG, "**tag", "**tag %d", tag); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_RECV_TAG(tag,err)                                  \
    if ((tag) < MPI_ANY_TAG || (tag) > MPIR_Process.attrs.tag_ub) {     \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_TAG, "**tag", "**tag %d", tag ); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_RANK(comm_ptr,rank,err)                            \
    if ((rank) < 0 || (rank) >= (comm_ptr)->remote_size) {              \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_RANK, "**rank", "**rank %d %d", rank, \
                                    (comm_ptr)->remote_size );          \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_SEND_RANK(comm_ptr,rank,err)                       \
    if ((rank) < MPI_PROC_NULL || (rank) >= (comm_ptr)->remote_size) {  \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_RANK, "**rank", "**rank %d %d", rank, \
                                    (comm_ptr)->remote_size );          \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_RECV_RANK(comm_ptr,rank,err)                       \
    if ((rank) < MPI_ANY_SOURCE || (rank) >= (comm_ptr)->remote_size) { \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_RANK, "**rank", "**rank %d %d", rank, \
                                    (comm_ptr)->remote_size );          \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_COUNT(count,err)                           \
    if ((count) < 0) {                                          \
        err = MPIR_Err_create_code(MPI_SUCCESS,                 \
				   MPIR_ERR_RECOVERABLE,        \
				   FCNAME, __LINE__,            \
				   MPI_ERR_COUNT,               \
				   "**countneg",                \
				   "**countneg %d",             \
				   count );                     \
        goto fn_fail;                                           \
    }

#define MPIR_ERRTEST_DISP(disp,err)                             \
    if ((disp) < 0) {                                           \
        err = MPIR_Err_create_code(MPI_SUCCESS,                 \
				   MPIR_ERR_RECOVERABLE,        \
				   FCNAME, __LINE__,            \
				   MPI_ERR_DISP,                \
				   "**rmadisp", 0 );            \
        goto fn_fail;                                           \
    }

#define MPIR_ERRTEST_ALIAS(ptr1,ptr2,err)			\
    if ((ptr1)==(ptr2) && (ptr1) != MPI_BOTTOM) {		\
        err = MPIR_Err_create_code(MPI_SUCCESS,			\
				   MPIR_ERR_RECOVERABLE,	\
				   FCNAME, __LINE__,		\
				   MPI_ERR_BUFFER,		\
				   "**bufalias", 0 );		\
        goto fn_fail;                                           \
    }

/* FIXME this test is probably too broad.  Comparing the (buffer,type,count)
 * tuples is really what is needed to check for aliasing. */
#define MPIR_ERRTEST_ALIAS_COLL(ptr1,ptr2,err)			\
    if (MPIR_CVAR_COLL_ALIAS_CHECK && (ptr1)==(ptr2)) {        \
        err = MPIR_Err_create_code(MPI_SUCCESS,			\
				   MPIR_ERR_RECOVERABLE,	\
				   FCNAME, __LINE__,		\
				   MPI_ERR_BUFFER,		\
				   "**bufalias", 0 );		\
        goto fn_fail;                                           \
    }

#define MPIR_ERRTEST_ARGNULL(arg,arg_name,err)                  \
    if (!(arg)) {                                               \
        err = MPIR_Err_create_code(MPI_SUCCESS,                 \
                                   MPIR_ERR_RECOVERABLE,        \
                                   FCNAME, __LINE__,            \
                                   MPI_ERR_ARG,                 \
                                   "**nullptr",                 \
                                   "**nullptr %s",              \
                                   arg_name );                  \
        goto fn_fail;                                           \
    } 

#define MPIR_ERRTEST_ARGNEG(arg,arg_name,err)                           \
    if ((arg) < 0) {                                                    \
        err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,   \
                                   FCNAME, __LINE__, MPI_ERR_ARG,       \
                                   "**argneg",                          \
                                   "**argneg %s %d", arg_name, arg );   \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_ARGNONPOS(arg,arg_name,err,errclass)      \
    if ((arg) <= 0) {                                           \
        err = MPIR_Err_create_code(MPI_SUCCESS,                 \
                                   MPIR_ERR_RECOVERABLE,        \
                                   FCNAME, __LINE__,            \
                                   errclass,                    \
                                   "**argnonpos",               \
                                   "**argnonpos %s %d",         \
                                   arg_name, arg );             \
        goto fn_fail;                                           \
    }

/* An intracommunicator must have a root between 0 and local_size-1. */
/* intercomm can be between MPI_PROC_NULL (or MPI_ROOT) and remote_size-1 */
#define MPIR_ERRTEST_INTRA_ROOT(comm_ptr,root,err)                      \
    if ((root) < 0 || (root) >= (comm_ptr)->local_size) {               \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_ROOT, "**root", "**root %d", root ); \
        goto fn_fail;                                                   \
    }

/* We use -2 (MPI_PROC_NULL and MPI_ROOT are negative) for the
   intercomm test */
#define MPIR_ERRTEST_INTER_ROOT(comm_ptr,root,err)                      \
    if ((root) < -3 || (root) >= (comm_ptr)->remote_size) {             \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_ROOT, "**root", "**root %d", root ); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_PERSISTENT(reqp,err)                               \
    if ((reqp)->kind != MPID_PREQUEST_SEND && reqp->kind != MPID_PREQUEST_RECV) { \
        err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                   MPI_ERR_REQUEST, "**requestnotpersist", 0 ); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_PERSISTENT_ACTIVE(reqp,err)                        \
    if (((reqp)->kind == MPID_PREQUEST_SEND ||                          \
         reqp->kind == MPID_PREQUEST_RECV) && reqp->partner_request != NULL) { \
        err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                   MPI_ERR_REQUEST, "**requestpersistactive", 0 ); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_COMM_INTRA(comm_ptr, err )                         \
    if ((comm_ptr)->comm_kind != MPID_INTRACOMM) {                      \
        err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                   MPI_ERR_COMM,"**commnotintra",0);    \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_COMM_TAG(tag,err)                                  \
    if ((tag) < 0 || (tag) > MPIR_Process.attrs.tag_ub) {               \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_TAG, "**tag", "**tag %d", tag); \
        goto fn_fail;                                                   \
    }

/* Tests for totally meaningless datatypes first, then for
 * MPI_DATATYPE_NULL as a separate case.
 */
#define MPIR_ERRTEST_DATATYPE(datatype, name_, err_)                    \
    {                                                                   \
        if (HANDLE_GET_MPI_KIND(datatype) != MPID_DATATYPE ||           \
            (HANDLE_GET_KIND(datatype) == HANDLE_KIND_INVALID &&        \
             datatype != MPI_DATATYPE_NULL))                            \
        {                                                               \
            err_ = MPIR_Err_create_code(MPI_SUCCESS,                    \
                                        MPIR_ERR_RECOVERABLE,           \
                                        FCNAME, __LINE__,               \
                                        MPI_ERR_TYPE,                   \
                                        "**dtype", 0 );                 \
            goto fn_fail;                                               \
        }                                                               \
        if (datatype == MPI_DATATYPE_NULL)                              \
        {                                                               \
            err_ = MPIR_Err_create_code(MPI_SUCCESS,                    \
                                        MPIR_ERR_RECOVERABLE,           \
                                        FCNAME, __LINE__,               \
                                        MPI_ERR_TYPE,                   \
                                        "**dtypenull",                  \
                                        "**dtypenull %s",               \
                                        name_);                         \
            goto fn_fail;                                               \
        }                                                               \
    }

#define MPIR_ERRTEST_TYPE_RMA_ATOMIC(datatype_, err_)                   \
    do {                                                                \
        if (!MPIR_Type_is_rma_atomic(datatype_)) {                      \
            err_ = MPIR_Err_create_code(MPI_SUCCESS,                    \
                                        MPIR_ERR_RECOVERABLE,           \
                                        FCNAME, __LINE__,               \
                                        MPI_ERR_TYPE,                   \
                                        "**rmatypenotatomic",           \
                                        "**rmatypenotatomic %D",        \
                                        datatype);                      \
            goto fn_fail;                                               \
        }                                                               \
    } while (0)

#define MPIR_ERRTEST_WIN_FLAVOR(win_, flavor_, err_)                    \
    do {                                                                \
        if ((win_)->create_flavor != (flavor_)) {                       \
            MPIR_ERR_SETANDSTMT1((err_), MPI_ERR_RMA_FLAVOR,            \
                                goto fn_fail, "**winflavor",            \
                                 "**winflavor %s", #flavor_);           \
        }                                                               \
    } while (0)

#define MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf,count,err)                 \
    if (count > 0 && sendbuf == MPI_IN_PLACE) {                         \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_BUFFER, "**sendbuf_inplace", 0 ); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf,count,err)                 \
    if (count > 0 && recvbuf == MPI_IN_PLACE) {                         \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_BUFFER, "**recvbuf_inplace", 0 ); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_BUF_INPLACE(buf,count,err)                         \
    if (count > 0 && buf == MPI_IN_PLACE) {                             \
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
                                    MPI_ERR_BUFFER, "**buf_inplace", 0 ); \
        goto fn_fail;                                                   \
    }

#define MPIR_ERRTEST_NAMED_BUF_INPLACE(buf, bufname, count, err)       \
    do {                                                               \
        if (count > 0 && buf == MPI_IN_PLACE) {                        \
            MPIR_ERR_SETANDJUMP1(err, MPI_ERR_BUFFER, "**buf_inplace", \
                                 "**buf_inplace %s", bufname);         \
        }                                                              \
    } while (0)

/*
 * Check that the triple (buf,count,datatype) does not specify a null
 * buffer.  This does not guarantee that the buffer is valid but does
 * catch the most common problems.
 * Question:
 * Should this be an (inlineable) routine?  
 * Since it involves extracting the datatype pointer for non-builtin
 * datatypes, should it take a dtypeptr argument (valid only if not
 * builtin)?
 */
#define MPIR_ERRTEST_USERBUFFER(buf,count,dtype,err)			\
    if (count > 0 && buf == 0) {					\
        int ferr = 0;							\
        if (HANDLE_GET_KIND(dtype) == HANDLE_KIND_BUILTIN) { ferr=1; }	\
        else {								\
            MPI_Aint errsize;                                           \
            MPID_Datatype *errdtypeptr;					\
            MPID_Datatype_get_ptr(dtype,errdtypeptr);			\
            MPID_Datatype_get_size_macro(dtype,errsize);                \
            if (errdtypeptr && errdtypeptr->true_lb == 0 &&             \
                errsize > 0) { ferr=1; }                        	\
        }								\
        if (ferr) {							\
            err = MPIR_Err_create_code(MPI_SUCCESS,			\
				       MPIR_ERR_RECOVERABLE,		\
				       FCNAME, __LINE__,		\
				       MPI_ERR_BUFFER,			\
				       "**bufnull", 0 );		\
            goto fn_fail;                                               \
	}								\
    }

/* The following are placeholders.  We haven't decided yet whether these
   should take a handle or pointer, or if they should take a handle and return 
   a pointer if the handle is valid.  These need to be rationalized with the
   MPID_xxx_valid_ptr and MPID_xxx_get_ptr.

   [BRT] They should not take a handle and return a pointer if they will be
   placed inside of a #ifdef HAVE_ERROR_CHECKING block.  Personally, I think
   the macros should take handles.  We already have macros for validating
   pointers to various objects.
*/
/* --BEGIN ERROR MACROS-- */
#define MPIR_ERRTEST_VALID_HANDLE(handle_,kind_,err_,errclass_,gmsg_) { \
        if (HANDLE_GET_MPI_KIND(handle_) != kind_ ||                    \
            HANDLE_GET_KIND(handle_) == HANDLE_KIND_INVALID) {          \
            MPIR_ERR_SETANDSTMT(err_,errclass_,goto fn_fail,gmsg_);     \
        }                                                               \
    }
/* --END ERROR MACROS-- */

#define MPIR_ERRTEST_OP(op,err)                                                 \
    do {                                                                        \
        if (op == MPI_OP_NULL) {                                                \
            MPIR_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnull");        \
        }                                                                       \
        else if (op == MPI_NO_OP || op == MPI_REPLACE) {                        \
            MPIR_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnotallowed");  \
        }                                                                       \
        else {                                                                  \
            MPIR_ERRTEST_VALID_HANDLE(op,MPID_OP,err,MPI_ERR_OP,"**op");        \
        }                                                                       \
    } while (0)

#define MPIR_ERRTEST_OP_ACC(op,err)                                             \
    do {                                                                        \
        if (op == MPI_OP_NULL) {                                                \
            MPIR_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnull");        \
        }                                                                       \
        else if (op == MPI_NO_OP) {                                             \
            MPIR_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnotallowed");  \
        }                                                                       \
        else {                                                                  \
            MPIR_ERRTEST_VALID_HANDLE(op,MPID_OP,err,MPI_ERR_OP,"**op");        \
        }                                                                       \
        if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {                       \
            MPIR_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnotpredefined"); \
        }                                                                       \
    } while (0)

#define MPIR_ERRTEST_OP_GACC(op,err)                                            \
    do {                                                                        \
        if (op == MPI_OP_NULL) {                                                \
            MPIR_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnull");        \
        }                                                                       \
        else {                                                                  \
            MPIR_ERRTEST_VALID_HANDLE(op,MPID_OP,err,MPI_ERR_OP,"**op");        \
        }                                                                       \
        if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {                       \
            MPIR_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnotpredefined"); \
        }                                                                       \
    } while (0)

#define MPIR_ERRTEST_GROUP(group,err)                                   \
    if (group == MPI_GROUP_NULL) {                                      \
        MPIR_ERR_SETANDSTMT(err,MPI_ERR_GROUP,goto fn_fail,"**groupnull"); \
    }                                                                   \
    else {                                                              \
        MPIR_ERRTEST_VALID_HANDLE(group,MPID_GROUP,err,MPI_ERR_GROUP,"**group"); \
    }

#define MPIR_ERRTEST_COMM(comm_, err_)					\
    {									\
        if ((comm_) == MPI_COMM_NULL)					\
        {                                                               \
            MPIR_ERR_SETANDSTMT((err_), MPI_ERR_COMM,goto fn_fail,"**commnull"); \
        }                                                               \
        else								\
        {                                                               \
            MPIR_ERRTEST_VALID_HANDLE((comm_), MPID_COMM, (err_), MPI_ERR_COMM, "**comm"); \
        }                                                               \
    }

#define MPIR_ERRTEST_WIN(win_, err_)					\
    {									\
        if ((win_) == MPI_WIN_NULL)                                     \
        {                                                               \
            MPIR_ERR_SETANDSTMT((err_), MPI_ERR_WIN,goto fn_fail, "**winnull"); \
        }                                                               \
        else								\
        {                                                               \
            MPIR_ERRTEST_VALID_HANDLE((win_), MPID_WIN, (err_), MPI_ERR_WIN, "**win"); \
        }                                                               \
    }

#define MPIR_ERRTEST_REQUEST(request_, err_)				\
    {									\
        if ((request_) == MPI_REQUEST_NULL)                             \
        {                                                               \
            MPIR_ERR_SETANDSTMT((err_), MPI_ERR_REQUEST,goto fn_fail, "**requestnull"); \
        }                                                               \
        else								\
        {                                                               \
            MPIR_ERRTEST_VALID_HANDLE((request_), MPID_REQUEST, (err_), MPI_ERR_REQUEST, "**request"); \
        }                                                               \
    }

#define MPIR_ERRTEST_REQUEST_OR_NULL(request_, err_)			\
    {									\
        if ((request_) != MPI_REQUEST_NULL)                             \
        {                                                               \
            MPIR_ERRTEST_VALID_HANDLE((request_), MPID_REQUEST, (err_), MPI_ERR_REQUEST, "**request"); \
        }                                                               \
    }

/* This macro does *NOT* jump to fn_fail - all uses check mpi_errno */
#define MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(request_, i_, err_)		\
    {									\
        if ((request_) != MPI_REQUEST_NULL)                             \
        {                                                               \
            if (HANDLE_GET_MPI_KIND(request_) != MPID_REQUEST) {        \
                MPIR_ERR_SETANDSTMT2(err_,MPI_ERR_REQUEST,goto fn_fail, \
                                     "**request_invalid_kind","**request_invalid_kind %d %d", \
                                     i_, HANDLE_GET_MPI_KIND(request_)); \
            }                                                           \
            else if (HANDLE_GET_KIND(request_) == HANDLE_KIND_INVALID) { \
                MPIR_ERR_SETANDSTMT1(err_,MPI_ERR_REQUEST,goto fn_fail, \
                                     "**request","**request %d", i_ );  \
            }                                                           \
        }                                                               \
    }

#define MPIR_ERRTEST_ERRHANDLER(errhandler_,err_)			\
    if (errhandler_ == MPI_ERRHANDLER_NULL) {				\
        MPIR_ERR_SETANDSTMT(err_,MPI_ERR_ARG,goto fn_fail,"**errhandlernull"); \
    }									\
    else {								\
        MPIR_ERRTEST_VALID_HANDLE(errhandler_,MPID_ERRHANDLER,		\
				  err_,MPI_ERR_ARG,"**errhandler");	\
    }

#define MPIR_ERRTEST_INFO(info_, err_)					\
    {									\
        if ((info_) == MPI_INFO_NULL)					\
        {                                                               \
            MPIR_ERR_SETANDSTMT(err_, MPI_ERR_ARG,goto fn_fail, "**infonull"); \
        }                                                               \
        else								\
        {                                                               \
            MPIR_ERRTEST_VALID_HANDLE((info_), MPID_INFO, (err_), MPI_ERR_ARG, "**info"); \
        }                                                               \
    }

#define MPIR_ERRTEST_INFO_OR_NULL(info_, err_)				\
    {									\
        if ((info_) != MPI_INFO_NULL)					\
        {                                                               \
            MPIR_ERRTEST_VALID_HANDLE((info_), MPID_INFO, (err_), MPI_ERR_ARG, "**info"); \
        }                                                               \
    }

#define MPIR_ERRTEST_KEYVAL(keyval_, object_, objectdesc_, err_)	\
    {									\
        if ((keyval_) == MPI_KEYVAL_INVALID)				\
        {                                                               \
            MPIR_ERR_SETANDSTMT(err_, MPI_ERR_KEYVAL,goto fn_fail, "**keyvalinvalid"); \
        }                                                               \
        else if (HANDLE_GET_MPI_KIND(keyval_) != MPID_KEYVAL)		\
        {                                                               \
            MPIR_ERR_SETANDSTMT(err_, MPI_ERR_KEYVAL,goto fn_fail, "**keyval"); \
        }                                                               \
        else if ((((keyval_) & 0x03c00000) >> 22) != (object_))		\
        {                                                               \
            MPIR_ERR_SETANDSTMT1(err_, MPI_ERR_KEYVAL,goto fn_fail, "**keyvalobj", \
                                 "**keyvalobj %s", (objectdesc_));      \
        }                                                               \
    }

#define MPIR_ERRTEST_KEYVAL_PERM(keyval_, err_)				\
    {									\
        if (HANDLE_GET_MPI_KIND(keyval_) == MPID_KEYVAL &&              \
            HANDLE_GET_KIND(keyval_) == HANDLE_KIND_BUILTIN)		\
        {                                                               \
            MPIR_ERR_SETANDSTMT(err_, MPI_ERR_KEYVAL,goto fn_fail, "**permattr"); \
        }                                                               \
    }

#ifdef HAVE_ERROR_CHECKING
#define MPIR_ERRTEST_MPIT_INITIALIZED(err_) \
    do { \
        if (!MPIR_T_is_initialized()) { \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_NOT_INITIALIZED, goto fn_fail, "**mpitinit"); \
        } \
    } while (0)
#else
#define MPIR_ERRTEST_MPIT_INITIALIZED(err_)
#endif

#define MPIR_ERRTEST_CAT_INDEX(index_,err_)  \
    do {   \
        if ((index_) < 0 || (index_) >= utarray_len(cat_table)) \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_INDEX, goto fn_fail, "**catindex");  \
        }  \
    } while (0)

#define MPIR_ERRTEST_ENUM_HANDLE(handle_, err_)  \
    do {   \
        if ((handle_) == MPI_T_ENUM_NULL)  \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_HANDLE, goto fn_fail, "**enumhandlenull");  \
        }  \
        else if ((handle_)->kind != MPIR_T_ENUM_HANDLE) \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_HANDLE, goto fn_fail, "**enumhandle"); \
        }  \
    } while (0)

#define MPIR_ERRTEST_ENUM_ITEM(enum_, index_, err_)  \
    do {   \
        if ((index_) < 0 || (index_) >= utarray_len((enum_)->items)) \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_ITEM, goto fn_fail, "**itemindex");  \
        }  \
    } while (0)

#define MPIR_ERRTEST_CVAR_INDEX(index_,err_)  \
    do {   \
        if ((index_) < 0 || (index_) >= utarray_len(cvar_table)) \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_INDEX, goto fn_fail, "**cvarindex");  \
        }  \
    } while (0)

#define MPIR_ERRTEST_CVAR_HANDLE(handle_, err_)  \
    do {   \
        if ((handle_) == MPI_T_CVAR_HANDLE_NULL)  \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_HANDLE, goto fn_fail, "**cvarhandlenull");  \
        }  \
        else if ((handle_)->kind != MPIR_T_CVAR_HANDLE) \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_HANDLE, goto fn_fail, "**cvarhandle"); \
        }  \
    } while (0)

#define MPIR_ERRTEST_PVAR_INDEX(index_,err_)  \
    do {   \
        if ((index_) < 0 || (index_) >= utarray_len(pvar_table)) \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_INDEX, goto fn_fail, "**pvarindex");  \
        }  \
    } while (0)

#define MPIR_ERRTEST_PVAR_HANDLE(handle_, err_) \
    do {   \
        if (handle_ == MPI_T_PVAR_HANDLE_NULL) \
        {  \
            MPIR_ERR_SETANDSTMT(err_,MPI_T_ERR_INVALID_HANDLE, goto fn_fail,"**pvarhandlenull"); \
        }  \
        else if ((handle_)->kind != MPIR_T_PVAR_HANDLE)  \
        {  \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_HANDLE, goto fn_fail, "**pvarhandle");  \
        }  \
    } while (0)

#define MPIR_ERRTEST_PVAR_SESSION(session_,err_)  \
    do {    \
        if ((session_) == MPI_T_PVAR_SESSION_NULL)  \
        {   \
            MPIR_ERR_SETANDSTMT(err_,MPI_T_ERR_INVALID_SESSION, goto fn_fail,"**pvarsessionnull");  \
        }   \
        else if ((session_)->kind != MPIR_T_PVAR_SESSION) \
        {   \
            MPIR_ERR_SETANDSTMT(err_, MPI_T_ERR_INVALID_SESSION, goto fn_fail, "**pvarsession");  \
        }   \
    } while (0)

/* some simple memcpy aliasing checks */
#define MPIR_ERR_CHKMEMCPYANDSTMT(err_,stmt_,src_,dst_,len_) \
    MPIR_ERR_CHKANDSTMT3(MPIU_MEM_RANGES_OVERLAP((dst_),(len_),(src_),(len_)),err_,MPI_ERR_INTERN,stmt_,"**memcpyalias","**memcpyalias %p %p %L",(src_),(dst_),(long long)(len_))
#define MPIR_ERR_CHKMEMCPYANDJUMP(err_,src_,dst_,len_) \
    MPIR_ERR_CHKMEMCPYANDSTMT((err_),goto fn_fail,(src_),(dst_),(len_))

/* Special MPI error "class/code" for out of memory */
/* FIXME: not yet done */
#define MPIR_ERR_MEMALLOCFAILED MPI_ERR_INTERN

/* 
 * Standardized error setting and checking macros
 * These are intended to simplify the insertion of standardized error
 * checks
 *
 */
/* --BEGIN ERROR MACROS-- */
#ifdef HAVE_ERROR_CHECKING
#define MPIR_ERR_POP(err_) \
    MPIR_ERR_SETANDSTMT(err_,MPI_ERR_OTHER,goto fn_fail,"**fail")
#define MPIR_ERR_POP_LABEL(err_, label_) \
    MPIR_ERR_SETANDSTMT(err_,MPI_ERR_OTHER,goto label_,"**fail")
#define MPIR_ERR_POPFATAL(err_) \
    MPIR_ERR_SETFATALANDSTMT(err_,MPI_ERR_OTHER,goto fn_fail,"**fail")
#define MPIR_ERR_POPFATAL_LABEL(err_, label_)                           \
    MPIR_ERR_SETFATALANDSTMT(err_,MPI_ERR_OTHER,goto label_,"**fail")
#else
#define MPIR_ERR_POP(err_) goto fn_fail
#define MPIR_ERR_POP_LABEL(err_, label_) goto label_
#define MPIR_ERR_POPFATAL(err_) goto fn_fail
#define MPIR_ERR_POPFATAL_LABEL(err_, label_) goto label_
#endif

/* If you add any macros to this list, make sure that you update
 maint/extracterrmsgs to handle the additional macros (see the hash 
 KnownErrRoutines in that script) 
 ERR_SETSIMPLE is like ERR_SET except that it just sets the error, it 
 doesn't add it to an existing error.  This is appropriate in cases
 where there can be no pre-existing error, and MPI_SUCCESS is needed for the
 first argument to MPIR_Err_create_code .
*/
#ifdef HAVE_ERROR_CHECKING
#define MPIR_ERR_SETSIMPLE(err_,class_,msg_)	\
    err_ = MPIR_Err_create_code( MPI_SUCCESS,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, msg_, 0 )
#define MPIR_ERR_SET(err_,class_,msg_) \
    err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, msg_, 0 )
#define MPIR_ERR_SET1(err_,class_,gmsg_,smsg_,arg1_) \
    err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_ )
#define MPIR_ERR_SET2(err_,class_,gmsg_,smsg_,arg1_,arg2_) \
    err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_, arg2_ )
#define MPIR_ERR_SETANDSTMT(err_,class_,stmt_,msg_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, msg_, 0 ); stmt_ ;} while (0)
#define MPIR_ERR_SETANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_ ); stmt_ ;} while (0)
#define MPIR_ERR_SETANDSTMT2(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_, arg2_ ); stmt_ ;} while (0)
#define MPIR_ERR_SETANDSTMT3(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_, arg3_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_, arg2_, arg3_ ); stmt_ ;} while (0)
#define MPIR_ERR_SETANDSTMT4(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_, arg3_, arg4_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
	    __LINE__, class_, gmsg_, smsg_, arg1_, arg2_, arg3_, arg4_ ); stmt_ ;} while (0)

#define MPIR_ERR_SETFATALSIMPLE(err_,class_,msg_) \
    err_ = MPIR_Err_create_code( MPI_SUCCESS,MPIR_ERR_FATAL,FCNAME,     \
           __LINE__, class_, msg_, 0 )
#define MPIR_ERR_SETFATAL(err_,class_,msg_) \
    err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, msg_, 0 )
#define MPIR_ERR_SETFATAL1(err_,class_,gmsg_,smsg_,arg1_) \
    err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_ )
#define MPIR_ERR_SETFATAL2(err_,class_,gmsg_,smsg_,arg1_,arg2_) \
    err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_, arg2_ )
#define MPIR_ERR_SETFATALANDSTMT(err_,class_,stmt_,msg_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, msg_, 0 ); stmt_ ;} while (0)
#define MPIR_ERR_SETFATALANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_ ); stmt_ ;} while (0)
#define MPIR_ERR_SETFATALANDSTMT2(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_, arg2_ ); stmt_ ;} while (0)
#define MPIR_ERR_SETFATALANDSTMT3(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_, arg3_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_, arg2_, arg3_ ); stmt_ ;} while (0)
#define MPIR_ERR_SETFATALANDSTMT4(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_, arg3_, arg4_) \
    do {err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
	    __LINE__, class_, gmsg_, smsg_, arg1_, arg2_, arg3_, arg4_ ); stmt_ ;} while (0)
#define MPIR_ERR_ADD(err_, newerr_) \
    do {(err_) = MPIR_Err_combine_codes((err_), (newerr_));} while (0)
#else
/* Simply set the class, being careful not to override a previously
   set class. */
#define MPIR_ERR_SETSIMPLE(err_,class_,msg_)	\
    do {err_ = class_;} while (0)
#define MPIR_ERR_SET(err_,class_,msg_) \
    do {if (!err_){err_=class_;}} while (0)
#define MPIR_ERR_SET1(err_,class_,gmsg_,smsg_,arg1_) \
      MPIR_ERR_SET(err_,class_,msg_)
#define MPIR_ERR_SET2(err_,class_,gmsg_,smsg_,arg1_,arg2_) \
      MPIR_ERR_SET(err_,class_,msg_)
#define MPIR_ERR_SETANDSTMT(err_,class_,stmt_,msg_) \
    do {if (!err_){err_=class_;} stmt_;} while (0)
#define MPIR_ERR_SETANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)
#define MPIR_ERR_SETANDSTMT2(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)
#define MPIR_ERR_SETANDSTMT3(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)
#define MPIR_ERR_SETANDSTMT4(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)

#define MPIR_ERR_SETFATALSIMPLE(err_,class_,msg_)	\
    do {err_ = class_;} while (0)
#define MPIR_ERR_SETFATAL(err_,class_,msg_) \
    do {if (!err_){err_=class_;}} while (0)
#define MPIR_ERR_SETFATAL1(err_,class_,gmsg_,smsg_,arg1_) \
      MPIR_ERR_SET(err_,class_,msg_)
#define MPIR_ERR_SETFATAL2(err_,class_,gmsg_,smsg_,arg1_,arg2_) \
      MPIR_ERR_SET(err_,class_,msg_)
#define MPIR_ERR_SETFATALANDSTMT(err_,class_,stmt_,msg_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,msg_)
#define MPIR_ERR_SETFATALANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)
#define MPIR_ERR_SETFATALANDSTMT2(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)
#define MPIR_ERR_SETFATALANDSTMT3(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)
#define MPIR_ERR_SETFATALANDSTMT4(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_) \
     MPIR_ERR_SETANDSTMT(err_,class_,stmt_,gmsg_)
    /* No-op - use original error class; discard newerr_ unless err is 
       MPI_SUCCESS*/
#define MPIR_ERR_ADD(err_, newerr_) \
    do {if (!err_) err_ = newerr_;} while (0)
#endif

/* The following definitions are the same independent of the choice of 
   HAVE_ERROR_CHECKING */
#define MPIR_ERR_SETANDJUMP(err_,class_,msg_) \
     MPIR_ERR_SETANDSTMT(err_,class_,goto fn_fail,msg_)
#define MPIR_ERR_SETFATALANDJUMP(err_,class_,msg_) \
     MPIR_ERR_SETFATALANDSTMT(err_,class_,goto fn_fail,msg_)
#define MPIR_ERR_CHKANDSTMT(cond_,err_,class_,stmt_,msg_) \
    do {if (cond_) { MPIR_ERR_SETANDSTMT(err_,class_,stmt_,msg_); }} while (0)
#define MPIR_ERR_CHKFATALANDSTMT(cond_,err_,class_,stmt_,msg_) \
    do {if (cond_) { MPIR_ERR_SETFATALANDSTMT(err_,class_,stmt_,msg_); }} while (0)
#define MPIR_ERR_CHKANDJUMP(cond_,err_,class_,msg_) \
     MPIR_ERR_CHKANDSTMT(cond_,err_,class_,goto fn_fail,msg_)
#define MPIR_ERR_CHKFATALANDJUMP(cond_,err_,class_,msg_) \
     MPIR_ERR_CHKFATALANDSTMT(cond_,err_,class_,goto fn_fail,msg_)

#define MPIR_ERR_SETANDJUMP1(err_,class_,gmsg_,smsg_,arg1_) \
     MPIR_ERR_SETANDSTMT1(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_)
#define MPIR_ERR_SETFATALANDJUMP1(err_,class_,gmsg_,smsg_,arg1_) \
     MPIR_ERR_SETFATALANDSTMT1(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_)
#define MPIR_ERR_CHKANDSTMT1(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_) \
    do {if (cond_) { MPIR_ERR_SETANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_); }} while (0)
#define MPIR_ERR_CHKFATALANDSTMT1(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_) \
    do {if (cond_) { MPIR_ERR_SETFATALANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_); }} while (0)
#define MPIR_ERR_CHKANDJUMP1(cond_,err_,class_,gmsg_,smsg_,arg1_) \
     MPIR_ERR_CHKANDSTMT1(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_)
#define MPIR_ERR_CHKFATALANDJUMP1(cond_,err_,class_,gmsg_,smsg_,arg1_) \
     MPIR_ERR_CHKFATALANDSTMT1(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_)

#define MPIR_ERR_SETANDJUMP2(err_,class_,gmsg_,smsg_,arg1_,arg2_) \
     MPIR_ERR_SETANDSTMT2(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_)
#define MPIR_ERR_SETFATALANDJUMP2(err_,class_,gmsg_,smsg_,arg1_,arg2_) \
     MPIR_ERR_SETFATALANDSTMT2(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_)
#define MPIR_ERR_CHKANDSTMT2(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_) \
    do {if (cond_) { MPIR_ERR_SETANDSTMT2(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_); }} while (0)
#define MPIR_ERR_CHKFATALANDSTMT2(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_) \
    do {if (cond_) { MPIR_ERR_SETFATALANDSTMT2(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_); }} while (0)
#define MPIR_ERR_CHKANDJUMP2(cond_,err_,class_,gmsg_,smsg_,arg1_,arg2_) \
     MPIR_ERR_CHKANDSTMT2(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_)
#define MPIR_ERR_CHKFATALANDJUMP2(cond_,err_,class_,gmsg_,smsg_,arg1_,arg2_) \
     MPIR_ERR_CHKFATALANDSTMT2(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_)

#define MPIR_ERR_SETANDJUMP3(err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
     MPIR_ERR_SETANDSTMT3(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_)
#define MPIR_ERR_SETFATALANDJUMP3(err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
     MPIR_ERR_SETFATALANDSTMT3(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_)
#define MPIR_ERR_CHKANDSTMT3(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
    do {if (cond_) { MPIR_ERR_SETANDSTMT3(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_); }} while (0)
#define MPIR_ERR_CHKFATALANDSTMT3(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
    do {if (cond_) { MPIR_ERR_SETFATALANDSTMT3(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_); }} while (0)
#define MPIR_ERR_CHKANDJUMP3(cond_,err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
     MPIR_ERR_CHKANDSTMT3(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_)
#define MPIR_ERR_CHKFATALANDJUMP3(cond_,err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_) \
     MPIR_ERR_CHKFATALANDSTMT3(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_)

#define MPIR_ERR_SETANDJUMP4(err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_) \
     MPIR_ERR_SETANDSTMT4(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_)
#define MPIR_ERR_SETFATALANDJUMP4(err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_) \
     MPIR_ERR_SETFATALANDSTMT4(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_)
#define MPIR_ERR_CHKANDSTMT4(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_, arg4_) \
    do {if (cond_) { MPIR_ERR_SETANDSTMT4(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_); }} while (0)
#define MPIR_ERR_CHKFATALANDSTMT4(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_, arg4_) \
    do {if (cond_) { MPIR_ERR_SETFATALANDSTMT4(err_,class_,stmt_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_); }} while (0)
#define MPIR_ERR_CHKANDJUMP4(cond_,err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_) \
     MPIR_ERR_CHKANDSTMT4(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_)
#define MPIR_ERR_CHKFATALANDJUMP4(cond_,err_,class_,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_) \
     MPIR_ERR_CHKFATALANDSTMT4(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_,arg2_,arg3_,arg4_)

#define MPIR_ERR_INTERNAL(err_, msg_)   \
    MPIR_ERR_SETFATAL1(err_, MPI_ERR_INTERN, "**intern", "**intern %s", msg_)
#define MPIR_ERR_INTERNALANDSTMT(err_, msg_, stmt_) \
    MPIR_ERR_SETANDSTMT1(err_, MPI_ERR_INTERN, stmt_, "**intern", "**intern %s", msg_)
#define MPIR_ERR_INTERNALANDJUMP(err_, msg_) \
    MPIR_ERR_INTERNALANDSTMT(err_, msg_, goto fn_fail)
#define MPIR_ERR_CHKINTERNAL(cond_, err_, msg_) \
    do {if (cond_) MPIR_ERR_INTERNALANDJUMP(err_, msg_);} while(0)

/* --END ERROR MACROS-- */

/* 
 * Special case for "is initialized".  
 * This should be used in cases where there is no
 * additional error checking
 */
#ifdef HAVE_ERROR_CHECKING
#define MPIR_ERRTEST_INITIALIZED_ORDIE()			\
do {								\
    if (OPA_load_int(&MPIR_Process.mpich_state) == MPICH_PRE_INIT || \
        OPA_load_int(&MPIR_Process.mpich_state) == MPICH_POST_FINALIZED) \
    {								\
	MPIR_Err_preOrPostInit();				\
    }                                                           \
} while (0)
#else
#define MPIR_ERRTEST_INITIALIZED_ORDIE() do {} while (0)
#endif

/* ------------------------------------------------------------------------- */
/* end of mpierrs.h */
/* ------------------------------------------------------------------------- */

#endif
