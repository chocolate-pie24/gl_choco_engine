// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_FRAME_OPERATIONS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_FRAME_OPERATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct renderer_frame_operations_vtable renderer_frame_operations_vtable_t;

const renderer_frame_operations_vtable_t* gl33_frame_operations_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
