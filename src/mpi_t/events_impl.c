/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>

int MPIR_T_event_callback_get_info_impl(MPI_T_event_registration event_registration,
                                        MPI_T_cb_safety cb_safety, MPIR_Info ** info_used_ptr)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_callback_set_info_impl(MPI_T_event_registration event_registration,
                                        MPI_T_cb_safety cb_safety, MPIR_Info * info_ptr)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_copy_impl(MPI_T_event_instance event_instance, void *buffer)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_get_index_impl(const char *name, int *event_index)
{
    return MPI_T_ERR_INVALID_NAME;
}

int MPIR_T_event_get_info_impl(int event_index, char *name, int *name_len, int *verbosity,
                               MPI_Datatype array_of_datatypes[], MPI_Aint array_of_displacements[],
                               int *num_elements, MPI_T_enum * enumtype, MPI_Info * info,
                               char *desc, int *desc_len, int *bind)
{
    return MPI_T_ERR_INVALID_INDEX;
}

int MPIR_T_event_get_num_impl(int *num_events)
{
    *num_events = 0;
    return MPI_SUCCESS;
}

int MPIR_T_event_get_source_impl(MPI_T_event_instance event_instance, int *source_index)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_get_timestamp_impl(MPI_T_event_instance event_instance,
                                    MPI_Count * event_timestamp)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_handle_alloc_impl(int event_index, void *obj_handle, MPIR_Info * info_ptr,
                                   MPI_T_event_registration * event_registration)
{
    return MPI_T_ERR_INVALID_INDEX;
}

int MPIR_T_event_handle_free_impl(MPI_T_event_registration event_registration, void *user_data,
                                  MPI_T_event_free_cb_function free_cb_function)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_handle_get_info_impl(MPI_T_event_registration event_registration,
                                      MPIR_Info ** info_used_ptr)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_handle_set_info_impl(MPI_T_event_registration event_registration,
                                      MPIR_Info * info_ptr)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_read_impl(MPI_T_event_instance event_instance, int element_index, void *buffer)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_register_callback_impl(MPI_T_event_registration event_registration,
                                        MPI_T_cb_safety cb_safety, MPIR_Info * info_ptr,
                                        void *user_data, MPI_T_event_cb_function event_cb_function)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_event_set_dropped_handler_impl(MPI_T_event_registration event_registration,
                                          MPI_T_event_dropped_cb_function dropped_cb_function)
{
    return MPI_T_ERR_INVALID_HANDLE;
}

int MPIR_T_source_get_info_impl(int source_index, char *name, int *name_len, char *desc,
                                int *desc_len, MPI_T_source_order * ordering,
                                MPI_Count * ticks_per_second, MPI_Count * max_ticks,
                                MPI_Info * info)
{
    return MPI_T_ERR_INVALID_INDEX;
}

int MPIR_T_source_get_num_impl(int *num_sources)
{
    *num_sources = 0;
    return MPI_SUCCESS;
}

int MPIR_T_source_get_timestamp_impl(int source_index, MPI_Count * timestamp)
{
    return MPI_T_ERR_INVALID_INDEX;
}

int MPIR_T_category_get_num_events_impl(int cat_index, int *num_events)
{
    return MPI_T_ERR_INVALID_INDEX;
}

int MPIR_T_category_get_events_impl(int cat_index, int len, int indices[])
{
    return MPI_T_ERR_INVALID_INDEX;
}
