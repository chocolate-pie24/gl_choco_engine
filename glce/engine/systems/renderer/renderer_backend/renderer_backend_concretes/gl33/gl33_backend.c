// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_backend.h"

#include <GL/glew.h>

void gl33_backend_initialize(void) {
    glEnable(GL_PROGRAM_POINT_SIZE);

    // GL33 BackendではTightly PackedなPixel Dataを使用する。
    // GL_UNPACK_ALIGNMENTはBackend初期化時に1へ設定し、
    // 以降のTexture Upload処理では変更しない。
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 4byte境界にアラインされていないテクスチャ(width * bytes_per_pixel が 4 の倍数でないテクスチャ)に対応させるため設定(Text rendering用のテクスチャが該当)
}
