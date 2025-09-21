/**
 * @file choco_message.c
 * @author chocolate-pie24
 * @brief メッセージ標準出力、標準エラー出力処理実装
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "engine/base/choco_message.h"

void message_output(MESSAGE_SEVERITY severity_, const char* format_, ...) {
    FILE* const out = (severity_ == MESSAGE_SEVERITY_ERROR) ? stderr : stdout;

    static const char head_err[] = "\033[1;31m[ERROR] ";
    static const char head_war[] = "\033[1;33m[WARNING] ";
    static const char head_inf[] = "\033[1;35m[INFORMATION] ";
    static const char head_dbg[] = "\033[1;34m[DEBUG] ";

    flockfile(out); // 同一ストリームの同時書き込みをまとめる

    switch (severity_) {
        case MESSAGE_SEVERITY_ERROR:       fputs(head_err, out); break;
        case MESSAGE_SEVERITY_WARNING:     fputs(head_war, out); break;
        case MESSAGE_SEVERITY_INFORMATION: fputs(head_inf, out); break;
        case MESSAGE_SEVERITY_DEBUG:       fputs(head_dbg, out); break;
        // -Wswitch-enumで足りないものは警告してくれるのでdefaultは削除
    }

    // body
    va_list args;
    va_start(args, format_);
    vfprintf(out, format_, args);
    va_end(args);

    // tail (色リセット + 改行)
    static const char s_color_reset[] = "\033[0m\n";
    fputs(s_color_reset, out);

    funlockfile(out);
}
