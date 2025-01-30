/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <assert.h>
#include <level_zero/ze_api.h>
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_zei.h"
#include <stdlib.h>

/* update event usage bracket [ev_lb, ev_ub] */
#define update_event_usage(device_state, event_idx)                     \
        if (event_idx == device_state->ev_ub)                           \
            device_state->ev_lb = device_state->ev_ub = -1;             \
        else if (event_idx >= device_state->ev_lb && device_state->ev_lb != -1) {                           \
            assert(event_idx < device_state->ev_ub);                    \
            device_state->ev_lb = event_idx + 1;                        \
        }

int create_ze_event(int dev_id, ze_event_handle_t * ze_event, int *idx)
{
    ze_result_t zerr;
    int rc = YAKSA_SUCCESS;
    int ev_idx;

    yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + dev_id;
    /* sanity check - abort when all events in the event pool are used up */
    assert(device_state->ev_ub - device_state->ev_lb < ZE_EVENT_POOL_CAP ||
           device_state->ev_lb != -1);
    ev_idx = device_state->ev_pool_idx;
    if (idx)
        *idx = ev_idx;
    /* reset the event before using it again */
    int pool_idx = ZE_EVENT_POOL_INDEX(ev_idx);
    if (device_state->events[pool_idx]) {
        zeEventHostReset(device_state->events[pool_idx]);
        *ze_event = device_state->events[pool_idx];
    } else {
        ze_event_desc_t event_desc = {
            .stype = ZE_STRUCTURE_TYPE_EVENT_DESC,
            .pNext = NULL,
            .index = pool_idx,
            .signal = 0,
            .wait = ZE_EVENT_SCOPE_FLAG_HOST
        };
        zerr = zeEventCreate(device_state->ep, &event_desc, ze_event);
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        device_state->events[pool_idx] = *ze_event;
    }
    device_state->ev_pool_idx++;
    /* update event bracket [ev_lb,ev_ub] */
    device_state->ev_ub = ev_idx;
    if (device_state->ev_lb == -1)
        device_state->ev_lb = device_state->ev_ub;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* return NULL event if there is no task left, this could happen
 * in multi-threading programs
 */
int yaksuri_zei_event_record(int device, void **event_)
{
    int rc = YAKSA_SUCCESS;
    ze_event_handle_t ze_event;
    yaksuri_zei_event_s *event;
    int idx;

    yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + device;
    pthread_mutex_lock(&device_state->mutex);
    if (device_state->last_event_idx < 0) {
        *event_ = NULL;
        goto fn_exit;
    }
    idx = ZE_EVENT_POOL_INDEX(device_state->last_event_idx);
    ze_event = device_state->events[idx];

    event = (yaksuri_zei_event_s *) malloc(sizeof(yaksuri_zei_event_s));
    event->dev_id = device;
    event->ze_event = ze_event;
    event->idx = idx;

    *event_ = event;

  fn_exit:
    pthread_mutex_unlock(&device_state->mutex);
    return rc;
}

int yaksuri_zei_event_query(void *event_, int *completed)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_zei_event_s *event = (yaksuri_zei_event_s *) event_;

    *completed = 1;
    if (event == NULL)
        goto fn_exit;

    ze_result_t zerr = zeEventQueryStatus(event->ze_event);
    if (zerr == ZE_RESULT_NOT_READY) {
        *completed = 0;
    } else {
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + event->dev_id;
        pthread_mutex_lock(&device_state->mutex);
        if (event->idx == device_state->last_event_idx) {
            device_state->last_event_idx = -1;
        }
        /* update event bracket [ev_lb, ev_ub] */
        /* freeing event->idx means all events below this index are done */
        update_event_usage(device_state, event->idx);
        pthread_mutex_unlock(&device_state->mutex);
        free(event);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_zei_add_dependency(int device1, int device2)
{
    ze_result_t zerr;
    int rc = YAKSA_SUCCESS;
    ze_event_handle_t last_event;

    /* wait for the last event on device1 to finish */
    yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + device1;
    pthread_mutex_lock(&device_state->mutex);
    assert(device_state->last_event_idx >= 0);
    last_event = device_state->events[ZE_EVENT_POOL_INDEX(device_state->last_event_idx)];
    if (last_event) {
        int completed = 0;
        while (!completed) {
            zerr = zeEventQueryStatus(last_event);
            if (zerr == ZE_RESULT_SUCCESS)
                completed = 1;
            else if (zerr != ZE_RESULT_NOT_READY) {
                YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            }
        }
        /* update event bracket [ev_lb, ev_ub] */
        update_event_usage(device_state, device_state->last_event_idx);
    }

  fn_exit:
    pthread_mutex_unlock(&device_state->mutex);
    return rc;
  fn_fail:
    goto fn_exit;
}
