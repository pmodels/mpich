#!/bin/sh
# 
# This script will safely convert generated functions that begin with "MPIX_" or
# "MPIX_T_" into non-extension, MPI Forum sanctioned functions that start with
# "MPI_" and "MPI_T_" respectively.
# 
# Script callers should set FIXUP_MPI_H="src/include/mpi.h.in src/mpi/errhan/errnames.txt"
# or similar if they want prototypes to also be rewritten.

set -x

perl -p -i -e 's/\b(P)?MPIX_T_init_thread\b/$1MPI_T_init_thread/g; s/\*\*mpix_t_init_thread\b/**mpi_t_init_thread/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/mpit_init_thread.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_finalize\b/$1MPI_T_finalize/g; s/\*\*mpix_t_finalize\b/**mpi_t_finalize/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/mpit_finalize.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_enum_get_info\b/$1MPI_T_enum_get_info/g; s/\*\*mpix_t_enum_get_info\b/**mpi_t_enum_get_info/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/enum_get_info.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_enum_get_item\b/$1MPI_T_enum_get_item/g; s/\*\*mpix_t_enum_get_item\b/**mpi_t_enum_get_item/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/enum_get_item.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_cvar_get_num\b/$1MPI_T_cvar_get_num/g; s/\*\*mpix_t_cvar_get_num\b/**mpi_t_cvar_get_num/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cvar_get_num.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_cvar_get_info\b/$1MPI_T_cvar_get_info/g; s/\*\*mpix_t_cvar_get_info\b/**mpi_t_cvar_get_info/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cvar_get_info.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_cvar_handle_alloc\b/$1MPI_T_cvar_handle_alloc/g; s/\*\*mpix_t_cvar_handle_alloc\b/**mpi_t_cvar_handle_alloc/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cvar_handle_alloc.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_cvar_handle_free\b/$1MPI_T_cvar_handle_free/g; s/\*\*mpix_t_cvar_handle_free\b/**mpi_t_cvar_handle_free/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cvar_handle_free.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_cvar_read\b/$1MPI_T_cvar_read/g; s/\*\*mpix_t_cvar_read\b/**mpi_t_cvar_read/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cvar_read.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_cvar_write\b/$1MPI_T_cvar_write/g; s/\*\*mpix_t_cvar_write\b/**mpi_t_cvar_write/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cvar_write.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_get_num\b/$1MPI_T_pvar_get_num/g; s/\*\*mpix_t_pvar_get_num\b/**mpi_t_pvar_get_num/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_get_num.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_get_info\b/$1MPI_T_pvar_get_info/g; s/\*\*mpix_t_pvar_get_info\b/**mpi_t_pvar_get_info/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_get_info.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_session_create\b/$1MPI_T_pvar_session_create/g; s/\*\*mpix_t_pvar_session_create\b/**mpi_t_pvar_session_create/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_session_create.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_session_free\b/$1MPI_T_pvar_session_free/g; s/\*\*mpix_t_pvar_session_free\b/**mpi_t_pvar_session_free/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_session_free.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_handle_alloc\b/$1MPI_T_pvar_handle_alloc/g; s/\*\*mpix_t_pvar_handle_alloc\b/**mpi_t_pvar_handle_alloc/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_handle_alloc.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_handle_free\b/$1MPI_T_pvar_handle_free/g; s/\*\*mpix_t_pvar_handle_free\b/**mpi_t_pvar_handle_free/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_handle_free.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_start\b/$1MPI_T_pvar_start/g; s/\*\*mpix_t_pvar_start\b/**mpi_t_pvar_start/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_start.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_stop\b/$1MPI_T_pvar_stop/g; s/\*\*mpix_t_pvar_stop\b/**mpi_t_pvar_stop/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_stop.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_read\b/$1MPI_T_pvar_read/g; s/\*\*mpix_t_pvar_read\b/**mpi_t_pvar_read/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_read.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_write\b/$1MPI_T_pvar_write/g; s/\*\*mpix_t_pvar_write\b/**mpi_t_pvar_write/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_write.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_reset\b/$1MPI_T_pvar_reset/g; s/\*\*mpix_t_pvar_reset\b/**mpi_t_pvar_reset/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_reset.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_pvar_readreset\b/$1MPI_T_pvar_readreset/g; s/\*\*mpix_t_pvar_readreset\b/**mpi_t_pvar_readreset/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/pvar_readreset.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_category_get_num\b/$1MPI_T_category_get_num/g; s/\*\*mpix_t_category_get_num\b/**mpi_t_category_get_num/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cat_get_num.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_category_get_info\b/$1MPI_T_category_get_info/g; s/\*\*mpix_t_category_get_info\b/**mpi_t_category_get_info/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cat_get_info.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_category_get_cvars\b/$1MPI_T_category_get_cvars/g; s/\*\*mpix_t_category_get_cvars\b/**mpi_t_category_get_cvars/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cat_get_cvars.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_category_get_pvars\b/$1MPI_T_category_get_pvars/g; s/\*\*mpix_t_category_get_pvars\b/**mpi_t_category_get_pvars/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cat_get_pv.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_category_get_categories\b/$1MPI_T_category_get_categories/g; s/\*\*mpix_t_category_get_categories\b/**mpi_t_category_get_categories/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cat_get_categories.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_T_category_changed\b/$1MPI_T_category_changed/g; s/\*\*mpix_t_category_changed\b/**mpi_t_category_changed/g; s/\bMPID_STATE_MPIX_/MPID_STATE_MPI_/g' \
    'src/mpi_t/cat_changed.c' $FIXUP_MPI_H
