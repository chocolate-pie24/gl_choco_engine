#include "application/diagnostics/application_diagnostics.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/event_system/core/engine_event_view.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"
#include "application/event/application_frame_state.h"

static application_result_t memory_status_report(const subsystem_allocator_t* subsystem_allocator_);
static application_result_t general_allocator_report(void);
static application_result_t subsystem_allocator_report(const subsystem_allocator_t* subsystem_allocator_);

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

application_result_t application_diagnostics_status_report(const subsystem_allocator_t* subsystem_allocator_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(subsystem_allocator_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "application_diagnostics_status_report", "subsystem_allocator_")

    INFO_MESSAGE("============================================================");
    INFO_MESSAGE(" GLCE Runtime Status");
    INFO_MESSAGE("============================================================");

    ret = memory_status_report(subsystem_allocator_);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_diagnostics_status_report(%s) - memory_status_report failed.", application_result_to_str(ret));
        goto cleanup;
    }

    INFO_MESSAGE("============================================================");

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static application_result_t memory_status_report(const subsystem_allocator_t* subsystem_allocator_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(subsystem_allocator_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "memory_status_report", "subsystem_allocator_")

    INFO_MESSAGE("");
    INFO_MESSAGE("[Memory]");
    INFO_MESSAGE("");

    ret = general_allocator_report();
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("memory_status_report(%s) - general_allocator_report failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = subsystem_allocator_report(subsystem_allocator_);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("memory_status_report(%s) - subsystem_allocator_report failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static application_result_t general_allocator_report(void) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    general_allocator_status_t status = { 0 };
    double pool_size_mib = 0.0;
    double allocated_block_size_mib = 0.0;
    double free_block_size_mib = 0.0;
    double largest_free_block_size_mib = 0.0;
    double max_allocation_size_mib = 0.0;
    double total_allocated_mib = 0.0;
    double usage_percent = 0.0;

    ret_general_allocator = general_allocator_status_get(&status);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("general_allocator_report(%s) - general_allocator_status_get failed.", application_result_to_str(ret));
        goto cleanup;
    }

    pool_size_mib = (double)status.memory_pool_size / (1024.0 * 1024.0);
    allocated_block_size_mib = (double)status.allocated_block_size / (1024.0 * 1024.0);
    free_block_size_mib = (double)status.free_block_size / (1024.0 * 1024.0);
    largest_free_block_size_mib = (double)status.largest_free_block_size / (1024.0 * 1024.0);
    max_allocation_size_mib = (double)status.max_allocation_size / (1024.0 * 1024.0);
    total_allocated_mib = (double)status.total_allocated / (1024.0 * 1024.0);

    if(0 != status.memory_pool_size) {
        usage_percent = ((double)status.allocated_block_size / (double)status.memory_pool_size) * 100.0;
    }

    INFO_MESSAGE("  General Allocator");
    INFO_MESSAGE("    Pool");
    INFO_MESSAGE("      Size                    : %10.2f MiB", pool_size_mib);
    INFO_MESSAGE("      Used block size         : %10.2f MiB", allocated_block_size_mib);
    INFO_MESSAGE("      Free block size         : %10.2f MiB", free_block_size_mib);
    INFO_MESSAGE("      Usage                   : %10.2f %%", usage_percent);

    INFO_MESSAGE("");
    INFO_MESSAGE("    Allocation");
    INFO_MESSAGE("      Payload allocated       : %10.2f MiB", total_allocated_mib);
    INFO_MESSAGE("      Allocated blocks        : %10zu", status.allocated_block_count);
    INFO_MESSAGE("      Free blocks             : %10zu", status.free_block_count);
    INFO_MESSAGE("      Largest free block      : %10.2f MiB", largest_free_block_size_mib);
    INFO_MESSAGE("      Max allocation size     : %10.2f MiB", max_allocation_size_mib);

    INFO_MESSAGE("");
    INFO_MESSAGE("    Memory Tags");

    for(size_t i = 0; i != GENERAL_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        const general_allocator_memory_tag_t memory_tag = (general_allocator_memory_tag_t)i;
        const double allocated_mib = (double)status.memory_tag_allocated[i] / (1024.0 * 1024.0);
        INFO_MESSAGE("      %-23s : %10.2f MiB", general_allocator_memory_tag_to_str(memory_tag), allocated_mib);
    }

    INFO_MESSAGE("");

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static application_result_t subsystem_allocator_report(const subsystem_allocator_t* subsystem_allocator_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    subsystem_allocator_status_t status = { 0 };
    double pool_size_kib = 0.0;
    double used_size_kib = 0.0;
    double free_size_kib = 0.0;
    double total_allocated_kib = 0.0;
    double usage_percent = 0.0;

    IF_ARG_NULL_GOTO_CLEANUP(subsystem_allocator_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "subsystem_allocator_report", "subsystem_allocator_")

    ret_subsystem_allocator = subsystem_allocator_status_get(subsystem_allocator_, &status);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = application_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("subsystem_allocator_report(%s) - subsystem_allocator_status_get failed.", application_result_to_str(ret));
        goto cleanup;
    }

    pool_size_kib = (double)status.memory_pool_size / 1024.0;
    used_size_kib = (double)status.used_size / 1024.0;
    free_size_kib = (double)status.free_size / 1024.0;
    total_allocated_kib = (double)status.total_allocated / 1024.0;

    if(0 != status.memory_pool_size) {
        usage_percent = ((double)status.used_size / (double)status.memory_pool_size) * 100.0;
    }

    INFO_MESSAGE("  Subsystem Allocator");
    INFO_MESSAGE("    Pool");
    INFO_MESSAGE("      Size                    : %10.2f KiB", pool_size_kib);
    INFO_MESSAGE("      Used                    : %10.2f KiB", used_size_kib);
    INFO_MESSAGE("      Free                    : %10.2f KiB", free_size_kib);
    INFO_MESSAGE("      Usage                   : %10.2f %%", usage_percent);

    INFO_MESSAGE("");
    INFO_MESSAGE("    Allocation");
    INFO_MESSAGE("      Payload allocated       : %10.2f KiB", total_allocated_kib);

    INFO_MESSAGE("");
    INFO_MESSAGE("    Memory Tags");

    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        const subsystem_allocator_memory_tag_t memory_tag = (subsystem_allocator_memory_tag_t)i;
        const double allocated_kib = (double)status.memory_tag_allocated[i] / 1024.0;
        INFO_MESSAGE("      %-23s : %10.2f KiB", subsystem_allocator_memory_tag_to_str(memory_tag), allocated_kib);
    }

    INFO_MESSAGE("");

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
