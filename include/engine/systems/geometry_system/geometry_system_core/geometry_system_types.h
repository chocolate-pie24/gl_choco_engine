#ifndef GLCE_ENGINE_SYSTEMS_GEOMETRY_SYSTEM_GEOMETRY_SYSTEM_CORE_GEOMETRY_SYSTEM_TYPES_H
#define GLCE_ENGINE_SYSTEMS_GEOMETRY_SYSTEM_GEOMETRY_SYSTEM_CORE_GEOMETRY_SYSTEM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define INVALID_GEOMETRY_ID (-1)

typedef struct geometry_system_config {
    size_t max_lit_mesh_geometry_count;
    size_t max_point_mesh_geometry_count;
    size_t max_line_mesh_geometry_count;
    size_t max_ui_mesh_geometry_count;
} geometry_system_config_t;

typedef enum {
    GEOMETRY_TYPE_LIT_MESH_GEOMETRY,
    GEOMETRY_TYPE_LINE_MESH_GEOMETRY,
    GEOMETRY_TYPE_POINT_MESH_GEOMETRY,
    GEOMETRY_TYPE_UI_MESH_GEOMETRY,
} geometry_type_t;

typedef enum {
    GEOMETRY_SYSTEM_SUCCESS = 0,
    GEOMETRY_SYSTEM_INVALID_ARGUMENT,
    GEOMETRY_SYSTEM_RUNTIME_ERROR,
    GEOMETRY_SYSTEM_LIMIT_EXCEEDED,
    GEOMETRY_SYSTEM_NO_MEMORY,
    GEOMETRY_SYSTEM_DATA_CORRUPTED,
    GEOMETRY_SYSTEM_BAD_OPERATION,
    GEOMETRY_SYSTEM_FILE_OPEN_ERROR,
    GEOMETRY_SYSTEM_FILE_READ_ERROR,
    GEOMETRY_SYSTEM_FILE_CLOSE_ERROR,
    GEOMETRY_SYSTEM_UNSUPPORTED_FILE,
    GEOMETRY_SYSTEM_OVERFLOW,
    GEOMETRY_SYSTEM_UNDEFINED_ERROR,
} geometry_system_result_t;

#ifdef __cplusplus
}
#endif
#endif
