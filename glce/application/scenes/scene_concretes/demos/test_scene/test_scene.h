// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_SCENES_SCENE_CONCRETES_DEMOS_TEST_SCENE_TEST_SCENE_H
#define GLCE_APPLICATION_SCENES_SCENE_CONCRETES_DEMOS_TEST_SCENE_TEST_SCENE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/camera/flight_camera.h"

#include "application/scenes/vtables/scene_vtables.h"

typedef struct test_scene_config {
    flight_camera_key_bind_t flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MAX];
} test_scene_config_t;

const scene_vtable_t* test_scene_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
