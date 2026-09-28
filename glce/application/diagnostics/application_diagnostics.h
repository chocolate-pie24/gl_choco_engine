// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DIAGNOSTICS_APPLICATION_DIAGNOSTICS_H
#define GLCE_APPLICATION_DIAGNOSTICS_APPLICATION_DIAGNOSTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/event/keyboard_event.h"

#include "application/core/application_types.h"

typedef struct application_frame_state application_frame_state_t;
typedef struct engine_event_view engine_event_view_t;
typedef struct subsystem_allocator subsystem_allocator_t;

typedef struct application_diagnostics_config {
    keycode_t runtime_status_report;
    keycode_t validation_report;
} application_diagnostics_config_t;

application_result_t application_diagnostics_update(const application_diagnostics_config_t* config_, const engine_event_view_t* event_view_, application_frame_state_t* frame_state_);

application_result_t application_diagnostics_status_report(const subsystem_allocator_t* subsystem_allocator_);

#ifdef __cplusplus
}
#endif
#endif
