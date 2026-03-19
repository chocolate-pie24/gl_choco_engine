/**
 * @ingroup view
 * @file camera.c
 * @author chocolate-pie24
 * @brief カメラモジュール実装
 *
 * @version 0.1
 * @date 2026-03-09
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdbool.h>

#include "engine/view/camera/camera.h"

#include "engine/view/view_core/view_err_utils.h"
#include "engine/view/view_core/view_memory.h"
#include "engine/view/view_core/view_types.h"

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/containers/choco_string.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <string.h>

static void NO_COVERAGE test_camera_create(void);
static void NO_COVERAGE test_camera_destroy(void);
static void NO_COVERAGE test_camera_name_get(void);
static void NO_COVERAGE test_camera_viewing_frustum_update(void);
static void NO_COVERAGE test_camera_perspective_matrix_get(void);
static void NO_COVERAGE test_camera_view_matrix_get(void);
static void NO_COVERAGE test_rslt_to_str(void);
static void NO_COVERAGE test_rslt_convert_choco_memory(void);
static void NO_COVERAGE test_rslt_convert_choco_string(void);
static void NO_COVERAGE test_is_valid_frustum(void);
#endif

/**
 * @brief 視錐台データホルダ
 *
 */
typedef struct viewing_frustum {
    float aspect;       /**< 画面縦横比 */
    float fovy;         /**< 画角(degree) */
    float near_clip;    /**< 描画範囲(near) */
    float far_clip;     /**< 描画範囲(far) */
} viewing_frustum_t;

/**
 * @brief カメラ内部状態管理構造体
 *
 * @note カメラ姿勢に関しては以下を念頭に置くこと
 * - Roll: Z軸回りの回転
 * - Pitch: X軸回りの回転
 * - Yaw: Y軸回りの回転
 * - カメラ前方方向: Z軸マイナス方向
 *
 * @todo 不要な行列計算を省くため、以下を検討する
 * - view_dirtyとview_matrixキャッシュを内部で持ち、view_dirty == falseで計算を行わない
 * - projection_dirtyとperspective_matrixキャッシュを内部で持ち、projection_dirty == falseで計算を行わない
 */
struct camera {
    vec3f_t euler;              /**< カメラ姿勢オイラー角(degree) */
    vec3f_t position;           /**< カメラ位置 */

    viewing_frustum_t frustum;  /**< 視錐台パラメータ */

    choco_string_t* name;       /**< カメラ名称文字列 */
};

static bool is_valid_frustum(const viewing_frustum_t* frustum_);

view_result_t camera_create(const char* name_, camera_t** out_camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    camera_t* tmp_camera = NULL;
    choco_string_result_t string_ret = CHOCO_STRING_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_create", "out_camera_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_create", "*out_camera_")

    ret = view_mem_allocate(sizeof(camera_t), (void**)&tmp_camera);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("camera_create(%s) - Failed to allocate memory for camera.", view_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_camera->name = NULL;

    string_ret = choco_string_create_from_c_string(&tmp_camera->name, name_);
    if(CHOCO_STRING_SUCCESS != string_ret) {
        ret = view_rslt_convert_choco_string(string_ret);
        ERROR_MESSAGE("camera_create(%s) - Failed to create string for camera name.", view_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_camera->frustum.aspect = 0.0f;
    tmp_camera->frustum.far_clip = 0.0f;
    tmp_camera->frustum.near_clip = 0.0f;
    tmp_camera->frustum.fovy = 0.0f;

    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->euler);
    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->position);

    *out_camera_ = tmp_camera;
    ret = VIEW_SUCCESS;

cleanup:
    if(VIEW_SUCCESS != ret) {
        if(NULL != tmp_camera) {
            if(NULL != tmp_camera->name) {
                // ここは現状では通ることがないためカバレッジは100にならないが許容
                choco_string_destroy(&tmp_camera->name);
            }
            camera_destroy(&tmp_camera);
        }
    }
    return ret;
}

void camera_destroy(camera_t** camera_) {
    if(NULL == camera_) {
        return;
    }
    if(NULL == *camera_) {
        return;
    }
    if(NULL != (*camera_)->name) {
        choco_string_destroy(&(*camera_)->name);
    }
    view_mem_free(*camera_, sizeof(camera_t));
    *camera_ = NULL;
}

const char* camera_name_get(const camera_t* camera_) {
    if(NULL == camera_) {
        ERROR_MESSAGE("camera_name_get(%s) - Argument camera_ requires a valid pointer.", view_rslt_to_str(VIEW_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == camera_->name) {
        ERROR_MESSAGE("camera_name_get(%s) - Provided camera_ is corrupted.", view_rslt_to_str(VIEW_DATA_CORRUPTED));
        return NULL;
    }
    return choco_string_c_str(camera_->name);
}

view_result_t camera_viewing_frustum_update(float fovy_, float aspect_, float near_clip_, float far_clip_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_viewing_frustum_update", "camera_")

    viewing_frustum_t frustum = { 0 };
    frustum.aspect = aspect_;
    frustum.far_clip = far_clip_;
    frustum.fovy = fovy_;
    frustum.near_clip = near_clip_;
    if(!is_valid_frustum(&frustum)) {
        ret = VIEW_INVALID_ARGUMENT;
        ERROR_MESSAGE("camera_viewing_frustum_update(%s) - Invalid frustum parameter.", view_rslt_to_str(ret));
        goto cleanup;
    }
    camera_->frustum = frustum;

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t camera_posture_update(const vec3f_t* euler_, const vec3f_t* position_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(euler_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_posture_update", "euler_")
    IF_ARG_NULL_GOTO_CLEANUP(position_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_posture_update", "position_")
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_posture_update", "camera_")

    camera_->euler.elem[0] = euler_->elem[0];
    camera_->euler.elem[1] = euler_->elem[1];
    camera_->euler.elem[2] = euler_->elem[2];

    camera_->position.elem[0] = position_->elem[0];
    camera_->position.elem[1] = position_->elem[1];
    camera_->position.elem[2] = position_->elem[2];

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t camera_posture_get(const camera_t* camera_, vec3f_t* out_euler_, vec3f_t* out_position_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_posture_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_euler_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_posture_get", "out_euler_")
    IF_ARG_NULL_GOTO_CLEANUP(out_position_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_posture_get", "out_position_")

    out_euler_->elem[0] = camera_->euler.elem[0];
    out_euler_->elem[1] = camera_->euler.elem[1];
    out_euler_->elem[2] = camera_->euler.elem[2];

    out_position_->elem[0] = camera_->position.elem[0];
    out_position_->elem[1] = camera_->position.elem[1];
    out_position_->elem[2] = camera_->position.elem[2];

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t camera_perspective_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_perspective_matrix_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_perspective_matrix_get", "out_mat_")
    IF_ARG_FALSE_GOTO_CLEANUP(is_valid_frustum(&camera_->frustum), ret, VIEW_BAD_OPERATION, view_rslt_to_str(VIEW_BAD_OPERATION), "camera_perspective_matrix_get", "camera_->frustum")

    const float dz = camera_->frustum.far_clip - camera_->frustum.near_clip;

    mat4f_identity(out_mat_);
    out_mat_->elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(camera_->frustum.fovy) * 0.5f);
    out_mat_->elem[0] = out_mat_->elem[5] / camera_->frustum.aspect;
    out_mat_->elem[10] = -1.0f * (camera_->frustum.far_clip + camera_->frustum.near_clip) / dz;
    out_mat_->elem[11] = -2.0f * camera_->frustum.far_clip * camera_->frustum.near_clip / dz;
    out_mat_->elem[14] = -1.0f;
    out_mat_->elem[15] = 0.0f;

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t camera_view_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_view_matrix_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_view_matrix_get", "out_mat_")

    mat4x4f_t rot = { 0 };
    mat4f_rot_xyz(CHOCO_DEG_TO_RAD(camera_->euler.elem[0]), CHOCO_DEG_TO_RAD(camera_->euler.elem[1]), CHOCO_DEG_TO_RAD(camera_->euler.elem[2]), &rot);

    mat4x4f_t trans = { 0 };
    mat4f_translation(&camera_->position, &trans); // ある座標をtranslate分平行移動する行列 = translate分座標が増える = カメラ->ワールド座標系への変換行列

    mat4x4f_t view = { 0 };
    // 後に変換するものを左から掛ける
    mat4f_mul(&trans, &rot, &view); // カメラ座標系のある座標をワールド座標系へ変換する行列
    if(!mat4f_inverse(&view)) {     // ワールド座標系のある座標をカメラ座標系へ変換する行列
        ERROR_MESSAGE("camera_view_matrix_get(%s) - Matrix(view) inversion failed because the determinant is zero or near zero.", view_rslt_to_str(VIEW_RUNTIME_ERROR));
        ret = VIEW_RUNTIME_ERROR;
        goto cleanup;
    }
    mat4f_copy(&view, out_mat_);

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 視錐台パラメータ有効 / 無効判定を行う
 *
 * @param[in] frustum_ 判定対象視錐台構造体インスタンスへのポインタ
 *
 * @retval true 視錐台パラメータ正常
 * @retval false 視錐台パラメータ異常
 */
static bool is_valid_frustum(const viewing_frustum_t* frustum_) {
    if(NULL == frustum_) {
        return false;
    } else if(frustum_->near_clip >= frustum_->far_clip) {
        return false;
    } else if(frustum_->aspect <= 0.0f) {
        return false;
    } else if(frustum_->fovy <= 0.0f) {
        return false;
    } else if(frustum_->fovy >= 180.0f) {
        return false;
    } else if(frustum_->near_clip <= 0.0f) {
        return false;
    } else if(frustum_->far_clip <= 0.0f) {
        return false;
    }
    return true;
}

#ifdef TEST_BUILD
void test_camera(void) {
    test_camera_create();
    test_camera_destroy();
    test_camera_name_get();
    test_camera_viewing_frustum_update();
    test_camera_perspective_matrix_get();
    test_camera_view_matrix_get();
    test_is_valid_frustum();
}

static void NO_COVERAGE test_camera_create(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // name_ == NULL
        camera_t* camera = NULL;
        const view_result_t ret = camera_create(NULL, &camera);

        assert(VIEW_INVALID_ARGUMENT == ret);
        assert(NULL == camera);
    }
    {
        // out_camera_ == NULL
        const view_result_t ret = camera_create("main_camera", NULL);

        assert(VIEW_INVALID_ARGUMENT == ret);
    }
    {
        // *out_camera_ != NULL
        int dummy = 0;
        camera_t* camera = (camera_t*)&dummy;
        const view_result_t ret = camera_create("main_camera", &camera);

        assert(VIEW_INVALID_ARGUMENT == ret);
        assert((camera_t*)&dummy == camera);
    }
    {
        // camera本体のメモリ確保失敗
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();
        // memory_system_test_param_set(0);

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_NO_MEMORY == ret);
        //     assert(NULL == camera);
        // }

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // memory_systemの結果コードを強制的にLIMIT_EXCEEDEDへ固定
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();
        // memory_system_rslt_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_LIMIT_EXCEEDED == ret);
        //     assert(NULL == camera);
        // }

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // choco_string_create_from_c_string内のtmp_string確保失敗
        // 0回目: camera本体
        // 1回目: choco_string_t本体
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();
        // memory_system_test_param_set(1);

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_NO_MEMORY == ret);
        //     assert(NULL == camera);
        // }

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // choco_string_create_from_c_string内のbuffer確保失敗
        // 0回目: camera本体
        // 1回目: choco_string_t本体
        // 2回目: 文字列バッファ
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();
        // memory_system_test_param_set(2);

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_NO_MEMORY == ret);
        //     assert(NULL == camera);
        // }

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 正常系
        // camera_t* camera = NULL;
        // const char* name = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_SUCCESS == ret);
        //     assert(NULL != camera);

        //     name = camera_name_get(camera);
        //     assert(NULL != name);
        //     assert(0 == strcmp("main_camera", name));
        // }

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
}

static void NO_COVERAGE test_camera_destroy(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // camera_ == NULL
        camera_destroy(NULL);
    }
    {
        // *camera_ == NULL
        camera_t* camera = NULL;
        camera_destroy(&camera);
        assert(NULL == camera);
    }
    {
        // 正常系: createしたcameraをdestroyできる
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_SUCCESS == ret);
        //     assert(NULL != camera);
        // }

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 2回destroyしても問題ない
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_SUCCESS == ret);
        //     assert(NULL != camera);
        // }

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
}

static void NO_COVERAGE test_camera_name_get(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // camera_ == NULL
        const char* name = camera_name_get(NULL);
        assert(NULL == name);
    }
    {
        // 正常系: create時に指定した名前を取得できる
        // camera_t* camera = NULL;
        // const char* name = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // {
        //     const view_result_t ret = camera_create("main_camera", &camera);
        //     assert(VIEW_SUCCESS == ret);
        //     assert(NULL != camera);
        // }

        // name = camera_name_get(camera);
        // assert(NULL != name);
        // assert(0 == strcmp("main_camera", name));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 複数回呼んでも同じ内容を取得できる
        // camera_t* camera = NULL;
        // const char* name1 = NULL;
        // const char* name2 = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // {
        //     const view_result_t ret = camera_create("sub_camera", &camera);
        //     assert(VIEW_SUCCESS == ret);
        //     assert(NULL != camera);
        // }

        // name1 = camera_name_get(camera);
        // name2 = camera_name_get(camera);

        // assert(NULL != name1);
        // assert(NULL != name2);
        // assert(0 == strcmp("sub_camera", name1));
        // assert(0 == strcmp("sub_camera", name2));
        // assert(0 == strcmp(name1, name2));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // camera_->name == NULL
        camera_t camera = { 0 };
        const char* name = camera_name_get(&camera);

        assert(NULL == name);
    }
}

static void NO_COVERAGE test_camera_viewing_frustum_update(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // camera_ == NULL
        const view_result_t ret = camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 0.1f, 100.0f, NULL);
        assert(VIEW_INVALID_ARGUMENT == ret);
    }
    {
        // fovy_ <= 0.0f
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(0.0f, 16.0f / 9.0f, 0.1f, 100.0f, camera));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // fovy_ >= 180.0f
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(180.0f, 16.0f / 9.0f, 0.1f, 100.0f, camera));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // aspect_ <= 0.0f
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(60.0f, 0.0f, 0.1f, 100.0f, camera));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // near_clip_ <= 0.0f
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 0.0f, 100.0f, camera));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // far_clip_ <= 0.0f
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 0.1f, 0.0f, camera));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // near_clip_ >= far_clip_
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 10.0f, 10.0f, camera));
        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 100.0f, 10.0f, camera));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 正常系: 更新後にperspective行列を取得できる
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };
        // const float fovy = 60.0f;
        // const float aspect = 16.0f / 9.0f;
        // const float near_clip = 0.1f;
        // const float far_clip = 100.0f;
        // const float dz = far_clip - near_clip;
        // const float expected_5 = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(fovy) * 0.5f);
        // const float expected_0 = expected_5 / aspect;
        // const float expected_10 = -1.0f * (far_clip + near_clip) / dz;
        // const float expected_11 = -2.0f * far_clip * near_clip / dz;
        // const float expected_14 = -1.0f;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_SUCCESS == camera_viewing_frustum_update(fovy, aspect, near_clip, far_clip, camera));
        // assert(VIEW_SUCCESS == camera_perspective_matrix_get(camera, &mat));

        // assert(is_equal_float(mat.elem[0], expected_0));
        // assert(is_equal_float(mat.elem[1], 0.0f));
        // assert(is_equal_float(mat.elem[2], 0.0f));
        // assert(is_equal_float(mat.elem[3], 0.0f));

        // assert(is_equal_float(mat.elem[4], 0.0f));
        // assert(is_equal_float(mat.elem[5], expected_5));
        // assert(is_equal_float(mat.elem[6], 0.0f));
        // assert(is_equal_float(mat.elem[7], 0.0f));

        // assert(is_equal_float(mat.elem[8], 0.0f));
        // assert(is_equal_float(mat.elem[9], 0.0f));
        // assert(is_equal_float(mat.elem[10], expected_10));
        // assert(is_equal_float(mat.elem[11], expected_11));

        // assert(is_equal_float(mat.elem[12], 0.0f));
        // assert(is_equal_float(mat.elem[13], 0.0f));
        // assert(is_equal_float(mat.elem[14], expected_14));
        // assert(is_equal_float(mat.elem[15], 0.0f));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 失敗更新時は以前の正常な視錐台パラメータが維持される
        // camera_t* camera = NULL;
        // mat4x4f_t before = { 0 };
        // mat4x4f_t after = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_SUCCESS == camera_viewing_frustum_update(60.0f, 4.0f / 3.0f, 0.5f, 200.0f, camera));
        // assert(VIEW_SUCCESS == camera_perspective_matrix_get(camera, &before));

        // assert(VIEW_INVALID_ARGUMENT == camera_viewing_frustum_update(0.0f, 4.0f / 3.0f, 0.5f, 200.0f, camera));
        // assert(VIEW_SUCCESS == camera_perspective_matrix_get(camera, &after));

        // assert(is_equal_float(before.elem[0], after.elem[0]));
        // assert(is_equal_float(before.elem[1], after.elem[1]));
        // assert(is_equal_float(before.elem[2], after.elem[2]));
        // assert(is_equal_float(before.elem[3], after.elem[3]));

        // assert(is_equal_float(before.elem[4], after.elem[4]));
        // assert(is_equal_float(before.elem[5], after.elem[5]));
        // assert(is_equal_float(before.elem[6], after.elem[6]));
        // assert(is_equal_float(before.elem[7], after.elem[7]));

        // assert(is_equal_float(before.elem[8], after.elem[8]));
        // assert(is_equal_float(before.elem[9], after.elem[9]));
        // assert(is_equal_float(before.elem[10], after.elem[10]));
        // assert(is_equal_float(before.elem[11], after.elem[11]));

        // assert(is_equal_float(before.elem[12], after.elem[12]));
        // assert(is_equal_float(before.elem[13], after.elem[13]));
        // assert(is_equal_float(before.elem[14], after.elem[14]));
        // assert(is_equal_float(before.elem[15], after.elem[15]));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
}

static void NO_COVERAGE test_camera_perspective_matrix_get(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // camera_ == NULL
        mat4x4f_t mat = { 0 };
        const view_result_t ret = camera_perspective_matrix_get(NULL, &mat);

        assert(VIEW_INVALID_ARGUMENT == ret);
    }
    {
        // // out_mat_ == NULL
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));

        // {
        //     const view_result_t ret = camera_perspective_matrix_get(camera, NULL);
        //     assert(VIEW_INVALID_ARGUMENT == ret);
        // }

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 視錐台未設定
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));

        // {
        //     const view_result_t ret = camera_perspective_matrix_get(camera, &mat);
        //     assert(VIEW_BAD_OPERATION == ret);
        // }

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 正常系
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };
        // const float fovy = 60.0f;
        // const float aspect = 16.0f / 9.0f;
        // const float near_clip = 0.1f;
        // const float far_clip = 100.0f;
        // const float dz = far_clip - near_clip;
        // const float expected_5 = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(fovy) * 0.5f);
        // const float expected_0 = expected_5 / aspect;
        // const float expected_10 = -1.0f * (far_clip + near_clip) / dz;
        // const float expected_11 = -2.0f * far_clip * near_clip / dz;
        // const float expected_14 = -1.0f;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_SUCCESS == camera_viewing_frustum_update(fovy, aspect, near_clip, far_clip, camera));

        // {
        //     const view_result_t ret = camera_perspective_matrix_get(camera, &mat);
        //     assert(VIEW_SUCCESS == ret);
        // }

        // assert(is_equal_float(mat.elem[0], expected_0));
        // assert(is_equal_float(mat.elem[1], 0.0f));
        // assert(is_equal_float(mat.elem[2], 0.0f));
        // assert(is_equal_float(mat.elem[3], 0.0f));

        // assert(is_equal_float(mat.elem[4], 0.0f));
        // assert(is_equal_float(mat.elem[5], expected_5));
        // assert(is_equal_float(mat.elem[6], 0.0f));
        // assert(is_equal_float(mat.elem[7], 0.0f));

        // assert(is_equal_float(mat.elem[8], 0.0f));
        // assert(is_equal_float(mat.elem[9], 0.0f));
        // assert(is_equal_float(mat.elem[10], expected_10));
        // assert(is_equal_float(mat.elem[11], expected_11));

        // assert(is_equal_float(mat.elem[12], 0.0f));
        // assert(is_equal_float(mat.elem[13], 0.0f));
        // assert(is_equal_float(mat.elem[14], expected_14));
        // assert(is_equal_float(mat.elem[15], 0.0f));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 複数回呼んでも同じ結果を取得できる
        // camera_t* camera = NULL;
        // mat4x4f_t mat1 = { 0 };
        // mat4x4f_t mat2 = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_SUCCESS == camera_viewing_frustum_update(45.0f, 4.0f / 3.0f, 0.5f, 500.0f, camera));

        // assert(VIEW_SUCCESS == camera_perspective_matrix_get(camera, &mat1));
        // assert(VIEW_SUCCESS == camera_perspective_matrix_get(camera, &mat2));

        // assert(is_equal_float(mat1.elem[0], mat2.elem[0]));
        // assert(is_equal_float(mat1.elem[1], mat2.elem[1]));
        // assert(is_equal_float(mat1.elem[2], mat2.elem[2]));
        // assert(is_equal_float(mat1.elem[3], mat2.elem[3]));

        // assert(is_equal_float(mat1.elem[4], mat2.elem[4]));
        // assert(is_equal_float(mat1.elem[5], mat2.elem[5]));
        // assert(is_equal_float(mat1.elem[6], mat2.elem[6]));
        // assert(is_equal_float(mat1.elem[7], mat2.elem[7]));

        // assert(is_equal_float(mat1.elem[8], mat2.elem[8]));
        // assert(is_equal_float(mat1.elem[9], mat2.elem[9]));
        // assert(is_equal_float(mat1.elem[10], mat2.elem[10]));
        // assert(is_equal_float(mat1.elem[11], mat2.elem[11]));

        // assert(is_equal_float(mat1.elem[12], mat2.elem[12]));
        // assert(is_equal_float(mat1.elem[13], mat2.elem[13]));
        // assert(is_equal_float(mat1.elem[14], mat2.elem[14]));
        // assert(is_equal_float(mat1.elem[15], mat2.elem[15]));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
}

static void NO_COVERAGE test_camera_view_matrix_get(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // camera_ == NULL
        mat4x4f_t mat = { 0 };
        const view_result_t ret = camera_view_matrix_get(NULL, &mat);

        assert(VIEW_INVALID_ARGUMENT == ret);
    }
    {
        // out_mat_ == NULL
        // camera_t* camera = NULL;

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));

        // {
        //     const view_result_t ret = camera_view_matrix_get(camera, NULL);
        //     assert(VIEW_INVALID_ARGUMENT == ret);
        // }

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // デフォルト姿勢: 単位行列
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));
        // assert(VIEW_SUCCESS == camera_view_matrix_get(camera, &mat));

        // assert(is_equal_float(mat.elem[0], 1.0f));
        // assert(is_equal_float(mat.elem[1], 0.0f));
        // assert(is_equal_float(mat.elem[2], 0.0f));
        // assert(is_equal_float(mat.elem[3], 0.0f));

        // assert(is_equal_float(mat.elem[4], 0.0f));
        // assert(is_equal_float(mat.elem[5], 1.0f));
        // assert(is_equal_float(mat.elem[6], 0.0f));
        // assert(is_equal_float(mat.elem[7], 0.0f));

        // assert(is_equal_float(mat.elem[8], 0.0f));
        // assert(is_equal_float(mat.elem[9], 0.0f));
        // assert(is_equal_float(mat.elem[10], 1.0f));
        // assert(is_equal_float(mat.elem[11], 0.0f));

        // assert(is_equal_float(mat.elem[12], 0.0f));
        // assert(is_equal_float(mat.elem[13], 0.0f));
        // assert(is_equal_float(mat.elem[14], 0.0f));
        // assert(is_equal_float(mat.elem[15], 1.0f));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // 位置のみ変更: 逆平行移動行列になる
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));

        // vec3f_initialize(1.5f, -2.0f, 3.25f, &camera->position);
        // vec3f_initialize(0.0f, 0.0f, 0.0f, &camera->euler);

        // assert(VIEW_SUCCESS == camera_view_matrix_get(camera, &mat));

        // assert(is_equal_float(mat.elem[0], 1.0f));
        // assert(is_equal_float(mat.elem[1], 0.0f));
        // assert(is_equal_float(mat.elem[2], 0.0f));
        // assert(is_equal_float(mat.elem[3], -1.5f));

        // assert(is_equal_float(mat.elem[4], 0.0f));
        // assert(is_equal_float(mat.elem[5], 1.0f));
        // assert(is_equal_float(mat.elem[6], 0.0f));
        // assert(is_equal_float(mat.elem[7], 2.0f));

        // assert(is_equal_float(mat.elem[8], 0.0f));
        // assert(is_equal_float(mat.elem[9], 0.0f));
        // assert(is_equal_float(mat.elem[10], 1.0f));
        // assert(is_equal_float(mat.elem[11], -3.25f));

        // assert(is_equal_float(mat.elem[12], 0.0f));
        // assert(is_equal_float(mat.elem[13], 0.0f));
        // assert(is_equal_float(mat.elem[14], 0.0f));
        // assert(is_equal_float(mat.elem[15], 1.0f));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // X軸回転のみ: viewはX軸逆回転行列
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));

        // vec3f_initialize(0.0f, 0.0f, 0.0f, &camera->position);
        // vec3f_initialize(90.0f, 0.0f, 0.0f, &camera->euler);

        // assert(VIEW_SUCCESS == camera_view_matrix_get(camera, &mat));

        // assert(is_equal_float(mat.elem[0], 1.0f));
        // assert(is_equal_float(mat.elem[1], 0.0f));
        // assert(is_equal_float(mat.elem[2], 0.0f));
        // assert(is_equal_float(mat.elem[3], 0.0f));

        // assert(is_equal_float(mat.elem[4], 0.0f));
        // assert(is_equal_float(mat.elem[5], 0.0f));
        // assert(is_equal_float(mat.elem[6], 1.0f));
        // assert(is_equal_float(mat.elem[7], 0.0f));

        // assert(is_equal_float(mat.elem[8], 0.0f));
        // assert(is_equal_float(mat.elem[9], -1.0f));
        // assert(is_equal_float(mat.elem[10], 0.0f));
        // assert(is_equal_float(mat.elem[11], 0.0f));

        // assert(is_equal_float(mat.elem[12], 0.0f));
        // assert(is_equal_float(mat.elem[13], 0.0f));
        // assert(is_equal_float(mat.elem[14], 0.0f));
        // assert(is_equal_float(mat.elem[15], 1.0f));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // Y軸回転のみ: viewはY軸逆回転行列
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));

        // vec3f_initialize(0.0f, 0.0f, 0.0f, &camera->position);
        // vec3f_initialize(0.0f, 90.0f, 0.0f, &camera->euler);

        // assert(VIEW_SUCCESS == camera_view_matrix_get(camera, &mat));

        // assert(is_equal_float(mat.elem[0], 0.0f));
        // assert(is_equal_float(mat.elem[1], 0.0f));
        // assert(is_equal_float(mat.elem[2], -1.0f));
        // assert(is_equal_float(mat.elem[3], 0.0f));

        // assert(is_equal_float(mat.elem[4], 0.0f));
        // assert(is_equal_float(mat.elem[5], 1.0f));
        // assert(is_equal_float(mat.elem[6], 0.0f));
        // assert(is_equal_float(mat.elem[7], 0.0f));

        // assert(is_equal_float(mat.elem[8], 1.0f));
        // assert(is_equal_float(mat.elem[9], 0.0f));
        // assert(is_equal_float(mat.elem[10], 0.0f));
        // assert(is_equal_float(mat.elem[11], 0.0f));

        // assert(is_equal_float(mat.elem[12], 0.0f));
        // assert(is_equal_float(mat.elem[13], 0.0f));
        // assert(is_equal_float(mat.elem[14], 0.0f));
        // assert(is_equal_float(mat.elem[15], 1.0f));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
    {
        // Z軸回転のみ: viewはZ軸逆回転行列
        // camera_t* camera = NULL;
        // mat4x4f_t mat = { 0 };

        // memory_system_destroy();
        // assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
        // memory_system_test_param_reset();

        // assert(VIEW_SUCCESS == camera_create("main_camera", &camera));

        // vec3f_initialize(0.0f, 0.0f, 0.0f, &camera->position);
        // vec3f_initialize(0.0f, 0.0f, 90.0f, &camera->euler);

        // assert(VIEW_SUCCESS == camera_view_matrix_get(camera, &mat));

        // assert(is_equal_float(mat.elem[0], 0.0f));
        // assert(is_equal_float(mat.elem[1], 1.0f));
        // assert(is_equal_float(mat.elem[2], 0.0f));
        // assert(is_equal_float(mat.elem[3], 0.0f));

        // assert(is_equal_float(mat.elem[4], -1.0f));
        // assert(is_equal_float(mat.elem[5], 0.0f));
        // assert(is_equal_float(mat.elem[6], 0.0f));
        // assert(is_equal_float(mat.elem[7], 0.0f));

        // assert(is_equal_float(mat.elem[8], 0.0f));
        // assert(is_equal_float(mat.elem[9], 0.0f));
        // assert(is_equal_float(mat.elem[10], 1.0f));
        // assert(is_equal_float(mat.elem[11], 0.0f));

        // assert(is_equal_float(mat.elem[12], 0.0f));
        // assert(is_equal_float(mat.elem[13], 0.0f));
        // assert(is_equal_float(mat.elem[14], 0.0f));
        // assert(is_equal_float(mat.elem[15], 1.0f));

        // camera_destroy(&camera);
        // assert(NULL == camera);

        // memory_system_test_param_reset();
        // memory_system_destroy();
    }
}

static void NO_COVERAGE test_is_valid_frustum(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // frustum_ == NULL
        assert(false == is_valid_frustum(NULL));
    }
    {
        // near_clip >= far_clip
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 10.0f,
            .far_clip = 10.0f,
        };
        assert(false == is_valid_frustum(&frustum));

        frustum.near_clip = 100.0f;
        frustum.far_clip = 10.0f;
        assert(false == is_valid_frustum(&frustum));
    }
    {
        // aspect <= 0.0f
        viewing_frustum_t frustum = {
            .aspect = 0.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f,
        };
        assert(false == is_valid_frustum(&frustum));

        frustum.aspect = -1.0f;
        assert(false == is_valid_frustum(&frustum));
    }
    {
        // fovy <= 0.0f
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 0.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f,
        };
        assert(false == is_valid_frustum(&frustum));

        frustum.fovy = -1.0f;
        assert(false == is_valid_frustum(&frustum));
    }
    {
        // fovy >= 180.0f
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 180.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f,
        };
        assert(false == is_valid_frustum(&frustum));

        frustum.fovy = 181.0f;
        assert(false == is_valid_frustum(&frustum));
    }
    {
        // near_clip <= 0.0f
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 0.0f,
            .far_clip = 100.0f,
        };
        assert(false == is_valid_frustum(&frustum));

        frustum.near_clip = -0.1f;
        assert(false == is_valid_frustum(&frustum));
    }
    {
        // far_clip <= 0.0f
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = 0.0f,
        };
        assert(false == is_valid_frustum(&frustum));

        frustum.far_clip = -100.0f;
        assert(false == is_valid_frustum(&frustum));
    }
    {
        // 正常系
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f,
        };
        assert(true == is_valid_frustum(&frustum));
    }
    {
        // 境界近傍の正常系
        viewing_frustum_t frustum = {
            .aspect = 1.0f,
            .fovy = 179.999f,
            .near_clip = 0.0001f,
            .far_clip = 0.0002f,
        };
        assert(true == is_valid_frustum(&frustum));
    }
}
#endif
