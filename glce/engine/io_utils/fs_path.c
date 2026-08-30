// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/io_utils/fs_path.h"

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/containers/choco_string.h"

#define FS_PATH_DEFAULT_BUFF_SIZE 128

#ifdef _WIN32
static const char s_path_separator = '\\';
#else
static const char s_path_separator = '/';
#endif

struct fs_path {
    choco_string_t* fullpath;
};

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

static fs_path_result_t executable_fullpath_get(char** out_fullpath_, size_t* out_bufsize_);
#if defined(__APPLE__)
static fs_path_result_t executable_fullpath_get_apple(char** out_fullpath_, size_t* out_bufsize_);
#elif defined(__linux__)
static fs_path_result_t executable_fullpath_get_linux(char** out_fullpath_, size_t* out_bufsize_);
#elif defined(__FreeBSD__)
static fs_path_result_t executable_fullpath_get_freebsd(char** out_fullpath_, size_t* out_bufsize_);
#endif

static const char* rslt_to_str(fs_path_result_t rslt_);
static fs_path_result_t rslt_convert_choco_memory(memory_system_result_t rslt_);
static fs_path_result_t rslt_convert_choco_string(choco_string_result_t rslt_);

static bool is_valid_shallow(const fs_path_t* fs_path_);

// NOTE:
// - path_の末尾は'/'
// - path_のseparatorはplatformによらず'/'
// - extension_はNULLを許可
// - extension_ != NULLの場合, 先頭に'.'は含まない
fs_path_result_t fs_path_create(fs_path_t** fs_path_, const char* path_, const char* name_, const char* extension_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_path_t* tmp_fs_path = NULL;
    choco_string_t* tmp_fullpath = NULL;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(fs_path_, ret, FS_PATH_INVALID_ARGUMENT, rslt_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "fs_path_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*fs_path_, ret, FS_PATH_INVALID_ARGUMENT, rslt_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "*fs_path_")
    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, FS_PATH_INVALID_ARGUMENT, rslt_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, FS_PATH_INVALID_ARGUMENT, rslt_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "name_")
    if('\0' == path_[0]) {
        ret = FS_PATH_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_path_create(%s) - Provided path_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == name_[0]) {
        ret = FS_PATH_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_path_create(%s) - Provided name_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(NULL != extension_ && ('\0' == extension_[0] || '.' == extension_[0])) {
        ret = FS_PATH_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_path_create(%s) - Provided extension_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    // fullpath生成
    ret_string = choco_string_create_from_c_string(path_, &tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("fs_path_create(%s) - choco_string_create_from_c_string failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret_string = choco_string_concat_from_c_string(name_, tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(NULL != extension_) {
        ret_string = choco_string_concat_from_c_string(".", tmp_fullpath);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", rslt_to_str(ret));
            goto cleanup;
        }
        ret_string = choco_string_concat_from_c_string(extension_, tmp_fullpath);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    // fs_path_t生成
    ret_memory = memory_system_allocate(sizeof(fs_path_t), MEMORY_TAG_FILE_IO, (void**)&tmp_fs_path);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("fs_path_create(%s) - memory_system_allocate failed.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_fs_path, 0, sizeof(fs_path_t));

    tmp_fs_path->fullpath = tmp_fullpath;
    tmp_fullpath = NULL;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_path_is_valid(tmp_fs_path)) {
        ret = FS_PATH_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_path_create(%s) - Postcondition validation failed for 'tmp_fs_path'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // commit
    *fs_path_ = tmp_fs_path;
    tmp_fs_path = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    if(NULL != tmp_fullpath) {
        choco_string_destroy(&tmp_fullpath);
    }
    if(NULL != tmp_fs_path) {
        fs_path_destroy(&tmp_fs_path);
    }
    return ret;
}

fs_path_result_t fs_path_create_from_executable_directory(fs_path_t** out_fs_path_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    char* executable_path = NULL;
    char* separator_ptr = NULL;
    size_t executable_path_buf_size = 0;
    fs_path_t* tmp_fs_path = NULL;
    choco_string_t* tmp_fullpath = NULL;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(out_fs_path_, ret, FS_PATH_INVALID_ARGUMENT, rslt_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create_from_executable_directory", "out_fs_path_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_fs_path_, ret, FS_PATH_INVALID_ARGUMENT, rslt_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create_from_executable_directory", "*out_fs_path_")

    // fullpath生成
    ret = executable_fullpath_get(&executable_path, &executable_path_buf_size);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - executable_fullpath_get failed.", rslt_to_str(ret));
        goto cleanup;
    }
    separator_ptr = strrchr(executable_path, s_path_separator);
    if(NULL == separator_ptr) {
        ret = FS_PATH_RUNTIME_ERROR;
        goto cleanup;
    }
    if(separator_ptr == executable_path) {
        separator_ptr[1] = '\0';
    } else {
        *separator_ptr = '\0';
    }

    ret_string = choco_string_create_from_c_string(executable_path, &tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - choco_string_create_from_c_string failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // fs_path_t生成
    ret_memory = memory_system_allocate(sizeof(fs_path_t), MEMORY_TAG_FILE_IO, (void**)&tmp_fs_path);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - memory_system_allocate failed.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_fs_path, 0, sizeof(fs_path_t));

    tmp_fs_path->fullpath = tmp_fullpath;
    tmp_fullpath = NULL;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_path_is_valid(tmp_fs_path)) {
        ret = FS_PATH_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - Postcondition validation failed for 'tmp_fs_path'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // commit
    *out_fs_path_ = tmp_fs_path;
    tmp_fs_path = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    if(NULL != tmp_fullpath) {
        choco_string_destroy(&tmp_fullpath);
    }
    if(NULL != tmp_fs_path) {
        fs_path_destroy(&tmp_fs_path);
    }
    if(0 != executable_path_buf_size && NULL != executable_path) {
        memory_system_free(executable_path, executable_path_buf_size, MEMORY_TAG_FILE_IO);
        executable_path = NULL;
    }
    return ret;
}

void fs_path_destroy(fs_path_t** fs_path_) {
    if(NULL == fs_path_) {
        return;
    }
    if(NULL == *fs_path_) {
        return;
    }
    choco_string_destroy(&(*fs_path_)->fullpath);
    memory_system_free(*fs_path_, sizeof(fs_path_t), MEMORY_TAG_FILE_IO);
    *fs_path_ = NULL;
}

const char* fs_path_fullpath_get(const fs_path_t* fs_path_) {
    if(NULL == fs_path_) {
        return NULL;
    }
#if defined(DEBUG_BUILD)
    if(!is_valid_shallow(fs_path_)) {
        ERROR_MESSAGE("fs_path_fullpath_get(%s) - Provided fs_path_ is corrupted.", rslt_to_str(FS_PATH_DATA_CORRUPTED));
        return NULL;
    }
#endif
#if defined(TEST_BUILD)
    if(!fs_path_is_valid(fs_path_)) {
        ERROR_MESSAGE("fs_path_fullpath_get(%s) - Provided fs_path_ is corrupted.", rslt_to_str(FS_PATH_DATA_CORRUPTED));
        return NULL;
    }
#endif
    return choco_string_c_str(fs_path_->fullpath);
}

bool fs_path_is_valid(const fs_path_t* fs_path_) {
    if(NULL == fs_path_) {
        return false;
    }
    if(!choco_string_is_valid(fs_path_->fullpath)) {
        return false;
    }
    if(0 == choco_string_length(fs_path_->fullpath)) {
        return false;
    }
    return true;
}

static fs_path_result_t executable_fullpath_get(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;
#if defined(__APPLE__)
    ret = executable_fullpath_get_apple(out_fullpath_, out_bufsize_);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("executable_fullpath_get(%s) - executable_fullpath_get_apple failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = FS_PATH_SUCCESS;
#elif defined(__linux__)
    ret = executable_fullpath_get_linux(out_fullpath_, out_bufsize_);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("executable_fullpath_get(%s) - executable_fullpath_get_linux failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = FS_PATH_SUCCESS;
#elif defined(__FreeBSD__)
    ret = executable_fullpath_get_freebsd(out_fullpath_, out_bufsize_);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("executable_fullpath_get(%s) - executable_fullpath_get_freebsd failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = FS_PATH_SUCCESS;
#else
    ret = FS_PATH_RUNTIME_ERROR;
    ERROR_MESSAGE("executable_fullpath_get(%s) - Unsupported platform.", rslt_to_str(ret));
    goto cleanup;
#endif

cleanup:
    return ret;
}

#ifdef __APPLE__
static fs_path_result_t executable_fullpath_get_apple(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    uint32_t bufsize = FS_PATH_DEFAULT_BUFF_SIZE;
    uint32_t allocated_size = 0;
    char* buf = NULL;

    ret_memory_system = memory_system_allocate(bufsize, MEMORY_TAG_FILE_IO, (void**)&buf);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("executable_fullpath_get(%s) - memory_system_allocate failed.", rslt_to_str(ret));
        goto cleanup;
    }
    allocated_size = bufsize;
    if(0 != _NSGetExecutablePath(buf, &bufsize)) {
        memory_system_free(buf, allocated_size, MEMORY_TAG_FILE_IO);
        buf = NULL;

        ret_memory_system = memory_system_allocate(bufsize, MEMORY_TAG_FILE_IO, (void**)&buf);
        if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
            ret = rslt_convert_choco_memory(ret_memory_system);
            ERROR_MESSAGE("executable_fullpath_get(%s) - memory_system_allocate failed.", rslt_to_str(ret));
            goto cleanup;
        }
        allocated_size = bufsize;
        if(0 != _NSGetExecutablePath(buf, &bufsize)) {
            memory_system_free(buf, allocated_size, MEMORY_TAG_FILE_IO);
            buf = NULL;

            ret = FS_PATH_UNDEFINED_ERROR;
            ERROR_MESSAGE("executable_fullpath_get(%s) - _NSGetExecutablePath failed.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    *out_fullpath_ = buf;
    *out_bufsize_ = (size_t)allocated_size;
    buf = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    return ret;
}
#endif

#ifdef __linux__
static fs_path_result_t executable_fullpath_get_linux(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    bool success = false;
    ssize_t result = 0;
    size_t bufsize = FS_PATH_DEFAULT_BUFF_SIZE;
    size_t allocated_size = 0;
    char* buf = NULL;

    while(!success) {
        ret_memory_system = memory_system_allocate(bufsize, MEMORY_TAG_FILE_IO, (void**)&buf);
        if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
            ret = rslt_convert_choco_memory(ret_memory_system);
            ERROR_MESSAGE("executable_fullpath_get_linux(%s) - memory_system_allocate failed.", rslt_to_str(ret));
            goto cleanup;
        }
        allocated_size = bufsize;

        result = readlink("/proc/self/exe", buf, allocated_size);
        if (-1 == result) {
            memory_system_free(buf, allocated_size, MEMORY_TAG_FILE_IO);
            buf = NULL;

            ret = FS_PATH_RUNTIME_ERROR;
            ERROR_MESSAGE("executable_fullpath_get_linux(%s) - readlink failed.", rslt_to_str(ret));
            goto cleanup;
        }

        if((size_t)result < allocated_size) {
            buf[result] = '\0';
            success = true;
        } else {    // 切り詰められている可能性があるためバッファを拡張し再取得
            memory_system_free(buf, allocated_size, MEMORY_TAG_FILE_IO);
            buf = NULL;

            if((SIZE_MAX / 2) < bufsize) {
                ret = FS_PATH_OVERFLOW;
                ERROR_MESSAGE("executable_fullpath_get_linux(%s) - buffer size overflow.", rslt_to_str(ret));
                goto cleanup;
            }
            bufsize *= 2;
        }
    }

    if(!success) {
        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("executable_fullpath_get_linux(%s) - executable_fullpath_get_linux failed.", rslt_to_str(ret));
        goto cleanup;
    }

    *out_fullpath_ = buf;
    *out_bufsize_ = (size_t)allocated_size;
    buf = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    return ret;
}
#endif

#ifdef __FreeBSD__
static fs_path_result_t executable_fullpath_get_freebsd(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    size_t allocated_size = 0;
    size_t required_size = 0;
    char* buf = NULL;
    int mib[4] = { 0 };

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;

    // バッファサイズ取得
    if(0 != sysctl(mib, 4, NULL, &required_size, NULL, 0)) {
        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - sysctl failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == required_size) {
        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - sysctl failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_memory_system = memory_system_allocate(required_size, MEMORY_TAG_FILE_IO, (void**)&buf);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - memory_system_allocate failed.", rslt_to_str(ret));
        goto cleanup;
    }
    allocated_size = required_size;

    // パス文字列取得
    if(0 != sysctl(mib, 4, buf, &required_size, NULL, 0)) {
        memory_system_free(buf, allocated_size, MEMORY_TAG_FILE_IO);
        buf = NULL;

        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - sysctl failed.", rslt_to_str(ret));
        goto cleanup;
    }

    *out_fullpath_ = buf;
    *out_bufsize_ = allocated_size;
    buf = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    return ret;
}
#endif

static const char* rslt_to_str(fs_path_result_t rslt_) {
    switch(rslt_) {
    case FS_PATH_SUCCESS:
        return s_rslt_str_success;
    case FS_PATH_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case FS_PATH_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case FS_PATH_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case FS_PATH_NO_MEMORY:
        return s_rslt_str_no_memory;
    case FS_PATH_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case FS_PATH_OVERFLOW:
        return s_rslt_str_overflow;
    case FS_PATH_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case FS_PATH_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static fs_path_result_t rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return FS_PATH_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return FS_PATH_UNDEFINED_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return FS_PATH_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return FS_PATH_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return FS_PATH_NO_MEMORY;
    default:
        return FS_PATH_UNDEFINED_ERROR;
    }
}

static fs_path_result_t rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return FS_PATH_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return FS_PATH_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return FS_PATH_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return FS_PATH_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return FS_PATH_UNDEFINED_ERROR;
    case CHOCO_STRING_RUNTIME_ERROR:
        return FS_PATH_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return FS_PATH_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return FS_PATH_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return FS_PATH_LIMIT_EXCEEDED;
    default:
        return FS_PATH_UNDEFINED_ERROR;
    }
}

static bool is_valid_shallow(const fs_path_t* fs_path_) {
    if(NULL == fs_path_) {
        return false;
    }
    if(NULL == fs_path_->fullpath) {
        return false;
    }
    if(0 == choco_string_length(fs_path_->fullpath)) {
        return false;
    }
    return true;
}
