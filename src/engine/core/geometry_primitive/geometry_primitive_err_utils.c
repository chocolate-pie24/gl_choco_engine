#include "engine/core/geometry_primitive/geometry_primitive_err_utils.h"
#include "engine/core/geometry_primitive/geometry_primitive_types.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

const char* geometry_primitive_rslt_to_str(geometry_primitive_result_t rslt_) {
    switch(rslt_) {
    case GEOMETRY_PRIMITIVE_SUCCESS:
        return s_rslt_str_success;
    case GEOMETRY_PRIMITIVE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case GEOMETRY_PRIMITIVE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case GEOMETRY_PRIMITIVE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case GEOMETRY_PRIMITIVE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case GEOMETRY_PRIMITIVE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case GEOMETRY_PRIMITIVE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}
