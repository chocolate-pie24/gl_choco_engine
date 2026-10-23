// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_mesh_shader.h
 * @author chocolate-pie24
 * @brief 法線・光源・材質を使用するUntextured Mesh描画用
 *        Shader ResourceのAPIを提供する。
 *
 * @details
 * Untextured Mesh描画用Shader ProgramとVAOを生成・所有し、
 * Model / View / Projection Matrix、Directional Light、
 * Ambient Light、およびMaterialのUniform設定機能を提供する。
 *
 * @section untextured_mesh_shader_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - 本ModuleはShader Resource本体、Shader Program、
 *   およびVAOを所有する。
 * - Shader Resource本体はGeneral Allocatorから確保する。
 * - Shader ProgramとVAOの生成・破棄はRenderer Backendへ委譲する。
 * - Shader Resourceの破棄時には、所有するShader Program、
 *   VAO、およびShader Resource本体を解放する。
 * - VBOおよびVBO Managerは所有・破棄しない。
 * - Geometry、Material Resource、Light Resourceも所有しない。
 * - 頂点データの転送、VBO Allocation、およびDraw Rangeの管理は
 *   本Moduleの責務に含まない。
 *
 * @par Backend Context / VBO Manager Borrow
 * - Renderer Backend Contextは各API呼び出し時にborrowする。
 * - Shader Resource内部にBackend Contextのポインタを保持しない。
 * - VBO ManagerはVAO生成時にborrowする。
 * - Shader Resource内部にVBO Managerのポインタを保持しない。
 * - VAOは生成時に指定されたVBOとのVertex Attribute Bindingを
 *   構築する。
 * - 対応するVBOは、このShaderを使用した描画の間、
 *   有効である必要がある。
 * - Shader Resourceの破棄には、有効なBackend Contextが必要である。
 *
 * @par Vertex Attribute
 * - VAOの生成にはpoint_normal_vertex_tのレイアウトを使用する。
 * - Vertex Attribute Location 0にpositionを割り当てる。
 * - positionは3成分のfloat型として設定する。
 * - Vertex Attribute Location 1にnormalを割り当てる。
 * - normalは3成分のsigned byte型として設定し、
 *   normalized attributeとして扱う。
 * - normalの4番目の要素はpaddingであり、
 *   Vertex Attributeの成分には含めない。
 * - Vertex Strideはsizeof(point_normal_vertex_t)とする。
 * - positionとnormalは同一のVBO内に格納する。
 * - VBOへの頂点データ書込はGeometry Pipelineへ委譲する。
 *
 * @par Shader Program / Uniform
 * - Vertex ShaderとFragment Shaderを指定ファイルから生成する。
 * - Shader Programの構築はShader Program Builderへ委譲する。
 * - 生成時にModel / View / Projection Matrix、
 *   LightおよびMaterialのUniform Locationを取得・保持する。
 * - Matrixはmat4x4f_tとして受け取る。
 * - Directional LightとAmbient Lightの色はvec3f_t、
 *   強度はfloatとして受け取る。
 * - Directional Lightの方向はView座標系のvec3f_tとして受け取る。
 * - Materialにはuntextured_mesh_material_tを使用する。
 * - Matrix、Light、Materialの値はShader Resource内に
 *   別途保持せず、BackendのUniform設定APIへ送信する。
 *
 * @par Binding / Rendering
 * - use()はShader Programを有効化する。
 * - vao_bind()はVAOをBindする。
 * - 両APIは独立しており、互いの処理を暗黙に実行しない。
 * - Uniform設定APIはShader Programの有効化を行わない。
 * - callerはUniform設定に先立って、対象Shader Programを
 *   use()によって有効化する必要がある。
 * - Draw Callの発行は本Moduleの責務に含まない。
 *
 * @par Construction / Destruction
 * - create()はShader ProgramとVAOを含む
 *   Shader Resourceの構築を行う。
 * - 回復可能な構築失敗では、生成済みResourceを解放する。
 * - DATA_CORRUPTED発生時は通常のcleanupを保証しない。
 * - destroy()は所有Resourceを破棄し、
 *   正常終了時にShader Resourceの所有ポインタをNULLにする。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 *
 * @date 2026-06-03
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UNTEXTURED_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UNTEXTURED_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct untextured_mesh_shader untextured_mesh_shader_t;                   /**< 単色ライティング描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */
typedef struct vbo_manager vbo_manager_t;
typedef struct mat4x4f mat4x4f_t;
typedef struct untextured_mesh_material untextured_mesh_material_t;

/**
 * @brief Untextured Mesh描画用Shader Resourceを生成する。
 *
 * @par Public API Contract
 * - 指定されたVertex ShaderとFragment Shaderから
 *   Shader Programを生成する。
 * - Shader Programの構築はShader Program Builderへ委譲する。
 * - Model / View / Projection Matrix、Light、
 *   MaterialのUniform Locationを取得する。
 * - VBO Managerを使用してUntextured Mesh用VAOを生成する。
 * - VAOにはpoint_normal_vertex_tのposition属性と
 *   normal属性を設定する。
 * - positionは3成分のfloat型とする。
 * - normalは3成分のsigned byte型とし、正規化を有効にする。
 * - Shader Resource本体はGeneral Allocatorから確保する。
 * - 全Resourceの構築が成功した場合、
 *   完成したShader Resourceをcallerへ出力する。
 *
 * @par Ownership
 * - 成功時、callerは生成されたShader Resourceの所有権を取得する。
 * - Shader ResourceはShader ProgramとVAOを所有する。
 * - Renderer Backend ContextとVBO Managerの所有権は取得しない。
 * - Shader File Pathのownershipは取得しない。
 * - callerは不要になったShader Resourceをdestroy()で破棄する。
 *
 * @par Failure Contract
 * - 回復可能な構築失敗では、生成済みのShader Program、
 *   VAOおよびShader Resource本体の解放を行う。
 * - 回復可能な失敗ではout_shader_の値を変更しない。
 * - DATA_CORRUPTEDの場合は通常のcleanupを行わず、
 *   構築途中のResourceの解放完了を保証しない。
 *
 * @param[in] vertex_shader_fullpath_
 *            Vertex Shader FileのFull Path。
 * @param[in] fragment_shader_fullpath_
 *            Fragment Shader FileのFull Path。
 * @param[in,out] backend_context_
 *                Shader ProgramとVAOの生成に使用する
 *                Renderer Backend Context。
 * @param[in,out] vbo_manager_
 *                VAO構築時に使用するVBO Manager。
 * @param[out] out_shader_
 *             生成したShader Resourceの所有ポインタ出力先。
 *
 * @retval SHADER_SUCCESS 生成成功。
 * @retval SHADER_INVALID_ARGUMENT NULL引数または空のShader File Path。
 * @retval SHADER_BAD_OPERATION 出力先がNULLでない、
 *         または下位Operationの実行条件が成立しない。
 * @retval SHADER_NO_MEMORY Shader Resource等のメモリ確保に失敗。
 * @retval SHADER_COMPILE_ERROR Shaderのコンパイルに失敗。
 * @retval SHADER_LINK_ERROR Shader Programのリンクに失敗。
 * @retval SHADER_LIMIT_EXCEEDED 下位Resourceの制限を超過。
 * @retval SHADER_RUNTIME_ERROR 下位処理で実行時エラーが発生。
 * @retval SHADER_OVERFLOW 下位処理でサイズ計算等のoverflowが発生。
 * @retval SHADER_DATA_CORRUPTED 内部状態の整合性異常、
 *         またはVAO構築時のrollback失敗。
 * @retval SHADER_UNDEFINED_ERROR 下位処理で未分類のエラーが発生。
 *
 * @pre backend_context_とvbo_manager_は利用可能であること。
 * @pre Shader File Pathは空文字列ではない
 *      読み取り可能なNUL終端文字列であること。
 * @pre out_shader_は書き込み可能なポインタを指し、
 *      *out_shader_はNULLに初期化されていること。
 *
 * @post 成功時、*out_shader_はShader ProgramとVAOを
 *       所有する有効なShader Resourceを指す。
 * @post VBO Managerの所有権は移転しない。
 */
shader_result_t untextured_mesh_shader_create(const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, renderer_backend_context_t* backend_context_, vbo_manager_t* vbo_manager_, untextured_mesh_shader_t** out_shader_);

/**
 * @brief Untextured Mesh Shader Resourceを破棄する。
 *
 * @par Public API Contract
 * - Shader Resourceが所有するVAO、Shader Program、
 *   およびShader Resource本体を解放する。
 * - VAOとShader Programの破棄はRenderer Backendへ委譲する。
 * - Shader Resource本体はGeneral Allocatorへ返却する。
 * - VBO ManagerとVBO Allocationは解放しない。
 * - shader_または*shader_がNULLの場合は何も行わない。
 * - Shader Resourceの内部所有ポインタに異常を検出した場合、
 *   破棄処理を中断する。
 * - 本APIはvoidであり、破棄失敗をResultとして返さない。
 *
 * @param[in,out] backend_context_
 *                Shader ProgramとVAOの破棄に使用する
 *                Renderer Backend Context。
 * @param[in,out] shader_
 *                破棄するShader Resourceの所有ポインタ。
 *
 * @pre shader_が有効なShader Resourceを保持する場合、
 *      backend_context_は利用可能であること。
 * @pre shader_が有効なShader Resourceを保持する場合、
 *      Shader ProgramとVAOの内部所有関係が成立していること。
 *
 * @post 正常終了時、Shader Resourceとその所有Resourceは
 *       解放され、*shader_はNULLとなる。
 * @post VBO ManagerおよびVBO Allocationの
 *       所有状態は変更されない。
 */
void untextured_mesh_shader_destroy(renderer_backend_context_t* backend_context_, untextured_mesh_shader_t** shader_);

/**
 * @brief Untextured Mesh描画用VAOをBindする。
 *
 * @par Public API Contract
 * - Shader Resourceが所有するVAOをRenderer Backendへ渡し、
 *   現在のVertex Array Bindingとして設定する。
 * - VAOには生成時に設定されたPositionとNormalの
 *   Vertex Attribute、およびVBOとのBinding関係が保持される。
 * - 本APIはShader Programを有効化しない。
 * - VBOへの頂点データ書込やDraw Callは実行しない。
 * - Shader Resourceの所有状態は変更しない。
 *
 * @param[in] backend_context_
 *            VAOのBindに使用するRenderer Backend Context。
 * @param[in] shader_
 *            Bind対象のVAOを所有するShader Resource。
 *
 * @retval SHADER_SUCCESS VAOのBind成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre VAOに対応するVBOは有効であること。
 *
 * @post 成功時、Shader ResourceのVAOがBackendにBindされる。
 */
shader_result_t untextured_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_);

/**
 * @brief Untextured Mesh描画用Shader Programを有効化する。
 *
 * @par Public API Contract
 * - Shader Resourceが所有するShader Programを
 *   Renderer Backendの現在のProgramとして設定する。
 * - Uniform設定APIの呼び出し前に本APIを実行する。
 * - 本APIはVAOのBindを行わない。
 * - Shader Resourceの内部Uniform値や所有状態は変更しない。
 *
 * @param[in] backend_context_
 *            Shader Programの有効化に使用する
 *            Renderer Backend Context。
 * @param[in] shader_
 *            有効化するShader Resource。
 *
 * @retval SHADER_SUCCESS Shader Programの有効化成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 *
 * @post 成功時、対象Shader Programが有効化される。
 */
shader_result_t untextured_mesh_shader_use(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_);

/**
 * @brief Untextured Mesh描画用Model MatrixをUniformへ設定する。
 *
 * @par Public API Contract
 * - Shader Program生成時に取得したg_model_matrixの
 *   Uniform Locationを使用する。
 * - 指定されたmat4x4f_tの行列要素をRenderer Backendへ送信する。
 * - should_transpose_はBackendのMatrix Uniform設定APIへ
 *   そのまま渡す。
 * - Shader Resource内にMatrixのコピーは保持しない。
 * - 本APIはShader Programの有効化を行わない。
 *
 * @param[in] backend_context_
 *            Uniform設定に使用するRenderer Backend Context。
 * @param[in] shader_
 *            Model MatrixのUniform Locationを保持するShader Resource。
 * @param[in] model_matrix_
 *            設定するModel Matrix。
 * @param[in] should_transpose_
 *            Matrix送信時の転置指定。
 *
 * @retval SHADER_SUCCESS Uniform設定成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre model_matrix_は読み取り可能なmat4x4f_tを指していること。
 * @pre 対象Shader Programがuse()によって有効化されていること。
 *
 * @post 成功時、Model MatrixのUniform送信が完了する。
 */
shader_result_t untextured_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

/**
 * @brief Untextured Mesh描画用View MatrixをUniformへ設定する。
 *
 * @par Public API Contract
 * - Shader Program生成時に取得したg_view_matrixの
 *   Uniform Locationを使用する。
 * - 指定されたmat4x4f_tの行列要素をRenderer Backendへ送信する。
 * - should_transpose_はBackendのMatrix Uniform設定APIへ
 *   そのまま渡す。
 * - Shader Resource内にMatrixのコピーは保持しない。
 * - 本APIはShader Programの有効化を行わない。
 *
 * @param[in] backend_context_
 *            Uniform設定に使用するRenderer Backend Context。
 * @param[in] shader_
 *            View MatrixのUniform Locationを保持するShader Resource。
 * @param[in] view_matrix_
 *            設定するView Matrix。
 * @param[in] should_transpose_
 *            Matrix送信時の転置指定。
 *
 * @retval SHADER_SUCCESS Uniform設定成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre view_matrix_は読み取り可能なmat4x4f_tを指していること。
 * @pre 対象Shader Programがuse()によって有効化されていること。
 *
 * @post 成功時、View MatrixのUniform送信が完了する。
 */
shader_result_t untextured_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

/**
 * @brief Untextured Mesh描画用Projection MatrixをUniformへ設定する。
 *
 * @par Public API Contract
 * - Shader Program生成時に取得したg_projection_matrixの
 *   Uniform Locationを使用する。
 * - 指定されたmat4x4f_tの行列要素をRenderer Backendへ送信する。
 * - should_transpose_はBackendのMatrix Uniform設定APIへ
 *   そのまま渡す。
 * - Shader Resource内にMatrixのコピーは保持しない。
 * - 本APIはShader Programの有効化を行わない。
 *
 * @param[in] backend_context_
 *            Uniform設定に使用するRenderer Backend Context。
 * @param[in] shader_
 *            Projection MatrixのUniform Locationを保持するShader Resource。
 * @param[in] projection_matrix_
 *            設定するProjection Matrix。
 * @param[in] should_transpose_
 *            Matrix送信時の転置指定。
 *
 * @retval SHADER_SUCCESS Uniform設定成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre projection_matrix_は読み取り可能なmat4x4f_tを指していること。
 * @pre 対象Shader Programがuse()によって有効化されていること。
 *
 * @post 成功時、Projection MatrixのUniform送信が完了する。
 */
shader_result_t untextured_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

/**
 * @brief Directional Lightの方向をUniformへ設定する。
 *
 * @par Public API Contract
 * - Shader Program生成時に取得した
 *   g_directional_light_directionのUniform Locationを使用する。
 * - direction_はView座標系で表現された光源方向とする。
 * - direction_の3成分をRenderer Backendへ送信する。
 * - 本APIでは座標変換や正規化を行わない。
 * - Directional Light Resourceのownershipは取得しない。
 * - 本APIはShader Programの有効化を行わない。
 *
 * @param[in] backend_context_
 *            Uniform設定に使用するRenderer Backend Context。
 * @param[in] shader_
 *            Directional LightのUniform Locationを保持するShader Resource。
 * @param[in] direction_
 *            View座標系で表現された光源方向。
 *
 * @retval SHADER_SUCCESS Uniform設定成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre direction_は対象Shader Programが要求する
 *      View座標系の光源方向として与えられていること。
 * @pre 対象Shader Programがuse()によって有効化されていること。
 *
 * @post 成功時、Directional Lightの方向Uniform送信が完了する。
 */
shader_result_t untextured_mesh_shader_directional_light_direction_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t direction_);

/**
 * @brief Directional Lightの色と強度をUniformへ設定する。
 *
 * @par Public API Contract
 * - Shader Program生成時に取得した
 *   g_directional_light_colorと
 *   g_directional_light_intensityのUniform Locationを使用する。
 * - color_の3成分をvec3f Uniformとして送信する。
 * - intensity_をfloat Uniformとして送信する。
 * - ColorとIntensityは独立したUniformとして設定する。
 * - 本APIでは色と強度の正規化や範囲検証を行わない。
 * - Directional Light Resourceのownershipは取得しない。
 * - 本APIはShader Programの有効化を行わない。
 *
 * @par Failure Contract
 * - Color Uniformの設定に失敗した場合、
 *   Intensity Uniformの設定は実行しない。
 * - Color Uniformの設定成功後にIntensity Uniformの設定が
 *   失敗した場合、Colorの更新はrollbackしない。
 * - 本Operationは複数Uniformのatomic updateを保証しない。
 *
 * @param[in] backend_context_
 *            Uniform設定に使用するRenderer Backend Context。
 * @param[in] shader_
 *            Directional LightのUniform Locationを保持するShader Resource。
 * @param[in] color_
 *            Directional LightのRGB色。
 * @param[in] intensity_
 *            Directional Lightの強度。
 *
 * @retval SHADER_SUCCESS ColorとIntensityのUniform設定成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre color_とintensity_は対象Lightのsemantic contractに
 *      適合する値であること。
 * @pre 対象Shader Programがuse()によって有効化されていること。
 *
 * @post 成功時、Directional LightのColorとIntensityの
 *       Uniform送信が完了する。
 */
shader_result_t untextured_mesh_shader_directional_light_color_intensity_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_);

/**
 * @brief Ambient Lightの色と強度をUniformへ設定する。
 *
 * @par Public API Contract
 * - Shader Program生成時に取得した
 *   g_ambient_light_colorと
 *   g_ambient_light_intensityのUniform Locationを使用する。
 * - color_の3成分をvec3f Uniformとして送信する。
 * - intensity_をfloat Uniformとして送信する。
 * - ColorとIntensityは独立したUniformとして設定する。
 * - 本APIでは色と強度の正規化や範囲検証を行わない。
 * - Ambient Light Resourceのownershipは取得しない。
 * - 本APIはShader Programの有効化を行わない。
 *
 * @par Failure Contract
 * - Color Uniformの設定に失敗した場合、
 *   Intensity Uniformの設定は実行しない。
 * - Color Uniformの設定成功後にIntensity Uniformの設定が
 *   失敗した場合、Colorの更新はrollbackしない。
 * - 本Operationは複数Uniformのatomic updateを保証しない。
 *
 * @param[in] backend_context_
 *            Uniform設定に使用するRenderer Backend Context。
 * @param[in] shader_
 *            Ambient LightのUniform Locationを保持するShader Resource。
 * @param[in] color_
 *            Ambient LightのRGB色。
 * @param[in] intensity_
 *            Ambient Lightの強度。
 *
 * @retval SHADER_SUCCESS ColorとIntensityのUniform設定成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre color_とintensity_は対象Lightのsemantic contractに
 *      適合する値であること。
 * @pre 対象Shader Programがuse()によって有効化されていること。
 *
 * @post 成功時、Ambient LightのColorとIntensityの
 *       Uniform送信が完了する。
 */
shader_result_t untextured_mesh_shader_ambient_light_color_intensity_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_);

/**
 * @brief Untextured Mesh Materialの反射特性をUniformへ設定する。
 *
 * @par Public API Contract
 * - Shader Program生成時に取得したg_materialの
 *   各Uniform Locationを使用する。
 * - material_->ambientをvec3f Uniformとして送信する。
 * - material_->diffuseをvec3f Uniformとして送信する。
 * - material_->specularをvec3f Uniformとして送信する。
 * - material_->shininessをfloat Uniformとして送信する。
 * - Materialの値をShader Resource内部に保持しない。
 * - Material Resourceのownershipは取得しない。
 * - 本APIはShader Programの有効化を行わない。
 *
 * @par Validation
 * - backend_context_、shader_、material_のNULLを検査する。
 * - Shader Resourceのshallow validityを確認する。
 * - DEBUG / TEST Buildでは、
 *   untextured_mesh_material_is_valid()によって
 *   Materialのcanonical validityを確認する。
 * - RELEASE BuildではMaterialのsemantic validityを
 *   本API内で検証しない。
 *
 * @par Failure Contract
 * - UniformはAmbient、Diffuse、Specular、Shininessの順に設定する。
 * - 途中のUniform設定に失敗した場合は、後続の設定を中断する。
 * - 既に成功したUniformの更新はrollbackしない。
 * - 本Operationは4つのUniformのatomic updateを保証しない。
 *
 * @param[in] backend_context_
 *            Uniform設定に使用するRenderer Backend Context。
 * @param[in] shader_
 *            MaterialのUniform Locationを保持するShader Resource。
 * @param[in] material_
 *            設定するUntextured Mesh Material。
 *
 * @retval SHADER_SUCCESS 全Material Uniformの設定成功。
 * @retval SHADER_INVALID_ARGUMENT 引数がNULL、
 *         またはDEBUG / TEST BuildでMaterialが無効。
 * @retval SHADER_DATA_CORRUPTED Shader Resourceの内部状態、
 *         または下位処理で整合性異常が発生。
 * @retval SHADER_BAD_OPERATION 下位BackendのOperationが成立しない。
 * @retval SHADER_RUNTIME_ERROR 下位Backendで実行時エラーが発生。
 * @retval SHADER_UNDEFINED_ERROR 下位Backendで未分類のエラーが発生。
 *
 * @pre backend_context_とshader_は利用可能であること。
 * @pre material_は有効なuntextured_mesh_material_tを指していること。
 * @pre 対象Shader Programがuse()によって有効化されていること。
 *
 * @post 成功時、Ambient、Diffuse、Specular、Shininessの
 *       各Uniform送信が完了する。
 * @post Materialのownershipは移転しない。
 */
shader_result_t untextured_mesh_shader_material_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const untextured_mesh_material_t* material_);

#ifdef __cplusplus
}
#endif
#endif
