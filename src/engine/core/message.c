/**
 * @file message.c
 * @author chocolate-pie24
 * @brief メッセージ標準出力機能実装
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "engine/core/message.h"

#define MESSAGE_BUFSIZE 1024
#define HEADER_BUFSIZE 128
#define TAIL_BUFSIZE 128

static void msg_header_create(MESSAGE_SEVERITY severity_, char header_[HEADER_BUFSIZE]);

void message_output(MESSAGE_SEVERITY severity_, const char* const format_, ...) {
    FILE* out = (MESSAGE_SEVERITY_ERROR == severity_) ? stderr : stdout;

    char message[MESSAGE_BUFSIZE] = { 0 };
    char header[HEADER_BUFSIZE] = { 0 };
    char tail[TAIL_BUFSIZE] = { 0 };

    const size_t message_len = strlen(format_);
    if(message_len > (MESSAGE_BUFSIZE + HEADER_BUFSIZE + TAIL_BUFSIZE)) {
        return;
    }

    msg_header_create(severity_, header);
    strcpy(tail, "\033[0m\n");

    // TODO: %s/%f等を展開した時の正確なバッファ長を計算し溢れを検知する
    strcpy(message, header);
    strcat(message, format_);
    strcat(message, tail);

    va_list args;
    va_start(args, format_);
    vfprintf(out, message, args);
    va_end(args);
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
