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
 * \file src/mpidi_bufmm.c
 * \brief Memory management for early arrivals
 */

 /*******************************************************************/
 /*   DESCRIPTION:                                                  */
 /*   Dynamic memory manager which allows allocation and            */
 /*   deallocation for earl arrivals sent via eager protocol.       */
 /*                                                                 */
 /*   The basic method is for buffers of size between MIN_SIZE      */
 /*   and MAX_SIZE. The allocation scheme is a modified version     */
 /*   of Knuth's buddy algorithm. Regardless of what size buffer    */
 /*   is requested the size is rounded up to the nearest power of 2.*/
 /*   Note that there is a buddy_header overhead per buffer (8 bytes*/
 /*   in 32 bit and 16 bytes in 64 bit). So, for example, if a      */
 /*   256 byte buffer is requested that would require 512 bytes     */
 /*   of memory. A 248-byte buffer needs 256 bytes of space.        */
 /*   Only for the maxsize buffers, it is guaranteed that the       */
 /*   allocation is a power of two, since typically applications    */
 /*   have such requirements.                                       */
 /*                                                                 */
 /*   To speed up the buddy algorithm there are some preallocated   */
 /*   buffers. There are FLEX_NUM number of buffers from the        */
 /*   FLEX_COUNT number of smallest buffers. So, for example, if    */
 /*   MIN_SIZE is 16, FLEX_COUNT is 4, and FLEX_NUM is 256 then     */
 /*   there are buffers of size 16, 32, 64, and 128 preallocted     */
 /*   (256 buffers each). These buffers are arranged into stacks.   */
 /*                                                                 */
 /*   If the system runs out of preallocated buffers or the size    */
 /*   is bigger than the biggest preallocated one then the buddy    */
 /*   algorithm is applied. Originally there is a list of MAX_SIZE  */
 /*   buffers.  (The size is MAX_SIZE + the 8 or 16 byte overhead.) */
 /*   These are not merged as the traditional buddy system would    */
 /*   require, since we never need bigger buffers than these.       */
 /*   Originally, the lists of smaller size buffers are empty. When */
 /*   there is an allocation request the program searches for the   */
 /*   smallest free buffer available in the lists. If it is bigger  */
 /*   than the requested one then it is repeatedly split into half. */
 /*   The other halves are inserted into the appropriate list of    */
 /*   free buffers. At deallocation the program attempts to merge   */
 /*   the buffer with it's buddy repeatedly to get the largest      */
 /*   buffer possible.                                              */
 /*******************************************************************/

#include <mpidimpl.h>

#define NO  0
#define YES 1
int application_set_eager_limit=0;

#if TOKEN_FLOW_CONTROL
#define BUFFER_MEM_MAX    (1<<26)   /* 64MB */
#define MAX_BUF_BKT_SIZE  (1<<18)   /* Max eager_limit is 256K              */
#define MIN_BUF_BKT_SIZE  (64)
#define MIN_LOG2SIZE        6       /* minimum buffer size log-2            */
#define MIN_SIZE           64       /* minimum buffer size                  */
#define MAX_BUCKET         13       /* log of maximum buffer size/MIN_SIZE  */
#define MAX_SIZE          (MIN_SIZE<<(MAX_BUCKET-1)) /* maximum buffer size */
#define FLEX_COUNT          5       /* num of buf types to preallocate      */
#define FLEX_NUM           32       /* num of preallocated buffers          */
     /* overhead for each buffer             */
#define MAX_BUDDIES        50       /* absolutely maximum number of buddies */
#define BUDDY               1
#define FLEX                0
#define MAX_MALLOCS        10

#define OVERHEAD        (sizeof(buddy_header) - 2 * sizeof (void *))
#define TAB_SIZE        (MAX_BUCKET+1)
#define ALIGN8(x)       ((((unsigned long)(x)) + 0x7) & ~0x7L)
#ifndef max
#define max(a,b)      ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)      (a>b ? b : a)
#endif

/* normalize the size to number of MIN_SIZE blocks required to hold  */
/* required size. This includes the OVERHEAD                         */
#define NORMSIZE(sz) (((sz)+MIN_SIZE+OVERHEAD-1) >> MIN_LOG2SIZE)

typedef struct bhdr_struct{
   char                 buddy;           /* buddy or flex alloc     */
   char                 free;            /* available or not        */
   char                 bucket;          /* log of buffer size      */
   char                 *base_buddy;     /* addr of max size buddy  */
   struct bhdr_struct   *next;           /* list ptr, used if free  */
   struct bhdr_struct   *prev;           /* list bptr, used if free */
} buddy_header;

typedef struct fhdr_struct{
   char                 buddy;           /* buddy or flex alloc     */
   char                 ind;             /* which stack             */
                                         /* bucket - min_bucket     */
} flex_header;

typedef struct{
  void  *ptr;                             /* malloc ptr need to be freed  */
  int   size;                             /* space allocted               */
  int   type;                             /* FLEX or BUDDY                */
} malloc_list_t;
malloc_list_t  *malloc_list;

static int    nMallocs, maxMallocs;       /* no. of malloc() issued       */
static int    maxMallocs;                 /* max. no. of malloc() allowed */
static int    max_bucket;                 /* max. number of "buddy" bucket*/
static int    flex_count;                 /* pre-allocated bucket         */
static size_t max_size;                   /* max. size for each msg       */
static size_t flex_size;                  /* size for flex slot           */
static char  *buddy_heap_ptr;             /* ptr points to beg. of buddy  */
static char  *end_heap_ptr;               /* ptr points to end of buddy   */
static char  *heap;                       /* begin. address of flex stack */
static long mem_inuse;                    /* memory in use                */
long mem_hwmark;                          /* highest memory usage         */


static int  sizetable[TAB_SIZE + 1];      /* from bucket to size            */
static int  sizetrans[(1<<(MAX_BUCKET-1))+2];/* from size to power of 2 size*/

static char** flex_stack[TAB_SIZE];       /* for flex size alloc            */
static int    flex_sp   [TAB_SIZE];       /* flex stack pointer             */

static buddy_header *free_buddy[TAB_SIZE];/* for buddy alloc                */

int MPIDI_tfctrl_enabled=0;                     /* token flow control enabled     */
int MPIDI_tfctrl_hwmark=0;                      /* high water mark for tfc        */
int application_set_buf_mem=0;            /* MP_BUFFER_MEM set by the user? */
char *EagerLimit=NULL;                    /* export MP_EAGER_LIMIT if the   */
                                          /* number is adjusted             */

/***************************************************************************/
/*  calculate number of tokens available for each pair-wise communication. */
/***************************************************************************/

void MPIDI_calc_tokens(int nTasks,uint *eager_limit_in, unsigned long *buf_mem_in )
{
 char *cp;
 unsigned long new_buf_mem_max,buf_mem_max;
 int  val;
 int  rc;
 int  i;

       /* Round up passed eager limit to power of 2 */
    if (MPIDI_Process.mp_buf_mem_max > *buf_mem_in) 
        buf_mem_max = MPIDI_Process.mp_buf_mem_max;
    else 
        buf_mem_max= *buf_mem_in;

    if (*eager_limit_in != 0) {
       for (val=1 ; val < *eager_limit_in ; val *= 2);
       if (val > MAX_BUF_BKT_SIZE) {   /* Maximum eager_limit is 256K */
           val = MAX_BUF_BKT_SIZE;
       }
       if (val < MIN_BUF_BKT_SIZE) {   /* Minimum eager_limit is 64   */
           val = MIN_BUF_BKT_SIZE;
       }
       MPIDI_tfctrl_enabled = buf_mem_max / ((long)nTasks * val);

       /* We need to have a minimum of 2 tokens.  If number of tokens is
        * less than 2, re-calculate by reducing the eager-limit.
        * If the number of tokens is still less than 2 then suggest a
        * new minimum buf_mem.
        */
       if (MPIDI_tfctrl_enabled < 2) {
          for (val ; val>=MIN_BUF_BKT_SIZE; val/=2) {
             MPIDI_tfctrl_enabled = (buf_mem_max) / ((long)nTasks * val);
             if (MPIDI_tfctrl_enabled >= 2) {
                break;
             }
          }
          /* If the number of flow control tokens is still less than two   */
          /* even with the eager_limit being reduced to 64, calculate a    */
          /* new buf_mem value for 2 tokens and eager_limit = 64.       */
          /* This will only happen if tasks>4K and original buf_mem=1M. */
          if (MPIDI_tfctrl_enabled < 2) {
             /* Sometimes we are off by 1 - due to integer arithmetic. */
             new_buf_mem_max = (2 * nTasks * MIN_BUF_BKT_SIZE);
             if ( (new_buf_mem_max <= BUFFER_MEM_MAX) && (!application_set_buf_mem) ) {
                 MPIDI_tfctrl_enabled = 2;
                 /* Reset val to mini (64) because the for loop above     */
                 /* would have changed it to 32.                          */
                 val = MIN_BUF_BKT_SIZE;
                 buf_mem_max = new_buf_mem_max;
                 if ( application_set_buf_mem ) {
                     TRACE_ERR("informational messge \n");
                 }
             }
             else {
                 /* Still not enough ...... Turn off eager send protocol */
                 MPIDI_tfctrl_enabled = 0;
                 val = 0;
                 if (((MPIDI_Process.verbose >= MPIDI_VERBOSE_SUMMARY_0)|| (MPIDI_Process.mp_infolevel > 0))
                      && (MPIR_Process.comm_world->rank == 0)) {
                      if ( application_set_buf_mem ) {
                           printf("ATTENTION: MP_BUFFER_MEM=%d was set too low, eager send protocol is disabled\n",
                                   *buf_mem_in); 
                      }
                 }
             }
          }
       }
       MPIDI_tfctrl_hwmark  = (MPIDI_tfctrl_enabled+1) / 2;  /* high water mark         */
       /* Eager_limit may have been changed -- either rounded up or reduced */
       if ( *eager_limit_in != val ) {
          if ( application_set_eager_limit && (*eager_limit_in > val)) {
             /* Only give warning on reduce. */
             printf("ATTENTION: eager limit is reduced from %d to %d \n",*eager_limit_in,val); fflush(stdout);
          }
          *eager_limit_in = val;

          /* putenv MP_EAGER_LIMIT if it is changed                          */
          /* MP_EAGER_LIMIT always has a value.                              */
          /* need to check whether MP_EAGER_LIMIT has been export            */
          EagerLimit = (char*)MPIU_Malloc(32 * sizeof(char));
          sprintf(EagerLimit, "MP_EAGER_LIMIT=%d",val);
          rc = putenv(EagerLimit);
          if (rc !=0) {
              TRACE_ERR("PUTENV with Eager Limit failed \n"); 
          }
       }
      }
    else {
       /* Eager_limit = 0, all messages will be sent using rendezvous protocol */
       MPIDI_tfctrl_enabled = 0;
       MPIDI_tfctrl_hwmark = 0;
    }
    /* user may want to set MP_EAGER_LIMIT to 0 or less than 256 */
    if (*eager_limit_in < MPIDI_Process.pt2pt.limits.application.immediate.remote)
        MPIDI_Process.pt2pt.limits.application.immediate.remote= *eager_limit_in;
    if (*eager_limit_in < MPIDI_Process.pt2pt.limits.application.eager.remote)
        MPIDI_Process.pt2pt.limits.application.eager.remote= *eager_limit_in;
#   ifdef DUMP_MM
     printf("MPIDI_tfctrl_enabled=%d eager_limit=%d  buf_mem=%d  buf_mem_max=%d\n",
             MPIDI_tfctrl_enabled,*eager_limit_in,*buf_mem_in,buf_mem_max); fflush(stdout);
#   endif 

}



/***************************************************************************/
/*  Initialize flex stack and stack pointers for each of FLEX_NUM slots    */
/***************************************************************************/

static char *MPIDI_init_flex(char *hp)
{
   int i,j,fcount;
   char** area;
   char *temp;
   int  size;
   int  kk;

   fcount = flex_count;
   if ((fcount = flex_count) == 0) {
       flex_size = 0;
       return hp;
   }
#  ifdef DUMP_MM
   printf("fcount=%d sizetable[fcount+1]=%d sizetable[1]=%d FLEX_NUM=%d overhead=%d\n",
         fcount,sizetable[fcount+1],sizetable[1],FLEX_NUM,OVERHEAD); fflush(stdout);
#  endif
   flex_size = (sizetable[fcount+1] - sizetable[1]) *FLEX_NUM +
       OVERHEAD * fcount * FLEX_NUM;
   kk=fcount *FLEX_NUM *sizeof(char *);
   size = ALIGN8(kk);
   area = (char **) MPIU_Malloc(size);
   malloc_list[nMallocs].ptr=(void *) area;
   malloc_list[nMallocs].size=size;
   malloc_list[nMallocs].type=FLEX;
   nMallocs++;


   flex_stack[0] = NULL;
   for(i =1; i <=fcount;  hp +=FLEX_NUM *(OVERHEAD +sizetable[i]), i++) {
#  ifdef DEBUG
       flex_heap[i] =hp;
#  endif
       flex_stack[i] =area;
       area += FLEX_NUM;
       flex_sp[i] =0;
#  ifdef DUMP_MM
       printf("MPIDI_init_flex() i=%d FLEX_NUM=%d fcount=%d i=%d flex_stack[i]=%p OVERHEAD=%d\n",
               i,FLEX_NUM,fcount,i,flex_stack[i],OVERHEAD); fflush(stdout);
#  endif
       for(j =0; j <FLEX_NUM; j++){
           flex_stack[i][j] =hp +j *(OVERHEAD +sizetable[i]);
#  ifdef DUMP_MM
            printf("j=%d hp=%p advance =%d final=%x\n", j,(int *)hp,j *(OVERHEAD +sizetable[i]),
                   (int *) (hp +j *(OVERHEAD +sizetable[i])));
            fflush(stdout);
#  endif
           ((flex_header *)flex_stack[i][j])->buddy  =FLEX;
           ((flex_header *)flex_stack[i][j])->ind    =i;
#  ifdef DUMP_MM
           printf("j=%d  flex_stack[%02d][%02d] = %p advance=%d flag buddy=0x%x ind=0x%x\n",
                   j,i,j,((int *)(flex_stack[i][j])),(sizetable[i]+OVERHEAD),
                   ((flex_header *)flex_stack[i][j])->buddy,
                   ((flex_header *)flex_stack[i][j])->ind); fflush(stdout);
#endif
       }
   }
   return hp;
}

/***************************************************************************/
/*  Initialize buddy heap for each of MAX_BUDDIES  slots                   */
/***************************************************************************/


static void MPIDI_alloc_buddies(int nums, int *space)
{
    int i;
        uint size;
    char *buddy,*prev;

    size = nums * (max_size +OVERHEAD);
    buddy = buddy_heap_ptr;
    if ((buddy_heap_ptr + size) > end_heap_ptr) {
      /* preallocated space is exhausted, the caller needs to make */
      /* a malloc() call for storing the message                   */
        *space=NO;
        return;
    }
    buddy_heap_ptr += size;
    free_buddy[max_bucket] = (buddy_header *)buddy;
    for(i =0, prev =NULL; i <nums; i++){
        ((buddy_header *)buddy)->buddy      =BUDDY;
        ((buddy_header *)buddy)->free       =1;
        ((buddy_header *)buddy)->bucket    =max_bucket;
        ((buddy_header *)buddy)->base_buddy =buddy;
        ((buddy_header *)buddy)->prev       =(buddy_header *)prev;
        prev =buddy;
#      ifdef DUMP_MM
       printf("ALLOC_BUDDY i=%2d buddy=%d free=%d bucket=%d base_buddy=%p prev=%p next=%p max_size=%d \n",
              i,(int)((buddy_header *)buddy)->buddy,(int)((buddy_header *)buddy)->free,
              (int)((buddy_header *)buddy)->bucket,(int *) ((buddy_header *)buddy)->base_buddy,
              (int *)((buddy_header *)buddy)->prev,(int *)(buddy + max_size +OVERHEAD),max_size);
              fflush(stdout);
#      endif
        buddy +=max_size +OVERHEAD;
        ((buddy_header *)prev)->next        =(buddy_header *)buddy;
    }
    ((buddy_header *)prev)->next =NULL;
}

/***************************************************************************/
/*  Initialize each of buddy slot                                          */
/***************************************************************************/

static void MPIDI_init_buddy(unsigned long buf_mem)
{
    int i;
    int buddy_num;
    size_t size;
    int space=YES;

#   ifdef DEBUG
    buddy_heap =buddy_heap_ptr;
#   endif
    for(i =0; i <= max_bucket; i++)
        free_buddy[i] =NULL;

    /* figure out how many buddies we wanna preallocate
     * size = BUFFER_MEM_SIZE >> 2;
     * size -= flex_size;
     */
    size = buf_mem;
    size = size / (max_size + OVERHEAD);
    size = (size == 0) ? 1 : (size > MAX_BUDDIES) ? MAX_BUDDIES : size;
    MPIDI_alloc_buddies(size,&space);
    if ( space == NO ) {
        TRACE_ERR("out of memory %s(%d)\n",__FILE__,__LINE__); 
        MPID_abort();
    }
}


/***************************************************************************/
/* initializ memory buffer for eager messages                              */
/***************************************************************************/

int MPIDI_mm_init(int nTasks,uint *eager_limit_in,unsigned long *buf_mem_in)
{
    int    i, bucket;
    size_t size;
    unsigned int    eager_limit;
    unsigned long buf_mem;
    unsigned long  buf_mem_max;
    int	   need_allocation = 1;

    MPIDI_calc_tokens(nTasks,eager_limit_in, buf_mem_in);
    buf_mem = *buf_mem_in;
    eager_limit = *eager_limit_in;
#   ifdef DEBUG
    printf("Eager Limit=%d  buf_mem=%ld tokens=%d hwmark=%d\n",
            eager_limit,buf_mem, MPIDI_tfctrl_enabled,MPIDI_tfctrl_hwmark);
    fflush(stdout);
#   endif
    if (eager_limit_in == 0) return 0;  /* no EA buffer is needed */
    maxMallocs=MAX_MALLOCS;
    malloc_list=(malloc_list_t *) MPIU_Malloc(maxMallocs * sizeof(malloc_list_t));
    if (malloc_list == NULL) return errno;
    nMallocs=0;
    mem_inuse=0;
    mem_hwmark=0;


    for (max_bucket=0,size=1 ; size < eager_limit ; max_bucket++,size *= 2);
    max_size = 1 << max_bucket;
    max_bucket -= (MIN_LOG2SIZE - 1);
    flex_count = min(FLEX_COUNT,max_bucket);
    for(i =1, size =MIN_SIZE; i <=MAX_BUCKET+1; i++, size =size << 1)
        sizetable[i] =size;
    sizetable[0] = 0;

    for (bucket=1, size = 1, i=1 ; bucket <= max_bucket; ) {
        sizetrans[i++] = bucket;
        if (i > size) {
            size *= 2;
            bucket++;
        }
    }
    sizetrans[i] = sizetrans[i-1];
     /* 65536 is for flex stack which is not part of buf_mem_size */
     heap = MPIU_Malloc(buf_mem + 65536);
     if (heap == NULL) return errno;
    malloc_list[nMallocs].ptr=(void *) heap;
    malloc_list[nMallocs].size=buf_mem + 65536;
    malloc_list[nMallocs].type=BUDDY;
    buddy_heap_ptr = heap;
    end_heap_ptr   = heap + buf_mem + 65536;
#   ifdef DUMP_MM
    printf("OVERHEAD=%d MAX_BUCKET=%d  TAB_SIZE=%d buddy_header size=%d mem_size=%ld\n",
            OVERHEAD,MAX_BUCKET,TAB_SIZE,sizeof(buddy_header),buf_mem); fflush(stdout);
#   endif
#   ifdef DEBUG
    printf("nMallocs=%d ptr=%p  size=%d  type=%d bPtr=%p ePtr=%p\n",
    nMallocs,(void *)malloc_list[nMallocs].ptr,malloc_list[nMallocs].size,
    malloc_list[nMallocs].type,buddy_heap_ptr,end_heap_ptr);
    fflush(stdout);
#   endif
    nMallocs++;

    buddy_heap_ptr = MPIDI_init_flex(heap);
    MPIDI_init_buddy(buf_mem);
#   ifdef MPIMM_DEBUG
    if (mpimm_std_debug) {
        printf("DevMemMgr uses\n");
        printf("\tmem-size=%d, maxbufsize=%ld\n",mem_size,max_size);
        printf("\tfcount=%d fsize=%ld, max_bucket=%d\n",
               flex_count,flex_size,max_bucket);
        fflush(stdout);
    }
#   endif
    return 0;
}


void MPIDI_close_mm()
{
    int i;

    if (nMallocs != 0) {
      for (i=0; i< nMallocs; i++) {
        MPIU_Free((void *) (malloc_list[i].ptr));
      }
     MPIU_Free(malloc_list);
    }
}

/****************************************************************************/
/* macros for MPIDI_mm_alloc() and MPIDI_mm_free()                          */
/****************************************************************************/

#define MPIDI_flex_alloc(bucket)   ((flex_sp[bucket] >=FLEX_NUM) ?          \
                                 NULL : (char *)(flex_stack[bucket]         \
                                          [flex_sp[bucket]++]) +OVERHEAD)

#define MPIDI_flex_free(ptr)                                                \
                                 int n;                                     \
                                 ptr =(char *)ptr -OVERHEAD;                \
                                 n =((flex_header *)ptr)->ind;              \
                                 flex_stack[n][--flex_sp[n]] =(char *)ptr;


#define MPIDI_remove_head(ind)                                             \
                                 {                                         \
                                    if((free_buddy[ind] =                  \
                                          free_buddy[ind]->next) !=NULL)   \
                                       free_buddy[ind]->prev =NULL;        \
                                 }

#define MPIDI_remove(bud)                                                  \
                                 {                                         \
                                    if(bud->prev !=NULL)                   \
                                       bud->prev->next =bud->next;         \
                                    else                                   \
                                       free_buddy[bud->bucket] =bud->next; \
                                    if(bud->next !=NULL)                   \
                                       bud->next->prev =bud->prev;         \
                                 }

#define MPIDI_add_head(bud,ind)                                            \
                                 {                                         \
                                    if((bud->next =free_buddy[ind]) !=NULL)\
                                       free_buddy[ind]->prev =bud;         \
                                    free_buddy[ind] =bud;                  \
                                    bud->prev =NULL;                       \
                                 }

#define MPIDI_fill_header(bud,ind,base)                                    \
                                 {                                         \
                                    bud->buddy        =BUDDY;              \
                                    bud->free         =1;                  \
                                    bud->bucket      =ind;                 \
                                    bud->base_buddy   =base;               \
                                 }

#define MPIDI_pair(bud,size) (((char*)(bud) - (bud)->base_buddy) & (size) ? \
                        (buddy_header *)((char*)(bud) - (size)) :           \
                        (buddy_header *)((char*)(bud) + (size)))

static buddy_header *MPIDI_split_buddy(int big,int bucket)
{
   buddy_header *bud,*buddy;
   char *base;
   int i;

   bud =free_buddy[big];
   MPIDI_remove_head(big);
   base =bud->base_buddy;
   for(i =big -1; i >=bucket; i--){
      buddy =(buddy_header *)((char*)bud +sizetable[i]);
      MPIDI_fill_header(buddy,i,base);
      MPIDI_add_head(buddy,i);
   }
   bud->bucket =bucket;
   bud->free =0;
   return bud;
}

static void *MPIDI_buddy_alloc(int bucket)
{
   int i;
   buddy_header *bud;
   int space=YES;

#  ifdef TRACE
   printf("(buddy) ");
#  endif
   if((bud =free_buddy[i =bucket]) ==NULL){
       i++;
       do {
           for(; i <=max_bucket; i++)
               if(free_buddy[i] !=NULL){
                   bud =MPIDI_split_buddy(i,bucket);
                   return (char *)bud +OVERHEAD;
               }
           MPIDI_alloc_buddies(1,&space);
           if (space == NO)
              return NULL;
           i = max_bucket;
       } while (1);
   }
   else{
      MPIDI_remove_head(bucket);
      bud->free =0;
      return (char *)bud +OVERHEAD;
   }
}

static buddy_header *MPIDI_merge_buddy(buddy_header *bud)
{
   buddy_header *buddy;
   int size;

   while( (bud->bucket <max_bucket)      &&
          (size = sizetable[bud->bucket]) &&
          ((buddy =MPIDI_pair(bud,size))->free)  &&
          (buddy->bucket ==bud->bucket) )
       {
           MPIDI_remove(buddy);
           bud =min(bud,buddy);
           bud->bucket++;
       }
   return bud;
}

static void MPIDI_buddy_free(void *ptr)
{
   buddy_header *bud;

#  ifdef TRACE
   printf("(buddy) ");
#  endif
   bud =(buddy_header *)((char *)ptr -OVERHEAD);
   if(bud->bucket <max_bucket)
      bud =MPIDI_merge_buddy(bud);
   bud->free =1;
   MPIDI_add_head(bud,bud->bucket);
}
#  ifdef TRACE
   int nAllocs =0;   /* number of times MPIDI_mm_alloc() is called */
   int nFree =0;   /* number of times MPIDI_mm_free() is called */
   int nM=0;       /* number of times MPIU_Malloc() is called   */
   int nF=0;       /* number of times MPIU_Free() is called     */
#  endif 
void *MPIDI_mm_alloc(size_t size)
{
   void *pt;
   int bucket,tmp;

   MPID_assert(size <= max_size);
   tmp = NORMSIZE(size);
   tmp =bucket =sizetrans[tmp];
   if(bucket >flex_count || (pt =MPIDI_flex_alloc(tmp)) ==NULL) {
      pt =MPIDI_buddy_alloc(bucket);
      if (MPIDI_Process.mp_statistics) {
          mem_inuse = mem_inuse + sizetable[tmp];
          if (mem_inuse > mem_hwmark) {
              mem_hwmark = mem_inuse;
          }
       }
   }
   if (pt == NULL) {
       MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
       pt=MPIU_Malloc(size);
       MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
       if (MPIDI_Process.mp_statistics) {
           mem_inuse = mem_inuse + sizetable[tmp];
          if (mem_inuse > mem_hwmark) {
             mem_hwmark = mem_inuse;
          }
       }
#      ifdef TRACE
       nM++;
       if (pt == NULL) {
           printf("ERROR  line=%d\n",__LINE__); fflush(stdout);
       } else {
         printf("malloc nM=%d size=%d pt=0x%p \n",nM,size,pt); fflush(stdout);
       }
#      endif
   }
#  ifdef TRACE
   printf("MPIDI_mm_alloc(%4d): %p\n",size,pt);
   nAllocs++;
#  endif
   return pt;
}

void MPIDI_mm_free(void *ptr, size_t size)
{
   int tmp,bucket;

   if (size > MAX_SIZE) {
      TRACE_ERR("Out of memory in %s(%d)\n",__FILE__,__LINE__);
      MPID_abort(); 
   }
   if ((ptr >= (void *) heap) && (ptr < (void *)end_heap_ptr)) {
     if(*((char *)ptr -OVERHEAD) ==FLEX){
        MPIDI_flex_free(ptr);
     }
     else
        MPIDI_buddy_free(ptr);
     if (MPIDI_Process.mp_statistics) {
         tmp = NORMSIZE(size);
         bucket =sizetrans[tmp];
         mem_inuse = mem_inuse - sizetable[bucket];
         if (mem_inuse > mem_hwmark) {
             mem_hwmark = mem_inuse;
         }
     }
   } else {
     if (!ptr) {
        TRACE_ERR("NULL ptr passed MPIDI_mm_free() in %s(%d)\n",__FILE__,__LINE__);
        MPID_abort();
     }
     if (MPIDI_Process.mp_statistics) {
        tmp = NORMSIZE(size);
        bucket =sizetrans[tmp];
         mem_inuse = mem_inuse - sizetable[bucket];
         if (mem_inuse > mem_hwmark) {
             mem_hwmark = mem_inuse;
         }
     }
     MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
#    ifdef TRACE
     nF++;
     printf("free nF=%d size=%d ptr=0x%p \n",nF,sizetable[bucket],ptr); fflush(stdout);
#    endif
     MPIU_Free(ptr);
     MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
   }
#  ifdef TRACE
   nFrees++;
   printf("MPIDI_mm_free:     %p \n",ptr);
#  endif
}
#endif   /* #if TOKEN_FLOW_CONTROL   */
