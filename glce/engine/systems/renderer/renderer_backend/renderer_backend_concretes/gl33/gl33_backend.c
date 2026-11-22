// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_backend.h"

#include <GL/glew.h>

void gl33_backend_initialize(void) {
    glEnable(GL_PROGRAM_POINT_SIZE);
}
