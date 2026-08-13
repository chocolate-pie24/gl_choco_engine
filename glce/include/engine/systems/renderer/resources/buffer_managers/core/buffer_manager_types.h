/**
 * @file buffer_manager_types.h
 * @brief Buffer Manager層で共通使用するデータ型を定義する
 *
 * @details
 * resources/buffer_managers以下の各モジュールで共有するデータ型を提供する。
 *
 * 現在は、Buffer Managerの公開APIが処理結果を上位層へ通知するための
 * buffer_manager_result_tを定義する。
 *
 * 下位モジュールの実行結果コードはBuffer Manager層の結果コードへ変換され、
 * Buffer Managerの各公開APIから本headerで定義された共通の結果コードとして
 * 呼び出し側へ通知される。
 *
 * @date 2026-07-31
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_CORE_BUFFER_MANAGER_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_CORE_BUFFER_MANAGER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buffer Managerの実行結果コード
 *
 * @details
 * Buffer Manager層の処理結果を表す。
 *
 * 引数そのものの不備、API契約に反する操作、Renderer Backendの
 * 実行時エラー、管理上限への到達、メモリまたはbuffer rangeの不足、
 * 内部データ破損、および算術overflowを区別して呼び出し側へ通知する。
 *
 * Range Allocator、Renderer、およびMemory Systemから通知された
 * 下位層の実行結果コードは、対応するBuffer Managerの結果コードへ
 * 変換して伝播される。
 *
 * 各結果コードの具体的な発生条件と、失敗時の内部状態および
 * 出力引数に関する保証は、それぞれのAPIのDoxygenに記載する。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
typedef enum {
    BUFFER_MANAGER_SUCCESS = 0,         /**< 処理に成功した */
    BUFFER_MANAGER_INVALID_ARGUMENT,    /**< NULL pointer、0を許可しないsize、
                                         *   または出力先の初期状態など、引数の基本的な
                                         *   事前条件を満たしていない */
    BUFFER_MANAGER_RUNTIME_ERROR,       /**< Renderer Backendによるbufferの生成、
                                         *   bind、データ転送、またはunbindなどの
                                         *   実行時処理に失敗した */
    BUFFER_MANAGER_LIMIT_EXCEEDED,      /**< allocation数、Range Allocatorのnode pool、
                                         *   または下位メモリ管理機構の管理上限に到達した */
    BUFFER_MANAGER_NO_MEMORY,           /**< Buffer Manager、Range Allocator、または
                                         *   backend resourceの生成に必要なメモリを
                                         *   確保できなかった、あるいは要求サイズを
                                         *   収容できる連続した空きrangeが存在しない */
    BUFFER_MANAGER_DATA_CORRUPTED,      /**< Buffer Manager、Range Allocator、または
                                         *   Renderer Backendの内部状態が正常ではない、
                                         *   あるいはrollback中など正常な内部状態では
                                         *   成功するはずの処理が失敗した */
    BUFFER_MANAGER_BAD_OPERATION,       /**< configに許可されていない値が指定された、
                                         *   または引数の基本形式は有効だが、
                                         *   現在の状態もしくはBuffer Managerの
                                         *   API契約と両立しない操作が要求された */
    BUFFER_MANAGER_OVERFLOW,            /**< Buffer Managerまたは下位モジュールにおける
                                         *   size、allocation数、buffer rangeなどの
                                         *   算術演算が表現可能範囲を超えた */
    BUFFER_MANAGER_UNDEFINED_ERROR,     /**< 下位モジュールから未定義の結果コード、
                                         *   またはBuffer Managerに対応する変換先がない
                                         *   結果コードを受け取った */
} buffer_manager_result_t;

#ifdef __cplusplus
}
#endif
#endif
