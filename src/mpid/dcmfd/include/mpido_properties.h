/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/defines.h
 * \brief ???
 */

#ifndef _MPIDO_PROPERTIES_H_
#define _MPIDO_PROPERTIES_H_


/* Each #define below corresponds to a bit position in the Embedded_Info_Set */

/****************************************************************************/
/* Properties bits
 * Each #define corresponds to a bit position in the Embedded_Info_Set and is
 * used for picking proper algorithms for collectives, etc
 ****************************************************************************/

#define MPIDO_MAX_NUM_BITS 128
/****************************/
/* Communicator properties  */
/****************************/

/* if comm size is power of 2 */
#define MPIDO_POF2_COMM                                                    0  
/* Is comm in threaded mode? */
#define MPIDO_SINGLE_THREAD_MODE                                           1
/* is comm a torus? */
#define MPIDO_TORUS_COMM                                                   2
/* does comm support tree? */
#define MPIDO_TREE_COMM                                                    3 
#define MPIDO_RECT_COMM                                                    4
#define MPIDO_IRREG_COMM                                                   5
/* does comm have global context? */
#define MPIDO_GLOBAL_CONTEXT                                               6
/* alg works only with tree op/type */
#define MPIDO_TREE_OP_TYPE                                                 7
/* alg works with supported op/type */
#define MPIDO_TORUS_OP_TYPE                                                8
/* alg works with any op/type */
/* #warning this might not actually be used. Ahmad needs to find out if it is */
#define MPIDO_ANY_OP_TYPE                                                  9
/* does send buff need to be contig? */
#define MPIDO_SBUFF_CONTIG                                                 10
/* does recv buff need to be contig?*/
#define MPIDO_RBUFF_CONTIG                                                 11
/* does recv buff need to be contin?*/
#define MPIDO_RBUFF_CONTIN                                                 12
/* is buff size multiple of 4? */
#define MPIDO_BUFF_SIZE_MUL4                                               13
#define MPIDO_BUFF_ALIGNED                                                 14
#define MPIDO_THREADED_MODE                                                15
/*#define MPIDO_USE_SMP_TREE_SHORTCUT                                      16*/

/*******************/
/* Collective bits */
/*******************/
#define MPIDO_USE_NOTREE_OPT_COLLECTIVES                                   17
#define MPIDO_USE_MPICH                                                    18

/* Collectives with complicated cutoffs from multiple algorithms have this
 * set if the user does a MPIDO_{}={} selection to override the cutoffs */
#define MPIDO_ALLGATHER_ENVVAR                                             19
#define MPIDO_ALLGATHERV_ENVVAR                                            20
#define MPIDO_ALLREDUCE_ENVVAR                                             21
#define MPIDO_BCAST_ENVVAR                                                 22
#define MPIDO_REDUCE_ENVVAR                                                23
/* In case we have complicated cutoffs in future algorithms */
#define MPIDO_ENVVAR_RESERVED1                                             24
#define MPIDO_ENVVAR_RESERVED2                                             25

/* this indicates the bit index that represent properties for collectives */
#define MPIDO_COLL_PROP                                                    26

/* Allgather protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_ABCAST_ALLGATHER                                         26
#define MPIDO_USE_ABINOM_BCAST_ALLGATHER                                   27
#define MPIDO_USE_ALLTOALL_ALLGATHER                                       28
#define MPIDO_USE_ARECT_BCAST_ALLGATHER                                    29
#define MPIDO_USE_ALLREDUCE_ALLGATHER                                      30
#define MPIDO_USE_BCAST_ALLGATHER                                          31
#define MPIDO_USE_BINOM_BCAST_ALLGATHER                                    32
#define MPIDO_USE_RECT_BCAST_ALLGATHER                                     33
#define MPIDO_USE_RECT_DPUT_ALLGATHER                                      34
#define MPIDO_USE_MPICH_ALLGATHER                                          35  
#define MPIDO_ALLGATHER_RESERVED1                                          36
/* Allgather preallreduce flag. "Nice" datatypes can use this to bypass a 
 * sanity-checking allreduce inside allgather */
#define MPIDO_USE_PREALLREDUCE_ALLGATHER                                   37


/* Allgatherv protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_ALLREDUCE_ALLGATHERV                                     38
#define MPIDO_USE_BCAST_ALLGATHERV                                         39
#define MPIDO_USE_ALLTOALL_ALLGATHERV                                      40
#define MPIDO_USE_ABCAST_ALLGATHERV                                        41
#define MPIDO_USE_ARECT_BCAST_ALLGATHERV                                   42
#define MPIDO_USE_ABINOM_BCAST_ALLGATHERV                                  43
#define MPIDO_USE_RECT_BCAST_ALLGATHERV                                    44
#define MPIDO_USE_BINOM_BCAST_ALLGATHERV                                   45
#define MPIDO_USE_RECT_DPUT_ALLGATHERV                                     46
#define MPIDO_USE_MPICH_ALLGATHERV                                         47   
#define MPIDO_ALLGATHERV_RESERVED1                                         48
#define MPIDO_USE_PREALLREDUCE_ALLGATHERV                                  49

/* Allreduce protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_TREE_ALLREDUCE                                           50
#define MPIDO_USE_CCMI_TREE_ALLREDUCE                                      51
#define MPIDO_USE_PIPELINED_TREE_ALLREDUCE                                 52
#define MPIDO_USE_RECT_ALLREDUCE                                           53
#define MPIDO_USE_RECTRING_ALLREDUCE                                       54
#define MPIDO_USE_BINOM_ALLREDUCE                                          55
#define MPIDO_USE_ARECT_ALLREDUCE                                          56
#define MPIDO_USE_ARECTRING_ALLREDUCE                                      57
#define MPIDO_USE_ABINOM_ALLREDUCE                                         58
#define MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE                               59
#define MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE                              60
#define MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE                            61
#define MPIDO_USE_MPICH_ALLREDUCE                                          62
#define MPIDO_ALLREDUCE_RESERVED1                                          63
/* Controls whether or not we reuse storage in allreduce */
#define MPIDO_USE_STORAGE_ALLREDUCE                                        64



/* Alltoall(vw) protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_TORUS_ALLTOALL                                           65
#define MPIDO_USE_MPICH_ALLTOALL                                           66

/*---------------------------------------------------------------------------*/
#define MPIDO_USE_TORUS_ALLTOALLV                                          67
#define MPIDO_USE_MPICH_ALLTOALLV                                          68  

/*---------------------------------------------------------------------------*/
#define MPIDO_USE_TORUS_ALLTOALLW                                          69
#define MPIDO_USE_MPICH_ALLTOALLW                                          70  

#define MPIDO_ALLTOALL_RESERVED1                                           71

/* Alltoall(vw) use a large number of arrays. This lazy allocs them at comm
 * create time */
#define MPIDO_USE_PREMALLOC_ALLTOALL                                       72


/* Barrier protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_BINOM_BARRIER                                            73
#define MPIDO_USE_GI_BARRIER                                               74
#define MPIDO_USE_RECT_BARRIER                                             75
#define MPIDO_USE_MPICH_BARRIER                                            76

/* Local barriers */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_BINOM_LBARRIER                                           77
#define MPIDO_USE_LOCKBOX_LBARRIER                                         78
#define MPIDO_USE_RECT_LOCKBOX_LBARRIER                                    79
#define MPIDO_BARRIER_RESERVED1                                            80

/* Bcast protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_ABINOM_BCAST                                             81
#define MPIDO_USE_ARECT_BCAST                                              82
#define MPIDO_USE_BINOM_BCAST                                              83
#define MPIDO_USE_BINOM_SINGLETH_BCAST                                     84
#define MPIDO_USE_RECT_BCAST                                               85
#define MPIDO_USE_RECT_DPUT_BCAST                                          86
#define MPIDO_USE_RECT_SINGLETH_BCAST                                      87
#define MPIDO_USE_SCATTER_GATHER_BCAST                                     88
#define MPIDO_USE_TREE_BCAST                                               89
#define MPIDO_USE_MPICH_BCAST                                              90
#define MPIDO_BCAST_RESERVED1                                              91
#define MPIDO_BCAST_RESERVED2                                              92

/* Exscan in case someone implements something */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_MPICH_EXSCAN                                             93
#define MPIDO_EXSCAN_RESERVED1                                             94

/* Gather protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_REDUCE_GATHER                                            95
#define MPIDO_USE_MPICH_GATHER                                             96
#define MPIDO_GATHER_RESERVED1                                             97

/* Gatherv protocols in case someone implements something */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_MPICH_GATHERV                                            98
#define MPIDO_GATHERV_RESERVED1                                            99

/* Reduce protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_BINOM_REDUCE                                             100
#define MPIDO_USE_CCMI_TREE_REDUCE                                         101
#define MPIDO_USE_RECT_REDUCE                                              102
#define MPIDO_USE_RECTRING_REDUCE                                          103
#define MPIDO_USE_TREE_REDUCE                                              104
#define MPIDO_USE_MPICH_REDUCE                                             105
#define MPIDO_USE_ALLREDUCE_REDUCE                                         106
#define MPIDO_USE_PREMALLOC_REDUCE                                         107
/* Controls whether or not we reuse storage in reduce */
#define MPIDO_USE_STORAGE_REDUCE                                           108

/* Reduce_scatter protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_REDUCESCATTER                                            109
#define MPIDO_USE_MPICH_REDUCESCATTER                                      110
#define MPIDO_REDUCESCATTER_RESERVED1                                      111

/* Scan protocols in case someone implements something */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_MPICH_SCAN                                               112
#define MPIDO_SCAN_RESERVED1                                               113
                                              
/* Scatter protocols */                                              
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_BCAST_SCATTER                                            114  
#define MPIDO_USE_MPICH_SCATTER                                            115
#define MPIDO_SCATTER_RESERVED1                                            116

/* Scatterv protocols */
/*---------------------------------------------------------------------------*/
#define MPIDO_USE_BCAST_SCATTERV                                           117
#define MPIDO_USE_ALLTOALL_SCATTERV                                        118
#define MPIDO_USE_ALLREDUCE_SCATTERV                                       119
#define MPIDO_USE_MPICH_SCATTERV                                           120
#define MPIDO_SCATTERV_RESERVED1                                           121
#define MPIDO_USE_PREALLREDUCE_SCATTERV                                    122

#define MPIDO_USE_CCMI_TREE_BCAST                                          123
#define MPIDO_USE_CCMI_TREE_DPUT_BCAST                                     124
#define MPIDO_USE_CCMI_GI_BARRIER                                          125

extern char * mpido_algorithms[];
#endif
