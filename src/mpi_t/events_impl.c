/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>

static int global_num_sources;
static MPIR_T_source_t *sources;
static int global_num_events;
static MPIR_T_event_t *events;

void MPIR_T_register_event(int source_index, const char *name, MPIR_T_verbosity_t verbosity,
                           MPI_Datatype array_of_datatypes[], MPI_Aint array_of_displacements[],
                           MPI_Aint num_elements, const char *desc, MPIR_T_bind_t bind,
                           const char *category, int *index)
{
#ifdef HAVE_MPIT_EVENTS
    MPIR_T_event_t *event = MPL_malloc(sizeof(MPIR_T_event_t), MPL_MEM_MPIT);

#ifdef HAVE_ERROR_CHECKING
    event->kind = MPIR_T_EVENT;
#endif
    event->index = global_num_events++;
    event->source_index = source_index;
    event->name = MPL_strdup(name);
    event->verbosity = verbosity;
    event->array_of_datatypes = MPL_malloc(sizeof(MPI_Datatype) * num_elements, MPL_MEM_MPIT);
    MPIR_Memcpy(event->array_of_datatypes, array_of_datatypes, sizeof(MPI_Datatype) * num_elements);
    event->array_of_displacements = MPL_malloc(sizeof(MPI_Aint) * num_elements, MPL_MEM_MPIT);
    MPIR_Memcpy(event->array_of_displacements, array_of_displacements,
                sizeof(MPI_Aint) * num_elements);
    event->num_elements = num_elements;
    event->desc = MPL_strdup(desc);
    event->bind = bind;

    /* initialize registration handle list */
    event->reg_list_head = NULL;
    event->reg_list_tail = NULL;

    /* stash structure in hash table */
    HASH_ADD_INT(events, index, event, MPL_MEM_MPIT);

    /* add event type to category */
    MPIR_T_cat_add_event(category, event->index);

    *index = event->index;
#endif /* HAVE_MPIT_EVENTS */
}

void MPIR_T_register_source(const char *name, const char *desc, MPI_T_source_order ordering,
                            MPIR_T_timestamp_fn timestamp_fn, MPI_Count ticks_per_second,
                            MPI_Count max_ticks, int *index)
{
#ifdef HAVE_MPIT_EVENTS
    MPIR_T_source_t *source = MPL_malloc(sizeof(MPIR_T_source_t), MPL_MEM_MPIT);

#ifdef HAVE_ERROR_CHECKING
    source->kind = MPIR_T_SOURCE;
#endif
    source->index = global_num_sources++;
    source->name = MPL_strdup(name);
    source->desc = MPL_strdup(desc);
    source->ordering = ordering;
    source->timestamp_fn = timestamp_fn;
    if (timestamp_fn != NULL) {
        source->ticks_per_second = ticks_per_second;
        source->max_ticks = max_ticks;
    } else {
        long long int default_ticks;
        MPL_ticks_per_second(&default_ticks);
        source->ticks_per_second = (MPI_Count) default_ticks;
        source->max_ticks = MPIR_COUNT_MAX;     /* FIXME: MPL should report max ticks */
    }

    /* stash structure in hash table */
    HASH_ADD_INT(sources, index, source, MPL_MEM_MPIT);

    *index = source->index;
#endif /* HAVE_MPIT_EVENTS */
}

void MPIR_T_event_instance(int event_index, MPI_T_cb_safety cb_safety, void *data)
{
    MPIR_T_event_t *event;
    HASH_FIND_INT(events, &event_index, event);
    if (event == NULL) {
        return;
    }

    /* iterate over all registration handles for this event */
    for (MPIR_T_event_registration_t * event_reg = event->reg_list_head; event_reg;
         event_reg = event_reg->next) {
        int i;
        /* search for callback with appropriate safety */
        for (i = cb_safety; i < 4; i++) {
            if (event_reg->callbacks[i].cb_function != NULL) {
                /* handle any previously dropped events */
                if (event_reg->dropped_count > 0) {
                    event_reg->dropped_cb(event_reg->dropped_count, event_reg,
                                          event_reg->event->source_index, cb_safety,
                                          event_reg->callbacks[i].user_data);

                    /* reset dropped counter */
                    event_reg->dropped_count = 0;
                }

                MPIR_T_source_t *source;
                MPIR_T_event_instance_t event_instance;
                HASH_FIND_INT(sources, &event->source_index, source);
#ifdef HAVE_ERROR_CHECKING
                event_instance.kind = MPIR_T_EVENT_INSTANCE;
#endif
                event_instance.event = event;
                MPIR_T_source_get_timestamp_impl(event->source_index, &event_instance.timestamp);
                event_instance.data = data;
                event_reg->callbacks[i].cb_function(&event_instance, event_reg, cb_safety,
                                                    event_reg->callbacks[i].user_data);

                /* callback was found and executed */
                break;
            }
        }

        /* if no callback was executed, increment the dropped count */
        if (i == 4 && event_reg->dropped_cb != NULL) {
            event_reg->dropped_count++;
        }
    }

    return;
}

static MPI_Count default_timestamp(void)
{
    MPL_time_t t;
    long long ticks;

    MPL_wtime(&t);
    MPL_wtime_to_ticks(&t, &ticks);

    return (MPI_Count) ticks;
}

void MPIR_T_events_finalize(void)
{
    MPIR_T_event_t *event, *etmp;
    HASH_ITER(hh, events, event, etmp) {
        HASH_DEL(events, event);
        MPL_free(event->name);
        MPL_free(event->array_of_datatypes);
        MPL_free(event->array_of_displacements);
        MPL_free(event->desc);
        MPL_free(event);
    }

    MPIR_T_source_t *source, *stmp;
    HASH_ITER(hh, sources, source, stmp) {
        HASH_DEL(sources, source);
        MPL_free(source->name);
        MPL_free(source->desc);
        MPL_free(source);
    }
}

int MPIR_T_event_callback_get_info_impl(MPI_T_event_registration event_registration,
                                        MPI_T_cb_safety cb_safety, MPIR_Info ** info_used_ptr)
{
    /* no info support */
    *info_used_ptr = NULL;

    return MPI_SUCCESS;
}

int MPIR_T_event_callback_set_info_impl(MPI_T_event_registration event_registration,
                                        MPI_T_cb_safety cb_safety, MPIR_Info * info_ptr)
{
    /* no info support */
    return MPI_SUCCESS;
}

int MPIR_T_event_copy_impl(MPI_T_event_instance event_instance, void *buffer)
{
    MPIR_T_event_t *event = event_instance->event;

    /* should we create a struct type? */
    for (int i = 0; i < event->num_elements; i++) {
        MPI_Datatype dt = event->array_of_datatypes[i];
        MPI_Aint displ = event->array_of_displacements[i];

        MPIR_Localcopy((char *) event_instance->data + displ, 1, dt, (char *) buffer + displ, 1,
                       dt);
    }

    return MPI_SUCCESS;
}

int MPIR_T_event_get_index_impl(const char *name, int *event_index)
{
    MPIR_T_event_t *event, *etmp;
    HASH_ITER(hh, events, event, etmp) {
        if (strcmp(name, event->name) == 0) {
            *event_index = event->index;
            return MPI_SUCCESS;
        }
    }

    return MPI_T_ERR_INVALID_NAME;
}

int MPIR_T_event_get_info_impl(int event_index, char *name, int *name_len, int *verbosity,
                               MPI_Datatype array_of_datatypes[], MPI_Aint array_of_displacements[],
                               int *num_elements, MPI_T_enum * enumtype, MPI_Info * info,
                               char *desc, int *desc_len, int *bind)
{
    MPIR_T_event_t *event;
    HASH_FIND_INT(events, &event_index, event);
    if (event == NULL) {
        return MPI_T_ERR_INVALID_INDEX;
    }

    MPIR_T_strncpy(name, event->name, name_len);
    *verbosity = event->verbosity;
    if (num_elements != NULL) {
        for (int i = 0; i < MPL_MIN(*num_elements, event->num_elements); i++) {
            if (array_of_datatypes != NULL)
                array_of_datatypes[i] = event->array_of_datatypes[i];
            if (array_of_displacements != NULL)
                array_of_displacements[i] = event->array_of_displacements[i];
        }
        *num_elements = event->num_elements;
    }
    if (enumtype != NULL)
        *enumtype = event->enumtype;
    if (info != NULL)
        *info = MPI_INFO_NULL;  /* no info support yet */
    MPIR_T_strncpy(desc, event->desc, desc_len);
    *bind = event->bind;

    return MPI_SUCCESS;
}

int MPIR_T_event_get_num_impl(int *num_events)
{
    *num_events = global_num_events;
    return MPI_SUCCESS;
}

int MPIR_T_event_get_source_impl(MPI_T_event_instance event_instance, int *source_index)
{
    *source_index = event_instance->event->source_index;

    return MPI_SUCCESS;
}

int MPIR_T_event_get_timestamp_impl(MPI_T_event_instance event_instance,
                                    MPI_Count * event_timestamp)
{
    *event_timestamp = event_instance->timestamp;

    return MPI_SUCCESS;
}

int MPIR_T_event_handle_alloc_impl(int event_index, void *obj_handle, MPIR_Info * info_ptr,
                                   MPI_T_event_registration * event_registration)
{
    MPIR_T_event_t *event;
    HASH_FIND_INT(events, &event_index, event);
    if (event == NULL) {
        return MPI_T_ERR_INVALID_INDEX;
    }

    MPIR_T_event_registration_t *event_reg =
        MPL_malloc(sizeof(MPIR_T_event_registration_t), MPL_MEM_MPIT);
#ifdef HAVE_ERROR_CHECKING
    event_reg->kind = MPIR_T_EVENT_REG;
#endif
    event_reg->event = event;
    LL_APPEND(event->reg_list_head, event->reg_list_tail, event_reg);
    if (event->bind != MPI_T_BIND_NO_OBJECT)
        event_reg->obj_handle = obj_handle;

    /* initialize callbacks */
    memset(event_reg->callbacks, 0, sizeof(event_reg->callbacks));
    event_reg->dropped_cb = NULL;
    event_reg->dropped_count = 0;

    *event_registration = event_reg;

    return MPI_SUCCESS;
}

int MPIR_T_event_handle_free_impl(MPI_T_event_registration event_registration, void *user_data,
                                  MPI_T_event_free_cb_function free_cb_function)
{
    LL_DELETE(event_registration->event->reg_list_head, event_registration->event->reg_list_tail,
              event_registration);
    if (free_cb_function != NULL)
        (free_cb_function) (event_registration, MPI_T_CB_REQUIRE_NONE, user_data);
    MPL_free(event_registration);

    return MPI_SUCCESS;
}

int MPIR_T_event_handle_get_info_impl(MPI_T_event_registration event_registration,
                                      MPIR_Info ** info_used_ptr)
{
    /* no info support */
    *info_used_ptr = NULL;

    return MPI_SUCCESS;
}

int MPIR_T_event_handle_set_info_impl(MPI_T_event_registration event_registration,
                                      MPIR_Info * info_ptr)
{
    /* no info support */
    return MPI_SUCCESS;
}

int MPIR_T_event_read_impl(MPI_T_event_instance event_instance, int element_index, void *buffer)
{
    MPIR_T_event_t *event = event_instance->event;
    if (element_index >= event->num_elements) {
        return MPI_T_ERR_INVALID_INDEX;
    }

    MPI_Datatype dt = event->array_of_datatypes[element_index];
    MPIR_Localcopy((char *) event_instance->data + event->array_of_displacements[element_index], 1,
                   dt, buffer, 1, dt);

    return MPI_SUCCESS;
}

int MPIR_T_event_register_callback_impl(MPI_T_event_registration event_registration,
                                        MPI_T_cb_safety cb_safety, MPIR_Info * info_ptr,
                                        void *user_data, MPI_T_event_cb_function event_cb_function)
{
    event_registration->callbacks[cb_safety].cb_function = event_cb_function;
    event_registration->callbacks[cb_safety].user_data = user_data;

    return MPI_SUCCESS;
}

int MPIR_T_event_set_dropped_handler_impl(MPI_T_event_registration event_registration,
                                          MPI_T_event_dropped_cb_function dropped_cb_function)
{
    event_registration->dropped_cb = dropped_cb_function;

    return MPI_SUCCESS;
}

int MPIR_T_source_get_info_impl(int source_index, char *name, int *name_len, char *desc,
                                int *desc_len, MPI_T_source_order * ordering,
                                MPI_Count * ticks_per_second, MPI_Count * max_ticks,
                                MPI_Info * info)
{
    MPIR_T_source_t *source;
    HASH_FIND_INT(sources, &source_index, source);
    if (source == NULL) {
        return MPI_T_ERR_INVALID_INDEX;
    }

    MPIR_T_strncpy(name, source->name, name_len);
    MPIR_T_strncpy(desc, source->desc, desc_len);
    *ordering = source->ordering;
    *ticks_per_second = source->ticks_per_second;
    *max_ticks = source->max_ticks;
    if (info != NULL)
        *info = MPI_INFO_NULL;  /* no info support yet */

    return MPI_SUCCESS;
}

int MPIR_T_source_get_num_impl(int *num_sources)
{
    *num_sources = global_num_sources;
    return MPI_SUCCESS;
}

int MPIR_T_source_get_timestamp_impl(int source_index, MPI_Count * timestamp)
{
    MPIR_T_source_t *source;
    HASH_FIND_INT(sources, &source_index, source);
    if (source == NULL) {
        return MPI_T_ERR_INVALID_INDEX;
    }

    MPI_Count ts = source->timestamp_fn ? source->timestamp_fn() : default_timestamp();
    if (ts < 0) {
        return MPI_T_ERR_NOT_SUPPORTED;
    }

    *timestamp = ts;

    return MPI_SUCCESS;
}

int MPIR_T_category_get_num_events_impl(int cat_index, int *num_events)
{
    cat_table_entry_t *cat;

    cat = (cat_table_entry_t *) utarray_eltptr(cat_table, cat_index);
    if (cat == NULL) {
        return MPI_T_ERR_INVALID_INDEX;
    }

    *num_events = utarray_len(cat->event_indices);

    return MPI_SUCCESS;
}

int MPIR_T_category_get_events_impl(int cat_index, int len, int indices[])
{
    cat_table_entry_t *cat;
    int num_events;

    cat = (cat_table_entry_t *) utarray_eltptr(cat_table, cat_index);
    if (cat == NULL) {
        return MPI_T_ERR_INVALID_INDEX;
    }

    num_events = utarray_len(cat->event_indices);
    for (int i = 0; i < MPL_MIN(len, num_events); i++) {
        indices[i] = *(int *) utarray_eltptr(cat->event_indices, i);
    }

    return MPI_SUCCESS;
}
