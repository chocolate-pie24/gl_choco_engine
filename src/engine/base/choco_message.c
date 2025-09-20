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

#define HEADER_BUFSIZE 128

static void msg_header_create(MESSAGE_SEVERITY severity_, char header_[HEADER_BUFSIZE]);

void message_output(MESSAGE_SEVERITY severity_, const char* const format_, ...) {
    FILE* out = (severity_ == MESSAGE_SEVERITY_ERROR) ? stderr : stdout;

    char header[HEADER_BUFSIZE] = {0};
    msg_header_create(severity_, header);

    // header
    fputs(header, out);

    // body
    va_list args;
    va_start(args, format_);
    vfprintf(out, format_, args);
    va_end(args);

    // tail (色リセット + 改行)
    fputs("\033[0m\n", out);
}

static void msg_header_create(MESSAGE_SEVERITY severity_, char header_[HEADER_BUFSIZE]) {
    memset(header_, 0, HEADER_BUFSIZE);

    switch(severity_) {
        case MESSAGE_SEVERITY_ERROR:
            strcpy(header_, "\033[1;31m[ERROR] ");
            break;
        case MESSAGE_SEVERITY_WARNING:
            strcpy(header_, "\033[1;33m[WARNING] ");
            break;
        case MESSAGE_SEVERITY_INFORMATION:
            strcpy(header_, "\033[1;35m[INFORMATION] ");
            break;
        case MESSAGE_SEVERITY_DEBUG:
            strcpy(header_, "\033[1;34m[DEBUG] ");
            break;
        default:
            break;
    }
}
