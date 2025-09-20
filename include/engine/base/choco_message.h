/** @addtogroup base
 * @{
 *
 * @file choco_message.h
 * @author chocolate-pie24
 * @brief メッセージ標準出力、標準エラー出力処理
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef GLCE_ENGINE_BASE_CHOCO_MESSAGE_H
#define GLCE_ENGINE_BASE_CHOCO_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 出力メッセージ重要度リスト
 * @author chocolate-pie24
 * @date 2024-10-13
 */
typedef enum MESSAGE_SEVERITY {
    MESSAGE_SEVERITY_ERROR,         /**< 重要度: エラー */
    MESSAGE_SEVERITY_WARNING,       /**< 重要度: ワーニング */
    MESSAGE_SEVERITY_INFORMATION,   /**< 重要度: インフォメーション */
    MESSAGE_SEVERITY_DEBUG,         /**< 重要度: デバッグ情報 */
} MESSAGE_SEVERITY;

/**
 * @brief ビルドモードによるエラーメッセージ出力切り替えスイッチ用マクロ定義
 * @note エラーメッセージなのでデバッグビルド/リリースビルド共に出力を有効化するが、無効化できるようにもしておく
 *
 */
#define ENABLE_MESSAGE_SEVERITY_ERROR 1

/**
 * @brief ビルドモードによるワーニングメッセージ出力切り替えスイッチ用マクロ定義
 * @note ワーニングメッセージなのでデバッグビルド/リリースビルド共に出力を有効化するが、無効化できるようにもしておく
 *
 */
#define ENABLE_MESSAGE_SEVERITY_WARNING 1

#ifdef RELEASE_BUILD
    /**
     * @brief デバッグ用メッセージよりは重要だが、ワーニングほど重要でないメッセージに関する出力切り替えスイッチ用マクロ定義
     * @note 現状はリリースビルド時には出力を無効にする
     */
    #define ENABLE_MESSAGE_SEVERITY_INFORMATION 0

    /**
     * @brief デバッグ用メッセージに関する出力切り替えスイッチ用マクロ定義
     * @note 現状はリリースビルド時には出力を無効にする
     */
    #define ENABLE_MESSAGE_SEVERITY_DEBUG 0
#else
    /**
     * @brief デバッグメッセージよりは重要だが、ワーニングほど重要でないメッセージに関する出力切り替えスイッチ用マクロ定義
     * @note 現状はデバッグビルド時には出力を有効にする
     */
    #define ENABLE_MESSAGE_SEVERITY_INFORMATION 1

    /**
     * @brief デバッグ用メッセージに関する出力切り替えスイッチ用マクロ定義
     * @note 現状はリリースビルド時には出力を有効にする
     */
    #define ENABLE_MESSAGE_SEVERITY_DEBUG 1
#endif

/**
 * @brief メッセージ出力関数(メッセージの重要度に応じて出力フォーマットを変える)
 *
 * @param severity_ メッセージの重要度
 * @param fmt_ メッセージ内容(printfの"message %s %f"と同様のフォーマット)
 * @param ... メッセージ内容に付加する各種値(printfの%sや%fに対する値に相当)
 */
void message_output(MESSAGE_SEVERITY severity_, const char* const fmt_,  ...);

#if ENABLE_MESSAGE_SEVERITY_ERROR
    /**
     * @brief エラーメッセージ出力処理マクロ定義
     *
     */
    #define ERROR_MESSAGE(...) message_output(MESSAGE_SEVERITY_ERROR, __VA_ARGS__)
#else
    /**
     * @brief エラーメッセージ出力処理マクロ定義(エラーメッセージ出力スイッチが無効であれば空行で置き換えられる)
     *
     */
    #define ERROR_MESSAGE(...)
#endif

#if ENABLE_MESSAGE_SEVERITY_WARNING
    /**
     * @brief ワーニングメッセージ出力処理マクロ定義
     *
     */
    #define WARN_MESSAGE(...) message_output(MESSAGE_SEVERITY_WARNING, __VA_ARGS__)
#else
    /**
     * @brief ワーニングメッセージ出力処理マクロ定義(ワーニングメッセージ出力スイッチが無効であれば空行で置き換えられる)
     *
     */
    #define WARN_MESSAGE(...)
#endif

#if ENABLE_MESSAGE_SEVERITY_INFORMATION
    /**
     * @brief インフォメーションメッセージ出力処理マクロ定義
     *
     */
    #define INFO_MESSAGE(...) message_output(MESSAGE_SEVERITY_INFORMATION, __VA_ARGS__)
#else
    /**
     * @brief インフォメーションメッセージ出力処理マクロ定義(インフォメーションメッセージ出力スイッチが無効であれば空行で置き換えられる)
     *
     */
    #define INFO_MESSAGE(...)
#endif

#if ENABLE_MESSAGE_SEVERITY_DEBUG
    /**
     * @brief デバッグメッセージ出力処理マクロ定義
     *
     */
    #define DEBUG_MESSAGE(...) message_output(MESSAGE_SEVERITY_DEBUG, __VA_ARGS__)
#else
    /**
     * @brief デバッグメッセージ出力処理マクロ定義(デバッグメッセージ出力スイッチが無効であれば空行で置き換えられる)
     *
     */
    #define DEBUG_MESSAGE(...)
#endif

#ifdef __cplusplus
}
#endif
#endif

/*@}*/
