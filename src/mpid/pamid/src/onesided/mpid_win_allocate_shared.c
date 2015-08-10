/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/onesided/mpid_win_allocate_shared.c
 * \brief
 */
#include "mpidi_onesided.h"
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#undef FUNCNAME 
#define FUNCNAME MPID_Win_allocate_shared
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)

#define SHM_KEY_TAIL 0xfdbc         /* If this value is changed, change  */
                                      /* in PMD (mp_pmd.c) as well.        */

extern int mpidi_dynamic_tasking;
#define MPIDI_PAGESIZE_MASK(y) (~(((MPI_Aint)y)-1))
#define MPIDI_ROUND_UP_PAGESIZE(x,y) ((((MPI_Aint)x)+(~MPIDI_PAGESIZE_MASK(y))) & MPIDI_PAGESIZE_MASK(y))
#define ALIGN_BOUNDARY 128     /* Align data structures to cache line */
#define PAD_SIZE(s) (ALIGN_BOUNDARY - (sizeof(s) & (ALIGN_BOUNDARY-1)))

int CheckRankOnNode(MPID_Comm  * comm_ptr,int *onNode ) {
    int comm_size, i;
    int mpi_errno = PAMI_SUCCESS;

    comm_size = comm_ptr->local_size;

    *onNode = 1;

    for (i = 0; i < comm_size; i++) {
        if (comm_ptr->intranode_table[i] == -1) {
            *onNode = 0;
            break;
        }
     }

     if (*onNode== 0) {
         MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_CONFLICT,
                             return mpi_errno, "**rmaconflict");
     }

     return mpi_errno;
}

int CheckSpaceType(MPID_Win **win_ptr, MPID_Info *info,int *noncontig) {
    int mpi_errno=MPI_SUCCESS;
  /* Check if we are allowed to allocate space non-contiguously */
    if (info != NULL) {
        int alloc_shared_nctg_flag = 0;
        char alloc_shared_nctg_value[MPI_MAX_INFO_VAL+1];
        MPIR_Info_get_impl(info, "alloc_shared_noncontig", MPI_MAX_INFO_VAL,
                           alloc_shared_nctg_value, &alloc_shared_nctg_flag);
        if (alloc_shared_nctg_flag == 1) {
            if (!strncmp(alloc_shared_nctg_value, "true", strlen("true")))
                (*win_ptr)->mpid.info_args.alloc_shared_noncontig = 1;
            if (!strncmp(alloc_shared_nctg_value, "false", strlen("false")))
                (*win_ptr)->mpid.info_args.alloc_shared_noncontig = 0;
        }
    }
     *noncontig = (*win_ptr)->mpid.info_args.alloc_shared_noncontig;
    return mpi_errno;
}

int GetPageSize(void *addr, ulong *pageSize)
{
  pid_t pid;
  FILE *fp;
  char Line[201],A1[50],A2[50];
  char fileName[100];
  char A3[50],A4[50];
  int  i=0;
  char *t1,*t2;
  #ifndef REDHAT
  char a='-';
  char *search = &a;
  #endif
  void *beg, *end;
  int  found=0;
  int  ps;
  int   j,k;

  *pageSize = 0;
  pid = getpid();
  sprintf(fileName,"/proc/%d/smaps",pid);
  /* linux-2.6.29 or greater, KernelPageSize in /proc/pid/smaps includes 4K, 16 MB */
  TRACE_ERR("fileName = %s   addr=%p\n",fileName,addr);
  fp = fopen(fileName,"r");
  if (fp == NULL) {
      TRACE_ERR("fileName = %s open failed errno=%d\n",fileName,errno);
      return errno;
  }
  while(fgets(Line,200,fp)) {
    i++;
    sscanf(Line,"%s  %s %s %s \n",A1,A2,A3,A4);
    if ((found == 1) && (memcmp(A1,"KernelPageSize",14)==0)) {
         j=atoi(A2);
         if ((A3[0]=='k') || (A3[0]=='K'))
               k=1024;
         else if ((A3[0]=='m') || (A3[0]=='M'))
               k=1048576;
         else if ((A3[0]=='g') || (A3[0]=='G'))
               k=0x40000000;  /* 1 GB  */
         else {
             TRACE_ERR("ERROR unrecognized unit A3=%s\n",A3);
             break;
         }
         *pageSize = (ulong)(j * k);
         TRACE_ERR(" addr=%p pageSize=%ld %s(%d)\n", addr,*pageSize,__FILE__,__LINE__);
         break;
    }
    if ((strlen(A2) == 4) && ((A2[0]=='r') || (A2[3]=='p'))) {
       #ifndef REDHAT
         t1=strtok(A1,search);
       #else
         t1=strtok(A1,"-");
       #endif
         t2 = A1+strlen(t1)+1;
         sscanf(t1,"%p \n",&beg);
         sscanf(t2,"%p \n",&end);
         if (((ulong) addr >= (ulong)beg) && ((ulong)addr <= (ulong)end)) {
             found=1;
             TRACE_ERR("found addr=%p i=%d between beg=%p and end=%p in %s\n",
                    addr,i,beg,end,fileName);
         }
    }
  }
  fclose(fp);
  if (*pageSize == 0) {
       ps = getpagesize();
       *pageSize = (ulong) ps;
       TRACE_ERR("LinuxPageSize %p not in %s  getpagesize=%ld\n", addr,fileName,*pageSize);
  }
  return 0;
}

int
MPID_getSharedSegment_mmap(MPID_Win * win)
{
  int rank, rc, fd;
  int mpi_errno = MPI_SUCCESS;
  MPIR_Errflag_t errflag = MPIR_ERR_NONE;
  int first = 0;

  snprintf (win->mpid.shm->shm_key, 63, "/mpich.comm-%d.win_shared", win->comm_ptr->context_id);
  rc = shm_open (win->mpid.shm->shm_key, O_RDWR | O_CREAT | O_EXCL, 0600);
  if (0 == rc)
  {
    first = 1;
  } else {
    rc = shm_open (win->mpid.shm->shm_key, O_RDWR, 0);
    MPIU_ERR_CHKANDJUMP((rc == -1), mpi_errno, MPI_ERR_RMA_SHARED, "**rmashared");
  }

  fd = rc;
  rc = ftruncate (fd, win->mpid.shm->segment_len);
  MPIU_ERR_CHKANDJUMP((rc == -1), mpi_errno, MPI_ERR_RMA_SHARED, "**rmashared");

  win->mpid.shm->base_addr = mmap (NULL, win->mpid.shm->segment_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  MPIU_ERR_CHKANDJUMP((win->mpid.shm->base_addr == MAP_FAILED), mpi_errno, MPI_ERR_RMA_SHARED, "**rmashared");

  close (fd); /* no longer needed */

  /* set mutex_lock address and initialize it   */
  win->mpid.shm->ctrl = (MPIDI_Win_shm_ctrl_t *) win->mpid.shm->base_addr;
  if (1 == first) {
    MPIDI_SHM_MUTEX_INIT(win);
  }

  mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);
  MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

  win->mpid.shm->allocated = 1;

fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    shm_unlink (win->mpid.shm->shm_key);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int
MPID_getSharedSegment_sysv(MPID_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    uint32_t shm_key;
    int rank;
    char *cp;
    int shm_flag = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;

    shm_key = (uint32_t) -1;

    rank = win->comm_ptr->rank;

    if (rank == 0) {
#ifdef DYNAMIC_TASKING
        /* generate an appropriate key */
        if (!mpidi_dynamic_tasking) {
            cp = getenv("MP_I_PMD_PID");
            if (cp) {
                shm_key = atoi(cp);
                shm_key = shm_key & 0x07ffffff;
                shm_key = shm_key | 0x80000000;
            } else {
                cp = getenv("MP_PARTITION");
                if (cp ) {
                    shm_key = atol(cp);
                    shm_key = (shm_key << 16) + SHM_KEY_TAIL;
                } else {
                    TRACE_ERR("ERROR MP_PARTITION not set \n");
                }
            }
        } else {
            cp = getenv("MP_I_KEY_RANGE");
            if (cp) {
                sscanf(cp, "0x%x", &shm_key);
                shm_key = shm_key | 0x80;
            } else {
                TRACE_ERR("ERROR MP_I_KEY_RANGE not set \n");
            }
        }
#else
        cp = getenv("MP_I_PMD_PID");
        if (cp) {
            shm_key = atoi(cp);
            shm_key = shm_key & 0x07ffffff;
            shm_key = shm_key | 0x80000000;
        } else {
            cp = getenv("MP_PARTITION");
            if (cp ) {
                shm_key = atol(cp);
                shm_key = (shm_key << 16);
#ifdef SHMCC_KEY_TAIL
                shm_key += SHMCC_KEY_TAIL;
#endif
            } else {
                TRACE_ERR("ERROR MP_PARTITION not set \n");
            }
        }
#endif

        MPID_assert(shm_key != -1);

        win->mpid.shm->shm_id = shmget(shm_key, win->mpid.shm->segment_len, shm_flag);
        MPIU_ERR_CHKANDJUMP((win->mpid.shm->shm_id == -1), mpi_errno, MPI_ERR_RMA_SHARED, "**rmashared");

        win->mpid.shm->base_addr = (void *) shmat(win->mpid.shm->shm_id,0,0);
        MPIU_ERR_CHKANDJUMP((win->mpid.shm->base_addr == (void*) -1), mpi_errno,MPI_ERR_BUFFER, "**bufnull");

        /* set mutex_lock address and initialize it */
        win->mpid.shm->ctrl = (MPIDI_Win_shm_ctrl_t *) win->mpid.shm->base_addr;
        MPIDI_SHM_MUTEX_INIT(win);

        /* successfully created shm segment - shared the key with other tasks */
        mpi_errno = MPIR_Bcast_impl((void *) &shm_key, sizeof(int), MPI_CHAR, 0, win->comm_ptr, &errflag);

    } else { /* task other than task 0  */
        mpi_errno = MPIR_Bcast_impl((void *) &shm_key,  sizeof(int), MPI_CHAR, 0, win->comm_ptr, &errflag);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        win->mpid.shm->shm_id = shmget(shm_key, 0, 0);
        if (win->mpid.shm->shm_id != -1) { /* shm segment is available */
            win->mpid.shm->base_addr = (void *) shmat(win->mpid.shm->shm_id,0,0);
        }
        win->mpid.shm->ctrl = (MPIDI_Win_shm_ctrl_t *) win->mpid.shm->base_addr;
    }

    win->mpid.shm->allocated = 1;

fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int
MPID_getSharedSegment(MPI_Aint     size,
                      int          disp_unit,
                      MPID_Comm  * comm_ptr,
                      MPID_Win   **win_ptr,
                      MPI_Aint   * pSize,
                      int        * noncontig)
{
    int mpi_errno = MPI_SUCCESS;
    int i, comm_size, rank;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPI_Aint pageSize,pageSize2, len,new_size;
    MPID_Win  *win;
    int    padSize;
    void   *base_pp;

    win =  *win_ptr;
    comm_size = win->comm_ptr->local_size;
    rank = win->comm_ptr->rank;

    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    GetPageSize((void *) win_ptr, (ulong *) &pageSize);
    *pSize = pageSize;
    win->mpid.shm->segment_len = 0;

    if (comm_size == 1) {
        /* Do not use shared memory when there is only one rank on the node */

        /* 'size' must not be < 0 */
        MPIU_ERR_CHKANDSTMT(size < 0 , mpi_errno, MPI_ERR_SIZE,return mpi_errno, "**rmasize");

        /* The beginning of the heap allocation contains a control block
         * before the data begins.
         */
        new_size = MPIDI_ROUND_UP_PAGESIZE((sizeof(MPIDI_Win_shm_ctrl_t)+ ((comm_size+1) * sizeof(void *))),pageSize);

        if (size > 0) {
            if (*noncontig)
                new_size += MPIDI_ROUND_UP_PAGESIZE(size,pageSize);
            else
                new_size += size;
        }

        base_pp = MPIU_Malloc(new_size);
        MPID_assert(base_pp !=NULL);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        win->mpid.shm->segment_len = new_size;
        win->mpid.shm->base_addr = base_pp;
        if (size !=0) {
            win->mpid.info[rank].base_addr = (void *)((MPI_Aint) base_pp + MPIDI_ROUND_UP_PAGESIZE((sizeof(MPIDI_Win_shm_ctrl_t) +  ((comm_size+1) * sizeof(void *))),pageSize));
        } else {
            win->mpid.info[rank].base_addr = NULL;
        }
        win->base = win->mpid.info[rank].base_addr;
        win->mpid.info[rank].base_size = size;

        /* set mutex_lock address and initialize it   */
        win->mpid.shm->ctrl = (MPIDI_Win_shm_ctrl_t *) win->mpid.shm->base_addr;
        MPIDI_SHM_MUTEX_INIT(win);
        OPA_fetch_and_add_int((OPA_int_t *) &win->mpid.shm->ctrl->shm_count,(int) 1);

    } else {
        /* allocate a temporary buffer to gather the 'size' of each buffer on
         * the node to determine the amount of shared memory to allocate
         */
        MPI_Aint * size_array;
        size_array = MPIU_Malloc (2*comm_size*sizeof(MPI_Aint));
        size_array[rank] = (MPI_Aint) size;
        mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                        size_array, 1 * sizeof(MPI_Aint), MPI_BYTE,
                                        (*win_ptr)->comm_ptr, &errflag);
        if (mpi_errno) {
            MPIU_Free(size_array);
            MPIU_ERR_POP(mpi_errno);
        }

        /* calculate total number of bytes needed */
        MPI_Aint actual_size;
        win->mpid.info[0].base_addr = NULL;
        for (i = 0; i < comm_size; ++i) {
            win->mpid.info[i].base_size = size_array[i];

            actual_size = (*noncontig)?MPIDI_ROUND_UP_PAGESIZE(size_array[i],pageSize):size_array[i];

            win->mpid.shm->segment_len += actual_size;

            /* Save the OFFSET to each rank's private shared memory area. This
             * will be added to the BASE ADDRESS of the entire shared memory
             * allocation to determine the virtual address.
             */
            if (i < comm_size-1)
                win->mpid.info[i+1].base_addr =
                    (void *) ((uintptr_t)win->mpid.info[i].base_addr + actual_size);
        }

        /* The beginning of the shared memory allocation contains a control
         * block before the data begins.
         */
        win->mpid.shm->segment_len += MPIDI_ROUND_UP_PAGESIZE((sizeof(MPIDI_Win_shm_ctrl_t) + ((comm_size+1) * sizeof(void *))),pageSize);

        /* Get the shared segment which includes the control block header and
         * data buffer - possibly padded if non-contiguous.
         */
#ifdef USE_SYSV_SHM
        mpi_errno = MPID_getSharedSegment_sysv(win);
#elif  USE_MMAP_SHM
        mpi_errno = MPID_getSharedSegment_mmap(win);
#else
        MPID_Abort(NULL, MPI_ERR_RMA_SHARED, -1, "RMA shared segment error");
#endif
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* assign base address here                                                    */
        /* compute the base addresses of each process within the shared memory segment */
        /* will be done after MPI_Allgather in MID_Win_allocate_shared                 */
        win->base = (void *) ((long) win->mpid.shm->base_addr + (long ) MPIDI_ROUND_UP_PAGESIZE((sizeof(MPIDI_Win_shm_ctrl_t) + ((comm_size+1) * sizeof(void *))),pageSize));


        MPIU_Free(size_array);

        /* increment the shared counter */
        OPA_fetch_and_add_int((OPA_int_t *) &win->mpid.shm->ctrl->shm_count,(int) 1);

        /* wait for all ranks complete */
        while((int) win->mpid.shm->ctrl->shm_count != comm_size) MPIDI_QUICKSLEEP;
    }

fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */

}

/**
 * \brief MPI-PAMI glue for MPI_Win_allocate_shared function
 *
 * Create a window object. Allocates a MPID_Win object and initializes it,
 * then allocates the collective info array, initalizes our entry, and
 * performs an Allgather to distribute/collect the rest of the array entries.
 * On each process, it allocates memory of at least size bytes that is shared
 * among all processes in comm, and returns a pointer to the locally allocated
 * segment in base_ptr that can be used for load/store accesses on the calling
 * process. The locally allocated memory can be the target of load/store accessses
 * by remote processes; the base pointers for other processes can be queried using
 * the function 'MPI_Win_shared_query'.
 *
 * The call also returns a window object that can be used by all processes in comm
 * to perform RMA operations. The size argument may be different at each process
 * and size = 0 is valid. It is the user''s responsibility to ensure that the
 * communicator comm represents a group of processes that can create a shared
 * memory segment that can be accessed by all processes in the group. The
 * allocated memory is contiguous across process ranks unless the info key
 * alloc_shared_noncontig is specified. Contiguous across process ranks means that
 * the first address in the memory segment of process i is consecutive with the
 * last address in the memory segment of process i - 1.  This may enable the user
 * to calculate remote address offsets with local information only.
 *
 * Input Parameters:
 * \param[in] size      size of window in bytes (nonnegative integer)
 * \param[in] disp_unit local unit size for displacements, in bytes (positive integer)
 * \param[in] info      info argument (handle))
 * \param[in] comm_ptr  intra-Communicator (handle)
 * \param[out] base_ptr  address of local allocated window segment
 * \param[out] win_ptr  window object returned by the call (handle)
 * \return MPI_SUCCESS, MPI_ERR_ARG, MPI_ERR_COMM, MPI_ERR_INFO. MPI_ERR_OTHER,
 *         MPI_ERR_SIZE
 *
 *  win->mpid.shm->base_addr  \* return address from shmat                                *\
 *  win->base                 \* address for data starts here == win->mpid.shm->base_addr *\          
 *                            \* + space for mutex_lock and shm_count                     *\
 *
 */
int
MPID_Win_allocate_shared(MPI_Aint     size,   
                         int          disp_unit,
                         MPID_Info  * info,
                         MPID_Comm  * comm_ptr,
                         void *base_ptr,
                         MPID_Win  ** win_ptr)
{
  int mpi_errno  = MPI_SUCCESS;
  MPIR_Errflag_t errflag = MPIR_ERR_NONE;
  int onNode     = 0;
  MPID_Win    *win = NULL;
  int rank, prev_size;

  MPIDI_Win_info  *winfo;
  int         comm_size,i;
  int         noncontig=FALSE;
  MPI_Aint    pageSize=0;

  /* Verify all ranks are on-node */
  mpi_errno=CheckRankOnNode(comm_ptr,&onNode);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);
  MPIU_ERR_CHKANDJUMP((onNode == 0), mpi_errno, MPI_ERR_RMA_SHARED, "**rmashared");
  
  /* Initialize the window */
  mpi_errno =MPIDI_Win_init(size,disp_unit,win_ptr, info, comm_ptr, MPI_WIN_FLAVOR_SHARED, MPI_WIN_UNIFIED);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);
  win = *win_ptr;
  win->mpid.shm = MPIU_Malloc(sizeof(MPIDI_Win_shm_t));
  MPID_assert(win->mpid.shm != NULL);
  memset(win->mpid.shm, 0, sizeof(MPIDI_Win_shm_t));

  rank = comm_ptr->rank;
  win->mpid.info[rank].win = win;
  win->mpid.info[rank].disp_unit = disp_unit;

  mpi_errno=CheckSpaceType(win_ptr,info,&noncontig);
  comm_size = (*win_ptr)->comm_ptr->local_size;
  mpi_errno = MPID_getSharedSegment(size, disp_unit,comm_ptr, win_ptr, &pageSize, &noncontig);
  if (mpi_errno != MPI_SUCCESS)
      return mpi_errno;

  mpi_errno = MPIDI_Win_allgather(size,win_ptr);
  if (mpi_errno != MPI_SUCCESS)
      return mpi_errno;

  win->mpid.info[0].base_addr = win->base;
  char *cur_base = win->base;
  prev_size=win->mpid.info[0].base_size;
  for (i = 1; i < comm_size; ++i) {
       if (win->mpid.info[i].base_size) {
            if (prev_size) {
               if (noncontig) {
                  /* Round up to next page size */
                   win->mpid.info[i].base_addr =
                  (void *) ((MPI_Aint) cur_base + (MPI_Aint) MPIDI_ROUND_UP_PAGESIZE(prev_size,pageSize));
               } else {
                    win->mpid.info[i].base_addr = (void *) ((MPI_Aint) cur_base + (MPI_Aint) prev_size);
               }
            } else
                    win->mpid.info[i].base_addr = (void *) ((MPI_Aint) cur_base);
              prev_size=win->mpid.info[i].base_size;
              cur_base = win->mpid.info[i].base_addr;
        } else {
                 win->mpid.info[i].base_addr = NULL;
        }
  }


  *(void**) base_ptr = (void *) win->mpid.info[rank].base_addr;

  mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);
fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    if (win != NULL)
      if (win->mpid.shm != NULL)
        MPIU_Free(win->mpid.shm);
    goto fn_exit;
    /* --END ERROR HANDLING-- */

}

