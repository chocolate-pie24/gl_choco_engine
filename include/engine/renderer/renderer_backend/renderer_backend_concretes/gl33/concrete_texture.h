#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_TEXTURE_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_texture.h"

const renderer_texture_vtable_t* gl33_texture_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
