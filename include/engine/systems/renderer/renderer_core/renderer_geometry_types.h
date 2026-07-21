#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_RENDERER_GEOMETRY_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_RENDERER_GEOMETRY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct draw_range {
    size_t first_vertex_count;
    size_t vertex_count;
} draw_range_t;

typedef struct vertex_buffer_range {
    draw_range_t draw_range;
    size_t allocation_size;
} vertex_buffer_range_t;

#ifdef __cplusplus
}
#endif
#endif
