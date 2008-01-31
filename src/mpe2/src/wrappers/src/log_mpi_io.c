/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/*
    This file should be INCLUDED into log_mpi_core.c when adding
    the IO routines to the profiling list

    Also set MPE_MAX_KNOWN_STATES to 180
*/

#define MPE_FILE_OPEN_ID 128
#define MPE_FILE_CLOSE_ID 129
#define MPE_FILE_DELETE_ID 130
#define MPE_FILE_SET_SIZE_ID 131
#define MPE_FILE_PREALLOCATE_ID 132
#define MPE_FILE_GET_SIZE_ID 133
#define MPE_FILE_GET_GROUP_ID 134
#define MPE_FILE_GET_AMODE_ID 135
#define MPE_FILE_SET_INFO_ID 136
#define MPE_FILE_GET_INFO_ID 137
#define MPE_FILE_SET_VIEW_ID 138
#define MPE_FILE_GET_VIEW_ID 139
#define MPE_FILE_READ_AT_ID 140
#define MPE_FILE_READ_AT_ALL_ID 141
#define MPE_FILE_WRITE_AT_ID 142
#define MPE_FILE_WRITE_AT_ALL_ID 143
#define MPE_FILE_IREAD_AT_ID 144
#define MPE_FILE_IWRITE_AT_ID 145
#define MPE_FILE_READ_ID 146
#define MPE_FILE_READ_ALL_ID 147
#define MPE_FILE_WRITE_ID 148
#define MPE_FILE_WRITE_ALL_ID 149
#define MPE_FILE_IREAD_ID 150
#define MPE_FILE_IWRITE_ID 151
#define MPE_FILE_SEEK_ID 152
#define MPE_FILE_GET_POSITION_ID 153
#define MPE_FILE_GET_BYTE_OFFSET_ID 154
#define MPE_FILE_READ_SHARED_ID 155
#define MPE_FILE_WRITE_SHARED_ID 156
#define MPE_FILE_IREAD_SHARED_ID 157
#define MPE_FILE_IWRITE_SHARED_ID 158
#define MPE_FILE_READ_ORDERED_ID 159
#define MPE_FILE_WRITE_ORDERED_ID 160
#define MPE_FILE_SEEK_SHARED_ID 161
#define MPE_FILE_GET_POSITION_SHARED_ID 162
#define MPE_FILE_READ_AT_ALL_BEGIN_ID 163
#define MPE_FILE_READ_AT_ALL_END_ID 164
#define MPE_FILE_WRITE_AT_ALL_BEGIN_ID 165
#define MPE_FILE_WRITE_AT_ALL_END_ID 166
#define MPE_FILE_READ_ALL_BEGIN_ID 167
#define MPE_FILE_READ_ALL_END_ID 168
#define MPE_FILE_WRITE_ALL_BEGIN_ID 169
#define MPE_FILE_WRITE_ALL_END_ID 170
#define MPE_FILE_READ_ORDERED_BEGIN_ID 171
#define MPE_FILE_READ_ORDERED_END_ID 172
#define MPE_FILE_WRITE_ORDERED_BEGIN_ID 173
#define MPE_FILE_WRITE_ORDERED_END_ID 174
#define MPE_FILE_GET_TYPE_EXTENT_ID 175
#define MPE_REGISTER_DATAREP_ID 176
#define MPE_FILE_SET_ATOMICITY_ID 177
#define MPE_FILE_GET_ATOMICITY_ID 178
#define MPE_FILE_SYNC_ID 179

#if defined( HAVE_NO_MPIO_REQUEST )
#define  MPIO_Request  MPI_Request
#endif


void MPE_Init_mpi_io( void )
{
  MPE_State *state;

  state = &states[MPE_FILE_OPEN_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_open";
  state->color = "HotPink";

  state = &states[MPE_FILE_CLOSE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_close";
  state->color = "pink";

  state = &states[MPE_FILE_DELETE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_delete";
  state->color = "orchid";

  state = &states[MPE_FILE_SET_SIZE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_set_size";
  state->color = "DarkOrchid";

  state = &states[MPE_FILE_PREALLOCATE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_preallocate";
  state->color = "brown";

  state = &states[MPE_FILE_GET_SIZE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_size";
  state->color = "MediumOrchid";

  state = &states[MPE_FILE_GET_GROUP_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_group";
  state->color = "azure";

  state = &states[MPE_FILE_GET_AMODE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_amode";
  state->color = "ivory";

  state = &states[MPE_FILE_SET_INFO_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_set_info";
  state->color = "brown";

  state = &states[MPE_FILE_GET_INFO_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_info";
  state->color = "brown";

  state = &states[MPE_FILE_SET_VIEW_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_set_view";
  state->color = "turquoise";

  state = &states[MPE_FILE_GET_VIEW_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_view";
  state->color = "cyan";

  state = &states[MPE_FILE_READ_AT_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_at";
  state->color = "SeaGreen";

  state = &states[MPE_FILE_READ_AT_ALL_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_at_all";
  state->color = "LimeGreen";

  state = &states[MPE_FILE_WRITE_AT_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_at";
  state->color = "BlueViolet";

  state = &states[MPE_FILE_WRITE_AT_ALL_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_at_all";
  state->color = "SlateBlue";

  state = &states[MPE_FILE_IREAD_AT_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_iread_at";
  state->color = "PaleGreen";

  state = &states[MPE_FILE_IWRITE_AT_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_iwrite_at";
  state->color = "AliceBlue";

  state = &states[MPE_FILE_READ_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read";
  state->color = "CornflowerBlue";

  state = &states[MPE_FILE_READ_ALL_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_all";
  state->color = "DarkSeaGreen";

  state = &states[MPE_FILE_WRITE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write";
  state->color = "LawnGreen";

  state = &states[MPE_FILE_WRITE_ALL_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_all";
  state->color = "DodgerBlue";

  state = &states[MPE_FILE_IREAD_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_iread";
  state->color = "ForestGreen";

  state = &states[MPE_FILE_IWRITE_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_iwrite";
  state->color = "aquamarine";

  state = &states[MPE_FILE_SEEK_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_seek";
  state->color = "maroon";

  state = &states[MPE_FILE_GET_POSITION_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_position";
  state->color = "brown";

  state = &states[MPE_FILE_GET_BYTE_OFFSET_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_byte_offset";
  state->color = "RosyBrown";

  state = &states[MPE_FILE_READ_SHARED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_shared";
  state->color = "MediumSlateBlue";

  state = &states[MPE_FILE_WRITE_SHARED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_shared";
  state->color = "SpringGreen4";

  state = &states[MPE_FILE_IREAD_SHARED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_iread_shared";
  state->color = "SteelBlue";

  state = &states[MPE_FILE_IWRITE_SHARED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_iwrite_shared";
  state->color = "SpringGreen";

  state = &states[MPE_FILE_READ_ORDERED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_ordered";
  state->color = "SteelBlue1";

  state = &states[MPE_FILE_WRITE_ORDERED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_ordered";
  state->color = "SpringGreen1";

  state = &states[MPE_FILE_SEEK_SHARED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_seek_shared";
  state->color = "VioletRed";

  state = &states[MPE_FILE_GET_POSITION_SHARED_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_position_shared";
  state->color = "DarkViolet";

  state = &states[MPE_FILE_READ_AT_ALL_BEGIN_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_at_all_begin";
  state->color = "SpringGreen2";

  state = &states[MPE_FILE_READ_AT_ALL_END_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_at_all_end";
  state->color = "SpringGreen3";

  state = &states[MPE_FILE_WRITE_AT_ALL_BEGIN_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_at_all_begin";
  state->color = "SteelBlue2";

  state = &states[MPE_FILE_WRITE_AT_ALL_END_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_at_all_end";
  state->color = "SteelBlue3";

  state = &states[MPE_FILE_READ_ALL_BEGIN_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_all_begin";
  state->color = "DarkSeaGreen1";

  state = &states[MPE_FILE_READ_ALL_END_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_all_end";
  state->color = "DarkSeaGreen2";

  state = &states[MPE_FILE_WRITE_ALL_BEGIN_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_all_begin";
  state->color = "LightSteelBlue1";

  state = &states[MPE_FILE_WRITE_ALL_END_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_all_end";
  state->color = "LightSteelBlue2";

  state = &states[MPE_FILE_READ_ORDERED_BEGIN_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_ordered_begin";
  state->color = "DarkSeaGreen3";

  state = &states[MPE_FILE_READ_ORDERED_END_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_read_ordered_end";
  state->color = "DarkSeaGreen4";

  state = &states[MPE_FILE_WRITE_ORDERED_BEGIN_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_ordered_begin";
  state->color = "LightSteelBlue3";

  state = &states[MPE_FILE_WRITE_ORDERED_END_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_write_ordered_end";
  state->color = "LightSteelBlue4";

  state = &states[MPE_FILE_GET_TYPE_EXTENT_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_type_extent";
  state->color = "brown";

  state = &states[MPE_REGISTER_DATAREP_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_Register_datarep";
  state->color = "brown";

  state = &states[MPE_FILE_SET_ATOMICITY_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_set_atomicity";
  state->color = "brown";

  state = &states[MPE_FILE_GET_ATOMICITY_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_get_atomicity";
  state->color = "brown";

  state = &states[MPE_FILE_SYNC_ID];
  state->kind_mask = MPE_KIND_FILE;
  state->name = "MPI_File_sync";
  state->color = "YellowGreen";
}


int MPI_File_open( MPI_Comm  comm,char * filename,int  amode,MPI_Info  info,MPI_File * fh  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_open - prototyping replacement for MPI_File_open
    Log the beginning and ending of the time spent in MPI_File_open calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(comm,MPE_FILE_OPEN_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_open( comm, filename, amode, info, fh );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(comm,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_close( MPI_File * fh  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_close - prototyping replacement for MPI_File_close
    Log the beginning and ending of the time spent in MPI_File_close calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_CLOSE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_close( fh );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_delete( char * filename,MPI_Info  info  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_delete - prototyping replacement for MPI_File_delete
    Log the beginning and ending of the time spent in MPI_File_delete calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_DELETE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_delete( filename, info );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_set_size( MPI_File  fh,MPI_Offset  size  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_set_size - prototyping replacement for MPI_File_set_size
    Log the beginning and ending of the time spent in MPI_File_set_size calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_SET_SIZE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_set_size( fh, size );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_preallocate( MPI_File  fh,MPI_Offset  size  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_preallocate - prototyping replacement for MPI_File_preallocate
    Log the beginning and ending of the time spent in MPI_File_preallocate calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_PREALLOCATE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_preallocate( fh, size );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_size( MPI_File  fh,MPI_Offset * size  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_size - prototyping replacement for MPI_File_get_size
    Log the beginning and ending of the time spent in MPI_File_get_size calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_SIZE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_size( fh, size );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_group( MPI_File  fh,MPI_Group * group  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_group - prototyping replacement for MPI_File_get_group
    Log the beginning and ending of the time spent in MPI_File_get_group calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_GROUP_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_group( fh, group );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_amode( MPI_File  fh,int * amode  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_amode - prototyping replacement for MPI_File_get_amode
    Log the beginning and ending of the time spent in MPI_File_get_amode calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_AMODE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_amode( fh, amode );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_set_info( MPI_File  fh,MPI_Info  info  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_set_info - prototyping replacement for MPI_File_set_info
    Log the beginning and ending of the time spent in MPI_File_set_info calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_SET_INFO_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_set_info( fh, info );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_info( MPI_File  fh,MPI_Info * info_used  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_info - prototyping replacement for MPI_File_get_info
    Log the beginning and ending of the time spent in MPI_File_get_info calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_INFO_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_info( fh, info_used );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_set_view( MPI_File  fh,MPI_Offset  disp,MPI_Datatype  etype,MPI_Datatype  filetype,char * datarep,MPI_Info  info  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_set_view - prototyping replacement for MPI_File_set_view
    Log the beginning and ending of the time spent in MPI_File_set_view calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_SET_VIEW_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_set_view( fh, disp, etype, filetype, datarep, info );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_view( MPI_File  fh,MPI_Offset * disp,MPI_Datatype * etype,MPI_Datatype * filetype,char * datarep  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_view - prototyping replacement for MPI_File_get_view
    Log the beginning and ending of the time spent in MPI_File_get_view calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_VIEW_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_view( fh, disp, etype, filetype, datarep );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_at( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_at - prototyping replacement for MPI_File_read_at
    Log the beginning and ending of the time spent in MPI_File_read_at calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_AT_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_at( fh, offset, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_at_all( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_at_all - prototyping replacement for MPI_File_read_at_all
    Log the beginning and ending of the time spent in MPI_File_read_at_all calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_AT_ALL_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_at_all( fh, offset, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_at( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_at - prototyping replacement for MPI_File_write_at
    Log the beginning and ending of the time spent in MPI_File_write_at calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_AT_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_at( fh, offset, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_at_all( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_at_all - prototyping replacement for MPI_File_write_at_all
    Log the beginning and ending of the time spent in MPI_File_write_at_all calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_AT_ALL_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_at_all( fh, offset, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_iread_at( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype,MPIO_Request * request  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_iread_at - prototyping replacement for MPI_File_iread_at
    Log the beginning and ending of the time spent in MPI_File_iread_at calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_IREAD_AT_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_iread_at( fh, offset, buf, count, datatype, request );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_iwrite_at( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype,MPIO_Request * request  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_iwrite_at - prototyping replacement for MPI_File_iwrite_at
    Log the beginning and ending of the time spent in MPI_File_iwrite_at calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_IWRITE_AT_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_iwrite_at( fh, offset, buf, count, datatype, request );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read - prototyping replacement for MPI_File_read
    Log the beginning and ending of the time spent in MPI_File_read calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_all( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_all - prototyping replacement for MPI_File_read_all
    Log the beginning and ending of the time spent in MPI_File_read_all calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_ALL_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_all( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write - prototyping replacement for MPI_File_write
    Log the beginning and ending of the time spent in MPI_File_write calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_all( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_all - prototyping replacement for MPI_File_write_all
    Log the beginning and ending of the time spent in MPI_File_write_all calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_ALL_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_all( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_iread( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPIO_Request * request  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_iread - prototyping replacement for MPI_File_iread
    Log the beginning and ending of the time spent in MPI_File_iread calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_IREAD_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_iread( fh, buf, count, datatype, request );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_iwrite( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPIO_Request * request  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_iwrite - prototyping replacement for MPI_File_iwrite
    Log the beginning and ending of the time spent in MPI_File_iwrite calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_IWRITE_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_iwrite( fh, buf, count, datatype, request );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_seek( MPI_File  fh,MPI_Offset  offset,int  whence  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_seek - prototyping replacement for MPI_File_seek
    Log the beginning and ending of the time spent in MPI_File_seek calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_SEEK_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_seek( fh, offset, whence );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_position( MPI_File  fh,MPI_Offset * offset  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_position - prototyping replacement for MPI_File_get_position
    Log the beginning and ending of the time spent in MPI_File_get_position calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_POSITION_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_position( fh, offset );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_byte_offset( MPI_File  fh,MPI_Offset  offset,MPI_Offset * disp  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_byte_offset - prototyping replacement for MPI_File_get_byte_offset
    Log the beginning and ending of the time spent in MPI_File_get_byte_offset calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_BYTE_OFFSET_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_byte_offset( fh, offset, disp );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_shared( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_shared - prototyping replacement for MPI_File_read_shared
    Log the beginning and ending of the time spent in MPI_File_read_shared calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_SHARED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_shared( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_shared( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_shared - prototyping replacement for MPI_File_write_shared
    Log the beginning and ending of the time spent in MPI_File_write_shared calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_SHARED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_shared( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_iread_shared( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPIO_Request * request  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_iread_shared - prototyping replacement for MPI_File_iread_shared
    Log the beginning and ending of the time spent in MPI_File_iread_shared calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_IREAD_SHARED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_iread_shared( fh, buf, count, datatype, request );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_iwrite_shared( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPIO_Request * request  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_iwrite_shared - prototyping replacement for MPI_File_iwrite_shared
    Log the beginning and ending of the time spent in MPI_File_iwrite_shared calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_IWRITE_SHARED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_iwrite_shared( fh, buf, count, datatype, request );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_ordered( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_ordered - prototyping replacement for MPI_File_read_ordered
    Log the beginning and ending of the time spent in MPI_File_read_ordered calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_ORDERED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_ordered( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_ordered( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_ordered - prototyping replacement for MPI_File_write_ordered
    Log the beginning and ending of the time spent in MPI_File_write_ordered calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_ORDERED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_ordered( fh, buf, count, datatype, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_seek_shared( MPI_File  fh,MPI_Offset  offset,int  whence  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_seek_shared - prototyping replacement for MPI_File_seek_shared
    Log the beginning and ending of the time spent in MPI_File_seek_shared calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_SEEK_SHARED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_seek_shared( fh, offset, whence );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_position_shared( MPI_File  fh,MPI_Offset * offset  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_position_shared - prototyping replacement for MPI_File_get_position_shared
    Log the beginning and ending of the time spent in MPI_File_get_position_shared calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_POSITION_SHARED_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_position_shared( fh, offset );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_at_all_begin( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_at_all_begin - prototyping replacement for MPI_File_read_at_all_begin
    Log the beginning and ending of the time spent in MPI_File_read_at_all_begin calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_AT_ALL_BEGIN_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_at_all_begin( fh, offset, buf, count, datatype );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_at_all_end( MPI_File  fh,void * buf,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_at_all_end - prototyping replacement for MPI_File_read_at_all_end
    Log the beginning and ending of the time spent in MPI_File_read_at_all_end calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_AT_ALL_END_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_at_all_end( fh, buf, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_at_all_begin( MPI_File  fh,MPI_Offset  offset,void * buf,int  count,MPI_Datatype  datatype  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_at_all_begin - prototyping replacement for MPI_File_write_at_all_begin
    Log the beginning and ending of the time spent in MPI_File_write_at_all_begin calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_AT_ALL_BEGIN_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_at_all_begin( fh, offset, buf, count, datatype );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_at_all_end( MPI_File  fh,void * buf,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_at_all_end - prototyping replacement for MPI_File_write_at_all_end
    Log the beginning and ending of the time spent in MPI_File_write_at_all_end calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_AT_ALL_END_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_at_all_end( fh, buf, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_all_begin( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_all_begin - prototyping replacement for MPI_File_read_all_begin
    Log the beginning and ending of the time spent in MPI_File_read_all_begin calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_ALL_BEGIN_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_all_begin( fh, buf, count, datatype );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_all_end( MPI_File  fh,void * buf,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_all_end - prototyping replacement for MPI_File_read_all_end
    Log the beginning and ending of the time spent in MPI_File_read_all_end calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_ALL_END_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_all_end( fh, buf, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_all_begin( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_all_begin - prototyping replacement for MPI_File_write_all_begin
    Log the beginning and ending of the time spent in MPI_File_write_all_begin calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_ALL_BEGIN_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_all_begin( fh, buf, count, datatype );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_all_end( MPI_File  fh,void * buf,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_all_end - prototyping replacement for MPI_File_write_all_end
    Log the beginning and ending of the time spent in MPI_File_write_all_end calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_ALL_END_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_all_end( fh, buf, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_ordered_begin( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_ordered_begin - prototyping replacement for MPI_File_read_ordered_begin
    Log the beginning and ending of the time spent in MPI_File_read_ordered_begin calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_ORDERED_BEGIN_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_ordered_begin( fh, buf, count, datatype );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_read_ordered_end( MPI_File  fh,void * buf,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_read_ordered_end - prototyping replacement for MPI_File_read_ordered_end
    Log the beginning and ending of the time spent in MPI_File_read_ordered_end calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_READ_ORDERED_END_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_read_ordered_end( fh, buf, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_ordered_begin( MPI_File  fh,void * buf,int  count,MPI_Datatype  datatype  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_ordered_begin - prototyping replacement for MPI_File_write_ordered_begin
    Log the beginning and ending of the time spent in MPI_File_write_ordered_begin calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_ORDERED_BEGIN_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_ordered_begin( fh, buf, count, datatype );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_write_ordered_end( MPI_File  fh,void * buf,MPI_Status * status  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_write_ordered_end - prototyping replacement for MPI_File_write_ordered_end
    Log the beginning and ending of the time spent in MPI_File_write_ordered_end calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_WRITE_ORDERED_END_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_write_ordered_end( fh, buf, status );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_type_extent( MPI_File  fh,MPI_Datatype  datatype,MPI_Aint * extent  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_type_extent - prototyping replacement for MPI_File_get_type_extent
    Log the beginning and ending of the time spent in MPI_File_get_type_extent calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_TYPE_EXTENT_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_type_extent( fh, datatype, extent );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_set_atomicity( MPI_File  fh,int  flag  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_set_atomicity - prototyping replacement for MPI_File_set_atomicity
    Log the beginning and ending of the time spent in MPI_File_set_atomicity calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_SET_ATOMICITY_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_set_atomicity( fh, flag );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_get_atomicity( MPI_File  fh,int * flag  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_get_atomicity - prototyping replacement for MPI_File_get_atomicity
    Log the beginning and ending of the time spent in MPI_File_get_atomicity calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_GET_ATOMICITY_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_get_atomicity( fh, flag );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}

int MPI_File_sync( MPI_File  fh  )
{
  int returnVal;
  MPE_LOG_STATE_DECL
  MPE_LOG_THREADSTM_DECL

/*
    MPI_File_sync - prototyping replacement for MPI_File_sync
    Log the beginning and ending of the time spent in MPI_File_sync calls.
*/

  MPE_LOG_THREADSTM_GET
  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_FILE_SYNC_ID)
  MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

  returnVal = PMPI_File_sync( fh );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

  MPE_LOG_THREAD_LOCK
  MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
  MPE_LOG_THREAD_UNLOCK

  return returnVal;
}
