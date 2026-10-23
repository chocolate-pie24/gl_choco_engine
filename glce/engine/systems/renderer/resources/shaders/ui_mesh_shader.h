// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file ui_mesh_shader.h
 * @author chocolate-pie24
 * @brief UI Mesh描画用Shader ProgramとVAOの管理、
 *        およびUniform設定APIを提供する。
 *
 * @details
 * UI Mesh描画用のShader ProgramとVAOを生成・所有し、
 * Model / View / Projection MatrixのUniform設定機能を提供する。
 *
 * @section ui_mesh_shader_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - 本ModuleはShader Resource本体、Shader Program、
 *   およびVAOを所有する。
 * - Shader Resource本体はGeneral Allocatorから確保する。
 * - Shader ProgramとVAOの生成・破棄はRenderer Backendへ委譲する。
 * - Shader Resourceの破棄時には、所有するShader Program、
 *   VAO、およびShader Resource本体を解放する。
 * - VBOおよびVBO Managerは所有・破棄しない。
 * - Texture Resourceの生成、Binding、Ownership管理は行わない。
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
 * - Shader Resourceの破棄時には、対応するBackend Contextが
 *   利用可能である必要がある。
 *
 * @par Vertex Attribute
 * - VAOの生成にはui_vertex_tのレイアウトを使用する。
 * - Vertex Attribute Location 0にpositionを割り当てる。
 * - positionは2成分のfloat型として設定する。
 * - Vertex Attribute Location 1にtex_coordを割り当てる。
 * - tex_coordは2成分のfloat型として設定する。
 * - 両Attributeのnormalized指定はfalseとする。
 * - Vertex Strideはsizeof(ui_vertex_t)とする。
 * - positionとtex_coordは同一のVBO内に格納する。
 * - VBOへの頂点データ書込はGeometry Pipelineへ委譲する。
 *
 * @par Shader Program / Uniform
 * - Vertex ShaderとFragment Shaderを指定ファイルから生成する。
 * - Shader Programの構築はShader Program Builderへ委譲する。
 * - 生成時にModel / View / Projection Matrixの
 *   Uniform Locationを取得・保持する。
 * - 各Matrixはmat4x4f_tとして受け取る。
 * - 行列の転置指定はshould_transpose_によって受け取る。
 * - Uniform値をShader Resource内に別途保持しない。
 * - Texture Samplingに関するResource Bindingは
 *   本Moduleの公開APIでは管理しない。
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
 * @date 2026-03-11
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UI_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UI_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct ui_mesh_shader ui_mesh_shader_t;                     /**< UI描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */
typedef struct vbo_manager vbo_manager_t;
typedef struct mat4x4f mat4x4f_t;

/**
 * @brief UI Mesh描画用Shader Resourceを生成する。
 *
 * @par Public API Contract
 * - 指定されたVertex ShaderとFragment Shaderから
 *   Shader Programを生成する。
 * - Shader Programの構築はShader Program Builderへ委譲する。
 * - Model / View / Projection MatrixのUniform Locationを取得する。
 * - VBO Managerを使用してUI Vertex用VAOを生成する。
 * - VAOにはui_vertex_tのposition属性とtex_coord属性を設定する。
 * - positionとtex_coordはいずれも2成分のfloat型とする。
 * - Shader Resource本体はGeneral Allocatorから確保する。
 * - Shader Program、VAO、本体の構築が成功した場合、
 *   完成したShader Resourceをcallerへ出力する。
 *
 * @par Ownership
 * - 成功時、callerは生成されたShader Resourceの所有権を取得する。
 * - Shader ResourceはShader ProgramとVAOを所有する。
 * - Renderer Backend ContextとVBO Managerの所有権は取得しない。
 * - Shader File Pathのownershipは取得しない。
 * - Texture Resourceの所有権は取得しない。
 * - callerは不要になったShader Resourceをdestroy()で破棄する。
 *
 * @par Failure Contract
 * - 回復可能な構築失敗では、生成済みのShader Program、
 *   VAOおよびShader Resource本体を解放する。
 * - 回復可能な失敗ではout_shader_の値を変更しない。
 * - DATA_CORRUPTEDの場合は通常のcleanupを行わず、
 *   構築途中のResourceの解放完了を保証しない。
 *
 * @param[in,out] backend_context_
 *                Shader ProgramとVAOの生成に使用する
 *                Renderer Backend Context。
 * @param[in] vertex_shader_fullpath_
 *            Vertex Shader FileのFull Path。
 * @param[in] fragment_shader_fullpath_
 *            Fragment Shader FileのFull Path。
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
 *         またはVAO構築時のcleanup失敗。
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
shader_result_t ui_mesh_shader_create(renderer_backend_context_t* backend_context_, const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, vbo_manager_t* vbo_manager_, ui_mesh_shader_t** out_shader_);

/**
 * @brief UI Mesh Shader Resourceを破棄する。
 *
 * @par Public API Contract
 * - Shader Resourceが所有するVAO、Shader Program、
 *   およびShader Resource本体を解放する。
 * - VAOとShader Programの破棄はRenderer Backendへ委譲する。
 * - Shader Resource本体はGeneral Allocatorへ返却する。
 * - VBO ManagerとVBO Allocationは解放しない。
 * - Texture Resourceの状態は変更しない。
 * - shader_または*shader_がNULLの場合は何も行わない。
 * - Shader Resourceの内部ポインタに異常を検出した場合、
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
 * @post VBO Manager、VBO Allocation、およびTexture Resourceの
 *       所有状態は変更されない。
 */
void ui_mesh_shader_destroy(renderer_backend_context_t* backend_context_, ui_mesh_shader_t** shader_);

/**
 * @brief UI Mesh描画用VAOをBindする。
 *
 * @par Public API Contract
 * - Shader Resourceが所有するVAOをRenderer Backendへ渡し、
 *   現在のVertex Array Bindingとして設定する。
 * - VAOには生成時に設定されたUI Vertex Attributeと
 *   VBOとのBinding関係が保持されている。
 * - positionとtex_coordはそれぞれLocation 0、Location 1を使用する。
 * - 本APIはShader Programを有効化しない。
 * - Texture ResourceのBindは行わない。
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
shader_result_t ui_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* shader_);

/**
 * @brief UI Mesh描画用Shader Programを有効化する。
 *
 * @par Public API Contract
 * - Shader Resourceが所有するShader Programを
 *   Renderer Backendの現在のProgramとして設定する。
 * - Uniform設定APIの呼び出し前に本APIを実行する。
 * - 本APIはVAOのBindを行わない。
 * - Texture ResourceのBindは行わない。
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
shader_result_t ui_mesh_shader_use(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* shader_);

/**
 * @brief UI Mesh描画用Model MatrixをUniformへ設定する。
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
shader_result_t ui_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

/**
 * @brief UI Mesh描画用View MatrixをUniformへ設定する。
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
shader_result_t ui_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

/**
 * @brief UI Mesh描画用Projection MatrixをUniformへ設定する。
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
shader_result_t ui_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

#ifdef __cplusplus
}
#endif
#endif
