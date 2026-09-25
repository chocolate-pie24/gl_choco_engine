#include "application/diagnostics/application_diagnostics.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/event_system/core/engine_event_view.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"
#include "application/event/application_frame_state.h"

static bool config_is_valid(const application_diagnostics_config_t* config_);

application_result_t application_diagnostics_update(const application_diagnostics_config_t* config_, const engine_event_view_t* event_view_, application_frame_state_t* frame_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    bool runtime_status_report_requested = false;
    bool validation_report_requested = false;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "application_diagnostics_update", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "application_diagnostics_update", "event_view_")
    IF_ARG_NULL_GOTO_CLEANUP(frame_state_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "application_diagnostics_update", "frame_state_")
    if(!config_is_valid(config_)) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_diagnostics_update(%s) - Provided config_ is not valid.", application_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != event_view_->keyboard_event_count; ++i) {
        if(event_view_->keyboard_events[i].key == config_->runtime_status_report && event_view_->keyboard_events[i].event_args.pressed) {
            runtime_status_report_requested = true;
        }
        if(event_view_->keyboard_events[i].key == config_->validation_report && event_view_->keyboard_events[i].event_args.pressed) {
            validation_report_requested = true;
        }
    }

    frame_state_->runtime_status_report_requested = runtime_status_report_requested;
    frame_state_->validation_report_requested = validation_report_requested;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static bool config_is_valid(const application_diagnostics_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(config_->runtime_status_report >= KEY_CODE_MAX || 0 > (int)config_->runtime_status_report) {
        return false;
    }
    if(config_->validation_report >= KEY_CODE_MAX || 0 > (int)config_->validation_report) {
        return false;
    }
    return true;
}
